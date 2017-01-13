#include "eudaq/DataCollector.hh"
#include <iostream>
namespace eudaq {
  class DirectSaveDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoReceive(std::shared_ptr<const ConnectionInfo> id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("DirectSaveDataCollector");
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<DirectSaveDataCollector, const std::string&, const std::string&>
      (DirectSaveDataCollector::m_id_factory);
  }

  void DirectSaveDataCollector::DoReceive(std::shared_ptr<const ConnectionInfo> id, EventUP ev){
    ev->Print(std::cout);
    WriteEvent(std::move(ev));
  }
}
