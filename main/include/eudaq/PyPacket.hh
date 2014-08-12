
#ifndef EUDAQ_INCLUDED_PyPacket
#define EUDAQ_INCLUDED_PyPacket

#include <inttypes.h> /* uint64_t */
#include <string>
#include <vector>
#include "eudaq/Platform.hh"


namespace eudaq {

class JSON;
class AidaPacket;

class DLLEXPORT PyPacket {
  public:
	PyPacket( const std::string & type, const std::string & subType );
	~PyPacket();

	void addMetaData(  bool tlu, int type, uint64_t data );
	void setTags( const std::string & jsonString );
	void setDataSize( uint64_t size );
	void setData( uint64_t* data, uint64_t size );
	void nextToSend();

	static PyPacket * getNextToSend();

	AidaPacket * packet;
};


}

#endif // EUDAQ_INCLUDED_PyPacketFactory
