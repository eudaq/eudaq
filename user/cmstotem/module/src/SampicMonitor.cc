#include "eudaq/Monitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TGraph.h"

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
  std::unique_ptr<TFile> m_file;
  eudaq::TTreeEventSP m_stdev;

  bool m_en_print;
  bool m_sampic_last_sample;
  int m_sampic_num_baseline;

  float m_sampic_samples_time[64];

  std::unique_ptr<MonitorWindow> m_main;
  std::future<void> m_daemon;
  std::future<void> m_test_daemon;

  unsigned long long m_num_evt_mon = 0ull;

  TGraph* m_g_trg_time, *m_g_evt_time;
  TH1D* m_h_maxamp_per_chan[32], *m_h_occup_per_chan[32];
  TGraph* m_g_last_sample[32];
  TH1D* m_h_occup_allchan;

  void test(TH1D* h){h->Fill(rand()/RAND_MAX*10.);usleep(1000);}
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

  // auxiliary info
  for (uint16_t i = 0; i < 64; ++i)
    m_sampic_samples_time[i] = i*sampic::kSamplingPeriod;

  auto hist = m_main->Book<TH1D>("test", "Test", "test_hist", "testing;x;y", 100, 0., 10.);
  if (!m_test_daemon.valid())
    m_test_daemon = std::async(std::launch::async, &SampicMonitor::test, this, hist);
}

void SampicMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
  m_main->ResetCounters();
}

void SampicMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  //conf->Print(std::cout);
  m_en_print = conf->Get("SAMPIC_ENABLE_PRINT", 1);
  m_sampic_last_sample = conf->Get("SAMPIC_SHOW_LAST_SAMPLE", 0);
  m_sampic_num_baseline = conf->Get("SAMPIC_NUM_BASELINE", 10);

  // book all monitoring elements
  m_g_trg_time = m_main->Book<TGraph>("trig_vs_time", "Trigger time");
  m_g_evt_time = m_main->Book<TGraph>("event_vs_time", "Event time");
  m_h_occup_allchan = m_main->Book<TH1D>("channels_occup", "Occupancy, all channels", "ch_occup", "Channels occupancy;Channel number;Entries", 32, 0., 32.);
  for (size_t i = 0; i < 32; ++i) {
    m_h_maxamp_per_chan[i] = m_main->Book<TH1D>(Form("ch%d/max_ampl", i), "Max amplitude", Form("max_ampl_ch%d", i), Form("Channel %d;Maximum amplitude (V);Entries", i), 100, 0., 1.);
    m_h_occup_per_chan[i] = m_main->Book<TH1D>(Form("ch%d/occupancy", i), "Occupancy", Form("occup_ch%d", i), Form("Channel %d;Samples multiplicity;Entries", i), 20, 0., 20.);
    if (m_sampic_last_sample)
      m_g_last_sample[i] = m_main->Book<TGraph>(Form("ch%d/last_sample", i), "Last sample");
  }


  m_main->SetStatus(MonitorWindow::Status::configured);
  m_main->ResetCounters();
}

void SampicMonitor::DoStartRun(){
  m_main->SetRunNumber(GetRunNumber());
  m_file.reset(TFile::Open(Form("run%d.root", GetRunNumber()), "recreate"));
  m_stdev.reset(new TTree);
  m_main->SetStatus(MonitorWindow::Status::running);
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);

  // update the counters
  m_main->SetLastEventNum(ev->GetEventN());
  m_main->SetMonitoredEventsNum(++m_num_evt_mon);

  auto event = std::make_shared<eudaq::SampicEvent>(*ev);

  m_main->Get<TH1D>("test")->Fill(ev->GetEventN());
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
    float max_ampl = 100.;
    size_t i = 0;
    for (const auto& s_val : samples) {
      if (m_sampic_last_sample)
        m_g_last_sample[ch_id]->SetPoint(i++, m_sampic_samples_time[i], s_val-baseline);
      if (s_val < max_ampl)
        max_ampl = s_val;
    }
    m_h_maxamp_per_chan[ch_id]->Fill(max_ampl-baseline);
  }
  for (const auto& oc : occup_chan)
    m_h_occup_per_chan[oc.first]->Fill(oc.second);
}

void SampicMonitor::DoStopRun(){
  m_stdev->Write("monitor");
  m_file->Close();
  m_main->SetStatus(MonitorWindow::Status::configured);
}

void SampicMonitor::DoReset(){
  m_main->SetStatus(MonitorWindow::Status::idle);
  m_main->ResetCounters();
}

void SampicMonitor::DoTerminate(){
  TApplication::Terminate(1);
}
