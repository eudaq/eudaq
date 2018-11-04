#ifndef EUDAQ_INCLUDED_DEPFETEvent
#define EUDAQ_INCLUDED_DEPFETEvent

#include <vector>
#include <ostream>
#include "eudaq/DetectorEvent.hh"

namespace eudaq {

  class DLLEXPORT DEPFETBoard : public Serializable {
  public:
    typedef std::vector<unsigned char> vec_t;
    DEPFETBoard(unsigned id = 0) : m_id(id) {}

    DEPFETBoard(Deserializer &);
    virtual void Serialize(Serializer &) const;

    unsigned GetID() const { return m_id; }
    const vec_t &GetDataVector() const { return m_data; }
    void Print(std::ostream &) const;

  private:
    unsigned char GetByte(size_t i) const { return m_data[i]; }
    template <typename T>
    static vec_t make_vector(const T *data, size_t bytes) {
      const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data);
      return vec_t(ptr, ptr + bytes);
    }
    template <typename T> static vec_t make_vector(const std::vector<T> &data) {
      const unsigned char *ptr =
          reinterpret_cast<const unsigned char *>(&data[0]);
      return vec_t(ptr, ptr + data.size() * sizeof(T));
    }
    unsigned m_id;
    vec_t m_data;
  };

  inline std::ostream &operator<<(std::ostream &os, const DEPFETBoard &fr) {
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

    DEPFETEvent(Deserializer &);

    virtual void Print(std::ostream &) const;

    /// Return "DEPFETEvent" as type.
    virtual std::string GetType() const { return "DEPFETEvent"; }

    unsigned NumBoards() const { return m_boards.size(); }
    DEPFETBoard &GetBoard(unsigned i) { return m_boards[i]; }
    const DEPFETBoard &GetBoard(unsigned i) const { return m_boards[i]; }
    void Debug();

  private:

    std::vector<DEPFETBoard> m_boards;
  };


}

#endif // EUDAQ_INCLUDED_DEPFETEvent
