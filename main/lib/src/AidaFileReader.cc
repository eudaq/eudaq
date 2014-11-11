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
#include <memory>


namespace eudaq {

	AidaFileReader::AidaFileReader(const std::string & file) :baseFileReader(file),
	  m_runNumber( -1 )
  {
	  m_des = new FileDeserializer(Filename() );
	  m_des->read( m_json_config );
	  readNext();
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
	 
	  auto evPack = std::dynamic_pointer_cast<EventPacket>(m_packet);
	  if (evPack!=nullptr)
	  {
		  return GetNextEventFromEventPacket(evPack);
	  }
	  
	  return GetNextEventFromPacket();
	

  }

  std::shared_ptr<eudaq::Event> AidaFileReader::GetNextEventFromPacket()
  {
	  static size_t itter = 0;
	  if (itter < PluginManager::GetNumberOfROF(*m_packet))
	  {
		  return PluginManager::ExtractEventN(m_packet, itter++);
	  }
	  else
	  {

		  itter = 0;

		  if (readNext())
		  {
			  return GetNextEvent();
		  }
	  }

	  return nullptr;
  }

  std::shared_ptr<eudaq::Event> AidaFileReader::GetNextEventFromEventPacket(std::shared_ptr<EventPacket>& eventPack)
  {
	 
	  auto ev = std::dynamic_pointer_cast<DetectorEvent>(eventPack->getEventPointer());
	  if (ev==nullptr )
	  {
		  return eventPack->getEventPointer();
	  }
	  
	  if (itter<ev->NumEvents())
	  {
		  return ev->GetEventPtr(itter++);
	  }
	  else
	  {
		  itter = 0;
		  if (readNext())
		  {
			  return GetNextEvent();
		  }
	  }
	  return nullptr;
  }

  bool FileIsAIDA(const std::string& in)
  {
	  auto pos_of_Dot = in.find_last_of('.');
	  if (pos_of_Dot < in.size())
	  {
		  auto sub = in.substr(pos_of_Dot + 1);
		  if (sub.compare("raw2") == 0)
		  {
			  return true;
		  }
	  }

	  return false;
  }


}
