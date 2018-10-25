#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "unpacker.h"
#include "compressor.h"
#include <algorithm>
#include <bitset>

#define HGC_NUMBER_OF_TS 13

#define STDPLANE_X_SIZE 4
#define STDPLANE_Y_SIZE 64

#define HalfMipCut 25
#define HighEnergyThr 500
#define STDPLANE_SIZE 30 //13 low gain, 13 high gain, toa fall and rise, tot fast and slow

#define RUNMODE 2 // 0 for pedestal data, 1 for TOA selection, 2 for Time sample 3 selection

struct HGCalHit
{
  HGCalHit(uint16_t m_board, uint16_t m_chip, uint16_t m_channel) : board(m_board), chip(m_chip), channel(m_channel){;}
  uint16_t board,chip,channel;
  //In HGCal hit, ADC data are gray decoded, 4 bits header has been removed, LG and HG are time ordered (using the roll mask)
  uint16_t highgain[HGC_NUMBER_OF_TS];
  uint16_t lowgain[HGC_NUMBER_OF_TS];
  uint16_t tot_slow;
  uint16_t tot_fast;
  uint16_t toa_fall;
  uint16_t toa_rise;
};

std::ostream& operator<< (std::ostream& stream, const HGCalHit hit)
{
  stream << "BoardId,ChipId,ChannelId = " << hit.board << "," << hit.chip << "," << hit.channel;
  stream << "\n \t High gain :\t\t";
  for( int ts=0; ts<HGC_NUMBER_OF_TS-2; ts++) 
    stream << " " << hit.highgain[ts];
  stream << "\n \t Low gain :\t\t";
  for( int ts=0; ts<HGC_NUMBER_OF_TS-2; ts++) 
    stream << " " << hit.lowgain[ts];
  stream << "\n \t ToT slow,ToA rise : " << hit.tot_slow << ", " << hit.toa_rise;
}

class HGCalRawEvent2StdEventConverter: public eudaq::StdEventConverter
{
public:
  HGCalRawEvent2StdEventConverter();
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("HexaBoardRawDataEvent");
private:
  std::vector<HGCalHit> GetZeroSuppressHits(const Skiroc2CMSData skdata, int layer, int chip) const;
  const std::map<int, int> m_orm_module_sum = { //{ormId,nModuleBeforeThisORM}
    { 0, 0 }, 
    { 1, 8 }, 
    { 2, 15 },
    { 3, 22 },
    { 4, 29 }, 
    { 5, 36 }, 
    { 6, 43 },
    { 7, 50 },
    { 8, 57 }, 
    { 9, 64 },
    { 10, 71 },
    { 11, 78 },
    { 12, 85 }, 
    { 13, 92 }
    // { 0, 0 }, // 1st orm, no module before
    // { 1, 6 }, // 2nd orm, 6 modules before
    // { 2, 14 },// 3rd orm, 6+8 modules before
    // { 3, 21 } // 4th orm, 6+8+7 modules before
  };
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<HGCalRawEvent2StdEventConverter>(HGCalRawEvent2StdEventConverter::m_id_factory);
}

HGCalRawEvent2StdEventConverter::HGCalRawEvent2StdEventConverter()
{
  //std::cout<<"Initialize HGCAL Converter"<<std::endl;  
}


bool HGCalRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const
{
  // std::cout<<"Converting HGCAL"<<std::endl;
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks = ev->NumBlocks();
  int event_nr=ev->GetEventNumber();

  std::ostringstream os(std::ostringstream::ate);

  unpacker _unpacker;
  uint32_t firstWord[nblocks][5];
  
  int ormId;
  for( int iblo=0; iblo<nblocks; iblo++ ){
    if( iblo%2==0 ){
      std::vector<unsigned char> block = ev->GetBlock(iblo);
      std::vector<uint32_t> rawData32;
      //std::cout << "block.size() = " << std::dec << block.size() << std::endl;
      rawData32.resize(block.size() / sizeof(uint32_t));
      std::memcpy(&rawData32[0], &block[0], block.size());
      //std::cout << "rawData32.size() = " << std::dec << rawData32.size() << std::endl;
      ormId=rawData32[0];
      // std::bitset<32> bits(rawData32[1]);
      // unsigned int nlayer=bits.count()/4;
      // std::cout << "orm id = " << std::dec << rawData32[0] << "\t nlayer = " << nlayer << std::endl;
    }
    else{
      std::vector<unsigned char> block = ev->GetBlock(iblo);
      std::vector<uint32_t> rawData32;
      std::string decompressed=compressor::decompress(block);
      std::vector<uint8_t> decompData( decompressed.begin(), decompressed.end() );
      rawData32.resize(decompData.size() / sizeof(uint32_t));
      std::memcpy(&rawData32[0], &decompData[0], decompData.size());

      firstWord[iblo][0]=rawData32[0];
      firstWord[iblo][1]=rawData32[1];
      firstWord[iblo][2]=rawData32[2];
      firstWord[iblo][3]=rawData32[3];
      firstWord[iblo][4]=rawData32[4];
      _unpacker.unpack_and_fill(rawData32);
    
      std::map< int,std::vector<HGCalHit> > hitMap;
      for( std::vector<Skiroc2CMSData>::iterator it=_unpacker.getChipData().begin(); it!=_unpacker.getChipData().end(); ++it ){
	int key=(*it).chipID();
	int chipIndex=key%100;
	int layer=m_orm_module_sum.at(ormId)+chipIndex/4;
	int chip=3-chipIndex%4;
	std::vector<HGCalHit> hits=GetZeroSuppressHits( (*it),layer,chip );
	if( hitMap.find(layer)==hitMap.end() )
	  hitMap.insert( std::pair<int,std::vector<HGCalHit> >(layer,hits) );
	else
	  hitMap[layer].insert( hitMap[layer].end(), hits.begin(), hits.end() );
      }
      for( std::map< int,std::vector<HGCalHit> >::iterator it=hitMap.begin(); it!=hitMap.end(); ++it ){
	os.str("");
	os<<"HexaBoard-RDB"<<std::setfill('0') << std::setw(2)<<ormId;
	std::unique_ptr<eudaq::StandardPlane> plane(new eudaq::StandardPlane(it->first, "HexaBoardRawDataEvent", os.str()));
	plane->SetSizeZS(STDPLANE_X_SIZE,STDPLANE_Y_SIZE,it->second.size(),STDPLANE_SIZE);
	int nhit=0;
	for(std::vector<HGCalHit>::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt){
	  //std::cout << (*jt) << std::endl;
	  for(int ts=0; ts<HGC_NUMBER_OF_TS; ts++){
	    plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).lowgain[ts],false,ts);
	    plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).highgain[ts],false,ts+HGC_NUMBER_OF_TS);
	  }
	  plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).toa_fall,false,HGC_NUMBER_OF_TS*2+0);
	  plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).toa_rise,false,HGC_NUMBER_OF_TS*2+1);
	  plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).tot_fast,false,HGC_NUMBER_OF_TS*2+2);
	  plane->SetPixel(nhit,(*jt).chip,(*jt).channel,(*jt).tot_slow,false,HGC_NUMBER_OF_TS*2+3);
	  nhit++;
	}
	d2->AddPlane(*plane);
      }
    }
  }
  if (event_nr%1000==0) {
    std::cout<<"Processing event "<<std::dec<<event_nr<<" with "<<nblocks<<" blocks" << std::endl;
    for( int iblo=0; iblo<nblocks; iblo++ )
      if( iblo%2!=0 )
	std::cout<<"\t\t data block "<<std::dec<<iblo<<" first 4 words = "<<std::hex<<firstWord[iblo][0]<<" "<<std::hex<<firstWord[iblo][1]<<" "<<std::hex<<firstWord[iblo][2]<<" "<<std::hex<<firstWord[iblo][3]<<" "<<std::hex<<firstWord[iblo][4]<<std::endl;
  } 
  return true;
}

std::vector<HGCalHit> HGCalRawEvent2StdEventConverter::GetZeroSuppressHits(const Skiroc2CMSData skdata,int layer,int chip) const
{
  std::vector<HGCalHit> zshits;
  for( int ichan=0; ichan<N_CHANNELS_PER_CHIP; ichan++ ){
    if(ichan%2!=0) continue;
    int hgTS0=skdata.ADCHigh(ichan,0);//pre-sample
    int hgTS3=skdata.ADCHigh(ichan,3);//expeced max
    int hgTS4=skdata.ADCHigh(ichan,4);//still close to max
    int hgTS6=skdata.ADCHigh(ichan,6);//undershoot
    if( RUNMODE==0 ){;}
    else if( RUNMODE==1 && skdata.TOAFall(ichan)==4 ) continue;
    else if( RUNMODE==2 && hgTS3-hgTS0<HighEnergyThr && (hgTS3-hgTS0<HalfMipCut || hgTS4-hgTS6<0 || hgTS0-hgTS6<0 ) && skdata.TOAFall(ichan)==4) continue;
    HGCalHit hit(layer, chip, ichan);
    for(int ts=0; ts<HGC_NUMBER_OF_TS; ts++){
      hit.highgain[ts]=skdata.ADCHigh(ichan,ts);
      hit.lowgain[ts]=skdata.ADCHigh(ichan,ts);
    }
    hit.tot_slow=skdata.TOTSlow(ichan);
    hit.tot_fast=skdata.TOTFast(ichan);
    hit.toa_fall=skdata.TOAFall(ichan);
    hit.toa_rise=skdata.TOARise(ichan);
    zshits.push_back(hit);
  }
  return zshits;
}
