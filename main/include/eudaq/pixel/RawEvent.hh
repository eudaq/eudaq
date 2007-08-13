#ifndef EUDAQ_INCLUDED_pixel_RawEvent
#define EUDAQ_INCLUDED_pixel_RawEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

  class RawEvent : public Event {
    EUDAQ_DECLARE_EVENT(RawEvent);
  public:
    virtual void Serialize(Serializer &) const;
    virtual ~RawEvent() {}
    virtual void Print(std::ostream &os) const { Event::Print(os); }
    RawEvent(unsigned run, unsigned event,
             const std::vector<unsigned short> & pixels);
    RawEvent(Deserializer &);
  private:
    unsigned m_width, m_height;
    std::vector<unsigned short> m_data;
  };

}

#endif // EUDAQ_INCLUDED_pixel_RawEvent
