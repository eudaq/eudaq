#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include <string>

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<RawDataEvent, Deserializer&>(RawDataEvent::m_id_factory);
    auto dummy7 = Factory<Event>::Register<RawDataEvent, const std::string&, const uint32_t&, const uint32_t&, const uint32_t&>(RawDataEvent::m_id_factory);
    auto dummy2 = Factory<Event>::Register<RawDataEvent, const uint32_t&, const uint32_t&, const uint32_t&>(RawDataEvent::m_id_factory);
    // auto &cccout = (std::cout<<"Resisted to Factory<"
    // 		    <<std::hex<<dummy2<<std::dec
    // 		    <<"> ID "<<RawDataEvent::m_id_factory<<std::endl);
  }

  std::shared_ptr<RawDataEvent> RawDataEvent::MakeShared(const std::string& dspt,
							 uint32_t dev_n, uint32_t run_n,
							 uint32_t ev_n){
    //TODO: WARNING, mightbe not fit the template, <ARG, > should be added.
    //See MakeUnique below
    EventSP ev = Factory<Event>::MakeShared(RawDataEvent::m_id_factory, 
					    cstr2hash("RawDataEvent"),
					    run_n, dev_n);
    auto rawev = std::dynamic_pointer_cast<RawDataEvent>(ev);
    rawev->SetExtendWord(eudaq::str2hash(dspt));
    rawev->SetDescription(dspt);
    rawev->SetEventN(ev_n);
    return rawev;
  }

  EventUP RawDataEvent::MakeUnique(const std::string& dspt){
    uint32_t dev_n = 0;
    uint32_t run_n = 0;
    EventUP ev = Factory<Event>::MakeUnique<const uint32_t&, const uint32_t&,
					    const uint32_t&>
      (RawDataEvent::m_id_factory, cstr2hash("RawDataEvent"), run_n, dev_n);

    ev->SetExtendWord(eudaq::str2hash(dspt));
    ev->SetDescription(dspt);
    return ev;
  }
    
  RawDataEvent::RawDataEvent(uint32_t type, uint32_t run_n, uint32_t ev_n)
    : Event(type, run_n, 0){
    SetEventN(ev_n);
  }


  RawDataEvent::RawDataEvent(const std::string& dspt, uint32_t dev_n, uint32_t run_n, uint32_t ev_n)
    : Event(cstr2hash("RawDataEvent"), run_n, dev_n){
    SetEventN(ev_n);
    SetExtendWord(eudaq::str2hash(dspt));
    SetDescription(dspt);
  }

  RawDataEvent::RawDataEvent(Deserializer &ds) : Event(ds) {
  }

  
}
