
#ifndef EUDAQ_INCLUDED_TLU2Packet
#define EUDAQ_INCLUDED_TLU2Packet

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <iostream>

#include "eudaq/SmartEnum.hh"
#include "eudaq/AidaPacket.hh"


namespace eudaq {

class DLLEXPORT TLU2Packet : public AidaPacket {
  EUDAQ_DECLARE_PACKET();
  public:

    TLU2Packet( uint64_t subtype ) {
			m_header.data.marker = identifier().number;
			m_header.data.packetType = get_type();
			m_header.data.packetSubType = subtype;
    };

    /*
    * packet header
    *
	* methods inherited from AidaPacket:
    inline uint64_t GetPacketNumber() const { return m_header.data.packetNumber; };
    inline void SetPacketNumber( uint64_t n ) { m_header.data.packetNumber = n; };
    inline uint64_t GetPacketType() const { return m_header.data.packetType; };
    inline uint64_t GetPacketSubType() const { return m_header.data.packetSubType; };
    inline void SetPacketSubType( uint64_t type ) { m_header.data.packetSubType = type; };
    */

    //
    // meta data
    //

	DECLARE_ENUM_CLASS( TLU2MetaDataType, EVENT_TYPE, INPUT_TRIGGER, TIMESTAMP, EVENT_NUMBER );


	/*
	 * methods inherited from AidaPacket:
	 *
    MetaData& GetMetaData() {
    	return m_meta_data;
    }

    static inline int GetType( uint64_t meta_data ) { return getBits(ENTRY_TYPE, meta_data ); };
    static inline void SetType( uint64_t& meta_data, int type ) { setBits(ENTRY_TYPE,  meta_data, type ); };
    static inline bool IsTLUBitSet( uint64_t meta_data ) { return getBits(TLU, meta_data ); };
    static inline uint64_t GetCounter( uint64_t meta_data ) { return getBits(COUNTER, meta_data ); };
    static inline void SetCounter( uint64_t& meta_data, uint64_t data ) { setBits(COUNTER, meta_data, data ); };
	*/

  protected:
  	  template <typename T_Packet> friend struct RegisterPacketType;
  	  TLU2Packet( PacketHeader& header, Deserializer & ds) : AidaPacket( header, ds ) {};
};


}

#endif // EUDAQ_INCLUDED_AidaPacket
