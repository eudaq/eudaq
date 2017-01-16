#include "Event.hh"
#include "BufferSerializer.hh"

namespace eudaq {
  
  template class DLLEXPORT Factory<Event>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)
	   (const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const uint32_t&, const uint32_t&, const uint32_t&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)
	   (const std::string&, const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const std::string&, const uint32_t&, const uint32_t&, const uint32_t&>();
  //RawDataEvent
  
  namespace{
    auto dummy0 = Factory<Event>::Register<Event, Deserializer&>(cstr2hash("BASE"));
    auto dummy1 = Factory<Event>::Register<Event, const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("BASE"));
    auto dummy2 = Factory<Event>::Register<Event, Deserializer&>(cstr2hash("TRIGGER"));
    auto dummy3 = Factory<Event>::Register<Event, const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("TRIGGER")); //Trigger
    auto dummy4 = Factory<Event>::Register<Event, Deserializer&>(cstr2hash("DUMMYDEV"));
    auto dummy5 = Factory<Event>::Register<Event, const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("DUMMYDEV")); //DummyEvent
    auto dummy6 = Factory<Event>::Register<Event, Deserializer&>(cstr2hash("SYNC"));
    auto dummy7 = Factory<Event>::Register<Event, const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("SYNC"));
  }

  EventSP Event::MakeShared(Deserializer& des){
    uint32_t evid;
    des.PreRead(evid);
    EventSP ev = Factory<Event>::Create(evid, des);
    return ev;
  }
  
  Event::Event()
    :m_type(0), m_version(2), m_flags(0), m_stm_n(0), m_run_n(0), m_ev_n(0), m_ts_begin(0), m_ts_end(0){
  }  
  
  Event::Event(const uint32_t type, const uint32_t run_n, const uint32_t stm_n)
    :m_type(type), m_version(2), m_flags(0), m_stm_n(stm_n), m_run_n(run_n), m_ev_n(0), m_ts_begin(0), m_ts_end(0){
    
  }

  Event::Event(Deserializer & ds) {
    ds.read(m_type);
    ds.read(m_version);
    ds.read(m_flags);
    ds.read(m_stm_n);
    ds.read(m_run_n);
    ds.read(m_ev_n);
    ds.read(m_ts_begin);
    ds.read(m_ts_end);
    ds.read(m_dspt);
    ds.read(m_tags);
    uint32_t n_subev;
    for(ds.read(n_subev); n_subev>0; n_subev--){
      uint32_t evid;
      ds.PreRead(evid);
      EventSP ev = Factory<Event>::Create<Deserializer&>(evid, ds);
      m_sub_events.push_back(std::const_pointer_cast<const Event>(ev));
    }
  }
  
  void Event::Serialize(Serializer & ser) const {
    ser.write(m_type);
    ser.write(m_version);
    ser.write(m_flags);
    ser.write(m_stm_n);
    ser.write(m_run_n);
    ser.write(m_ev_n);
    ser.write(m_ts_begin);
    ser.write(m_ts_end);
    ser.write(m_dspt);
    ser.write(m_tags);
    ser.write((uint32_t)m_sub_events.size());
    for(auto &ev: m_sub_events){
      ser.write(*ev);
    }
  }

  EventUP Event::Clone() const{ //TODO: clone directly
    BufferSerializer ser;
    Serialize(ser);
    uint32_t id;
    ser.PreRead(id);
    return Factory<Event>::Create<Deserializer&>(id, ser);
  }
  
  void Event::Print(std::ostream & os, size_t offset) const{
    os << std::string(offset, ' ') << "<Event>\n";
    os << std::string(offset + 2, ' ') << "<Type> " << m_type <<" </Type>\n";
    os << std::string(offset + 2, ' ') << "<Flag> 0x" << to_hex(m_flags, 8)<< " </Flag>\n";
    os << std::string(offset + 2, ' ') << "<RunN> " << m_run_n << " </RunN>\n";
    os << std::string(offset + 2, ' ') << "<StreamN> " << m_stm_n << " </StreamN>\n";
    os << std::string(offset + 2, ' ') << "<EventN> " << m_ev_n << " </EventN>\n";
    os << std::string(offset + 2, ' ') << "<Timestamp> 0x" << to_hex(m_ts_begin, 16)
       <<"  ->  0x"<< to_hex(m_ts_end, 16) << " </Timestamp>\n";
    os << std::string(offset + 2, ' ') << "<Timestamp> " << m_ts_begin
       <<"  ->  "<< m_ts_end << " </Timestamp>\n";
    if(!m_tags.empty()){
      os << std::string(offset + 2, ' ') << "<Tags> \n";
      for (auto &tag: m_tags){
	os << std::string(offset+4, ' ') << "<Tag> "<< tag.first << "=" << tag.second << " </Tag>\n";
      }
      os << std::string(offset + 2, ' ') << "</Tags> \n";
    }
    if(!m_sub_events.empty()){
      os << std::string(offset + 2, ' ') << "<SubEvents>\n";
      os << std::string(offset + 4, ' ') << "<Size> " << m_sub_events.size()<< " </Size>\n";
      for(auto &subev: m_sub_events){
	subev->Print(os, offset+4);
      }
      os << std::string(offset + 2, ' ') << "</SubEvents>\n";
    }
    os << std::string(offset, ' ') << "</Event>\n";
  }
  

  std::string Event::GetTag(const std::string & name, const std::string & def) const {
    auto i = m_tags.find(name);
    if (i == m_tags.end()) return def;
    return i->second;
  }

}
