#ifndef EUDAQ_INCLUDED_DEPFETEvent
#define EUDAQ_INCLUDED_DEPFETEvent

#include <vector>
#include <ostream>
#include "eudaq/DetectorEvent.hh"

namespace eudaq {

class DLLEXPORT DEPFETBoard : public Serializable {
public:
  typedef std::vector<unsigned char> vec_t;
  DEPFETBoard(unsigned id = 0);

  DEPFETBoard(Deserializer &);
  virtual void Serialize(Serializer &) const;

  unsigned GetID() const;

  const vec_t &GetDataVector() const;
  void Print(std::ostream &) const;

  void Print(std::ostream &os, size_t offset) const;
private:
  unsigned char GetByte(size_t i) const;
  template <typename T>
  static vec_t make_vector(const T *data, size_t bytes) {
    const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data);
    return vec_t(ptr, ptr + bytes);
  }

  template <typename T>
  static vec_t make_vector(const std::vector<T> &data) {
    const unsigned char *ptr =
      reinterpret_cast<const unsigned char *>(&data[0]);
    return vec_t(ptr, ptr + data.size() * sizeof(T));
  }
  unsigned m_id;
  vec_t m_data;
};

DLLEXPORT std::ostream &operator<<(std::ostream &os, const DEPFETBoard &fr);

/** An Event type consisting of just a vector of bytes.
 *
 */
class DLLEXPORT DEPFETEvent : public Event {
  EUDAQ_DECLARE_EVENT(DEPFETEvent);

public:
  virtual void Serialize(Serializer &) const;
  DEPFETEvent(Deserializer &);

  virtual void Print(std::ostream &) const;
  virtual void Print(std::ostream &os, size_t offset) const;
  virtual std::string GetType() const;

  unsigned NumBoards() const;
  DEPFETBoard &GetBoard(unsigned i);
  const DEPFETBoard &GetBoard(unsigned i) const;
  void Debug();
private:
  std::vector<DEPFETBoard> m_boards;
};

}

#endif // EUDAQ_INCLUDED_DEPFETEvent
