
#include <iostream>
#include <map>

#include "jsoncons/json.hpp"
#include "eudaq/JSON.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/MetaData.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/PyPacket.hh"

using jsoncons::json;

namespace eudaq {

static PyPacket * next = NULL;

  PyPacket::PyPacket( const std::string & type, const std::string & subType ) {
	 packet = new AidaPacket( AidaPacket::str2type( type ), AidaPacket::str2type( subType ) );
  }

  PyPacket::~PyPacket() {
	  delete packet;
  }

  void PyPacket::addMetaData(  bool tlu, int type, uint64_t data ) {
	 packet->GetMetaData().add( tlu, type, data );
  }

  void PyPacket::setTags( const std::string & jsonString ) {

  }

  void PyPacket::setDataSize( uint64_t size ) {
	 std::vector<uint64_t> data( size );
	 packet->SetData( data );
  }

  void PyPacket::setData( uint64_t * data, uint64_t size ) {
	 packet->SetData( data, size );
  }

  void PyPacket::nextToSend() {
	  next = this;
  }

  PyPacket * PyPacket::getNextToSend() {
	  return next;
  }
}
