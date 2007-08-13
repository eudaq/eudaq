#ifndef EUDAQ_INCLUDED_RawBytesEvent
#define EUDAQ_INCLUDED_RawBytesEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

  /** An Event type consisting of just a vector of bytes.
   *
   */
  class RawBytesEvent : public Event {
    EUDAQ_DECLARE_EVENT(RawBytesEvent);
  public:
    typedef std::vector<unsigned char> vec_t;
    virtual void Serialize(Serializer &) const;
    template <typename T>
    RawBytesEvent(unsigned run, unsigned event, const std::vector<T> & data) :
      Event(run, event),
      m_data(make_vector(data)) {}
    template <typename T>
    RawBytesEvent(unsigned run, unsigned event, const T * data, size_t bytes) :
      Event(run, event),
      m_data(make_vector(data, bytes)) {}
    RawBytesEvent(Deserializer &);
    virtual void Print(std::ostream &) const;
    static RawBytesEvent BORE(unsigned run) {
      return RawBytesEvent(run);
    }
    static RawBytesEvent EORE(unsigned run, unsigned event) {
      return RawBytesEvent(run, event);
    }
  private:
    RawBytesEvent(unsigned run, unsigned event = 0)
      : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE)
      {}
    template <typename T>
    static vec_t make_vector(const T * data, size_t bytes) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(data);
      return vec_t(ptr, ptr + bytes);
    }
    template <typename T>
    static vec_t make_vector(const std::vector<T> & data) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(&data[0]);
      return vec_t(ptr, ptr + data.size() * sizeof(T));
    }
    vec_t m_data;
  };

}

#endif // EUDAQ_INCLUDED_RawBytesEvent
