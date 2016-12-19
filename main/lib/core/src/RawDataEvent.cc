#include "eudaq/RawDataEvent.hh"

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<RawDataEvent, Deserializer&>(RawDataEvent::m_id_factory);
    auto dummy7 = Factory<Event>::Register<RawDataEvent, const std::string&, const uint32_t&, const uint32_t&, const uint32_t&>(RawDataEvent::m_id_factory);
  }

  std::shared_ptr<RawDataEvent> RawDataEvent::MakeShared(const std::string& dspt,
							 uint32_t dev_n, uint32_t run_n,
							 uint32_t ev_n){
    EventSP ev = Factory<Event>::MakeShared(RawDataEvent::m_id_factory,
					    cstr2hash("RawDataEvent"),
					    run_n, dev_n);
    auto rawev = std::dynamic_pointer_cast<RawDataEvent>(ev);
    rawev->SetSubType(dspt);
    rawev->SetEventN(ev_n);
    return rawev;
  }

  EventUP RawDataEvent::MakeUnique(const std::string& dspt){
    uint32_t dev_n = 0;
    uint32_t run_n = 0;
    EventUP ev = Factory<Event>::MakeUnique(RawDataEvent::m_id_factory,
					    cstr2hash("RawDataEvent"),
					    run_n, dev_n);
    ev->SetSubType(dspt);
    return ev;
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
  
  RawDataEvent::RawDataEvent(const std::string& dspt, uint32_t dev_n, uint32_t run_n, uint32_t ev_n)
    : Event(cstr2hash("RawDataEvent"), run_n, dev_n){
    SetEventN(ev_n);
    SetSubType(dspt);
  }

  RawDataEvent::RawDataEvent(Deserializer &ds) : Event(ds) {
    ds.read(m_blocks);
  }

  const std::vector<uint8_t> &RawDataEvent::GetBlock(size_t i) const {
    return m_blocks.at(i).data;
  }

  void RawDataEvent::Print(std::ostream & os, size_t offset) const{
    os << std::string(offset, ' ') << "<RawDataEvent> \n";
    os << std::string(offset + 2, ' ') << "<SubType> " << GetSubType() << "</SubType> \n";
    Event::Print(os,offset+2);
    os << std::string(offset + 2, ' ') << "<Block_Size> " << m_blocks.size() << "</Block_Size> \n";
    os << std::string(offset, ' ') << "</RawDataEvent> \n";
  }

  void RawDataEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_blocks);
  }
  
}
