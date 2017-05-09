#include "eudaq/StandardEvent.hh"
#include "eudaq/Exception.hh"

#include <memory>


namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<StandardEvent, Deserializer&>(StandardEvent::m_id_factory);
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
  }

  void StandardEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_planes);
  }

  void StandardEvent::Print(std::ostream & os, size_t offset) const
  {
    Event::Print(os,offset);
    os << std::string(offset, ' ') << ", " << m_planes.size() << " planes:\n";
    for (size_t i = 0; i < m_planes.size(); ++i) {
      os << std::string(offset, ' ') << "  " << m_planes[i] << "\n";
    }
  }

  size_t StandardEvent::NumPlanes() const { return m_planes.size(); }

  StandardPlane &StandardEvent::GetPlane(size_t i) { return m_planes[i]; }

  const StandardPlane &StandardEvent::GetPlane(size_t i) const {
    return m_planes[i];
  }

  StandardPlane &StandardEvent::AddPlane(const StandardPlane &plane) {
    m_planes.push_back(plane);
    return m_planes.back();
  }
}
