#include "DEPFETEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

EUDAQ_DEFINE_EVENT(DEPFETEvent, str2id("_DEP"));

DEPFETBoard::DEPFETBoard(Deserializer &ds) {
  ds.read(m_id);
  ds.read(m_data);
}

DEPFETBoard::DEPFETBoard(unsigned id /*= 0*/) : m_id(id) {

}

void DEPFETBoard::Serialize(Serializer &ser) const {
  ser.write(m_id);
  ser.write(m_data);
}


unsigned DEPFETBoard::GetID() const {
  return m_id;
}

const DEPFETBoard::vec_t & DEPFETBoard::GetDataVector() const {
  return m_data;
}

void DEPFETBoard::Print(std::ostream & os) const {
  Print(os, 0);
}



void DEPFETBoard::Print(std::ostream &os, size_t offset) const {
  os << std::string(offset, ' ') << "  ID            = " << m_id << "\n";
}

unsigned char DEPFETBoard::GetByte(size_t i) const {
  return m_data[i];
}

DEPFETEvent::DEPFETEvent(Deserializer & ds) :
Event(ds) {
  ds.read(m_boards);
}

void DEPFETEvent::Print(std::ostream & os) const {
  Print(os, 0);
}

void DEPFETEvent::Print(std::ostream &os, size_t offset) const {
  Event::Print(os, offset);
  os << std::string(offset, ' ') << ", " << m_boards.size() << " boards";
}

std::string DEPFETEvent::GetType() const {
  return "DEPFETEvent";
}

unsigned DEPFETEvent::NumBoards() const {
  return m_boards.size();
}

DEPFETBoard & DEPFETEvent::GetBoard(unsigned i) {
  return m_boards[i];
}

const DEPFETBoard & DEPFETEvent::GetBoard(unsigned i) const {
  return m_boards[i];
}

void DEPFETEvent::Debug() {
  for (unsigned i = 0; i < NumBoards(); ++i) {
    std::cout << GetBoard(i) << std::endl;
  }
}

void DEPFETEvent::Serialize(Serializer &ser) const {
  Event::Serialize(ser);
  ser.write(m_boards);
}

std::ostream & operator<<(std::ostream &os, const DEPFETBoard &fr) {
  fr.Print(os);
  return os;
}


}
