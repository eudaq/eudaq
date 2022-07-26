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

  EventSP Event::Make(const std::string& type, const std::string& argv){
    // auto ev =  Factory<Event>::MakeShared<const std::string&,const std::string&>
    //   (str2hash(type), argv);
    uint32_t typehash = str2hash(type);
    auto ev =  Factory<Event>::MakeShared<>(str2hash(type));
    if(typehash == cstr2hash("RawEvent")){
      ev->SetType(typehash);
      ev->SetExtendWord(eudaq::str2hash(argv));
      ev->SetDescription(argv);
    }
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
    /*if (m_tg_n<=400){
    EUDAQ_DEBUG("Event::Serialize:: WRITING TIGGGER NUMBER = " + std::to_string(m_tg_n));
    }*/
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
    os << std::string(offset + 2, ' ') << "<Block_Data_Size>" << m_blocks.find(0)->second.size() << "</Block_Data_Size>\n";

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


  bool Event::HasTag(const std::string &name) const {return m_tags.find(name) != m_tags.end();}
  void Event::SetTag(const std::string &name, const std::string &val) {m_tags[name] = val;}
  std::map<std::string, std::string> Event::GetTags() const {return m_tags;}
    
  void Event::SetFlagBit(uint32_t f) { m_flags |= f;}
  void Event::ClearFlagBit(uint32_t f) { m_flags &= ~f;}
  bool Event::IsFlagBit(uint32_t f) const { return (m_flags&f) == f;}

  void Event::SetBORE() {SetFlagBit(FLAG_BORE);}
  void Event::SetEORE() {SetFlagBit(FLAG_EORE);}
  void Event::SetFlagFake(){SetFlagBit(FLAG_FAKE);}
  void Event::SetFlagPacket(){SetFlagBit(FLAG_PACK);}
  void Event::SetFlagTimestamp(){SetFlagBit(FLAG_TIME);}
  void Event::SetFlagTrigger(){SetFlagBit(FLAG_TRIG);}
    
  bool Event::IsBORE() const { return IsFlagBit(FLAG_BORE);}
  bool Event::IsEORE() const { return IsFlagBit(FLAG_EORE);}
  bool Event::IsFlagFake() const {return IsFlagBit(FLAG_FAKE);}
  bool Event::IsFlagPacket() const {return IsFlagBit(FLAG_PACK);}
  bool Event::IsFlagTimestamp() const {return IsFlagBit(FLAG_TIME);}
  bool Event::IsFlagTrigger() const {return IsFlagBit(FLAG_TRIG);}    
    
  uint32_t Event::GetNumSubEvent() const {return m_sub_events.size();}
  EventSPC Event::GetSubEvent(uint32_t i) const {return m_sub_events.at(i);}
  std::vector<EventSPC> Event::GetSubEvents() const {return m_sub_events;}
    
  void Event::SetType(uint32_t id){m_type = id;}
  void Event::SetVersion(uint32_t v){m_version = v;}
  void Event::SetFlag(uint32_t f) {m_flags = f;}
  void Event::SetRunN(uint32_t n){m_run_n = n;}
  void Event::SetEventN(uint32_t n){m_ev_n = n;}
  void Event::SetDeviceN(uint32_t n){m_stm_n = n;}
  void Event::SetTriggerN(uint32_t n, bool flag){m_tg_n = n; if(flag) SetFlagBit(FLAG_TRIG);}
  void Event::SetExtendWord(uint32_t n){m_extend = n;}
  void Event::SetDescription(const std::string &t) {m_dspt = t;}
    
  uint32_t Event::GetType() const {return m_type;};
  uint32_t Event::GetVersion()const {return m_version;}
  uint32_t Event::GetFlag() const { return m_flags;}
  uint32_t Event::GetRunN()const {return m_run_n;}
  uint32_t Event::GetEventN()const {return m_ev_n;}
  uint32_t Event::GetDeviceN() const {return m_stm_n;}
  uint32_t Event::GetTriggerN() const {return m_tg_n;}
  uint32_t Event::GetExtendWord() const {return m_extend;}
  uint64_t Event::GetTimestampBegin() const {return m_ts_begin;}
  uint64_t Event::GetTimestampEnd() const {return m_ts_end;}
  std::string Event::GetDescription() const {return m_dspt;}

  void Event::SetEventID(uint32_t id){m_type = id;}
  uint32_t Event::GetEventID() const {return m_type;};
  void Event::SetStreamN(uint32_t n){m_stm_n = n;}
  uint32_t Event::GetStreamN() const {return m_stm_n;}
  uint32_t Event::GetEventNumber()const {return m_ev_n;}
  uint32_t Event::GetRunNumber()const {return m_run_n;}

  size_t Event::GetNumBlock() const { return m_blocks.size(); }
  size_t Event::NumBlocks() const { return m_blocks.size(); }

  std::string Event::GetTag(const std::string &name, const char *def) const{
    return GetTag(name, std::string(def));
  }

}
