
#include <iostream>
#include <map>

#include "jsoncons/json.hpp"
#include "eudaq/JSONimpl.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/MetaData.hh"
#include "eudaq/AidaPacket.hh"

using jsoncons::json;

namespace eudaq {

#define TLU_BITS   1
#define TLU_SHIFT 63

#define ENTRY_TYPE_BITS   4
#define ENTRY_TYPE_SHIFT 59

#define COUNTER_BITS  59
#define COUNTER_SHIFT  0

template <int shift, int bits> static uint64_t getBitsTemplate( uint64_t from ) {
	return shift > 0 ? (from >> shift) & AidaPacket::bit_mask()[bits] : from & AidaPacket::bit_mask()[bits];
};

template <int shift, int bits> static void setBitsTemplate( uint64_t& dest, uint64_t val ) {
	dest |= shift > 0 ? (val & AidaPacket::bit_mask()[bits]) << shift : val & AidaPacket::bit_mask()[bits];
};

#define getBits(FIELD,from)	getBitsTemplate<FIELD ## _SHIFT, FIELD ## _BITS>(from)
#define setBits(FIELD,dst, val)	setBitsTemplate<FIELD ## _SHIFT, FIELD ## _BITS>(dst,val)


MetaData::MetaData( Deserializer & ds ) {
	ds.read( m_metaData );
};

int MetaData::GetType( uint64_t meta_data ) {
	return getBits(ENTRY_TYPE, meta_data );
};

void MetaData::SetType( uint64_t& meta_data, int type ) {
	setBits(ENTRY_TYPE,  meta_data, type );
};

bool MetaData::IsTLUBitSet( uint64_t meta_data ) {
	return getBits(TLU, meta_data );
};

void MetaData::SetTLUBit( uint64_t& meta_data ) {
	setBits(TLU, meta_data, 1 );
};

uint64_t MetaData::GetCounter( uint64_t meta_data ) {
	return getBits(COUNTER, meta_data );
};

void MetaData::SetCounter( uint64_t& meta_data, uint64_t data ) {
	setBits(COUNTER, meta_data, data );
};

void MetaData::add( bool tlu, int type, uint64_t data ) {
	uint64_t meta_data = 0;
	if ( tlu )
		SetTLUBit( meta_data );
	SetType( meta_data, type );
	SetCounter( meta_data, data );
	//std::cout << "tlu: " << tlu << " data: " <<  std::hex << meta_data << std::dec << std::endl;
	m_metaData.push_back( meta_data );
};

void MetaData::Serialize( Serializer & ser ) const {
	ser.write( m_metaData );
}

void MetaData::toJson( std::shared_ptr<JSON> my, const std::string & objectName ) {
	json& json_metaData = JSONimpl::get( my.get() );
	if ( objectName.empty() ) {
		for ( auto data : m_metaData ) {
			json json_data;
			json_data["tlu"] = MetaData::IsTLUBitSet( data );
			json_data["type"] = MetaData::GetType( data );
			json_data["counter"] = MetaData::GetCounter( data );
			json_metaData.add( json_data );
		}
	}
	else {
		json_metaData[objectName] = jsoncons::json::an_array;
		for ( auto data : m_metaData ) {
			json json_data;
			json_data["tlu"] = MetaData::IsTLUBitSet( data );
			json_data["type"] = MetaData::GetType( data );
			json_data["counter"] = MetaData::GetCounter( data );
			json_metaData[objectName].add( json_data );
		}
	}
}

}
