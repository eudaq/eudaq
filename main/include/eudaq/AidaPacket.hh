
#ifndef EUDAQ_INCLUDED_AidaPacket
#define EUDAQ_INCLUDED_AidaPacket

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <iostream>
#include <memory>

#include "eudaq/Serializable.hh"
#include "eudaq/Platform.hh"
#include "eudaq/MetaData.hh"

#define EUDAQ_DECLARE_PACKET()                  \
  public:                                       \
  	  static uint64_t eudaq_static_type();      \
  	  static const std::string & eudaq_static_className(); \
  	  virtual uint64_t get_type() const {       \
  		  return eudaq_static_type();           \
  	  }											\
  	  virtual const std::string & getClassName() const { \
  		  return eudaq_static_className();      \
  	  }
//  private:                                    \
//static const int EUDAQ_DUMMY_VAR_DONT_USE = 0


#define EUDAQ_DEFINE_PACKET(type, name)      \
  uint64_t type::eudaq_static_type() {       \
    static const uint64_t type_(name);       \
    return type_;                            \
  }										     \
  const std::string & type::eudaq_static_className() {       \
    static const std::string type_( #type );       \
    return type_;                            \
  }										     \
  namespace _eudaq_dummy_ {                  \
  	  static eudaq::RegisterPacketType<type> eudaq_packet_reg;	\
  }


namespace eudaq {

class Serializer;
class JSON;
typedef std::shared_ptr<JSON> JSONp;
class Event;

class DLLEXPORT AidaPacket : public Serializable {
  public:

	AidaPacket( uint64_t type, uint64_t subtype ) : AidaPacket() {
		m_header.data.packetType = type;
		m_header.data.packetSubType = subtype;
	};
    AidaPacket( Deserializer & ds );

    //
    // packet header
    //

	class PacketHeader {
	  public:
		struct {
			  uint64_t marker; 			// 8 byte string: #PACKET#
			  uint64_t packetType;			// 8 byte string
			  uint64_t packetSubType;		// 8 byte string
			  uint64_t packetNumber;
		  } data;
	};

    void SerializeHeader( Serializer & ) const;

    uint64_t GetPacketMarker() const { return m_header.data.marker; };
    uint64_t GetPacketType() const { return m_header.data.packetType; };
    uint64_t GetPacketSubType() const { return m_header.data.packetSubType; };
    uint64_t GetPacketNumber() const { return m_header.data.packetNumber; };
    void SetPacketNumber( uint64_t n ) { m_header.data.packetNumber = n; };

    uint64_t GetPacketDataSize() const {return  m_data_size; };

    void SetPacketType( uint64_t type ) { m_header.data.packetType = type; };
    void SetPacketSubType( uint64_t type ) { m_header.data.packetSubType = type; };

    //
    // meta data
    //

    MetaData& GetMetaData() {
    	return m_meta_data;
    }

    void SerializeMetaData( Serializer & ) const;


    //
    // data
    //

	std::vector<uint64_t> & GetData();

	void SetData( uint64_t* data, uint64_t size ) {
    	m_data = data;
    	m_data_size = size;
    }

    void SetData( std::vector<uint64_t>& data ) {
    	m_data_size = data.size();
    	m_data = data.data();
    }

    void SetData( std::vector<uint64_t>* data ) {
    	m_data = data->data();
    	m_data_size = data->size();
    }

    virtual void DeserializeData( Deserializer & );

    virtual void Serialize(Serializer &) const;

    static PacketHeader DeserializeHeader( Deserializer & );

    enum { JSON_HEADER = 1, JSON_METADATA, JSON_DATA };
    JSONp toJson( int whatToAdd );
	virtual const std::string & getClassName() const {
		static std::string name = "AidaPacket";
		return name;
	}

	static const std::string & getCurrentSchema();
	static void setCurrentSchema( const std::string & );

	static const std::string & getDefaultSchema() {
		static std::string schema = "header:4 meta:[] data:[]";
		return schema;
	}

    //
    //	static helper methods
    //

	typedef struct {
		uint64_t 	number;
		std::string string;
	} PacketIdentifier;

	static const PacketIdentifier& identifier();

    static uint64_t str2type(const std::string & str);
    static std::string type2str(uint64_t id);

    static const uint64_t * const bit_mask();

  protected:
    friend class PacketFactory;
    friend class AidaIndexData;
    AidaPacket() : m_data_size( 0 ) {
    	m_header.data.marker = identifier().number;
    	m_header.data.packetNumber = 0;
    };

    AidaPacket( const PacketHeader& header, const MetaData& meta );
    AidaPacket( PacketHeader& header, Deserializer & ds );

    void SetMetaData( MetaData& m ) {
    	m_meta_data = m;
    }

    std::shared_ptr<JSON> HeaderToJson();
    void HeaderToJson( std::shared_ptr<JSON>, const std::string & objectName );
    virtual void DataToJson( std::shared_ptr<JSON>, const std::string & objectName = "" );


    static uint64_t getNextPacketNumber() {
    	static uint64_t packetCounter = 0;
    	return ++packetCounter;
    }

    PacketHeader m_header;
    MetaData m_meta_data;
    uint64_t checksum;
  private:
    std::unique_ptr<uint64_t[]> placeholder;
    uint64_t  m_data_size;
    uint64_t* m_data;
    std::vector<uint64_t> m_data_vector;

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
      static std::shared_ptr<AidaPacket> Create( Deserializer & ds);

      typedef std::shared_ptr<AidaPacket>  (* packet_creator)(AidaPacket::PacketHeader& header, Deserializer & ds);
      static void Register(uint64_t id, packet_creator func);
      static packet_creator GetCreator(int id);

    private:
      typedef std::map<int, packet_creator> map_t;
      static map_t & get_map();
};

// A utility template class for registering an Packet type.


template <typename T_Packet>
struct RegisterPacketType {
  RegisterPacketType() {
    PacketFactory::Register(T_Packet::eudaq_static_type(), &factory_func);
  }
  static std::shared_ptr<AidaPacket> factory_func( AidaPacket::PacketHeader& header, Deserializer & ds) {
    return std::shared_ptr<AidaPacket>( new T_Packet( header, ds ) );
  }
};



}

#endif // EUDAQ_INCLUDED_AidaPacket
