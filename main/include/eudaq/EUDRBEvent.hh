#ifndef EUDAQ_INCLUDED_EUDRBEvent
#define EUDAQ_INCLUDED_EUDRBEvent

#include <vector>
#include <ostream>
#include "eudaq/DetectorEvent.hh"

namespace eudaq {

class DLLEXPORT EUDRBBoard : public Serializable {
public:
  typedef std::vector<unsigned char> vec_t;
  EUDRBBoard(unsigned id = 0);
  EUDRBBoard(Deserializer &);
  virtual void Serialize(Serializer &) const;
  unsigned GetID() const;
  const vec_t & GetDataVector() const;
  void Print(std::ostream &) const;
  void Print(std::ostream &os, size_t offset) const;
private:
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

inline std::ostream & operator << (std::ostream & os, const EUDRBBoard & fr) {
  fr.Print(os);
  return os;
}

/** An Event type consisting of just a vector of bytes.
 *
 */
class DLLEXPORT EUDRBEvent : public Event {
  EUDAQ_DECLARE_EVENT(EUDRBEvent);
public:
  virtual void Serialize(Serializer &) const;
  EUDRBEvent(Deserializer &);
  virtual void Print(std::ostream &) const;
  virtual void Print(std::ostream &os, size_t offset) const;

  unsigned NumBoards() const;
  EUDRBBoard & GetBoard(unsigned i);
  const EUDRBBoard & GetBoard(unsigned i) const;
  void Debug();
private:
  std::vector<EUDRBBoard> m_boards;
};

}

#endif // EUDAQ_INCLUDED_EUDRBEvent
