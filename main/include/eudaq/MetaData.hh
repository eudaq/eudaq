
#ifndef EUDAQ_INCLUDED_MetaData
#define EUDAQ_INCLUDED_MetaData

#include <inttypes.h> /* uint64_t */
#include <string>
#include <vector>
#include "eudaq/Platform.hh"
#include "eudaq/Serializable.hh"


namespace eudaq {

class Deserializer;
class JSON;

class DLLEXPORT MetaData : public Serializable {
  public:

	MetaData() {};
	MetaData( Deserializer & ds );

    static int GetType( uint64_t meta_data );
    static void SetType( uint64_t& meta_data, int type );
    static bool IsTLUBitSet( uint64_t meta_data );
    static void SetTLUBit( uint64_t& meta_data );
    static uint64_t GetCounter( uint64_t meta_data );
    static void SetCounter( uint64_t& meta_data, uint64_t data );

    void add( bool tlu, int type, uint64_t data );
    std::vector<uint64_t> & getArray() {
    	return m_metaData;
    };

	virtual void Serialize(Serializer &) const;
    virtual void toJson( std::shared_ptr<JSON>, const std::string & objectName = "" );

  protected:
    std::vector<uint64_t> m_metaData;
};


}

#endif // EUDAQ_INCLUDED_MetaData
