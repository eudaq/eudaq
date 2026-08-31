#include "ScopedTimer.hh"
#include "HistogramPublisher.hh"

#include <algorithm>


HistogramPublisher::HistogramPublisher(HistogramRegistry& registry, int port, int pump_interval_ms,
                                       std::string http_output_dir)
    : m_registry(registry),
      m_port(port),
      m_pump_interval_ms(std::max(pump_interval_ms, 5)),
      m_http_output_dir(std::move(http_output_dir)) {}

void HistogramPublisher::Start() {
  if (m_server) {
    return;
  }

  // "http:<port>" makes THttpServer use civetweb as the HTTP backend. THttpServer spawns its own I/O threads; our pump
  // thread is separate and only responsible for draining the request queue.

  const std::string spec = "http:" + std::to_string(m_port);
  m_server = std::make_unique<THttpServer>(spec.c_str());

  m_server->SetItemField("/", "_monitoring", "500"); // auto-refresh for JSROOT 500ms. API clients not affected.

  // Publish the histogram-snapshot directory as a dedicated object so the
  // frontend's reference-overlay can discover the files without per-deployment
  // config. It sits at the server root (not under kFolder), so it does not
  // appear in the histogram listing; the frontend reads its title via
  // /histo_output_dir/root.json. Skipped when no output dir is set.
  if (!m_http_output_dir.empty()) {
    m_output_dir_obj.SetNameTitle("histo_output_dir", m_http_output_dir.c_str());
    m_server->Register("/", &m_output_dir_obj);
  }

  // Register all histograms booked so far. Start() is called after all
  // fillers are constructed, so the set is complete.
  //
  // The server stays read-only (no method calls over HTTP) except that every
  // TH2 is allowed ProjectionX/ProjectionY (+ GetNbinsX): the frontend reads one
  // channel of a per-channel TH2 (e.g. ADC_dist_*, FERS_*_dist_*) via a
  // server-side projection instead of transferring the whole 2D histogram, and
  // learns the channel count once from GetNbinsX — keeping THttpServer
  // responsive with few registered objects (issue #138). Only those read-only
  // methods are exposed; destructive ones (Reset, Delete, ...) stay denied.
  m_registry.ForEach([this](TH1* h) {
    m_server->Register(kFolder, h);
    if (h->InheritsFrom("TH2")) {
      const std::string path = std::string(kFolder) + "/" + h->GetName();
      m_server->Restrict(path.c_str(), "allow_method=ProjectionX,ProjectionY,GetNbinsX");
    }
  });

  m_pump_running.store(true);
  m_pump_thread = std::thread(&HistogramPublisher::PumpLoop, this);
}

void HistogramPublisher::Stop() {
  // First stop the pump thread (it may be inside ProcessRequests),
  // then destroy THttpServer (which joins civetweb worker threads).
  m_pump_running.store(false);
  if (m_pump_thread.joinable()) {
    m_pump_thread.join();
  }
  m_server.reset();
}

void HistogramPublisher::PumpLoop() {
  while (m_pump_running.load()) {
    {
      // Same mutex acquired by FillerChain around Fill().
      // Prevents TBufferJSON (inside ProcessRequests) from reading
      // a histogram while a filler is writing to it.
      std::lock_guard<std::mutex> lock(m_mutex);
      ScopedTimer t(m_process_requests);
      m_server->ProcessRequests();
    }
    // Release the lock between iterations to avoid starving fillers.
    std::this_thread::sleep_for(std::chrono::milliseconds(m_pump_interval_ms));
  }
}
