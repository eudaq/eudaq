#ifndef EUDAQ_INCLUDED_Event
#define EUDAQ_INCLUDED_Event

#include <string>
#include <vector>
#include <map>
#include <ostream>

#include "Serializable.hh"
#include "Serializer.hh"
#include "Exception.hh"
#include "Utils.hh"
#include "Platform.hh"
#include "Factory.hh"

namespace eudaq {
  class Event;
  using DetectorEvent = Event;

#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Event>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const uint32_t&, const uint32_t&, const uint32_t&>();

  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(const std::string&, const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const std::string&, const uint32_t&, const uint32_t&, const uint32_t&>();
  
#endif

  using EventUP = Factory<Event>::UP_BASE; 
  using EventSP = Factory<Event>::SP_BASE;
  using EventSPC = Factory<Event>::SPC_BASE;

  class DLLEXPORT Event : public Serializable{
  public:
    enum Flags {
      FLAG_BORE = 0x1, 
      FLAG_EORE = 0x2,
      FLAG_FAKE = 0x4,
      FLAG_PACK = 0x8,
      FLAG_TRIG = 0x10,
      FLAG_TIME = 0x20
    };

    Event();
    Event(const uint32_t type, const uint32_t run_n, const uint32_t stm_n);
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const;   
    EventUP Clone() const;

    virtual void Print(std::ostream & os, size_t offset = 0) const;

    bool HasTag(const std::string &name) const {return m_tags.find(name) != m_tags.end();}
    void SetTag(const std::string &name, const std::string &val) {m_tags[name] = val;}
    const std::map<std::string, std::string>& GetTags() const {return m_tags;}
    std::string GetTag(const std::string &name, const std::string &def = "") const;
    std::string GetTag(const std::string &name, const char *def) const {return GetTag(name, std::string(def));}
    template <typename T> T GetTag(const std::string & name, T def) const {
      return eudaq::from_string(GetTag(name), def);
    }
    template <typename T> void SetTag(const std::string &name, const T &val) {
      SetTag(name, eudaq::to_string(val));
    }
    
    void SetFlagBit(uint32_t f) { m_flags |= f;}
    void ClearFlagBit(uint32_t f) { m_flags &= ~f;}
    bool IsFlagBit(uint32_t f) const { return (m_flags&f) == f;}

    void SetFlagTimestamp(){SetFlagBit(FLAG_TIME);}
    void SetFlagTrigger(){SetFlagBit(FLAG_TRIG);}
    bool IsFlagTimestamp() const {return IsFlagBit(FLAG_TIME);}
    bool IsFlagTrigger() const {return IsFlagBit(FLAG_TRIG);}

    void SetFlagFake(){SetFlagBit(FLAG_FAKE);}
    bool IsFlagFake() const {return IsFlagBit(FLAG_FAKE);}

    void SetFlagPacket(){SetFlagBit(FLAG_PACK);}
    bool IsFlagPacket() const {return IsFlagBit(FLAG_PACK);}
    
    bool IsBORE() const { return IsFlagBit(FLAG_BORE);}
    bool IsEORE() const { return IsFlagBit(FLAG_EORE);}
    void SetBORE() {SetFlagBit(FLAG_BORE);}
    void SetEORE() {SetFlagBit(FLAG_EORE);}
    
    void AddSubEvent(EventSPC ev);
    uint32_t GetNumSubEvent() const {return m_sub_events.size();}
    EventSPC GetSubEvent(uint32_t i) const {return m_sub_events.at(i);}
    
    void SetEventID(uint32_t id){m_type = id;}
    void SetVersion(uint32_t v){m_version = v;}
    void SetFlag(uint32_t f) {m_flags = f;}
    void SetRunN(uint32_t n){m_run_n = n;}
    void SetEventN(uint32_t n){m_ev_n = n;}
    void SetDeviceN(uint32_t n){m_stm_n = n;}
    void SetTriggerN(uint32_t n, bool flag = true){m_tg_n = n; if(flag) SetFlagBit(FLAG_TRIG);}
    void SetExtendWord(uint32_t n){m_extend = n;}
    void SetTimestampBegin(uint64_t t){m_ts_begin = t; if(!m_ts_end) m_ts_end = t+1;}
    void SetTimestampEnd(uint64_t t){m_ts_end = t;}
    void SetTimestamp(uint64_t tb, uint64_t te, bool flag = true);
    void SetDescription(const std::string &t) {m_dspt = t;}
    
    uint32_t GetEventID() const {return m_type;};
    uint32_t GetVersion()const {return m_version;}
    uint32_t GetFlag() const { return m_flags;}
    uint32_t GetRunN()const {return m_run_n;}
    uint32_t GetEventN()const {return m_ev_n;}
    uint32_t GetDeviceN() const {return m_stm_n;}
    uint32_t GetTriggerN() const {return m_tg_n;}
    uint32_t GetExtendWord() const {return m_extend;}
    uint64_t GetTimestampBegin() const {return m_ts_begin;}
    uint64_t GetTimestampEnd() const {return m_ts_end;}
    std::string GetDescription() const {return m_dspt;}

    static EventSP MakeShared(Deserializer&);

    //TODO: the meanning of "stream" is not so clear
    void SetStreamN(uint32_t n){m_stm_n = n;}
    uint32_t GetStreamN() const {return m_stm_n;}
    // /////TODO: remove compatiable fun from EUDAQv1
    uint32_t GetEventNumber()const {return m_ev_n;}
    uint32_t GetRunNumber()const {return m_run_n;}

  private:
    uint32_t m_type;
    uint32_t m_version;
    uint32_t m_flags;
    uint32_t m_stm_n;
    uint32_t m_run_n;
    uint32_t m_ev_n;
    uint32_t m_tg_n;
    uint32_t m_extend; //reserved
    uint64_t m_ts_begin;
    uint64_t m_ts_end;
    std::string m_dspt;
    std::map<std::string, std::string> m_tags;
    std::vector<EventSPC> m_sub_events; //TODO::  std::set
  };
}

#endif // EUDAQ_INCLUDED_Event
