#include <iostream>
#include <iomanip>
#include <bitset>
#include <array>
#include "unpacker.h"

void unpacker::unpack_and_fill(std::vector<uint32_t> &raw_data)
{
  m_decoded_raw_data.clear();
  const std::bitset<32> ski_mask(raw_data[0]);
  const int mask_count = ski_mask.count();
  std::vector< std::array<uint16_t, 1924> > skirocs(mask_count, std::array<uint16_t, 1924>());
  uint32_t x;
  const int offset = 1; // first byte coming out of the hexaboard is FF
  for(int  i = 0; i < 1924; i++){
    for (int j = 0; j < 16; j++){
      x = raw_data[offset + i*16 + j];
      int k = 0;
      for (int fifo = 0; fifo < 32; fifo++){
	if (raw_data[0] & (1<<fifo)){
	  skirocs[k][i] = skirocs[k][i] | (uint16_t) (((x >> fifo ) & 1) << (15 - j));
	  k++;
	}
      }
    }
  }
  for( std::vector< std::array<uint16_t, 1924> >::iterator it=skirocs.begin(); it!=skirocs.end(); ++it )
    m_decoded_raw_data.insert(m_decoded_raw_data.end(),(*it).begin(),(*it).end());

  m_lastTimeStamp = ((uint64_t)raw_data[ raw_data.size()-1 ]<<32) | (uint64_t)raw_data[ raw_data.size()-2 ]  ;
  m_lastTriggerId = raw_data[raw_data.size()-3]>>8;
  m_timestampmap.insert( std::pair<uint32_t,uint64_t>(raw_data[raw_data.size()-3],m_lastTimeStamp) );
}

void unpacker::rawDataSanityCheck()
{
  unsigned int chipIndex=0;
  unsigned int offset=0;
  while(1){
    std::bitset<13> rollMask( m_decoded_raw_data[1920+offset]&MASK_ROLL );
    if( rollMask.count()!=2 ){
      std::cout << "Problem: wrong roll mask format in chip " << std::dec << chipIndex << "\t rollMask = "<< rollMask << std::endl;
    }
    uint16_t head[64];
    for( int ichan=0; ichan<64; ichan++ ){
      head[ichan]=(m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT) & MASK_HEAD)>>12;
      if(head[ichan]!=8&&head[ichan]!=9)
	std::cout << "ISSUE in chip,chan " << std::dec<<chipIndex <<","<<ichan << " : we expected 8(1000) or 9(1001) for the adc header and I find " << std::bitset<4>(head[ichan]) << std::endl;
    }
    bool lgflag(false),hgflag(false);
    for( int ichan=0; ichan<64; ichan++ ){
      for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
    	uint16_t new_head=( m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
    	if( new_head!=head[ichan] ){
	  lgflag=true;
	  //std::cout << "We have a major issue in chip,chan " << std::dec<<chipIndex<<","<<ichan << " : (LG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
	}
      }
    }
    for( int ichan=0; ichan<64; ichan++ ){
      for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
    	uint16_t new_head=( m_decoded_raw_data.at(offset+ichan+ADCHIGH_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
    	if( new_head!=head[ichan] ){
	  hgflag=true;
	  //std::cout << "We have a major issue in chip,chan " << std::dec<<chipIndex<<","<<ichan << " : (HG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
	}
      }
    }
    if( lgflag )
      std::cout << "Major issue in ADC headers of chip " << std::dec<<chipIndex<<" (LG)" << std::endl;
    if( hgflag )
      std::cout << "Major issue in ADC headers of chip " << std::dec<<chipIndex<<" (HG)" << std::endl;
    chipIndex++;
    offset=chipIndex*1924;
    if( offset>=m_decoded_raw_data.size() )
      break;
  }
}

void unpacker::printDecodedRawData()
{
  unsigned int chipIndex=0;
  unsigned int offset=0;
  while(1){
    std::bitset<13> rollMask( m_decoded_raw_data[1920+offset]&MASK_ROLL );
    std::cout << std::dec << "chip ID" << chipIndex << " rollMask = " << (m_decoded_raw_data[1920+offset]&MASK_ROLL) << " = " << rollMask << std::endl;
    // // Low gain, TOA fall, TOT fast 
    for( int ichan=0; ichan<64; ichan++ ){
      std::cout << "channel " << 63-ichan << "\t";
      for(int isca=0; isca<NUMBER_OF_SCA; isca++)
    	std::cout << std::dec << gray_to_binary( m_decoded_raw_data.at(offset+ichan+ADCLOW_SHIFT+SCA_SHIFT*isca) & MASK_ADC ) << " ";
      std::cout << std::endl;
    }
    // High gain, TOA rise, TOT slow 
    for( int ichan=0; ichan<64; ichan++ ){
      std::cout << "channel " << 63-ichan << "\t";
      for(int isca=0; isca<NUMBER_OF_SCA; isca++)
    	std::cout << std::dec << gray_to_binary( m_decoded_raw_data.at(offset+ichan+ADCHIGH_SHIFT+SCA_SHIFT*isca) & MASK_ADC ) << " ";
      std::cout << std::endl;
    }
    chipIndex++;
    offset=chipIndex*1924;
    if( offset>=m_decoded_raw_data.size() )
      break;
  }
}

uint16_t unpacker::gray_to_binary(uint16_t gray) const
{
  gray ^= (gray >> 8);
  gray ^= (gray >> 4);
  gray ^= (gray >> 2);
  gray ^= (gray >> 1);
  return gray;
}


void unpacker::checkTimingSync()
{
  std::map<int,uint64_t> prevTime;
  std::map<int,int> prevTrig;
  std::map<int,uint64_t> diffTime;
  int firstTrig=(m_timestampmap.begin()->first>>8);
  int maxOrmId=0;
  //std::cout << "first trigger of the run : " << std::dec<<firstTrig << ";\t first time stamps = ";
  for( std::map< uint32_t,uint64_t>::iterator it=m_timestampmap.begin(); it!=m_timestampmap.end(); ++it ){
    if( (it->first>>8) != firstTrig )
      break;
    else{
      int ormid=it->first&0xff;
      prevTime.insert( std::pair<int,uint64_t>(ormid,it->second) );
      prevTrig.insert( std::pair<int,int>(ormid,it->first>>8) );
      prevTrig.insert( std::pair<int,int>(ormid,0) );
      //std::cout << std::dec << it->second << " ";
      if( maxOrmId < ormid ) maxOrmId=ormid;
    }
  }
  //std::cout << std::endl;
  
  for( std::map< uint32_t,uint64_t>::iterator it=m_timestampmap.begin(); it!=m_timestampmap.end(); ++it ){
    int trigId=(it->first)>>8;
    int ormId=(it->first)&0xff;
    diffTime[ormId]=it->second-prevTime[ormId];
    //std::cout << std::dec << "trigger id = " << trigId << "\t orm id = " << ormId << "\t trigger diff = " << trigId-prevTrig[ormId] << "\t time diff = " << diffTime[ormId] << std::endl;
    prevTrig[ormId]=trigId;
    prevTime[ormId]=it->second;

    if( ormId==maxOrmId )
      for( std::map<int,uint64_t>::iterator it=diffTime.begin(); it!=diffTime.end(); ++it ){
	if( it->second-diffTime.begin()->second != 0 )
	  std::cout << "There is a timing issue, trigger id = " << trigId << std::endl;
      }
  }
}
