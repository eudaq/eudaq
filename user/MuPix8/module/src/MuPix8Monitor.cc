#include "eudaq/Monitor.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>

class MuPix8Monitor : public eudaq::Monitor {
public:
  MuPix8Monitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoReceive(eudaq::EventSP ev) override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("MuPix8Monitor");
  
private:
  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<MuPix8Monitor, const std::string&, const std::string&>(MuPix8Monitor::m_id_factory);
}

MuPix8Monitor::MuPix8Monitor(const std::string & name, const std::string & runcontrol)
  :eudaq::Monitor(name, runcontrol){  
}

void MuPix8Monitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
}

void MuPix8Monitor::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_en_print = conf->Get("MuPix8_ENABLE_PRINT", 1);
  m_en_std_converter = conf->Get("MuPix8_ENABLE_STD_CONVERTER", 0);
  m_en_std_print = conf->Get("MuPix8_ENABLE_STD_PRINT", 0);
}

void MuPix8Monitor::DoStartRun(){
}

void MuPix8Monitor::DoStopRun(){
}

void MuPix8Monitor::DoReset(){
}

void MuPix8Monitor::DoTerminate(){
}

void MuPix8Monitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);
  if(m_en_std_converter){
    auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(ev);
    if(!stdev){
      stdev = eudaq::StandardEvent::MakeShared();
      eudaq::StdEventConverter::Convert(ev, stdev, nullptr); //no conf
      if(m_en_std_print)
	stdev->Print(std::cout);
    }
  }
}
