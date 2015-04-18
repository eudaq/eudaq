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
    using counter_t = unsigned;

    PointerEvent(reference_t filePointer, counter_t counter);
    
    PointerEvent(Deserializer &);

      virtual void Print(std::ostream & os) const { 
        os << "PointerEvent \n";
        os << "reference = " << getReference() <<"\n" << "counter = "<< getCounter() <<"\n";
      
      };
    virtual void Serialize(Serializer &) const;

    /// Return the type string.
    virtual std::string GetSubType() const { return ""; }

    const reference_t& getReference()const {
      return m_timestamp[0];
    }
    const counter_t& getCounter()const {
      return m_eventnumber;
    }
  private:

      
 
  };



}

#endif // EUDAQ_INCLUDED_RawDataEvent
