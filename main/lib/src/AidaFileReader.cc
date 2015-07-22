#include <iostream>
#include <list>
#include "jsoncons/json.hpp"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/AidaFileReader.hh"

using jsoncons::json;

namespace eudaq {

  AidaFileReader::AidaFileReader(const std::string &file)
      : m_filename(file), m_runNumber(-1) {
    m_des = new FileDeserializer(m_filename);
    m_des->read(m_json_config);
  }

  AidaFileReader::~AidaFileReader() {
    if (m_des)
      delete m_des;
  }

  bool AidaFileReader::readNext() {
    if (!m_des || !m_des->HasData())
      return false;
    m_packet = PacketFactory::Create(*m_des);
    return true;
  }

  std::string AidaFileReader::getJsonPacketInfo() {
    if (!m_packet)
      return "";
    json data;

    json json_header;
    json_header["packetType"] = AidaPacket::type2str(m_packet->GetPacketType());
    json_header["packetSubType"] =
        AidaPacket::type2str(m_packet->GetPacketSubType());
    json_header["packetNumber"] = m_packet->GetPacketNumber();
    data["header"] = json_header;

    json json_metaData(json::an_array);
    for (auto data : m_packet->GetMetaData().getArray()) {
      json_metaData.add(data);
    }
    data["meta"] = json_metaData;

    return data.to_string();
  }
}
