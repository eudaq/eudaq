
#include "eudaq/AidaIndexData.hh"

using std::cout;


namespace eudaq {

	AidaIndexData::AidaIndexData( const AidaPacket & packet, uint64_t fileNo, uint64_t offsetInFile )
	  	  : m_packet( &packet ), m_fileNo( fileNo ), m_offsetInFile( offsetInFile ) {
	};

	AidaIndexData::AidaIndexData( Deserializer & ds ) : m_packet( 0 ) {
		m_header = AidaPacket::DeserializeHeader( ds );
		ds.read( m_meta_data );
		ds.read( m_fileNo );
		ds.read( m_offsetInFile );
	};

	void AidaIndexData::Serialize(Serializer & ser ) const {
		if ( !m_packet )
			EUDAQ_THROW("AidaIndexData: Attempt to serialize invalid index data");
		m_packet->SerializeHeader( ser );
		m_packet->SerializeMetaData( ser );
		ser.write( m_fileNo );
		ser.write( m_offsetInFile );
	};


}
