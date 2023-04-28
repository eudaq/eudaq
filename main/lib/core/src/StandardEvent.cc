#include "eudaq/StandardEvent.hh"
#include "eudaq/Exception.hh"

#include <memory>


namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<StandardEvent, Deserializer&>(StandardEvent::m_id_factory);
    auto dummy3 = Factory<Event>::Register<StandardEvent>(StandardEvent::m_id_factory);
  }

  StdEventSP StandardEvent::MakeShared(){
    auto ev = Factory<Event>::MakeShared(m_id_factory);
    return std::dynamic_pointer_cast<StandardEvent>(ev);
  }

  StandardEvent::StandardEvent(){
    SetType(m_id_factory);
  }

  StandardEvent::StandardEvent(Deserializer &ds) : Event(ds) {
    ds.read(m_planes);
    ds.read(time_begin);
    ds.read(time_end);
  }

  void StandardEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_planes);
    ser.write(time_begin);
    ser.write(time_end);
  }

  void StandardEvent::Print(std::ostream & os, size_t offset) const{
    os << std::string(offset, ' ') << "<StandardEvent>\n";
    if(!m_planes.empty()){
      os << std::string(offset+2, ' ') << "<Planes>\n";
      os << std::string(offset+4, ' ') << "<Plane_Size>" << m_planes.size() << "</Plane_Size>\n";
      for (auto &plane: m_planes){
	os << std::string(offset+4, ' ') << "<Plane>" << plane << "</Plane>\n";
      }
      os << std::string(offset+2, ' ') << "<Planes>\n";
    }
    Event::Print(os,offset+2);
    os << std::string(offset, ' ') << "</StandardEvent>\n";
  }

  size_t StandardEvent::NumPlanes() const { return m_planes.size(); }

  StandardPlane &StandardEvent::GetPlane(size_t i) { return m_planes[i]; }

  const StandardPlane &StandardEvent::GetPlane(size_t i) const {
    return m_planes[i];
  }

  uint64_t StandardEvent::GetTimeBegin() const {
    return time_begin;
  }

  uint64_t StandardEvent::GetTimeEnd() const {
    return time_end;
  }

  StandardPlane &StandardEvent::AddPlane(const StandardPlane &plane) {
    m_planes.push_back(plane);
    return m_planes.back();
  }
}
