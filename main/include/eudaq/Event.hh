#ifndef EUDAQ_INCLUDED_Event
#define EUDAQ_INCLUDED_Event

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <iostream>


#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Event_declarationMacros.hh"



namespace eudaq {


  static const uint64_t NOTIMESTAMP = (uint64_t)-1;

  class DLLEXPORT Event : public Serializable {
  public:
    using  MainType_t = unsigned;
    using SubType_t = std::string;
    using t_eventid = std::pair < MainType_t, SubType_t > ;
    using timeStamp_t = uint64_t;

    enum Flags { FLAG_BORE = 1, FLAG_EORE = 2, FLAG_HITS = 4, FLAG_FAKE = 8, FLAG_SIMU = 16, FLAG_EUDAQ2 = 32, FLAG_PACKET = 64, FLAG_ALL = (unsigned) -1 }; // Matches FLAGNAMES in .cc file
    Event(unsigned run, unsigned event, timeStamp_t timestamp = NOTIMESTAMP, unsigned flags = 0)
      : m_flags(flags|FLAG_EUDAQ2), // it is not desired that user use old EUDAQ 1 event format. If one wants to use it one has clear the flags first and then set flags with again.
      m_runnumber(run),
      m_eventnumber(event)  
    {
      m_timestamp.push_back(timestamp);
    }
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const = 0;

    unsigned GetRunNumber() const { return m_runnumber; }
    unsigned GetEventNumber() const { return m_eventnumber; }
    timeStamp_t GetTimestamp(size_t i=0) const { return m_timestamp[i]; }
    size_t   GetSizeOfTimeStamps() const { return m_timestamp.size(); }

    /** Returns the type string of the event implementation.
     *  Used by the plugin mechanism to identify the event type.
     */
    virtual SubType_t GetSubType() const { return ""; }

    virtual void Print(std::ostream & os) const = 0;

    Event & SetTag(const std::string & name, const std::string & val);
    template <typename T>
    Event & SetTag(const std::string & name, const T & val) {
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
    bool HasHits() const { return GetFlags(FLAG_HITS) != 0; }
    bool IsFake() const { return GetFlags(FLAG_FAKE) != 0; }
    bool IsSimulation() const { return GetFlags(FLAG_SIMU) != 0; }
    bool IsEUDAQ2() const { return GetFlags(FLAG_EUDAQ2) != 0; }
    bool IsPacket() const { return GetFlags(FLAG_PACKET) != 0; }


    static unsigned str2id(const std::string & idstr);
    static std::string id2str(unsigned id);
    unsigned GetFlags(unsigned f = FLAG_ALL) const { return m_flags & f; }
    void SetFlags(unsigned f) { m_flags |= f; }
    void SetTimeStampToNow(size_t i=0);
    void pushTimeStampToNow();
    void setTimeStamp(timeStamp_t timeStamp,size_t i=0){ m_timestamp[i]=timeStamp; }
    void pushTimeStamp(timeStamp_t timeStamp){ m_timestamp.push_back(timeStamp); }
    void setRunNumber(unsigned newRunNumber){ m_runnumber = newRunNumber; }
    void ClearFlags(unsigned f = FLAG_ALL) { m_flags &= ~f; }
    virtual unsigned get_id() const = 0;
  protected:
    typedef std::map<std::string, std::string> map_t;

    unsigned m_flags, m_runnumber, m_eventnumber;
    std::vector<timeStamp_t> m_timestamp;
    map_t m_tags; ///< Metadata tags in (name=value) pairs of strings
  };

  DLLEXPORT std::ostream &  operator << (std::ostream &, const Event &);

  class DLLEXPORT EventFactory {
  public:
    static Event * Create(Deserializer & ds) {
      unsigned id = 0;
      ds.read(id);
      //std::cout << "Create id = " << std::hex << id << std::dec << std::endl;
      event_creator cr = GetCreator(id);
      if (!cr) EUDAQ_THROW("Unrecognised Event type (" + Event::id2str(id) + ")");
      return cr(ds);
    }

    typedef Event * (*event_creator)(Deserializer & ds);
    static void Register(uint32_t id, event_creator func);
    static event_creator GetCreator(uint32_t id);

  private:
    typedef std::map<uint32_t, event_creator> map_t;
    static map_t & get_map();
  };

  /** A utility template class for registering an Event type.
   */
  template <typename T_Evt>
  struct RegisterEventType {
    RegisterEventType() {
      EventFactory::Register(T_Evt::eudaq_static_id(), &factory_func);
    }
    static Event * factory_func(Deserializer & ds) {
      return new T_Evt(ds);
    }
  };
}


#endif // EUDAQ_INCLUDED_Event
