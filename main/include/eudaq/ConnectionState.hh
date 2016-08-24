#ifndef EUDAQ_INCLUDED_ConnectionState
#define EUDAQ_INCLUDED_ConnectionState

#include "eudaq/Serializable.hh"
#include <string>
#include <map>
#include <ostream>
#include "eudaq/Platform.hh"

namespace eudaq {

  class Serializer;
  class Deserializer;

  class DLLEXPORT ConnectionState : public Serializable {
  public:
    enum State {
      STATE_UNINIT,
      STATE_UNCONF,
      STATE_CONF,
      STATE_RUNNING,
      STATE_ERROR
    };

    ConnectionState(const std::string &msg = "",int state = STATE_UNINIT)
        :m_msg(msg), m_state(state) {}
    ConnectionState(Deserializer &);

    virtual void Serialize(Serializer &) const;

    ConnectionState &SetTag(const std::string &name, const std::string &val);
    std::string GetTag(const std::string &name,
                       const std::string &def = "") const;

    static std::string State2String(int state);

    static int String2State(const std::string &);

    virtual ~ConnectionState() {}
    virtual void print(std::ostream &) const;

    int GetState() const { return m_state; }

    bool isBusy(){return isbusy;}


  protected:
    typedef std::map<std::string, std::string> map_t;

    bool isbusy=false;
    int m_state;

    std::string m_msg;
    map_t m_tags; ///< Metadata tags in (name=value) pairs of strings
  };

  inline std::ostream &operator<<(std::ostream &os, const ConnectionState &s) {
    s.print(os);
    return os;
  }
}

#endif // EUDAQ_INCLUDED_ConnectionState
