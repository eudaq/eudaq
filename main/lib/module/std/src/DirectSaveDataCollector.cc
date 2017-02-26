#include "eudaq/DataCollector.hh"
#include <iostream>
namespace eudaq {
  class DirectSaveDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoConfigure() override;
    void DoReceive(ConnectionSPC id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("DirectSaveDataCollector");

  private:
    uint32_t m_noprint;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
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
  
  void DirectSaveDataCollector::DoReceive(ConnectionSPC id, EventUP ev){
    if(!m_noprint)
      ev->Print(std::cout);
    WriteEvent(std::move(ev));
  }
}
