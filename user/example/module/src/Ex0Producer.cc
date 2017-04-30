#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
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
  bool m_flag_ts;
  bool m_flag_tg;
  std::ifstream m_ifile;
  std::string m_dummy_data_path;
  std::chrono::milliseconds m_ms_busy;
  
  std::thread m_thd_run;
  bool m_exit_of_run;
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<Ex0Producer, const std::string&, const std::string&>(Ex0Producer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
Ex0Producer::Ex0Producer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void Ex0Producer::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::ofstream ofile;
  std::string dummy_string;
  dummy_string = ini->Get("DUMMY_STRING", dummy_string);
  m_dummy_data_path = ini->Get("DUMMY_FILE_PATH", "ex0dummy.txt");
  ofile.open(m_dummy_data_path);
  if(!ofile.is_open()){
    EUDAQ_THROW("dummy data file (" + m_dummy_data_path +") can not open for writing");
  }
  ofile << dummy_string;
  ofile.close();
}
//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void Ex0Producer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_ms_busy = std::chrono::milliseconds(conf->Get("DURATION_BUSY_MS", 1000));
  m_flag_ts = conf->Get("ENABLE_TIEMSTAMP", 0);
  m_flag_tg = conf->Get("ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void Ex0Producer::DoStartRun(){
  m_exit_of_run = false;

  m_ifile.open(m_dummy_data_path);
  if(!m_ifile.is_open()){
    EUDAQ_THROW("dummy data file (" + m_dummy_data_path +") can not open for reading");
  }
  m_thd_run = std::thread(&Ex0Producer::Mainloop, this);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void Ex0Producer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  m_ifile.close();
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void Ex0Producer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_ifile.close();
  m_thd_run = std::thread();
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void Ex0Producer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void Ex0Producer::Mainloop(){  
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  while(m_exit_of_run){
    auto ev = eudaq::Event::MakeUnique("Ex0Event");
    
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if(m_flag_ts){
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
    }
    if(m_flag_tg)
      ev->SetTriggerN(trigger_n);

    std::vector<uint8_t> data_a(100, 0);
    uint32_t block_id_a = 0;
    ev->AddBlock(block_id_a, data_a);
    std::filebuf* pbuf = m_ifile.rdbuf();
    size_t len = pbuf->pubseekoff(0,m_ifile.end, m_ifile.in);
    pbuf->pubseekpos (0,m_ifile.in);
    std::vector<uint8_t> data_b(len);
    pbuf->sgetn ((char*)&(data_b[0]), len);
    uint32_t block_id_b = 2;
    ev->AddBlock(block_id_b, data_b);

    if(trigger_n == 0){
      ev->SetBORE();
    }
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);
  }
}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
