
#include <iostream>
#include <list>
#include "jsoncons/json.hpp"
#include "eudaq/JSON.hh"
#include "eudaq/IndexReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaIndexData.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"

using jsoncons::json;

namespace eudaq {

  IndexReader::IndexReader(const std::string & file )
    : m_filename( file ), m_des( 0 ), m_data( 0 ), m_runNumber( -1 ) {
	  m_des = new FileDeserializer( m_filename );
	  m_des->read( m_json_config );

  }

  IndexReader::~IndexReader() {
	  if ( m_des )
		  delete m_des;
	  if ( m_data )
		  delete m_data;
  }

  bool IndexReader::readNext() {
	  if ( !m_des || !m_des->HasData() )
		  return false;
	  m_data = new AidaIndexData( *m_des );
	  return true;
  }


/*
    std::shared_ptr<eudaq::AidaPacket> packet = nullptr;
    bool result = m_des.ReadPacket(m_ver, ev, skip);
    if (ev) m_ev =  ev;
    return result;
*/


}
