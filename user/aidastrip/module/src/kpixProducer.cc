#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
//--> kpix lib:
#include "KpixControl.h"
#include "UdpLink.h"

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class kpixReader : public eudaq::Producer {
  public:
  kpixReader(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixReader");
  bool stop; //kpix
   
private:
  bool m_flag_ts;
  bool m_flag_tg;
  uint32_t m_plane_id;
  std::chrono::milliseconds m_ms_busy;
  std::thread m_thd_run;
  bool m_exit_of_run;

};
//----------DOC-MARK-----END*DEC-----DOC-MARK---------

//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<kpixReader, const std::string&, const std::string&>(kpixReader::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------

//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
kpixReader::kpixReader(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}

//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void kpixReader::DoInitialise(){

  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("DEV_LOCK_PATH", "ex0lockfile.txt");

}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void kpixReader::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("PLANE_ID", 0);
  m_ms_busy = std::chrono::milliseconds(conf->Get("DURATION_BUSY_MS", 1000));
  m_flag_ts = conf->Get("ENABLE_TIEMSTAMP", 0);
  m_flag_tg = conf->Get("ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. \n  ==> Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void kpixReader::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&kpixReader::Mainloop, this);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void kpixReader::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void kpixReader::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_thd_run = std::thread();
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void kpixReader::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void kpixReader::Mainloop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> position(0, x_pixel*y_pixel-1);
  std::uniform_int_distribution<uint32_t> signal(0, 255);
  while(!m_exit_of_run){
    auto ev = eudaq::Event::MakeUnique("Ex0Raw");  
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if(m_flag_ts){
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());

      std::cout<<"[Loop]: Timestamp enabled to use\n"; //wmq
      std::cout<<"[Loop]: m_ms_busy ==>    "<< m_ms_busy.count()<<"\n"; //wmq
      std::cout<<"[Loop]: time begin at ==> "<<du_ts_beg_ns.count()<<"ns; ends at ==> "<<du_ts_end_ns.count()<<"ns\n";//wmq

    }
    if(m_flag_tg){
      ev->SetTriggerN(trigger_n);
      std::cout<<"[Loop]: TriggerNumber enabled to use\n"; //wmq
      std::cout<<" ==> Trigger #"<<trigger_n<<"\n"; //wmq
    }

    
    std::vector<uint8_t> hit(x_pixel*y_pixel, 0);
    hit[position(gen)] = signal(gen);
    std::vector<uint8_t> data;
    data.push_back(x_pixel);
    data.push_back(y_pixel);
    data.insert(data.end(), hit.begin(), hit.end());
    
    uint32_t block_id = m_plane_id;
    ev->AddBlock(block_id, data);

    //--> start of evt print <--// wmq dev
    std::filebuf fb;
    fb.open("out.txt", std::ios::out|std::ios::app);
    std::ostream os(&fb);
    ev->Print(os, 0);
    //--> end of evt print
    
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);

  }//--> end of while loop

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------

