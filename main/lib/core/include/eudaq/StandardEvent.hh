#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
#include "eudaq/StandardPlane.hh"
#include <vector>
#include <string>



namespace eudaq {
  class StandardEvent;
  
  using StdEventUP = Factory<StandardEvent>::UP;
  using StdEventSP = Factory<StandardEvent>::SP;
  using StdEventSPC = Factory<StandardEvent>::SPC;
  using StandardEventSP = StdEventSP;
  using StandardEventSPC = StdEventSPC;  

  class DLLEXPORT StandardEvent : public Event {
    public:
    StandardEvent();
    StandardEvent(Deserializer &);

    StandardPlane &AddPlane(const StandardPlane &);
    size_t NumPlanes() const;
    const StandardPlane &GetPlane(size_t i) const;
    StandardPlane &GetPlane(size_t i);
    virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream & os,size_t offset = 0) const;
    
    static StdEventSP MakeShared();    
    static const uint32_t m_id_factory = cstr2hash("StandardEvent");

  private:
    std::vector<StandardPlane> m_planes;
  };

  inline std::ostream &operator<<(std::ostream &os, const StandardEvent &ev) {
    ev.Print(os);
    return os;
  }

} // namespace eudaq

#endif // EUDAQ_INCLUDED_StandardEvent
