#include "RawDataEvent.hh"
#include "PluginManager.hh"

#include <ostream>

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<RawDataEvent, Deserializer&>(Event::str2id("_RAW"));
  }

  RawDataEvent::block_t::block_t(Deserializer &des) {
    des.read(id);
    des.read(data);
  }

  void RawDataEvent::block_t::Serialize(Serializer &ser) const {
    ser.write(id);
    ser.write(data);
  }

  void RawDataEvent::block_t::Append(const std::vector<uint8_t> &d) {
    data.insert(data.end(), d.begin(), d.end());
  }
  
  RawDataEvent::RawDataEvent(std::string type, uint32_t id_stream, uint32_t run, uint32_t event)
    : Event(Event::str2id("_RAW"), run, id_stream), m_type(type){
    SetEventN(event);
  }

  RawDataEvent::RawDataEvent(Deserializer &ds) : Event(ds) {
    ds.read(m_type);
    ds.read(m_blocks);
  }

  const std::vector<uint8_t> &RawDataEvent::GetBlock(size_t i) const {
    return m_blocks.at(i).data;
  }

  void RawDataEvent::Print(std::ostream & os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<RawDataEvent> \n";
    os << std::string(offset + 2, ' ') << "<SubType> " << m_type << "</SubType> \n";
    Event::Print(os,offset+2);
    os << std::string(offset + 2, ' ') << "<Block_Size> " << m_blocks.size() << "</Block_Size> \n";
    os << std::string(offset, ' ') << "</RawDataEvent> \n";
  }

  void RawDataEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_type);
    ser.write(m_blocks);
  }
  
}
