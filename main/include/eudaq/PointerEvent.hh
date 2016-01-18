#ifndef EUDAQ_INCLUDED_PointerEvent
#define EUDAQ_INCLUDED_PointerEvent

#include <sstream>
#include <memory>
#include <vector>
#include "eudaq/Event.hh"
#include "eudaq/Platform.hh"
namespace eudaq {

  /** An Event type consisting of just a vector of bytes.
   *
   */
  class DLLEXPORT PointerEvent : public Event {
    EUDAQ_DECLARE_EVENT(PointerEvent);
    public:
    using reference_t = uint64_t;
    using counter_t = uint32_t;

    PointerEvent(reference_t filePointer, counter_t counter);
    
    PointerEvent(Deserializer &);

    virtual void Print(std::ostream & os) const;
    virtual void Print(std::ostream & os,size_t offset) const;
    virtual void Serialize(Serializer &) const;

    virtual std::string GetSubType() const;

    reference_t getReference()const;
    counter_t getCounter()const;
    void setReference(reference_t ref);
    void setCounter(counter_t newCounter);
  private:
    
    reference_t m_ref;
    counter_t m_counter;
 
  };



}

#endif // EUDAQ_INCLUDED_RawDataEvent
