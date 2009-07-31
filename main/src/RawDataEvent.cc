#include "eudaq/RawDataEvent.hh"

#include <ostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(RawDataEvent, str2id("_RAW"));

  RawDataEvent::block_t::block_t(Deserializer & des) {
    des.read(id);
    des.read(data);
  }

  void RawDataEvent::block_t::Serialize(Serializer & ser) const {
    ser.write(id);
    ser.write(data);
  }

  void RawDataEvent::block_t::Append(const RawDataEvent::data_t & d) {
    data.insert(data.end(), d.begin(), d.end());
  }

  RawDataEvent::RawDataEvent(std::string type, unsigned run, unsigned event) :
    Event(run, event),
    m_type(type)
  {
  }

  RawDataEvent::RawDataEvent(Deserializer & ds) :
    Event(ds)
  {
    ds.read(m_type);
    ds.read(m_blocks);
  }

  unsigned RawDataEvent::GetID(size_t i) const {
    return m_blocks.at(i).id;
  }

  const RawDataEvent::data_t & RawDataEvent::GetBlock(size_t i) const {
    return m_blocks.at(i).data;
  }

  RawDataEvent::byte_t RawDataEvent::GetByte(size_t block, size_t index) const {
    return GetBlock(block).at(index);
  }

  void RawDataEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_blocks.size() << " blocks";
    if (m_type == "EUDRB" && m_blocks.size() > 0 && m_blocks[0].data.size() >= 8) {
      size_t offset = m_blocks[0].data.size() - 7;
      os << ", tluev=" << (GetByte(0, offset) << 8) + GetByte(0, offset + 1);
    }
  }

  void RawDataEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write(m_type);
    ser.write(m_blocks);
  }

}
