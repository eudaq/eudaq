#ifndef EUDAQ_INCLUDED_Event
#define EUDAQ_INCLUDED_Event

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <iostream>
#include <memory>

#include "Serializable.hh"
#include "Serializer.hh"
#include "Exception.hh"
#include "Utils.hh"
#include "Platform.hh"
#include "Factory.hh"

namespace eudaq {
  class Event;

#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Event>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();
#endif

  using EventUP = Factory<Event>::UP_BASE; 
  using EventSP = Factory<Event>::SP_BASE;
  using EventSPC = Factory<Event>::SPC_BASE;
  
  using event_sp = EventSP;
  
  static const uint64_t NOTIMESTAMP = (uint64_t)-1;

  class DLLEXPORT Event : public Serializable, public std::enable_shared_from_this<Event> {
  public:
    using t_eventid = std::pair < unsigned, std::string > ;
    using timeStamp_t = uint64_t;

    enum Flags {
      FLAG_BORE = 1, 
      FLAG_EORE = 2, 
      FLAG_HITS = 4, 
      FLAG_FAKE = 8, 
      FLAG_SIMU = 16, 
      FLAG_EUDAQ2 = 32, 
      FLAG_PACKET = 64, 
      FLAG_BROKEN = 128,
      FLAG_STATUS = 256, 
      FLAG_ALL = (unsigned)-1
    }; // Matches FLAGNAMES in .cc file

    Event();
    Event(uint32_t id_stream, unsigned run, unsigned event, unsigned flags = 0);
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const = 0;

    unsigned GetRunNumber() const;
    unsigned GetEventNumber() const;
    timeStamp_t GetTimestamp() const;

    EventUP Clone() const;
    
    virtual std::string GetSubType() const { return ""; }
    virtual void Print(std::ostream &os) const = 0;
    virtual void Print(std::ostream & os,size_t i) const = 0;
    bool HasTag(const std::string &name) const;
    Event &SetTag(const std::string &name, const std::string &val);
    template <typename T> Event &SetTag(const std::string &name, const T &val) {
      return SetTag(name, eudaq::to_string(val));
    }
    std::string GetTag(const std::string & name, const std::string & def = "") const;
    std::string GetTag(const std::string & name, const char * def) const { return GetTag(name, std::string(def)); }
    template <typename T>
    T GetTag(const std::string & name, T def) const {
      return eudaq::from_string(GetTag(name), def);
    }
    
    bool IsBORE() const { return GetFlags(FLAG_BORE) != 0; }
    bool IsEORE() const { return GetFlags(FLAG_EORE) != 0; }

    static unsigned str2id(const std::string & idstr);
    static std::string id2str(unsigned id);

    unsigned GetFlags(unsigned f = FLAG_ALL) const { return m_flags & f; }
    void SetFlags(unsigned f) { m_flags |= f; }
    void ClearFlags(unsigned f = FLAG_ALL) { m_flags &= ~f; }

    void SetTimeStampToNow(size_t i=0);
    void setRunNumber(unsigned newRunNumber){ m_runnumber = newRunNumber; }
    void setEventNumber(unsigned newEventNumber){ m_eventnumber = newEventNumber; }
    uint32_t get_id() const {return m_typeid;};

    void SetTimestampBegin(uint64_t t);
    void SetTimestampEnd(uint64_t t);
    void AddSubEvent(EventSPC ev);
    
    uint64_t GetTimestampBegin() const;
    virtual uint64_t GetTimestampEnd() const; //TODO
    uint32_t GetVersion() const {return m_version;};
    virtual uint32_t GetStreamID() const {return m_id_stream;};
    uint32_t GetEventType() const {return m_typeid;};
    uint32_t GetNumSubEvent() const {return m_sub_events.size();};
    EventSPC GetSubEvent(uint32_t i) const {return m_sub_events.at(i);};
    
  protected:
    typedef std::map<std::string, std::string> map_t;
    uint32_t m_typeid; //m_event_type
    uint32_t m_version;
    uint32_t m_flags;
    uint32_t m_id_stream;
    uint32_t m_runnumber; //m_id_run
    uint32_t m_eventnumber; //m_id_event
    uint64_t m_ts_begin;
    uint64_t m_ts_end;
    map_t m_tags;
    std::vector<EventSPC> m_sub_events;
  };
  // DLLEXPORT std::ostream &  operator<< (std::ostream &, const Event &);
}

#endif // EUDAQ_INCLUDED_Event
