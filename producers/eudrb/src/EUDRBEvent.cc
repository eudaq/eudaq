#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "EUDRBEvent.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

EUDAQ_DEFINE_EVENT(EUDRBEvent, str2id("_DRB"));



EUDRBBoard::EUDRBBoard(Deserializer & ds) {
  ds.read(m_id);
  ds.read(m_data);
}

EUDRBBoard::EUDRBBoard(unsigned id /*= 0*/) : m_id(id) {

}

void EUDRBBoard::Serialize(Serializer & ser) const {
  ser.write(m_id);
  ser.write(m_data);
}


unsigned EUDRBBoard::GetID() const {
  return m_id;
}

const EUDRBBoard::vec_t & EUDRBBoard::GetDataVector() const {
  return m_data;
}

void EUDRBBoard::Print(std::ostream & os) const {
  Print(os, 0);

}

void EUDRBBoard::Print(std::ostream &os, size_t offset) const {
  os << std::string(offset, ' ') << "  ID            = " << m_id << "\n";
}

EUDRBEvent::EUDRBEvent(Deserializer & ds) :
Event(ds) {
  ds.read(m_boards);
}

void EUDRBEvent::Print(std::ostream & os) const {
  Print(os, 0);
}

void EUDRBEvent::Print(std::ostream &os, size_t offset) const {
  Event::Print(os, offset);
  os << std::string(offset, ' ') << ", " << m_boards.size() << " boards";
}

unsigned EUDRBEvent::NumBoards() const {
  return m_boards.size();
}

const EUDRBBoard & EUDRBEvent::GetBoard(unsigned i) const {
  return m_boards[i];
}

EUDRBBoard & EUDRBEvent::GetBoard(unsigned i) {
  return m_boards[i];
}

void EUDRBEvent::Debug() {
  for (unsigned i = 0; i < NumBoards(); ++i) {
    std::cout << GetBoard(i) << std::endl;
  }
}

void EUDRBEvent::Serialize(Serializer & ser) const {
  Event::Serialize(ser);
  ser.write(m_boards);
}




}
