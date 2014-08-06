
#ifndef EUDAQ_INCLUDED_AidaIndexData
#define EUDAQ_INCLUDED_AidaIndexData

#include "eudaq/AidaPacket.hh"

namespace eudaq {

class Deserializer;
class MetaData;

class DLLEXPORT AidaIndexData : public AidaPacket {
  EUDAQ_DECLARE_PACKET();
  public:

	AidaIndexData( AidaPacket & packet, uint64_t fileNo, uint64_t offsetInFile );
	AidaIndexData( Deserializer & ds ) : AidaPacket( ds ) {};
	AidaIndexData( PacketHeader& header, Deserializer & ds ) : AidaPacket( header, ds ) {};

//	AidaPacket::PacketHeader& getHeader();

	uint64_t getFileNumber() const;
	uint64_t getOffsetInFile() const;

	static const PacketIdentifier& identifier();

  protected:
    virtual void DataToJson( std::shared_ptr<JSON>, const std::string & objectName = "" );

    std::vector<uint64_t> fileNumberOffset;
};

}

#endif // EUDAQ_INCLUDED_AidaIndexData
