#include <iostream>
#include <iomanip>
#include <bitset>
#include <array>
#include "unpacker.h"

void unpacker::unpack_and_fill(std::vector<uint32_t> &raw_data)
{
  m_skiroc2cms_data.clear();
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
  m_lastTimeStamp = ((uint64_t)raw_data[ raw_data.size()-1 ]<<32) | (uint64_t)raw_data[ raw_data.size()-2 ]  ;
  m_lastTriggerId = raw_data[raw_data.size()-3]>>8;
  m_ormID = raw_data[raw_data.size()-3]&0xff;//should be between 0 and 15
  m_timestampmap.insert( std::pair<uint32_t,uint64_t>(raw_data[raw_data.size()-3],m_lastTimeStamp) );
  int chipIndex=0;//from 0 to 31
  for( std::vector< std::array<uint16_t, 1924> >::iterator it=skirocs.begin(); it!=skirocs.end(); ++it ){
    Skiroc2CMSData sk( (*it),m_ormID*100+chipIndex );
    chipIndex++;
    m_skiroc2cms_data.push_back(sk);
  } 
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
