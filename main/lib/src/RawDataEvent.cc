#include "eudaq/RawDataEvent.hh"
#include "eudaq/PluginManager.hh"

#include <ostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(RawDataEvent, str2id("_RAW"));

  RawDataEvent::block_t::block_t(Deserializer &des) {
    des.read(id);
    des.read(data);
  }

  void RawDataEvent::block_t::Serialize(Serializer &ser) const {
    ser.write(id);
    ser.write(data);
  }

  void RawDataEvent::block_t::Append(const RawDataEvent::data_t &d) {
    data.insert(data.end(), d.begin(), d.end());
  }

  RawDataEvent::RawDataEvent(std::string type, unsigned run, unsigned event)
      : Event(run, event), m_type(type) {}

  RawDataEvent::RawDataEvent(Deserializer &ds) : Event(ds) {
    ds.read(m_type);
    ds.read(m_blocks);
  }

  unsigned RawDataEvent::GetID(size_t i) const { return m_blocks.at(i).id; }

  const RawDataEvent::data_t &RawDataEvent::GetBlock(size_t i) const {
    return m_blocks.at(i).data;
  }

  RawDataEvent::byte_t RawDataEvent::GetByte(size_t block, size_t index) const {
    return GetBlock(block).at(index);
  }

  void RawDataEvent::Print(std::ostream & os) const {
    Print(os, 0);
  }

  void RawDataEvent::Print(std::ostream & os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<RawDataEvent> \n";
    Event::Print(os,offset+2);
    std::string tluevstr = "unknown";
    try {
      unsigned tluev = PluginManager::GetTriggerID(*this);
      if (tluev != (unsigned)-1) {
        tluevstr = to_string(tluev);
      }
    } catch (const Exception &) {
      // ignore it
    }
    os << std::string(offset + 2, ' ') << "<blocks>" << m_blocks.size() << "</blocks> \n";
    os << std::string(offset + 2, ' ') << "<tluev>" << tluevstr << "</tluev> \n";
    
    os << std::string(offset, ' ') << "</RawDataEvent> \n";
  }

  void RawDataEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_type);
    ser.write(m_blocks);
  }
}
