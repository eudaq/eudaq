#ifndef EUDAQ_INCLUDED_Status
#define EUDAQ_INCLUDED_Status

#include "eudaq/Serializable.hh"
#include "eudaq/Deserializer.hh"
#include "eudaq/Platform.hh"

#include <string>
#include <map>
#include <ostream>

namespace eudaq {

  class Serializer;
  class Deserializer;
  class Status;

  using StatusUP = std::unique_ptr<Status>;
  using StatusSP = std::shared_ptr<Status>;
  using StatusSPC = std::shared_ptr<const Status>;
  
  class DLLEXPORT Status : public Serializable {
  public:
    enum Level {
      LVL_DEBUG,
      LVL_OK,
      LVL_THROW,
      LVL_EXTRA,
      LVL_INFO,
      LVL_WARN,
      LVL_ERROR,
      LVL_USER,
      LVL_NONE // The last value, any additions should go before this
    };

    enum State {
      STATE_ERROR,
      STATE_UNINIT,
      STATE_UNCONF,
      STATE_CONF,
      STATE_RUNNING
    };
    
    Status(int level = LVL_OK, const std::string &msg = "")
      : m_level(level), m_state(STATE_UNINIT), m_msg(msg) {}
    

    Status(Deserializer &);
    void Serialize(Serializer &) const override;

    void ResetStatus(State st, Level lvl, const std::string &msg);
    int GetLevel() const { return m_level;}
    int GetState() const { return m_state;}
    std::string GetStateString()const;
    std::string GetMessage() const {return m_msg;}
    
    Status &SetTag(const std::string &name, const std::string &val);
    std::string GetTag(const std::string &name,
                       const std::string &def = "") const;
    std::map<std::string, std::string> GetTags() const {return m_tags;};
    
    static std::string Level2String(int level);
    static int String2Level(const std::string &);
    virtual void Print(std::ostream &os, size_t offset = 0) const;
  private:
    int m_level;
    int m_state;
    std::string m_msg;
    std::map<std::string, std::string> m_tags;
    static std::map<uint32_t, std::string> m_map_state_str;
    static std::map<uint32_t, std::string> m_map_level_str;
  };

  inline std::ostream &operator<<(std::ostream &os, const Status &s) {
    s.Print(os);
    return os;
  }
}

#endif // EUDAQ_INCLUDED_Status
