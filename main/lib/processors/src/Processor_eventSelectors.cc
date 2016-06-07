#include "Processor_inspector.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "Processor_eventSelectors.hh"

#include "Processors.hh"
#include <algorithm>

namespace eudaq{
Processors::processor_i_up Processors::eventSelector(const std::vector<unsigned>& eventsOfIntresst, bool doBore, bool doEORE) {
  return __make_unique<select_events>(eventsOfIntresst, doBore, doEORE);
  

}

select_events::select_events(const std::vector<unsigned>& eventsOfIntresst,
    bool doBore /*= true*/, 
    bool doEORE /*= true*/) 
    : m_eventsOfInterest(eventsOfIntresst), 
    m_do_bore(doBore), 
    m_do_eore(doEORE) {

}

ProcessorBase::ReturnParam select_events::inspectEvent(const Event&ev, ConnectionName_ref con) {
  if (ev.IsBORE() && m_do_bore) {
    return ProcessorBase::sucess;
  }

  if (ev.IsEORE() && m_do_eore) {
    return ProcessorBase::sucess;
  }
  if (m_eventsOfInterest.empty()) {
    return ProcessorBase::sucess;
  }

  if (ev.GetEventNumber() > m_eventsOfInterest.back() && !m_do_eore) {
    return ProcessorBase::stop;
  }


  if (std::find(m_eventsOfInterest.begin(), m_eventsOfInterest.end(),
    ev.GetEventNumber()) != m_eventsOfInterest.end()) {
    return ProcessorBase::sucess;
  }
  return ProcessorBase::skip_event;
}

}
