#ifndef EUDAQ_INCLUDED_Event
#define EUDAQ_INCLUDED_Event

#include <string>
#include <vector>
#include <map>
#include <ostream>

#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/Deserializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"

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
    // Event(const &&ev);
    
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream & os, size_t offset = 0) const;
    
    bool HasTag(const std::string &name) const {return m_tags.find(name) != m_tags.end();}
    void SetTag(const std::string &name, const std::string &val) {m_tags[name] = val;}
    std::string GetTag(const std::string &name, const std::string &def = "") const;
    std::map<std::string, std::string> GetTags() const {return m_tags;}
    
    void SetFlagBit(uint32_t f) { m_flags |= f;}
    void ClearFlagBit(uint32_t f) { m_flags &= ~f;}
    bool IsFlagBit(uint32_t f) const { return (m_flags&f) == f;}

    void SetBORE() {SetFlagBit(FLAG_BORE);}
    void SetEORE() {SetFlagBit(FLAG_EORE);}
    void SetFlagFake(){SetFlagBit(FLAG_FAKE);}
    void SetFlagPacket(){SetFlagBit(FLAG_PACK);}
    void SetFlagTimestamp(){SetFlagBit(FLAG_TIME);}
    void SetFlagTrigger(){SetFlagBit(FLAG_TRIG);}
    
    bool IsBORE() const { return IsFlagBit(FLAG_BORE);}
    bool IsEORE() const { return IsFlagBit(FLAG_EORE);}
    bool IsFlagFake() const {return IsFlagBit(FLAG_FAKE);}
    bool IsFlagPacket() const {return IsFlagBit(FLAG_PACK);}
    bool IsFlagTimestamp() const {return IsFlagBit(FLAG_TIME);}
    bool IsFlagTrigger() const {return IsFlagBit(FLAG_TRIG);}    
    
    void AddSubEvent(EventSPC ev);
    uint32_t GetNumSubEvent() const {return m_sub_events.size();}
    EventSPC GetSubEvent(uint32_t i) const {return m_sub_events.at(i);}
    std::vector<EventSPC> GetSubEvents() const {return m_sub_events;}
    
    void SetType(uint32_t id){m_type = id;}
    void SetVersion(uint32_t v){m_version = v;}
    void SetFlag(uint32_t f) {m_flags = f;}
    void SetRunN(uint32_t n){m_run_n = n;}
    void SetEventN(uint32_t n){m_ev_n = n;}
    void SetDeviceN(uint32_t n){m_stm_n = n;}
    void SetTriggerN(uint32_t n, bool flag = true){m_tg_n = n; if(flag) SetFlagBit(FLAG_TRIG);}
    void SetExtendWord(uint32_t n){m_extend = n;}
    void SetTimestamp(uint64_t tb, uint64_t te, bool flag = true);
    void SetDescription(const std::string &t) {m_dspt = t;}
    
    uint32_t GetType() const {return m_type;};
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

    static EventUP MakeUnique(const std::string& dspt);
    static EventSP MakeShared(const std::string& dspt);

    void SetEventID(uint32_t id){m_type = id;}
    uint32_t GetEventID() const {return m_type;};
    //TODO: the meanning of "stream" is not so clear
    void SetStreamN(uint32_t n){m_stm_n = n;}
    uint32_t GetStreamN() const {return m_stm_n;}
    // /////TODO: remove compatiable fun from EUDAQv1
    uint32_t GetEventNumber()const {return m_ev_n;}
    uint32_t GetRunNumber()const {return m_run_n;}

    //from RawdataEvent
    std::vector<uint8_t> GetBlock(uint32_t i) const;
    size_t GetNumBlock() const { return m_blocks.size(); }
    size_t NumBlocks() const { return m_blocks.size(); }

    
    std::vector<uint32_t> GetBlockNumList() const;
    
    /// Add a data block as std::vector
    template <typename T>
    size_t AddBlock(uint32_t id, const std::vector<T> &data){
      m_blocks[id]=make_vector(data);
      return m_blocks.size();
    }

    /// Add a data block as array with given size
    template <typename T>
    size_t AddBlock(uint32_t id, const T *data, size_t bytes){
      m_blocks[id]=make_vector(data, bytes);
      return m_blocks.size();
    }

    /// Append data to a block as std::vector
    // template <typename T>
    // void AppendBlock(size_t index, const std::vector<T> &data) {
    //   auto &&src = make_vector(data);
    //   auto &&dst = m_blocks[index];
    //   dst.insert(dst.end(), src.begin(), src.end());
    // }

    // /// Append data to a block as array with given size
    // template <typename T>
    // void AppendBlock(size_t index, const T *data, size_t bytes) {
    //   auto &&src = make_vector(data, bytes);
    //   auto &&dst = m_blocks[index];
    //   dst.insert(dst.end(), src.begin(), src.end());
    // }

    //TODO: remove, clearn up
    std::string GetTag(const std::string &name, const char *def) const {return GetTag(name, std::string(def));}
    template <typename T> T GetTag(const std::string & name, T def) const {
      return eudaq::from_string(GetTag(name), def);
    }
    template <typename T> void SetTag(const std::string &name, const T &val) {
      SetTag(name, eudaq::to_string(val));
    }
    
  private:
    template <typename T>
      static std::vector<uint8_t> make_vector(const T *data, size_t bytes) {
      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
      return std::vector<uint8_t>(ptr, ptr + bytes);
    }

    template <typename T>
    static std::vector<uint8_t> make_vector(const std::vector<T> &data) {
      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&data[0]);
      return std::vector<uint8_t>(ptr, ptr + data.size() * sizeof(T));
    }
    
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
    std::map<uint32_t, std::vector<uint8_t>> m_blocks;
    std::vector<EventSPC> m_sub_events;
  };
}

#endif // EUDAQ_INCLUDED_Event
