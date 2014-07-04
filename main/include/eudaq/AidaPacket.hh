
#ifndef EUDAQ_INCLUDED_AidaPacket
#define EUDAQ_INCLUDED_AidaPacket

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <iostream>
#include <memory>

#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/Event.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include "eudaq/SmartEnum.hh"
#include "eudaq/Platform.hh"

#define EUDAQ_DECLARE_PACKET()                  \
  public:                                       \
  	  static uint64_t eudaq_static_type();      \
  	  virtual uint64_t get_type() const {       \
  		  return eudaq_static_type();           \
  	  }
//  private:                                    \
//static const int EUDAQ_DUMMY_VAR_DONT_USE = 0


#define EUDAQ_DEFINE_PACKET(type, name)      \
  uint64_t type::eudaq_static_type() {       \
    static const uint64_t type_(name);       \
    return type_;                            \
  }										     \
  namespace _eudaq_dummy_ {                  \
  	  static eudaq::RegisterPacketType<type> eudaq_packet_reg;	\
  }


namespace eudaq {

#define PACKET_MARKER 0xDEADBADFADEDCAFE

#define PACKET_NUMBER_BITS  59
#define PACKET_NUMBER_SHIFT  0

#define PACKET_TYPE_BITS    4
#define PACKET_TYPE_SHIFT 	60

// meta data
#define TLU_BITS   1
#define TLU_SHIFT 63

#define ENTRY_TYPE_BITS   4
#define ENTRY_TYPE_SHIFT 59

#define COUNTER_BITS  59
#define COUNTER_SHIFT  0

#define getBits(FIELD,from)	getBitsTemplate<FIELD ## _SHIFT, FIELD ## _BITS>(from)
#define setBits(FIELD,dst, val)	setBitsTemplate<FIELD ## _SHIFT, FIELD ## _BITS>(dst,val)


class DLLEXPORT AidaPacket : public Serializable {
  public:

	AidaPacket( uint64_t type, uint64_t subtype ) : m_data_size( 0 ) {
    	m_header.marker = identifier().number;
		m_header.packetType = type;
		m_header.packetSubType = subtype;
		m_header.packetNumber = -1;
	};

    //
    // packet header
    //

	typedef struct {
		uint64_t 	number;
		std::string string;
	} PacketIdentifier;

	static const PacketIdentifier& identifier() {
		static PacketIdentifier packet_identifier;
		if ( packet_identifier.string.empty() ) {
			packet_identifier.string = "#PACKET#";
			packet_identifier.number = str2type( packet_identifier.string );
		}
		return packet_identifier;
	}

	typedef struct {
		  uint64_t marker; 				// 8 byte string: #PACKET#
		  uint64_t packetType;			// 8 byte string
		  uint64_t packetSubType;		// 8 byte string
		  uint64_t packetNumber;
	  } PacketHeader;


    inline uint64_t GetPacketNumber() const { return m_header.packetNumber; };
    inline void SetPacketNumber( uint64_t n ) { m_header.packetNumber = n; };
    inline uint64_t GetPacketType() const { return m_header.packetType; };
    inline void SetPacketType( uint64_t type ) { m_header.packetType = type; };
    inline uint64_t GetPacketSubType() const { return m_header.packetSubType; };
    inline void SetPacketSubType( uint64_t type ) { m_header.packetSubType = type; };

    //
    // meta data
    //

	DECLARE_ENUM_CLASS( MetaDataType, RUN_NUMBER, TRIGGER_COUNTER, TRIGGER_TIMESTAMP );

    class MetaData {
      public:
    	std::vector<uint64_t> v;

    	void add( bool tlu, int type, uint64_t data ) {
        		uint64_t meta_data = 0;
        		SetType( meta_data, type );
        		SetCounter( meta_data, data );
        		v.push_back( meta_data );
    	};
    };

    MetaData& GetMetaData() {
    	return m_meta_data;
    }

    static inline int GetType( uint64_t meta_data ) { return getBits(ENTRY_TYPE, meta_data ); };
    static inline void SetType( uint64_t& meta_data, int type ) { setBits(ENTRY_TYPE,  meta_data, type ); };
    static inline bool IsTLUBitSet( uint64_t meta_data ) { return getBits(TLU, meta_data ); };
    static inline uint64_t GetCounter( uint64_t meta_data ) { return getBits(COUNTER, meta_data ); };
    static inline void SetCounter( uint64_t& meta_data, uint64_t data ) { setBits(COUNTER, meta_data, data ); };

    template <int shift, int bits> static uint64_t getBitsTemplate( uint64_t from ) {
    	return shift > 0 ? (from >> shift) & bit_mask()[bits] : from & bit_mask()[bits];
    };

    template <int shift, int bits> static void setBitsTemplate( uint64_t& dest, uint64_t val ) {
    	dest |= shift > 0 ? (val & bit_mask()[bits]) << shift : val & bit_mask()[bits];
    };


    void SetData( uint64_t* data, uint64_t size ) {
    	m_data = data;
    	m_data_size = size;
    }

    void SetData( std::vector<uint64_t>& data ) {
    	m_data = data.data();
    	m_data_size = data.size();
    }

    void SetData( std::vector<uint64_t>* data ) {
    	m_data = data->data();
    	m_data_size = data->size();
    }


	virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream & os) const;

    static uint64_t str2type(const std::string & str);
    static std::string type2str(uint64_t id);

  protected:
    friend class PacketFactory;
    AidaPacket() : m_data_size( 0 ) {
    	m_header.marker = identifier().number;
    };

    AidaPacket( PacketHeader& header, Deserializer & ds);

    void SerializeHeader( Serializer & ) const;
    void SerializeMetaData( Serializer & ) const;

    static PacketHeader DeserializeHeader( Deserializer & );
    static const uint64_t * const bit_mask();

    PacketHeader m_header;
    MetaData m_meta_data;
    uint64_t checksum;
  private:
    std::unique_ptr<uint64_t[]> placeholder;
    uint64_t  m_data_size;
    uint64_t* m_data;
};

class DLLEXPORT EventPacket : public AidaPacket {
  EUDAQ_DECLARE_PACKET();
  public:
	  EventPacket(const Event & ev );	// wrapper for old-style events
	  virtual void Serialize(Serializer &) const;

  protected:
	  template <typename T_Packet> friend struct RegisterPacketType;

	  EventPacket( PacketHeader& header, Deserializer & ds);
	  const Event* m_ev;

};

DLLEXPORT std::ostream &  operator << (std::ostream &, const AidaPacket &);


class DLLEXPORT PacketFactory {
    public:
      static AidaPacket * Create( Deserializer & ds);

      typedef AidaPacket * (* packet_creator)(AidaPacket::PacketHeader& header, Deserializer & ds);
      static void Register(uint64_t id, packet_creator func);
      static packet_creator GetCreator(int id);

    private:
      typedef std::map<int, packet_creator> map_t;
      static map_t & get_map();
};

/** A utility template class for registering an Packet type.
 */

template <typename T_Packet>
struct RegisterPacketType {
  RegisterPacketType() {
    PacketFactory::Register(T_Packet::eudaq_static_type(), &factory_func);
  }
  static AidaPacket * factory_func( AidaPacket::PacketHeader& header, Deserializer & ds) {
    return new T_Packet( header, ds );
  }
};


}

#endif // EUDAQ_INCLUDED_AidaPacket
