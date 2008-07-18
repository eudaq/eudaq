#include "eudaq/DEPFETEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(DEPFETEvent, str2id("_DEP"));

  DEPFETBoard::DEPFETBoard(Deserializer & ds) {
    ds.read(m_id);
    ds.read(m_data);
  }

  void DEPFETBoard::Serialize(Serializer & ser) const {
    ser.write(m_id);
    ser.write(m_data);
  }

  void DEPFETBoard::Print(std::ostream & os) const {
    os << "DEPFET board" << "\n";
  }

  DEPFETEvent::DEPFETEvent(Deserializer & ds) :
    Event(ds)
  {
    ds.read(m_boards);
  }

  void DEPFETEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_boards.size() << " boards";
  }

  void DEPFETEvent::Debug() {
    for (unsigned i = 0; i < NumBoards(); ++i) {
      std::cout << GetBoard(i) << std::endl;
    }
  }

  void DEPFETEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write(m_boards);
  }

}
