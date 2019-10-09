#ifndef EUDAQ_INCLUDED_Status
#define EUDAQ_INCLUDED_Status

#include "eudaq/Serializable.hh"
#include "eudaq/Deserializer.hh"
#include "eudaq/Platform.hh"

#include <string>
#include <map>
#include <ostream>

namespace eudaq{
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
      STATE_STOPPED,
      STATE_RUNNING
    };

    Status(int lvl = LVL_OK, const std::string &msg = "");
    Status(Deserializer &);
    ~Status() override;
    void Serialize(Serializer &) const override;
    virtual void Print(std::ostream &os, size_t offset = 0) const;
    void ResetStatus(State st, Level lvl, const std::string &msg);
    void SetMessage(const std::string &msg);
    void SetTag(const std::string &key, const std::string &val);
    int GetLevel() const;
    int GetState() const;
    std::string GetMessage() const;
    std::string GetStateString()const;
    std::string GetTag(const std::string &key,
                       const std::string &val= "") const;
    std::map<std::string, std::string> GetTags() const;
    static std::string Level2String(int lvl);
    static int String2Level(const std::string &str);
    static std::string State2String(int state);
  private:
    int m_level;
    int m_state;
    std::string m_msg;
    std::map<std::string, std::string> m_tags;
    static std::map<uint32_t, std::string> m_map_state_str;
    static std::map<uint32_t, std::string> m_map_level_str;
  };

}

#endif // EUDAQ_INCLUDED_Status
