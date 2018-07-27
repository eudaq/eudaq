#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "unpacker.h"
#include "compressor.h"

class HGCalRawEvent2StdEventConverter: public eudaq::StdEventConverter
{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("HexaBoardRawDataEvent");
private:
  bool _showUnpackedData;
  bool _compressedData;
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<HGCalRawEvent2StdEventConverter>(HGCalRawEvent2StdEventConverter::m_id_factory);
}

bool HGCalRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const
{
  unpacker _unpacker;
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks = ev->NumBlocks();
  int event_nr=ev->GetEventNumber();
  uint32_t firstWord[nblocks][5];
  for( int iblo=0; iblo<nblocks; iblo++ ){
    if( iblo%2==0 ) continue;
    std::vector<unsigned char> block = ev->GetBlock(iblo);
    //std::cout << "size of block = " << std::dec << block.size() << std::endl;
    std::vector<uint32_t> rawData32;
    if( 1||_compressedData==true ){
      std::string decompressed=compressor::decompress(block);
      std::vector<uint8_t> decompData( decompressed.begin(), decompressed.end() );
      rawData32.resize(decompData.size() / sizeof(uint32_t));
      std::memcpy(&rawData32[0], &decompData[0], decompData.size());
    }
    else{
      rawData32.resize(block.size() / sizeof(uint32_t));
      std::memcpy(&rawData32[0], &block[0], block.size());
    }
    firstWord[iblo][0]=rawData32[0];
    firstWord[iblo][1]=rawData32[1];
    firstWord[iblo][2]=rawData32[2];
    firstWord[iblo][3]=rawData32[3];
    firstWord[iblo][4]=rawData32[4];
    _unpacker.unpack_and_fill(rawData32);
    //_unpacker.rawDataSanityCheck();
    if(_showUnpackedData)_unpacker.printDecodedRawData();
    //sk2cmsData.insert(sk2cmsData.end(),_unpacker.getDecodedData().begin(),_unpacker.getDecodedData().end());
  }
  if (event_nr%100==0) {
    std::cout<<"Processing event "<<std::dec<<event_nr<<" with "<<nblocks<<" blocks" << std::endl;
    for( int iblo=0; iblo<nblocks; iblo++ )
      if( iblo%2!=0 )
	std::cout<<"\t\t data block "<<std::dec<<iblo<<" first 4 words = "<<std::hex<<firstWord[iblo][0]<<" "<<std::hex<<firstWord[iblo][1]<<" "<<std::hex<<firstWord[iblo][2]<<" "<<std::hex<<firstWord[iblo][3]<<" "<<std::hex<<firstWord[iblo][4]<<std::endl;
  } 
  return true;
}

