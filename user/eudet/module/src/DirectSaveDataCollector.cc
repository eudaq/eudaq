#include "eudaq/DataCollector.hh"
//#include <iostream>


class DirectSaveDataCollector :public eudaq::DataCollector{
public:
  using eudaq::DataCollector::DataCollector;
  void DoConfigure() override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DirectSaveDataCollector");

private:
  uint32_t m_noprint;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<DirectSaveDataCollector, const std::string&, const std::string&>
    (DirectSaveDataCollector::m_id_factory);
}

void DirectSaveDataCollector::DoConfigure(){
  m_noprint = 0;
  auto conf = GetConfiguration();
  if(conf){
    conf->Print();
    m_noprint = conf->Get("DISABLE_PRINT", 0);
  }
}
  
void DirectSaveDataCollector::DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev){
  if(!m_noprint)
    ev->Print(std::cout);
  WriteEvent(ev);
}
