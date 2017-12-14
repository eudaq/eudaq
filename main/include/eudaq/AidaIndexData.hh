
#ifndef EUDAQ_INCLUDED_AidaIndexData
#define EUDAQ_INCLUDED_AidaIndexData

#include "eudaq/Serializable.hh"

namespace eudaq {

  class AidaPacket;
  class PacketHeader;
  class Deserializer;
  class MetaData;

  class DLLEXPORT AidaIndexData : public Serializable {
  public:
    AidaIndexData(AidaPacket &packet, uint64_t fileNo, uint64_t offsetInFile);
    AidaIndexData(Deserializer &ds);
    virtual ~AidaIndexData();

    virtual void Serialize(Serializer &ser) const;

    AidaPacket::PacketHeader &getHeader();

    MetaData &getMetaData() { return m_packet->GetMetaData(); }

    uint64_t getFileNumber() const;
    uint64_t getOffsetInFile() const;

  protected:
    AidaPacket *m_packet;
    std::vector<uint64_t> fileNumberOffset;
  };
}

#endif // EUDAQ_INCLUDED_AidaIndexData
