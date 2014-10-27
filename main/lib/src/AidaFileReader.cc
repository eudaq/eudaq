#include <list>
#include <iostream>
#include "jsoncons/json.hpp"
#include "eudaq/JSONimpl.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/AidaFileReader.hh"
#include "eudaq/PluginManager.hh"


namespace eudaq {

	AidaFileReader::AidaFileReader(const std::string & file) :baseFileReader(file),
	  m_runNumber( -1 )
  {
	  m_des = new FileDeserializer(Filename() );
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

	  auto json = m_packet->toJson( AidaPacket::JSON_HEADER | AidaPacket::JSON_METADATA | AidaPacket::JSON_DATA );
	  return json->to_string();
  }


  std::vector<uint64_t> AidaFileReader::getData() {
	  return m_packet->GetData();
  }

  std::shared_ptr<eudaq::Event> AidaFileReader::GetNextEvent() 
  {
	  static size_t itter = 0;
	  if (itter< PluginManager::GetNumberOfROF(*m_packet))
	  {
		  return PluginManager::ExtractEventN(m_packet, itter++);
	  }
	  // else

	  itter = 0;

	  if (readNext())
	  {
		  return GetNextEvent();
	  }
	  //else
	
	  return nullptr;
	

  }


}
