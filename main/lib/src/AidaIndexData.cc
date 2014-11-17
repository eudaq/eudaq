
#include <memory>
#include "eudaq/BufferSerializer.hh"
#include "eudaq/JSONimpl.hh"
#include "eudaq/MetaData.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaIndexData.hh"

using std::cout;


namespace eudaq {
  /* packet marker is used to identify AidaIndexData */
  EUDAQ_DEFINE_PACKET(AidaIndexData, str2type( "?????") );

  enum { FileNumberIndex = 0, OffsetInFileIndex, DataLength };

  AidaIndexData::AidaIndexData( AidaPacket & data, uint64_t fileNo, uint64_t offsetInFile ) : fileNumberOffset( DataLength, 0 ) {
    m_header.data.marker = identifier().number;
    m_header.data.packetType = data.m_header.data.packetType;
    m_header.data.packetNumber = data.m_header.data.packetNumber;
    m_header.data.packetSubType = data.m_header.data.packetSubType;

    SetMetaData( data.GetMetaData() );
    fileNumberOffset[ FileNumberIndex ] = fileNo;
    fileNumberOffset[ OffsetInFileIndex ] = offsetInFile;
    SetData( fileNumberOffset );
  };

	uint64_t AidaIndexData::getFileNumber() const {
		return m_data[FileNumberIndex];
	}

	uint64_t AidaIndexData::getOffsetInFile() const {
		return m_data[OffsetInFileIndex];
	}

	const AidaPacket::PacketIdentifier& AidaIndexData::identifier() {
		static PacketIdentifier packet_identifier;
		if ( packet_identifier.string.empty() ) {
			packet_identifier.string = ".INDEX.";
			packet_identifier.number = str2type( packet_identifier.string );
		}
		return packet_identifier;
	}

	void AidaIndexData::DataToJson( std::shared_ptr<JSON> my, const std::string & objectName ) {
		jsoncons::json& json = JSONimpl::get( my.get() );
		if ( !objectName.empty() ) {
			json[objectName] = jsoncons::json::an_object;
			json[objectName]["fileNo"] = getFileNumber();
			json[objectName]["offset"] = getOffsetInFile();
		} else {
			json["fileNo"] = getFileNumber();
			json["offset"] = getOffsetInFile();
		}
	}

}
