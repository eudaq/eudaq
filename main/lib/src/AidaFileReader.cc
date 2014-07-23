#include <list>
#include "jsoncons/json.hpp"
#include "eudaq/JSON.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/AidaFileReader.hh"


namespace eudaq {

  AidaFileReader::AidaFileReader(const std::string & file )
    : m_filename( file ), m_runNumber( -1 )
  {
	  m_des = new FileDeserializer( m_filename );
	  m_des->read( m_json_config );
  }

  AidaFileReader::~AidaFileReader() {
	  if ( m_des )
		  delete m_des;
  }

  bool AidaFileReader::readNext() {
	  if ( !m_des || !m_des->HasData() )
		  return false;
	  m_packet = PacketFactory::Create( *m_des );
	  return true;
  }

  std::string AidaFileReader::getJsonPacketInfo() {
	  if ( !m_packet )
		  return "";

	  JSON json;
	  m_packet->toJson( json );
	  return json.get().to_string();
  }



}
