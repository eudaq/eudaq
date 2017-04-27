#ifndef Processor_eventSelectors_h__
#define Processor_eventSelectors_h__
#include "Processor_inspector.hh"
namespace eudaq {



class DLLEXPORT select_events :public Processor_Inspector {
public:
  select_events(const std::vector<unsigned>& eventsOfIntresst, bool doBore = true, bool doEORE = true);

  virtual ReturnParam inspectEvent(const Event&ev, ConnectionName_ref con) override;

  bool m_do_bore = false, m_do_eore = false;
  std::vector<unsigned> m_eventsOfInterest;
};
}
#endif // Processor_eventSelectors_h__
