
#ifndef EUDAQ_INCLUDED_MetaData
#define EUDAQ_INCLUDED_MetaData

#include <string>
#include "eudaq/Platform.hh"
#include "eudaq/Serializable.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/SmartEnum.hh"

namespace eudaq {

  class Deserializer;

  class DLLEXPORT MetaData : public Serializable {
  public:
    DECLARE_ENUM_CLASS(Type, TRIGGER_COUNTER, TRIGGER_TIMESTAMP);

    MetaData(){};
    MetaData(Deserializer &ds);

    static int GetType(uint64_t meta_data);
    static void SetType(uint64_t &meta_data, int type);
    static bool IsTLUBitSet(uint64_t meta_data);
    static uint64_t GetCounter(uint64_t meta_data);
    static void SetCounter(uint64_t &meta_data, uint64_t data);

    void add(bool tlu, int type, uint64_t data);
    std::vector<uint64_t> &getArray() { return m_metaData; };

    virtual void Serialize(Serializer &) const;

  protected:
    std::vector<uint64_t> m_metaData;
  };
}

#endif // EUDAQ_INCLUDED_MetaData
