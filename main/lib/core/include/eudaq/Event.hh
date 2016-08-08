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
#endif

  using EventUP = Factory<Event>::UP_BASE; 
  using EventSP = std::shared_ptr<Event>;
  using event_sp = EventSP;
  
  static const uint64_t NOTIMESTAMP = (uint64_t)-1;

  class DLLEXPORT Event : public Serializable {
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
    Event(unsigned run, unsigned event, timeStamp_t timestamp = NOTIMESTAMP, unsigned flags = 0);
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const = 0;

    unsigned GetRunNumber() const;
    unsigned GetEventNumber() const;
    timeStamp_t GetTimestamp(size_t i=0) const;
    size_t   GetSizeOfTimeStamps() const;
    uint64_t getUniqueID() const;
    Event* Clone() const;
    /** Returns the type string of the event implementation.
     *  Used by the plugin mechanism to identify the event type.
     */
    virtual std::string GetSubType() const { return ""; }
    virtual void GetSubType(std::string){}
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
    bool IsEUDAQ2() const { return GetFlags(FLAG_EUDAQ2) != 0; }
    bool IsPacket() const { return GetFlags(FLAG_PACKET) != 0; }


    static unsigned str2id(const std::string & idstr);
    static std::string id2str(unsigned id);
    static EventUP Create(Deserializer &ds);

    unsigned GetFlags(unsigned f = FLAG_ALL) const { return m_flags & f; }
    void SetFlags(unsigned f) { m_flags |= f; }
    void SetTimeStampToNow(size_t i=0);
    void pushTimeStampToNow();
    void setTimeStamp(timeStamp_t timeStamp, size_t i = 0);
   
    void pushTimeStamp(timeStamp_t timeStamp){ m_timestamp.push_back(timeStamp); }
    void setRunNumber(unsigned newRunNumber){ m_runnumber = newRunNumber; }
    void setEventNumber(unsigned newEventNumber){ m_eventnumber = newEventNumber; }
    void ClearFlags(unsigned f = FLAG_ALL) { m_flags &= ~f; }
    unsigned get_id() const {return m_typeid;};
  protected:
    typedef std::map<std::string, std::string> map_t;

    uint32_t m_typeid;
    unsigned m_flags, m_runnumber, m_eventnumber;
    std::vector<timeStamp_t> m_timestamp;
    map_t m_tags; ///< Metadata tags in (name=value) pairs of strings
 
  };

  DLLEXPORT std::ostream &  operator<< (std::ostream &, const Event &);

}


#endif // EUDAQ_INCLUDED_Event
