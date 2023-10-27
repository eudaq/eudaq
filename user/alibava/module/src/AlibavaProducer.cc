#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#include <string>
#ifndef _WIN32
#include <sys/file.h>
#endif
//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class AlibavaProducer : public eudaq::Producer {
  public:
  AlibavaProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("AlibavaProducer");
private:
  double m_hv;
  std::string m_hv_prog;
  std::string m_ali_prog;
  std::string m_ali_ini;
  std::string m_ali_out;
  uint32_t m_ali_events;
  uint32_t m_ali_ped_events;
  std::chrono::milliseconds m_ms_busy;
  bool m_exit_of_run;
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<AlibavaProducer, const std::string&, const std::string&>(AlibavaProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
AlibavaProducer::AlibavaProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol),  m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void AlibavaProducer::DoInitialise(){
    auto ini = GetInitConfiguration();
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void AlibavaProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_hv = conf->Get("HV", 0);
  m_hv_prog = conf->Get("HV_PROG", "");
  m_ali_prog = conf->Get("ALIBAVA_PROG", "");
  m_ali_events = conf->Get("ALIBAVA_EVENTS", 1);
  m_ali_ped_events = conf->Get("ALIBAVA_PED_EVENTS", 10000);
  m_ali_ini = conf->Get("ALIBAVA_INI", "");
  m_ali_out = conf->Get("ALIBAVA_OUT", "");
  system((m_hv_prog+std::string(" ")+std::to_string(m_hv)).c_str());
  system((std::string("echo ")+m_ali_prog+std::string(" --no-gui --ascii  --nevts=")+std::to_string(m_ali_ped_events)+std::string(" --out=")+m_ali_out+std::string("/run01")+std::to_string(GetRunNumber()+1)+std::string("_")+std::to_string(0-m_hv)+std::string("V.ped ")+m_ali_ini).c_str());
  system((m_ali_prog+std::string(" --no-gui --ascii  --nevts=")+std::to_string(m_ali_ped_events)+std::string(" --out=")+m_ali_out+std::string("/run01")+std::to_string(GetRunNumber()+1)+std::string("_")+std::to_string(0-m_hv)+std::string("V.ped ")+m_ali_ini).c_str());
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void AlibavaProducer::DoStartRun(){
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void AlibavaProducer::DoStopRun(){
  // system((std::string("echo killall")+m_ali_prog+std::string("-bin")).c_str());
  // system((std::string("killall")+m_ali_prog+std::string("-bin")).c_str());
  m_exit_of_run = true;
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void AlibavaProducer::DoReset(){
  m_exit_of_run = true;
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void AlibavaProducer::DoTerminate(){
  m_exit_of_run = true;
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void AlibavaProducer::RunLoop(){

  system((std::string("echo ")+m_ali_prog+std::string(" --no-gui --ascii --source --nevts=")+std::to_string(m_ali_events)+std::string(" --out=")+m_ali_out+std::string("/run00")+std::to_string(GetRunNumber())+std::string(".dat ")+m_ali_ini).c_str());
  // system blocks until alibava is done with data taking
  system((m_ali_prog+std::string(" --no-gui --ascii --source --nevts=")+std::to_string(m_ali_events)+std::string(" --out=")+m_ali_out+std::string("/run00")+std::to_string(GetRunNumber())+std::string(".dat ")+m_ali_ini).c_str());

  // send two fake events at end of data taking for scan to trigger alibava exit
  auto ev = eudaq::Event::MakeUnique("AlibavaRaw");
  ev->SetTriggerN(1);
  SendEvent(std::move(ev));
  ev = eudaq::Event::MakeUnique("AlibavaRaw");
  ev->SetTriggerN(2);
  SendEvent(std::move(ev));

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
