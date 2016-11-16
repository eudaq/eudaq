#ifndef EXPORTEVENTPS_HH
#define EXPORTEVENTPS_HH

#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Processor.hh"
#include <string>

namespace eudaq{
  class ExportEventPS : public Processor {
  public:
    ExportEventPS();
    void ProcessEvent(EventSPC ev) final override;
    void ProcessCommand(const std::string& key, const std::string& val) final override;
    EventSPC GetEvent();
    static constexpr const char* m_description = "ExportEventPS";
  private:
    std::deque<EventSPC> m_que_ev;
    std::mutex m_mtx_que;
  };
  
}

#endif
