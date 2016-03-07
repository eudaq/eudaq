
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaIndexData.hh"

using std::cout;

namespace eudaq {

  enum { FileNumberIndex = 0, OffsetInFileIndex, DataLength };

  AidaIndexData::AidaIndexData(AidaPacket &data, uint64_t fileNo,
                               uint64_t offsetInFile)
      : fileNumberOffset(DataLength, 0) {
    m_packet = new AidaPacket(data.m_header, data.GetMetaData());
    fileNumberOffset[FileNumberIndex] = fileNo;
    fileNumberOffset[OffsetInFileIndex] = offsetInFile;
    m_packet->SetData(fileNumberOffset);
  };

  AidaIndexData::AidaIndexData(Deserializer &ds) {
    m_packet = new AidaPacket(ds);
    if (m_packet->m_data_size < DataLength)
      EUDAQ_THROW("AidaIndexData: not enough data read in");
  };

  AidaIndexData::~AidaIndexData() { delete m_packet; }

  void AidaIndexData::Serialize(Serializer &ser) const {
    if (!m_packet)
      EUDAQ_THROW("AidaIndexData: Attempt to serialize invalid index data");
    m_packet->Serialize(ser);
  };

  AidaPacket::PacketHeader &AidaIndexData::getHeader() {
    return m_packet->m_header;
  };

  uint64_t AidaIndexData::getFileNumber() const {
    return m_packet->m_data[FileNumberIndex];
  }

  uint64_t AidaIndexData::getOffsetInFile() const {
    return m_packet->m_data[OffsetInFileIndex];
  }
}
