#include "eudaq/pixel/RawEvent.hh"

namespace eudaq {

  EUDAQ_DEFINE_EVENT(RawEvent, str2id("PRAW"));

  RawEvent::RawEvent(unsigned run, unsigned event,
                     const std::vector<unsigned short> & pixels) :
    Event(run, event),
    m_width(1 /* get_default_width */),
    m_height(1 /* get_default_height */),
    m_data(pixels)
  {
    // TODO: do it
  }

  RawEvent::RawEvent(Deserializer & ds) :
    Event(ds)
  {
    // TODO: do it
  }

  void RawEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    // TODO: do it
  }

}
