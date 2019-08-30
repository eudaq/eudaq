#include "eudaq/Monitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
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
  bool m_en_print;
  int m_sampic_num_last_samples;
  int m_sampic_num_baseline;

  std::unique_ptr<MonitorWindow> m_main;
  std::future<void> m_daemon;
  std::future<void> m_test_daemon;

  unsigned long long m_num_evt_mon = 0ull;
  size_t m_num_samples_proc;
  const size_t MAX_SAMPLES_DISP = 20;

  TGraph* m_g_trg_time, *m_g_evt_time;
  TH1D* m_h_maxamp_per_chan[32], *m_h_occup_per_chan[32];
  //TMultiGraph* m_mg_last_sample[32];
  TGraph* m_g_last_sample[32];
  TH1D* m_h_occup_allchan;
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
  std::cout << __PRETTY_FUNCTION__ << std::endl;

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
  if (m_sampic_num_last_samples > MAX_SAMPLES_DISP)
    EUDAQ_THROW("Too many samples ("+std::to_string(m_sampic_num_last_samples)
               +") requested at each readout! maximum is "
               +std::to_string(MAX_SAMPLES_DISP));
  m_sampic_num_baseline = conf->Get("SAMPIC_NUM_BASELINE", 10);

  // book all monitoring elements
  m_g_trg_time = m_main->Book<TGraph>("trig_vs_time", "Trigger time");
  m_g_trg_time->SetTitle("Trigger time;Time (?s?);Trigger number");
  m_g_evt_time = m_main->Book<TGraph>("event_vs_time", "Event time");
  m_g_evt_time->SetTitle("Event time;Time (?s?);Event number");
  m_h_occup_allchan = m_main->Book<TH1D>("channels_occup", "Occupancy, all channels", "ch_occup", "Channels occupancy;Channel number;Entries", 32, 0., 32.);
  m_main->SetDrawOptions(m_h_occup_allchan, "hist text0");
  for (size_t i = 0; i < 32; ++i) {
    m_h_maxamp_per_chan[i] = m_main->Book<TH1D>(Form("ch%d/max_ampl", i), "Max amplitude", Form("max_ampl_ch%d", i), Form("Channel %d;Maximum amplitude (V);Entries", i), 100, -1., 1.);
    m_h_occup_per_chan[i] = m_main->Book<TH1D>(Form("ch%d/occupancy", i), "Occupancy", Form("occup_ch%d", i), Form("Channel %d;Samples multiplicity;Entries", i), 20, 0., 20.);
    if (m_sampic_num_last_samples > 0) {
      /*m_mg_last_sample[i] = m_main->Book<TMultiGraph>(Form("ch%d/last_samples", i), "Last samples");
      m_main->SetPersistant(m_mg_last_sample[i], false);
      m_main->SetDrawOptions(m_mg_last_sample[i], "alp");
      m_mg_last_sample[i]->SetTitle(Form("Channel %d;Time (ns);Amplitude (V)", i));*/
      m_g_last_sample[i] = m_main->Book<TGraph>(Form("ch%d/last_sample", i), "Last sample");
      m_main->SetDrawOptions(m_g_last_sample[i], "alp");
      m_g_last_sample[i]->SetTitle(Form("Channel %d;Time (ns);Amplitude (V)", i));
    }
  }

  m_main->SetStatus(MonitorWindow::Status::configured);
  m_main->ResetCounters();
}

void SampicMonitor::DoStartRun(){
  m_main->SetRunNumber(GetRunNumber());
  m_main->SetStatus(MonitorWindow::Status::running);
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);

  // update the counters
  m_main->SetLastEventNum(ev->GetEventN());
  m_main->SetMonitoredEventsNum(++m_num_evt_mon);

  auto event = std::make_shared<eudaq::SampicEvent>(*ev);

  const float ts = event->header().sf2Timestamp()/1.e6;
  m_g_trg_time->SetPoint(m_g_trg_time->GetN(), ts, ev->GetTriggerN());
  m_g_evt_time->SetPoint(m_g_evt_time->GetN(), ts, ev->GetEventN());

  std::unordered_map<size_t, size_t> occup_chan;
  for (const auto& smp : *event) {
    const unsigned short ch_id = smp.header.channelIndex();
    m_h_occup_allchan->Fill(ch_id);
    occup_chan[ch_id]++;
    const auto& samples = smp.sampic.samples();
    const float baseline = 1./(m_sampic_num_baseline-1)*std::accumulate(
      samples.begin(),
      std::next(samples.begin(), m_sampic_num_baseline-1),
      0.);
    float max_ampl = 0.;
    ///
    /*TGraph* g_sample = nullptr;
    if (m_sampic_num_last_samples > 0 && m_mg_last_sample[ch_id] &&
        m_mg_last_sample[ch_id]->GetListOfGraphs()->GetEntries() < m_sampic_num_last_samples)
      g_sample = new TGraph;
    std::cout << "channel " << ch_id << ", adding graph #" << m_mg_last_sample[ch_id]->GetListOfGraphs()->GetEntries() << std::endl;*/
    ///
    for (size_t i = 0; i < samples.size(); ++i) {
      const auto& s_val = samples.at(i);
      if (m_g_last_sample[ch_id])
        m_g_last_sample[ch_id]->SetPoint(i, eudaq::SampicEvent::TIME_SAMPLES()[i], s_val-baseline);
      /*if (g_sample)
        g_sample->SetPoint(i, eudaq::SampicEvent::TIME_SAMPLES()[i], s_val-baseline);*/
      if (s_val > max_ampl)
        max_ampl = s_val;
    }
    /*if (g_sample)
      m_mg_last_sample[ch_id]->Add(g_sample);*/
    m_h_maxamp_per_chan[ch_id]->Fill(max_ampl-baseline);
  }
  for (const auto& oc : occup_chan)
    m_h_occup_per_chan[oc.first]->Fill(oc.second);
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
