#ifndef EUDAQ_INCLUDED_DEPFETEvent
#define EUDAQ_INCLUDED_DEPFETEvent

#include <vector>
#include <ostream>
#include "eudaq/DetectorEvent.hh"

namespace eudaq {

  class DEPFETBoard : public Serializable {
  public:
    typedef std::vector<unsigned char> vec_t;
    DEPFETBoard(unsigned id = 0) : m_id(id) {}
    template <typename T>
    DEPFETBoard(unsigned id, const std::vector<T> & data) :
      m_id(id), m_data(make_vector(data)) {}
    template <typename T>
    DEPFETBoard(unsigned id, const T * data, size_t bytes) :
      m_id(id), m_data(make_vector(data, bytes)) {}
    DEPFETBoard(Deserializer &);
    virtual void Serialize(Serializer &) const;
    //unsigned LocalEventNumber() const;
    //unsigned TLUEventNumber() const;
    //unsigned FrameNumber() const;
    //unsigned WordCount() const;
    //unsigned GetID() const { return m_id; }
    //size_t   DataSize() const;
    const unsigned char * GetData() const { return &m_data[0]; }
    void Print(std::ostream &) const;
  private:
    unsigned char GetByte(size_t i) const { return m_data[i]; }
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
    unsigned m_id;
    vec_t m_data;
  };

  inline std::ostream & operator << (std::ostream & os, const DEPFETBoard & fr) {
    fr.Print(os);
    return os;
  }

  /** An Event type consisting of just a vector of bytes.
   *
   */
  class DEPFETEvent : public Event {
    EUDAQ_DECLARE_EVENT(DEPFETEvent);
  public:
    virtual void Serialize(Serializer &) const;
    DEPFETEvent(unsigned run, unsigned event) :
      Event(run, event) {}
    template <typename T>
    DEPFETEvent(unsigned run, unsigned event, const std::vector<T> & data) :
      Event(run, event),
      m_boards(1, DEPFETBoard(data)) {}
    template <typename T>
    DEPFETEvent(unsigned run, unsigned event, const T * data, size_t bytes) :
      Event(run, event),
      m_boards(1, DEPFETBoard(data, bytes)) {}
    DEPFETEvent(Deserializer &);
    template <typename T>
    void AddBoard(unsigned id, const std::vector<T> & data) {
      m_boards.push_back(DEPFETBoard(id, data));
    }
    template <typename T>
    void AddBoard(unsigned id, const T * data, size_t bytes) {
      m_boards.push_back(DEPFETBoard(id, data, bytes));
    }
    virtual void Print(std::ostream &) const;
    unsigned NumBoards() const { return m_boards.size(); }
    DEPFETBoard & GetBoard(unsigned i) { return m_boards[i]; }
    const DEPFETBoard & GetBoard(unsigned i) const { return m_boards[i]; }
    void Debug();
    static DEPFETEvent BORE(unsigned run) {
      return DEPFETEvent((void*)0, run);
    }
    static DEPFETEvent EORE(unsigned run, unsigned event) {
      return DEPFETEvent((void*)0, run, event);
    }
  private:
    DEPFETEvent(void *, unsigned run, unsigned event = 0)
      : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE)
      {}
  private:
    //void Analyze();
    //bool m_analyzed;
    std::vector<DEPFETBoard> m_boards;
  };

}

#endif // EUDAQ_INCLUDED_DEPFETEvent
