#include <ostream>
#include <iostream>
#include <time.h>

#include "eudaq/BufferSerializer.hh"
#include "eudaq/TLU2Packet.hh"

using std::cout;

namespace eudaq {
	EUDAQ_DEFINE_PACKET(TLU2Packet, str2type( "-TLU2-") );

  TLU2Packet::TLU2Packet( PacketHeader& header, Deserializer & ds) {
		m_header = header;
//		ds.read( m_meta_data );
//		m_ev = EventFactory::Create( ds );
		ds.read( checksum );
  }

  void TLU2Packet::Serialize(Serializer & ser) const {
	  // std::cout << "Serialize ev# = " << std::hex << GetPacketNumber() << std::endl;
	  SerializeHeader( ser );
	  SerializeMetaData( ser );
//	  m_ev->Serialize( ser );
	  ser.write( ser.GetCheckSum() );
  }


}
