#include "eudaq/Producer.hh"

#include "EudetTluController.hh"

#include <iostream>
#include <ostream>
#include <cctype>
#include <memory>

using tlu::TLUController;
using tlu::TLU_PMTS;
using tlu::PMT_VCNTL_DEFAULT;
using tlu::Timestamp2Seconds;
// #ifdef _WIN32
// ZESTSC1_ERROR_FUNC ZestSC1_ErrorHandler = NULL;
// // Windows needs some parameters for this. i dont know where it will
// // be called so we need to check it in future
// char *ZestSC1_ErrorStrings[] = {"bla bla", "blub"};
// #endif

class EudetTluProducer : public eudaq::Producer {
public:
  EudetTluProducer(const std::string name, const std::string &runcontrol); //TODO: check para no ref
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoStatus() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("EudetTluProducer");
private:
  bool m_exit_of_run;
  uint32_t m_trigger_n;
  uint64_t m_ts_last;

  uint32_t trigger_interval, dut_mask, veto_mask, and_mask, or_mask,
    pmtvcntl[TLU_PMTS], pmtvcntlmod;
  uint32_t strobe_period, strobe_width;
  uint32_t enable_dut_veto, handshake_mode;
  uint32_t trig_rollover, readout_delay;
  bool timestamps, timestamp_per_run;
  std::shared_ptr<TLUController> m_tlu;
  std::string pmt_id[TLU_PMTS];
  double pmt_gain_error[TLU_PMTS], pmt_offset_error[TLU_PMTS];

  std::string m_STATUS;
  uint32_t m_TIMESTAMP;
  uint32_t m_LASTTIME;
  uint32_t m_PARTICLES;
  uint32_t m_SCALE_0;
  uint32_t m_SCALE_1;
  uint32_t m_SCALE_2;
  uint32_t m_SCALE_3;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<EudetTluProducer, const std::string&, const std::string&>(EudetTluProducer::m_id_factory);
}

EudetTluProducer::EudetTluProducer(const std::string name, const std::string &runcontrol)
  : eudaq::Producer(name, runcontrol), m_trigger_n(0),
  trigger_interval(0), dut_mask(0), veto_mask(0), and_mask(255),
  or_mask(0), pmtvcntlmod(0), strobe_period(0), strobe_width(0),
  enable_dut_veto(0), trig_rollover(0), readout_delay(100),
  timestamps(true), timestamp_per_run(false), m_ts_last(0){
  for (int i = 0; i < TLU_PMTS; i++) {
    pmtvcntl[i] = PMT_VCNTL_DEFAULT;
    pmt_id[i] = "<unknown>";
    pmt_gain_error[i] = 1.0;
    pmt_offset_error[i] = 0.0;
  }
}

void EudetTluProducer::RunLoop(){
  bool isbegin = true;
  if (timestamp_per_run)
    m_tlu->ResetTimestamp();
  eudaq::mSleep(5000);
  m_tlu->ResetTriggerCounter();
  if (timestamp_per_run)
    m_tlu->ResetTimestamp();
  m_tlu->ResetScalers();
  m_tlu->Update(timestamps);
  m_tlu->Start();

  m_ts_last = 0;
  while(true){
    eudaq::mSleep(readout_delay);
    m_tlu->Update(timestamps); // get new events
    if (trig_rollover > 0 && m_tlu->GetTriggerNum() > trig_rollover) {
      bool inhibit = m_tlu->InhibitTriggers();
      m_tlu->ResetTriggerCounter();
      m_tlu->InhibitTriggers(inhibit);
    }

    m_TIMESTAMP = Timestamp2Seconds(m_tlu->GetTimestamp());
    m_LASTTIME = Timestamp2Seconds(m_ts_last);
    m_PARTICLES =  m_tlu->GetParticles();
    m_STATUS = m_tlu->GetStatusString();
    m_SCALE_0 = m_tlu->GetScaler(0);
    m_SCALE_1 = m_tlu->GetScaler(1);
    m_SCALE_2 = m_tlu->GetScaler(2);
    m_SCALE_3 = m_tlu->GetScaler(3);
    
    for (size_t i = 0; i < m_tlu->NumEntries(); ++i) {
      auto tlu_entry =  m_tlu->GetEntry(i);
      auto ev = eudaq::Event::MakeUnique("TluRawDataEvent");
      m_trigger_n = tlu_entry.Eventnum();
      uint64_t ts_raw = tlu_entry.Timestamp();
      // uint64_t ts_ns = ts_raw; //TODO, convert to ns
      // uint64_t ts_ns = ts_raw / (tlu::TLUFREQUENCY * tlu::TLUFREQUENCYMULTIPLIER);
      // uint64_t ts_ns = ts_raw*125000/48001; //TODO: 
      uint64_t ts_ns = ts_raw*125/48;
      ev->SetTimestamp(ts_ns, ts_ns+8, false);//TODO, duration
      ev->SetTriggerN(m_trigger_n);
      if (m_ts_last && (m_trigger_n < 10 || m_trigger_n % 1000 == 0)) {
	uint64_t delta_ts = ts_ns - m_ts_last;
	float freq = 1. / Timestamp2Seconds(delta_ts);
	std::cout << "  " << tlu_entry << ", diff=" << delta_ts
		  << ", freq=" << freq << std::endl;
      }
      ev->SetTag("trigger", tlu_entry.trigger2String()); //TriggerParttern
      if(isbegin){
	isbegin = false;
	ev->SetBORE();
	ev->SetTag("TimestampZero", eudaq::to_string(m_tlu->TimestampZero()));
      }
      SendEvent(std::move(ev));
      m_ts_last = ts_ns;
    }

    if(m_exit_of_run){
      break;
    }
  }
  m_tlu->Stop();
}

void EudetTluProducer::DoConfigure() {
  auto conf = GetConfiguration();  
  if(!conf)
    EUDAQ_THROW("No configuration exists for tlu");
  trigger_interval = conf->Get("TriggerInterval", 0);
  dut_mask = conf->Get("DutMask", 2);
  and_mask = conf->Get("AndMask", 0xff);
  or_mask = conf->Get("OrMask", 0);
  strobe_period = conf->Get("StrobePeriod", 0);
  strobe_width = conf->Get("StrobeWidth", 0);
  enable_dut_veto = conf->Get("EnableDUTVeto", 0);
  handshake_mode = conf->Get("HandShakeMode", 63);
  veto_mask = conf->Get("VetoMask", 0);
  trig_rollover = conf->Get("TrigRollover", 0);
  timestamps = conf->Get("Timestamps", 1);

  int errorhandler = conf->Get("ErrorHandler", 2);

  for (int i = 0; i < TLU_PMTS; i++){
    pmtvcntl[i] = (unsigned)conf->Get("PMTVcntl" + std::to_string(i + 1),
				      "PMTVcntl", PMT_VCNTL_DEFAULT);
    pmt_id[i] = conf->Get("PMTID" + std::to_string(i + 1), "<unknown>");
    pmt_gain_error[i] = conf->Get("PMTGainError" + std::to_string(i + 1), 1.0);
    pmt_offset_error[i] = conf->Get("PMTOffsetError" + std::to_string(i + 1), 0.0);
  }
  

  pmtvcntlmod = conf->Get("PMTVcntlMod", 0); // If 0, it's a standard TLU;
  // if 1, the DAC output voltage
  // is doubled
  readout_delay = conf->Get("ReadoutDelay", 1000);
  timestamp_per_run = conf->Get("TimestampPerRun", 0);
  // ***
  
  if(!m_tlu)
    m_tlu = std::make_shared<TLUController>(errorhandler);
  m_tlu->SetDebugLevel(conf->Get("DebugLevel", 0));
  m_tlu->SetFirmware(conf->Get("BitFile", ""));
  m_tlu->SetVersion(conf->Get("Version", 0));
  m_tlu->Configure();
  for (int i = 0; i < tlu::TLU_LEMO_DUTS; ++i) {
    m_tlu->SelectDUT(conf->Get("DUTInput", "DUTInput" + std::to_string(i), "RJ45"), 1 << i,
		     false);
  }
  m_tlu->SetTriggerInterval(trigger_interval);
  m_tlu->SetPMTVcntlMod(pmtvcntlmod);
  m_tlu->SetPMTVcntl(pmtvcntl, pmt_gain_error, pmt_offset_error);
  m_tlu->SetDUTMask(dut_mask);
  m_tlu->SetVetoMask(veto_mask);
  m_tlu->SetAndMask(and_mask);
  m_tlu->SetOrMask(or_mask);
  m_tlu->SetStrobe(strobe_period, strobe_width);
  m_tlu->SetEnableDUTVeto(enable_dut_veto);
  m_tlu->SetHandShakeMode(handshake_mode);
  m_tlu->SetTriggerInformation(USE_TRIGGER_INPUT_INFORMATION);
  m_tlu->ResetTimestamp();

  eudaq::mSleep(1000);
  m_tlu->Update(timestamps);

}

void EudetTluProducer::DoStartRun(){
  m_exit_of_run = false;
}

void EudetTluProducer::DoStopRun(){
  m_exit_of_run = true;
}

void EudetTluProducer::DoTerminate(){
  m_exit_of_run = true;
}

void EudetTluProducer::DoReset(){
  m_exit_of_run = true;
  m_tlu.reset();
}

void EudetTluProducer::DoStatus(){
  SetStatusTag("TRIG", std::to_string(m_trigger_n));
  SetStatusTag("TIMESTAMP", std::to_string(m_TIMESTAMP));
  SetStatusTag("LASTTIME", std::to_string(m_LASTTIME));
  SetStatusTag("PARTICLES", std::to_string(m_PARTICLES));
  SetStatusTag("STATUS", m_STATUS);
  SetStatusTag("SCALER", std::to_string(m_SCALE_0)+":"+std::to_string(m_SCALE_1)+":"
	       +std::to_string(m_SCALE_2)+":"+std::to_string(m_SCALE_3));
}
