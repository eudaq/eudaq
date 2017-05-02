#include "eudaq/Event.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"

namespace eudaq {
  
  template class DLLEXPORT Factory<Event>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();

  EventUP Event::MakeUnique(const std::string& dspt){
    EventUP ev = Factory<Event>::MakeUnique<>(cstr2hash("RawEvent"));
    ev->SetType(cstr2hash("RawEvent"));
    ev->SetExtendWord(eudaq::str2hash(dspt));
    ev->SetDescription(dspt);
    return ev;
  }

  EventSP Event::MakeShared(const std::string& dspt){
    EventSP ev = MakeUnique(dspt);
    return ev;
  }
  
  Event::Event()
    :m_type(0), m_version(2), m_flags(0), m_stm_n(0), m_run_n(0), m_ev_n(0), m_tg_n(0), m_extend(0), m_ts_begin(0), m_ts_end(0){
  }  
  
  Event::Event(Deserializer & ds) {
    ds.read(m_type);
    ds.read(m_version);
    ds.read(m_flags);
    ds.read(m_stm_n);
    ds.read(m_run_n);
    ds.read(m_ev_n);
    ds.read(m_tg_n);
    ds.read(m_extend);
    ds.read(m_ts_begin);
    ds.read(m_ts_end);
    ds.read(m_dspt);
    ds.read(m_tags);
    ds.read(m_blocks);
    uint32_t n_subev;
    for(ds.read(n_subev); n_subev>0; n_subev--){
      uint32_t evid;
      ds.PreRead(evid);
      EventSP ev = Factory<Event>::Create<Deserializer&>(evid, ds);
      m_sub_events.push_back(std::const_pointer_cast<const Event>(ev));
    }
  }


  void Event::AddSubEvent(EventSPC ev){
    bool exist = false;
    for(auto &e : m_sub_events){
      if(ev == e){
	exist = true;
      }
    }
    if(!exist && ev)
      m_sub_events.push_back(ev);
    }
  
  void Event::SetTimestamp(uint64_t tb, uint64_t te, bool flag){
    m_ts_begin = tb;
    m_ts_end = te;
    if(flag)
      SetFlagBit(FLAG_TIME);
  }
  
  void Event::Serialize(Serializer & ser) const {
    ser.write(m_type);
    ser.write(m_version);
    ser.write(m_flags);
    ser.write(m_stm_n);
    ser.write(m_run_n);
    ser.write(m_ev_n);
    ser.write(m_tg_n);
    ser.write(m_extend);
    ser.write(m_ts_begin);
    ser.write(m_ts_end);
    ser.write(m_dspt);
    ser.write(m_tags);
    ser.write(m_blocks);
    ser.write((uint32_t)m_sub_events.size());
    for(auto &ev: m_sub_events){
      ser.write(*ev);
    }
  }

  std::vector<uint8_t> Event::GetBlock(uint32_t i) const{
    auto it = m_blocks.find(i);
    if(it == m_blocks.end()){
      EUDAQ_WARN(std::string("RAWDATAEVENT:: no bolck with ID ") + std::to_string(i) + " exists");
    }
    return it->second;
  }

  std::vector<uint32_t> Event::GetBlockNumList() const {
    std::vector<uint32_t> vnum;
    for(auto &e : m_blocks){
      vnum.push_back(e.first);
    }
    return vnum;
  }
  
  void Event::Print(std::ostream & os, size_t offset) const{
    os << std::string(offset, ' ') << "<Event>\n";
    os << std::string(offset + 2, ' ') << "<Type>" << m_type <<"</Type>\n";
    os << std::string(offset + 2, ' ') << "<Extendword>" << m_extend<< "</Extendword>\n";
    os << std::string(offset + 2, ' ') << "<Description>" << m_dspt<< "</Description>\n";
    os << std::string(offset + 2, ' ') << "<Flag>0x" << to_hex(m_flags, 8)<< "</Flag>\n";
    os << std::string(offset + 2, ' ') << "<RunN>" << m_run_n << "</RunN>\n";
    os << std::string(offset + 2, ' ') << "<StreamN>" << m_stm_n << "</StreamN>\n";
    os << std::string(offset + 2, ' ') << "<EventN>" << m_ev_n << "</EventN>\n";
    os << std::string(offset + 2, ' ') << "<TriggerN>" << m_tg_n << "</TriggerN>\n";
    os << std::string(offset + 2, ' ') << "<Timestamp>0x" << to_hex(m_ts_begin, 16)
       <<"  ->  0x"<< to_hex(m_ts_end, 16) << "</Timestamp>\n";
    os << std::string(offset + 2, ' ') << "<Timestamp>" << m_ts_begin
       <<"  ->  "<< m_ts_end << "</Timestamp>\n";
    if(!m_tags.empty()){
      os << std::string(offset + 2, ' ') << "<Tags>\n";
      for (auto &tag: m_tags){
	os << std::string(offset+4, ' ') << "<Tag>"<< tag.first << "=" << tag.second << "</Tag>\n";
      }
      os << std::string(offset + 2, ' ') << "</Tags>\n";
    }
    os << std::string(offset + 2, ' ')<<"<Block_Size>"<<m_blocks.size()<<"</Block_Size>\n";

    if(!m_sub_events.empty()){
      os << std::string(offset + 2, ' ') << "<SubEvents>\n";
      os << std::string(offset + 4, ' ') << "<Size>" << m_sub_events.size()<< "</Size>\n";
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
