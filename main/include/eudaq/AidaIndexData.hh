
#ifndef EUDAQ_INCLUDED_AidaIndexData
#define EUDAQ_INCLUDED_AidaIndexData


#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/AidaPacket.hh"

namespace eudaq {

class DLLEXPORT AidaIndexData : public Serializable {
  public:

	AidaIndexData( const AidaPacket & packet, uint64_t fileNo, uint64_t offsetInFile )
  	  	  : m_packet( &packet ), m_fileNo( fileNo ), m_offsetInFile( offsetInFile ) {};

	AidaIndexData( Deserializer & ds ) : m_packet( 0 ) {
		m_header = AidaPacket::DeserializeHeader( ds );
		ds.read( m_meta_data );
		ds.read( m_fileNo );
		ds.read( m_offsetInFile );
	}

	virtual void Serialize(Serializer & ser ) const {
		if ( !m_packet )
			EUDAQ_THROW("AidaIndexData: Attempt to serialize invalid index data");
		m_packet->SerializeHeader( ser );
		m_packet->SerializeMetaData( ser );
		ser.write( m_fileNo );
		ser.write( m_offsetInFile );
	};

  protected:
    const AidaPacket * m_packet;
    uint64_t m_fileNo;
    uint64_t m_offsetInFile;
    AidaPacket::PacketHeader m_header;
    std::vector<uint64_t> m_meta_data;
};

}



#endif // EUDAQ_INCLUDED_AidaIndexData
