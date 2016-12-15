#include "eudaq/DataCollector.hh"

namespace eudaq {
  class TimestampSyncDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoReceive(const ConnectionInfo &id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("TimestampSyncDataCollector");
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<TimestampSyncDataCollector, const std::string&, const std::string&>
      (TimestampSyncDataCollector::m_id_factory);
  }

  void TimestampSyncDataCollector::DoReceive(const ConnectionInfo &id, EventUP ev){
    WriteEvent(std::move(ev));
  }
}
