
#include "jsoncons/json.hpp"

#include "eudaq/IndexReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Logger.hh"
#include <list>
#include "eudaq/FileSerializer.hh"

namespace eudaq {

  IndexReader::IndexReader(const std::string & file )
    : m_filename( file ), m_des( 0 ) {
	  m_des = new FileDeserializer( m_filename );
	  m_des->read( m_json_config );

  }

  IndexReader::~IndexReader() {
	  if ( m_des )
		  delete m_des;
  }

/*
    std::shared_ptr<eudaq::AidaPacket> packet = nullptr;
    bool result = m_des.ReadPacket(m_ver, ev, skip);
    if (ev) m_ev =  ev;
    return result;
*/


}
