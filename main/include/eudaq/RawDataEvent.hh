#ifndef EUDAQ_INCLUDED_RawDataEvent
#define EUDAQ_INCLUDED_RawDataEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

  /** An Event type consisting of just a vector of bytes.
   *
   */
  class RawDataEvent : public Event {
    EUDAQ_DECLARE_EVENT(RawDataEvent);
  public:
    typedef std::vector<unsigned char> data_t;
    RawDataEvent(std::string type, unsigned run, unsigned event);
    RawDataEvent(Deserializer &);
    template <typename T>
    void AddBlock(const std::vector<T> & data) {
      m_data.push_back(make_vector(data));
    }
    template <typename T>
    void AddBlock(const T * data, size_t bytes) {
      m_data.push_back(make_vector(data, bytes));
    }
    virtual void Print(std::ostream &) const;
    static RawDataEvent BORE(unsigned run) {
      return RawDataEvent(run);
    }
    static RawDataEvent EORE(unsigned run, unsigned event) {
      return RawDataEvent(run, event);
    }
    virtual void Serialize(Serializer &) const;
  private:
    RawDataEvent(unsigned run, unsigned event = 0)
      : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE)
      {}
    template <typename T>
    static data_t make_vector(const T * data, size_t bytes) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(data);
      return data_t(ptr, ptr + bytes);
    }
    template <typename T>
    static data_t make_vector(const std::vector<T> & data) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(&data[0]);
      return data_t(ptr, ptr + data.size() * sizeof(T));
    }
    std::string m_type;
    std::vector<data_t> m_data;
  };

}

#endif // EUDAQ_INCLUDED_RawDataEvent
