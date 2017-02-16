#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include <iostream>
#include <ratio>
#include <chrono>
#include <thread>

class Ex0Producer : public eudaq::Producer {
  public:
  Ex0Producer(const std::string & name, const std::string & runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("Ex0Producer");
private:
  //init
  bool m_flag_ts;
  bool m_flag_tg;

  uint32_t m_event_amount;
  std::chrono::milliseconds m_ms_busy;
  std::thread m_thd_run;
  bool m_exit_of_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<Ex0Producer, const std::string&, const std::string&>(Ex0Producer::m_id_factory);
}


Ex0Producer::Ex0Producer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){
  
}

void Ex0Producer::DoInitialise(){
  auto conf = GetInitConfiguration();
  if(conf){
    conf->Print(std::cout);
  }
}

void Ex0Producer::DoConfigure(){
  auto conf = GetConfiguration();
  if(conf){
    conf->Print(std::cout);
    m_ms_busy = std::chrono::milliseconds(conf->Get("DURATION_BUSY_MS", 1000));
    m_event_amount = conf->Get("EVENT_AMOUNT", 1000);

    std::string role;
    role = conf->Get("DEVICE_ROLE", std::string());
    std::cout<<"DEVICE_ROLE = "<< role<<std::endl;
    if(role == "TLU" || role == "TRIGGER_GENERATOR"){
      m_flag_ts = true;
      m_flag_tg = true;
    }
    else if(role == "TRIGGER_ACCEPTOR_TIMESTAMP"){
      m_flag_ts = true;
      m_flag_tg = false;
    }
    else if(role == "TRIGGER_ACCEPTOR_TRIGGERNUM"){
      m_flag_ts = false;
      m_flag_tg = true;
    }
    else{
      EUDAQ_WARN("no item DEVICE_ROLE in config file, setting to default TRIGGER_ACCEPTOR_TRIGGERNUM");
      m_flag_ts = false;
      m_flag_tg = true;
    }

  }
  else{
    m_ms_busy = std::chrono::milliseconds(1000);
    m_event_amount = 1000;
    
    EUDAQ_WARN("no item DEVICE_ROLE in config file, setting to default TRIGGER_ACCEPTOR_TRIGGERNUM");
    m_flag_ts = false;
    m_flag_tg = true;

  }
  
}

void Ex0Producer::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&Ex0Producer::Mainloop, this);
}

void Ex0Producer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void Ex0Producer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  //reset everything
  m_thd_run = std::thread();
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}

void Ex0Producer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void Ex0Producer::Mainloop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  auto tp_trigger_last = tp_start_run;
  uint32_t trigger_n = 0;
  while(true){
    if(trigger_n == m_event_amount)
      m_exit_of_run = true;
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
    std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
    auto ev = eudaq::Event::MakeUnique("ExampleA");

    if(m_flag_ts)
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
    if(m_flag_tg)
      ev->SetTriggerN(trigger_n);

    std::vector<uint8_t> data_a(1000, 0);
    std::vector<uint8_t> data_b(10, 2);
    ev->AddBlock(0,data_a);
    ev->AddBlock(1,data_b);
    if(trigger_n == 0){
      ev->SetBORE();
    }
    if(m_exit_of_run){
      ev->SetEORE();
      SendEvent(std::move(ev));
      break;
    }
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);
  }
}
