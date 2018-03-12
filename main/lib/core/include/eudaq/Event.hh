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
    
    bool HasTag(const std::string &name) const;
    void SetTag(const std::string &name, const std::string &val);
    std::string GetTag(const std::string &name, const std::string &def = "") const;
    std::map<std::string, std::string> GetTags() const;
    
    void SetFlagBit(uint32_t f);
    void ClearFlagBit(uint32_t f);
    bool IsFlagBit(uint32_t f) const;

    void SetBORE();
    void SetEORE();
    void SetFlagFake();
    void SetFlagPacket();
    void SetFlagTimestamp();
    void SetFlagTrigger();
    
    bool IsBORE() const;
    bool IsEORE() const;
    bool IsFlagFake() const;
    bool IsFlagPacket() const;
    bool IsFlagTimestamp() const;
    bool IsFlagTrigger() const;    
    
    void AddSubEvent(EventSPC ev);
    uint32_t GetNumSubEvent() const;
    EventSPC GetSubEvent(uint32_t i) const;
    std::vector<EventSPC> GetSubEvents() const;
    
    void SetType(uint32_t id);
    void SetVersion(uint32_t v);
    void SetFlag(uint32_t f);
    void SetRunN(uint32_t n);
    void SetEventN(uint32_t n);
    void SetDeviceN(uint32_t n);
    void SetTriggerN(uint32_t n, bool flag = true);
    void SetExtendWord(uint32_t n);
    void SetTimestamp(uint64_t tb, uint64_t te, bool flag = true);
    void SetDescription(const std::string &t);
    
    uint32_t GetType() const;
    uint32_t GetVersion()const;
    uint32_t GetFlag() const;
    uint32_t GetRunN()const;
    uint32_t GetEventN()const;
    uint32_t GetDeviceN() const;
    uint32_t GetTriggerN() const;
    uint32_t GetExtendWord() const;
    uint64_t GetTimestampBegin() const;
    uint64_t GetTimestampEnd() const;
    std::string GetDescription() const;

    static EventUP MakeUnique(const std::string& dspt);
    static EventSP MakeShared(const std::string& dspt);
    static EventSP Make(const std::string& type, const std::string& argv);

    void SetEventID(uint32_t id);
    uint32_t GetEventID() const;
    //TODO: the meanning of "stream" is not so clear
    void SetStreamN(uint32_t n);
    uint32_t GetStreamN() const;
    // /////TODO: remove compatiable fun from EUDAQv1
    uint32_t GetEventNumber()const;
    uint32_t GetRunNumber()const;

    //from RawdataEvent
    std::vector<uint8_t> GetBlock(uint32_t i) const;
    size_t GetNumBlock() const;
    size_t NumBlocks() const;
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

    template <typename T>
    void AppendBlock(size_t index, const std::vector<T> &data) {
      auto &&src = make_vector(data);
      auto &&dst = m_blocks[index];
      dst.insert(dst.end(), src.begin(), src.end());
    }

    //TODO: remove, clearn up
    std::string GetTag(const std::string &name, const char *def) const;
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
