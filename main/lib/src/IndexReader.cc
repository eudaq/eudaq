
#include <iostream>
#include <list>
#include "jsoncons/json.hpp"
#include "eudaq/IndexReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaIndexData.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"

using jsoncons::json;

namespace eudaq {

  IndexReader::IndexReader(const std::string &file)
      : m_filename(file), m_des(0), m_data(0), m_runNumber(-1) {
    m_des = new FileDeserializer(m_filename);
    m_des->read(m_json_config);
  }

  IndexReader::~IndexReader() {
    if (m_des)
      delete m_des;
    if (m_data)
      delete m_data;
  }

  bool IndexReader::readNext() {
    if (!m_des || !m_des->HasData())
      return false;
    m_data = new AidaIndexData(*m_des);
    return true;
  }

  std::string IndexReader::data2json() {
    if (!m_data)
      return "";
    json data;

    json json_header;
    const AidaPacket::PacketHeader &header = m_data->getHeader();
    json_header["marker"] = AidaPacket::type2str(header.data.marker);
    json_header["packetType"] = AidaPacket::type2str(header.data.packetType);
    json_header["packetSubType"] = header.data.packetSubType;
    json_header["packetNumber"] = header.data.packetNumber;
    data["header"] = json_header;

    json json_metaData(json::make_array());
    for (auto data : m_data->getMetaData().getArray()) {
      json_metaData.add(data);
    }
    data["meta"] = json_metaData;

    data["fileNumber"] = m_data->getFileNumber();
    data["offset"] = m_data->getOffsetInFile();

    return data.to_string();
  }

  /*
      std::shared_ptr<eudaq::AidaPacket> packet = nullptr;
      bool result = m_des.ReadPacket(m_ver, ev, skip);
      if (ev) m_ev =  ev;
      return result;
  */
}
