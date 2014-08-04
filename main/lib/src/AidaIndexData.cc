
#include <memory>
#include "eudaq/BufferSerializer.hh"
#include "eudaq/MetaData.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaIndexData.hh"

using std::cout;


namespace eudaq {
  EUDAQ_DEFINE_PACKET(AidaIndexData, str2type( "-IDX-") );

  enum { FileNumberIndex = 0, OffsetInFileIndex, DataLength };

  AidaIndexData::AidaIndexData( AidaPacket & data, uint64_t fileNo, uint64_t offsetInFile ) : fileNumberOffset( DataLength, 0 ) {
    m_header.data.marker = identifier().number;
    m_header.data.packetType = get_type();
    m_header.data.packetNumber = data.m_header.data.packetNumber;
    m_header.data.packetSubType = data.m_header.data.packetType;

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



}
