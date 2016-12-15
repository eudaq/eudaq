#include "eudaq/DataCollector.hh"

namespace eudaq {
  class DirectSaveDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoReceive(const ConnectionInfo &id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("DirectSaveDataCollector");
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<DirectSaveDataCollector, const std::string&, const std::string&>
      (DirectSaveDataCollector::m_id_factory);
  }

  void DirectSaveDataCollector::DoReceive(const ConnectionInfo &id, EventUP ev){
    WriteEvent(std::move(ev));
  }
}
