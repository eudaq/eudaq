
#ifndef EUDAQ_INCLUDED_AidaIndexData
#define EUDAQ_INCLUDED_AidaIndexData


#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/AidaPacket.hh"

namespace eudaq {

class DLLEXPORT AidaIndexData : public Serializable {
  public:

	AidaIndexData( const AidaPacket & packet, uint64_t fileNo, uint64_t offsetInFile );
	AidaIndexData( Deserializer & ds );

	virtual void Serialize(Serializer & ser ) const;

	const AidaPacket::PacketHeader& getHeader() const {
		return m_header;
	}

	const std::vector<uint64_t> & getMetaData() const {
		return m_meta_data;
	}

	uint64_t getFileNumber() const {
		return m_fileNo;
	}

	uint64_t getOffsetInFile() const {
		return m_offsetInFile;
	}

  protected:
    const AidaPacket * m_packet;
    uint64_t m_fileNo;
    uint64_t m_offsetInFile;
    AidaPacket::PacketHeader m_header;
    std::vector<uint64_t> m_meta_data;
};

}

#endif // EUDAQ_INCLUDED_AidaIndexData
