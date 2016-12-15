#include "eudaq/DataCollector.hh"

namespace eudaq {
  class EventnumberSyncDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoReceive(const ConnectionInfo &id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("EventnumberSyncDataCollector");
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<EventnumberSyncDataCollector, const std::string&, const std::string&>
      (EventnumberSyncDataCollector::m_id_factory);
  }

  void EventnumberSyncDataCollector::DoReceive(const ConnectionInfo &id, EventUP ev){
    WriteEvent(std::move(ev));
  }
}
