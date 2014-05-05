#ifndef EUDAQ_INCLUDED_AidaPacket
#define EUDAQ_INCLUDED_AidaPacket

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


#define EUDAQ_DECLARE_EVENT(type)           \
  public:                                   \
static unsigned eudaq_static_id();      \
virtual unsigned get_id() const {       \
  return eudaq_static_id();             \
}                                       \
private:                                  \
static const int EUDAQ_DUMMY_VAR_DONT_USE = 0

#define EUDAQ_DEFINE_EVENT(type, id)       \
  unsigned type::eudaq_static_id() {       \
    static const unsigned id_(id);         \
    return id_;                            \
  }                                        \
namespace _eudaq_dummy_ {                \
  static eudaq::RegisterEventType<type> eudaq_reg;	\
}                                        \
static const int EUDAQ_DUMMY_VAR_DONT_USE = 0

namespace eudaq {

  static const unsigned long long NOTIMESTAMP = (unsigned long long)-1;

  class DLLEXPORT AidaPacket : public Serializable {
    public:
      enum Flags { FLAG_BORE=1, FLAG_EORE=2, FLAG_HITS=4, FLAG_FAKE=8, FLAG_SIMU=16, FLAG_ALL=(unsigned)-1 }; // Matches FLAGNAMES in .cc file
      AidaPacket(unsigned run, unsigned packet, unsigned long long timestamp = NOTIMESTAMP, unsigned flags=0)
        : m_flags(flags), m_runnumber(run), m_packetnumber(packet), m_timestamp(timestamp) {}
      AidaPacket(Deserializer & ds);
      virtual void Serialize(Serializer &) const = 0;

      unsigned GetRunNumber() const { return m_runnumber; }
      unsigned GetPacketNumber() const { return m_packetnumber; }
      unsigned long long GetTimestamp() const { return m_timestamp; }

      /** Returns the type string of the packet implementation.
       *  Used by the plugin mechanism to identfy the packet type.
       */
      virtual std::string GetSubType() const { return ""; }

      virtual void Print(std::ostream & os) const = 0;

      AidaPacket & SetTag(const std::string & name, const std::string & val);
      template <typename T>
        AidaPacket & SetTag(const std::string & name, const T & val) {
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

      static unsigned str2id(const std::string & idstr);
      static std::string id2str(unsigned id);
      unsigned GetFlags(unsigned f = FLAG_ALL) const { return m_flags & f; }
      void SetFlags(unsigned f) { m_flags |= f; }
      void ClearFlags(unsigned f = FLAG_ALL) { m_flags &= ~f; }
      virtual unsigned get_id() const = 0;
    protected:
      typedef std::map<std::string, std::string> map_t;

      unsigned m_flags, m_runnumber, m_packetnumber;
      unsigned long long m_timestamp;
      map_t m_tags; ///< Metadata tags in (name=value) pairs of strings
  };

  DLLEXPORT std::ostream &  operator << (std::ostream &, const AidaPacket &);

  class DLLEXPORT AidaPacketFactory {
    public:
      static AidaPacket * Create(Deserializer & ds) {
        unsigned id = 0;
        ds.read(id);
        //std::cout << "Create id = " << std::hex << id << std::dec << std::endl;
        packet_creator cr = GetCreator(id);
        if (!cr) EUDAQ_THROW("Unrecognised AidaPacket type (" + AidaPacket::id2str(id) + ")");
        return cr(ds);
      }

      typedef AidaPacket * (* packet_creator)(Deserializer & ds);
      static void Register(unsigned long id, packet_creator func);
      static packet_creator GetCreator(unsigned long id);

    private:
      typedef std::map<unsigned long, packet_creator> map_t;
      static map_t & get_map();
  };

  /** A utility template class for registering an packet type.
   */
  template <typename T_Evt>
    struct RegisterPacketType {
      RegisterPacketType() {
        AidaPacketFactory::Register(T_Evt::eudaq_static_id(), &factory_func);
      }
      static AidaPacket * factory_func(Deserializer & ds) {
        return new T_Evt(ds);
      }
    };
}

#endif // EUDAQ_INCLUDED_AidaPacket
