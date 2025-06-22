#include "eudaq/ROOTMonitor.hh"

#include "TGraph2D.h"
#include "TH1.h"
#include "TProfile.h"

// let there be a user-defined Ex0EventDataFormat, containing e.g. three
// double-precision attributes get'ters:
//   double GetQuantityX(), double GetQuantityY(), and double GetQuantityZ()
struct Ex0EventDataFormat {
  Ex0EventDataFormat(const eudaq::Event &) {}
  double GetQuantityX() const { return (double)rand() / RAND_MAX; }
  double GetQuantityY() const { return (double)rand() / RAND_MAX; }
  double GetQuantityZ() const { return (double)rand() / RAND_MAX; }
};

class Ex0ROOTMonitor final : public eudaq::ROOTMonitor {
public:
  Ex0ROOTMonitor(const std::string &name, const std::string &runcontrol)
      : eudaq::ROOTMonitor(name, "Ex0 ROOT monitor", runcontrol, false) {}

  void AtConfiguration() override;
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Ex0ROOTMonitor");

private:
  static constexpr size_t kNumChannels = 4;
  TH1D *m_my_hist[kNumChannels];
  TGraph2D *m_my_graph[kNumChannels];
  TProfile *m_my_prof[kNumChannels];
};

namespace {
  auto mon_rootmon = eudaq::Factory<eudaq::Monitor>::Register<
      Ex0ROOTMonitor, const std::string &, const std::string &>(
      Ex0ROOTMonitor::m_id_factory);
}

void Ex0ROOTMonitor::AtConfiguration() {
  for (size_t ch_id = 0; ch_id < kNumChannels; ++ch_id) {
    m_my_hist[ch_id] = m_monitor->Book<TH1D>(
        Form("Channel %zu/my_hist", ch_id), "Example histogram",
        Form("h_example_ch%zu", ch_id), "A histogram;x-axis title;y-axis title",
        100, 0., 1. * (ch_id + 1));
    m_my_graph[ch_id] = m_monitor->Book<TGraph2D>(
        Form("Channel %zu/my_graph", ch_id), "Example graph");
    m_my_graph[ch_id]->SetTitle(Form(
        "A graph (channel %zu);x-axis title;y-axis title;z-axis title", ch_id));
    m_monitor->SetDrawOptions(m_my_graph[ch_id], "colz");
    m_my_prof[ch_id] = m_monitor->Book<TProfile>(
        Form("Channel %zu/my_profile", ch_id), "Example profile",
        Form("p_example_ch%zu", ch_id),
        "A profile histogram;x-axis title;y-axis title", 100, 0.,
        1. * (ch_id + 1));
  }
  for (size_t ch_id = 0; ch_id < kNumChannels; ++ch_id)
    m_monitor->AddSummary("Summary/my_hists", m_my_hist[ch_id]);
}

void Ex0ROOTMonitor::AtEventReception(eudaq::EventSP ev) {
  auto event = std::make_shared<Ex0EventDataFormat>(*ev);
  for (size_t ch_id = 0; ch_id < kNumChannels; ++ch_id) {
    m_my_hist[ch_id]->Fill(event->GetQuantityX() * (ch_id + 1));
    m_my_graph[ch_id]->SetPoint(
        m_my_graph[ch_id]->GetN(), event->GetQuantityX() * (ch_id + 1),
        event->GetQuantityY() * ch_id, event->GetQuantityZ() * (ch_id + 1));
    m_my_prof[ch_id]->Fill(event->GetQuantityX() * (ch_id + 1),
                           event->GetQuantityY() * (ch_id + 1));
  }
}
