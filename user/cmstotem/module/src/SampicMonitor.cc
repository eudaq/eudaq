#include "eudaq/Monitor.hh"
#include "eudaq/StdEventConverter.hh"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TMultiGraph.h"

#include "MonitorWindow.hh"
#include "SampicEvent.hh"

#include "TApplication.h"

#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#include <unordered_map>

class SampicMonitor : public eudaq::Monitor, public TApplication {
public:
  SampicMonitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");

private:
  static const uint32_t m_sampic_hash = eudaq::cstr2hash("SampicRaw");

  bool m_en_print;
  int m_sampic_num_last_samples;
  int m_sampic_num_baseline;
  double m_sampic_sampling_period;

  std::unique_ptr<MonitorWindow> m_main;
  std::future<void> m_daemon;
  std::future<void> m_test_daemon;

  unsigned long long m_num_evt_mon = 0ull;
  const size_t MAX_SAMPLES_DISP = 20;

  static constexpr unsigned short kNumCh = 32;
  static constexpr unsigned short kTrigSep = 2; // in s

  TGraph* m_g_trg_time, *m_g_evt_time, *m_g_trg_evt;
  TH1D* m_h_maxamp_per_chan[kNumCh];
  TH1D* m_h_noise_rms[kNumCh];
  TH1D* m_h_snratio[kNumCh];
  //TMultiGraph* m_mg_last_sample[kNumCh];
  TGraph* m_g_last_sample[kNumCh];
  TH1D* m_h_occup_allchan;
  TGraph* m_g_freq_vs_time[kNumCh];
  TH2D* m_h2_tel_tomo[kNumCh];

  unsigned long m_last_trg_num = 0;
  float m_last_trg_time = 0.;
  unsigned long m_num_smp_last_trg[kNumCh] = {0ul};
  //TMultiGraph* m_mg_last_sample_allchan;
  size_t m_num_ts_overfl = 0;
  uint64_t m_last_ts = 0ull;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SampicMonitor, const std::string&, const std::string&>(SampicMonitor::m_id_factory);
}

SampicMonitor::SampicMonitor(const std::string & name, const std::string & runcontrol)
  :eudaq::Monitor(name, runcontrol),
   TApplication(name.c_str(), nullptr, nullptr),
   m_main(new MonitorWindow(this, "Sampic monitor")){
  if (!m_main)
    EUDAQ_THROW("Error Allocationg main window");

  m_main->SetStatus(MonitorWindow::Status::idle);

  // launch the run loop
  TApplication::SetReturnFromRun(true);
  if (!m_daemon.valid())
    m_daemon = std::async(std::launch::async, &TApplication::Run, this, 0);
}

void SampicMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
  m_main->ResetCounters();
}

void SampicMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  m_en_print = conf->Get("SAMPIC_ENABLE_PRINT", 1);
  m_sampic_num_last_samples = conf->Get("SAMPIC_SHOW_LAST_SAMPLE", 0);
  m_plane_tel_tomo = conf->Get("EUTEL_PLANE_TOMO", 0);
  if (m_sampic_num_last_samples > MAX_SAMPLES_DISP)
    EUDAQ_THROW("Too many samples ("+std::to_string(m_sampic_num_last_samples)
               +") requested at each readout! maximum is "
               +std::to_string(MAX_SAMPLES_DISP));
  m_sampic_num_baseline = conf->Get("SAMPIC_NUM_BASELINE", 10);
  m_sampic_sampling_period = conf->Get("SAMPIC_SAMPLING_PERIOD", 1./120.e6);

  // book all monitoring elements
  m_g_trg_time = m_main->Book<TGraph>("trig_vs_time", "Trigger time");
  m_g_trg_time->SetTitle("Trigger time;Time (s);Trigger number");
  m_g_evt_time = m_main->Book<TGraph>("event_vs_time", "Event time");
  m_g_evt_time->SetTitle("Event time;Time (s);Event number");
  m_g_trg_evt = m_main->Book<TGraph>("trig_vs_event", "Event vs. trigger number");
  m_g_trg_evt->SetTitle(";Event number;Trigger number");
  m_h_occup_allchan = m_main->Book<TH1D>("channels_occup", "Occupancy, all channels", "ch_occup", "Channels occupancy;Channel number;Entries", kNumCh, 0., kNumCh);
  m_main->SetDrawOptions(m_h_occup_allchan, "hist text0");
  for (size_t i = 0; i < kNumCh; ++i) {
    m_h_maxamp_per_chan[i] = m_main->Book<TH1D>(Form("Channel %zu/max_ampl", i), "Max amplitude", Form("max_ampl_ch%zu", i), Form("Channel %zu;Maximum amplitude (V);Entries", i), 100, -1., 1.);
    m_h_noise_rms[i] = m_main->Book<TH1D>(Form("Channel %zu/noise_rms", i), "Noise RMS", Form("noise_rms_ch%zu", i), Form("Channel %zu;Noise RMS;Entries", i), 100, 0., 1.);
    m_h_snratio[i] = m_main->Book<TH1D>(Form("Channel %zu/sn_ratio", i), "S/N ratio", Form("snratio_ch%zu", i), Form("Channel %zu;S/N;Entries", i), 200, 0., 100.);
    m_h2_tel_tomo[i] = m_main->Book<TH2D>(Form("Channel %zu/tel_tomogr", i), "Tomography", Form("tel_tomo_ch%zu", i), Form("Channel %zu;Pixel column (plane 0);Pixel row (plane 0)", i), 144, 0., 1152., 72, 0., 576.);
    m_main->SetDrawOptions(m_h2_tel_tomo[i], "colz");
    m_g_freq_vs_time[i] = m_main->Book<TGraph>(Form("Channel %zu/scaler_vs_time", i), "Scaler vs time");
    m_g_freq_vs_time[i]->SetTitle(Form("Channel %zu;Time (s);Rate (Hz)", i));
    m_main->SetDrawOptions(m_g_freq_vs_time[i], "a*");
    if (m_sampic_num_last_samples > 0) {
      m_g_last_sample[i] = m_main->Book<TGraph>(Form("Channel %zu/last_sample", i), "Last sample");
      m_main->SetDrawOptions(m_g_last_sample[i], "alp");
      m_main->SetRangeY(m_g_last_sample[i], 0., 1.1);
      m_g_last_sample[i]->SetTitle(Form("Channel %zu;Time (ns);Amplitude (V)", i));
    }
  }
  for (size_t i = 0; i < kNumCh; ++i) {
    const unsigned short board_id = i/16; // 16 channels per board
    m_main->AddSummary(Form("Summary/Board %u/Maximum amplitude", board_id), m_h_maxamp_per_chan[i]);
    m_main->AddSummary(Form("Summary/Board %u/SN ratio", board_id), m_h_snratio[i]);
    m_main->AddSummary(Form("Summary/Board %u/Tomography", board_id), m_h2_tel_tomo[i]);
    m_main->AddSummary(Form("Summary/Board %u/Rate", board_id), m_g_freq_vs_time[i]);
    m_main->AddSummary(Form("Summary/Board %u/Last sample", board_id), m_g_last_sample[i]);
  }

  m_main->SetStatus(MonitorWindow::Status::configured);
  m_main->ResetCounters();
}

void SampicMonitor::DoStartRun(){
  m_main->ResetCounters();
  m_main->SetStatus(MonitorWindow::Status::running);
  m_main->SetRunNumber(GetRunNumber());
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);

  // update the counters
  m_main->SetCounters(ev->GetEventN(), ++m_num_evt_mon);

  std::shared_ptr<eudaq::SampicEvent> event;
  std::vector<std::pair<int,int> > tel_hits;

  auto sub_evts = ev->GetSubEvents();
  if (sub_evts.empty() && ev->GetDescription() == "SampicRaw")
    event = std::make_shared<eudaq::SampicEvent>(*ev);
  for (auto& sub_evt : sub_evts) {
    if (sub_evt->GetDescription() == "SampicRaw")
      event = std::make_shared<eudaq::SampicEvent>(*sub_evt);
    else if (sub_evt->GetDescription() == "NiRawDataEvent") {
      // unpack the telescope information (plane 0 only)
      auto tel_event = eudaq::StandardEvent::MakeShared();
      eudaq::StdEventConverter::Convert(sub_evt, tel_event, nullptr); // no configuration word
      if (tel_event->NumPlanes() > 0) {
	const auto& plane = tel_event->GetPlane(0);
	for (unsigned int lvl1 = 0; lvl1 < plane.NumFrames(); ++lvl1)
	  for (unsigned int i = 0; i < plane.HitPixels(lvl1); ++i)
	    tel_hits.emplace_back(std::make_pair(plane.GetX(i, lvl1), plane.GetY(i, lvl1)));
      }
    }
  }
  if (!event)
      return;

  // trigger/event information
  if (event->header().sf2Timestamp() < m_last_ts)
    ++m_num_ts_overfl;
  m_last_ts = event->header().sf2Timestamp();
  const double trg_time = (pow(2., 40)*m_num_ts_overfl+m_last_ts)*m_sampic_sampling_period;

  m_g_trg_time->SetPoint(m_g_trg_time->GetN(), trg_time, ev->GetTriggerN());
  m_g_evt_time->SetPoint(m_g_evt_time->GetN(), trg_time, ev->GetEventN());
  m_g_trg_evt->SetPoint(m_g_trg_evt->GetN(), ev->GetEventN(), ev->GetTriggerN());

  for (const auto& smp : *event) {
    const unsigned short ch_id = smp.header.channelIndex();
    m_h_occup_allchan->Fill(ch_id);
    const auto& samples = smp.sampic.samples();
    // compute the baseline from first few samples in the waveform
    const float baseline = 1./(m_sampic_num_baseline-1)*std::accumulate(
      samples.begin(),
      std::next(samples.begin(), m_sampic_num_baseline-1),
      0.);
    const float max_ampl = *std::min_element(samples.begin(), samples.end());
    double noise_rms = 0.;
    for (size_t i = 0; i < samples.size(); ++i) {
      const auto& s_val = samples.at(i);
      if (i < m_sampic_num_baseline)
	noise_rms += pow(s_val-baseline, 2);
      if (m_g_last_sample[ch_id])
        m_g_last_sample[ch_id]->SetPoint(i, eudaq::SampicEvent::TIME_SAMPLES()[i], s_val);
    }
    noise_rms = sqrt(noise_rms);
    // waveform analysis
    m_h_maxamp_per_chan[ch_id]->Fill(baseline-max_ampl);
    m_h_noise_rms[ch_id]->Fill(noise_rms);
    m_h_snratio[ch_id]->Fill((baseline-max_ampl)/noise_rms);
    // tomography with telescope
    for (const auto& pix : tel_hits)
      m_h2_tel_tomo[ch_id]->Fill(pix.first, pix.second);
    ++m_num_smp_last_trg[ch_id];
  }

  // time-dependent scaler
  if (trg_time > m_last_trg_time+kTrigSep) {
    const float time_since_last_trg = trg_time-m_last_trg_time;
    for (size_t i = 0; i < kNumCh; ++i) {
      m_g_freq_vs_time[i]->SetPoint(m_g_freq_vs_time[i]->GetN(), trg_time, m_num_smp_last_trg[i]/time_since_last_trg);
      m_num_smp_last_trg[i] = 0;
    }
    m_last_trg_time = trg_time;
    m_last_trg_num = ev->GetTriggerN();
  }
}

void SampicMonitor::DoStopRun(){
  m_main->SetStatus(MonitorWindow::Status::configured);
}

void SampicMonitor::DoReset(){
  m_main->SetStatus(MonitorWindow::Status::idle);
  m_main->ResetCounters();
}

void SampicMonitor::DoTerminate(){
  TApplication::Terminate(1);
}
