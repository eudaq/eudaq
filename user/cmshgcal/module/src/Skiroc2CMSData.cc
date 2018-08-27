#include "Skiroc2CMSData.h"
#include <bitset>

Skiroc2CMSData::Skiroc2CMSData(const Skiroc2CMSRawData &data, int chipId)
{
  m_data=data;
  m_chipId=chipId;

  uint16_t rollMask = m_data.at(CHIP_DATA_SIZE-4)&MASK_ROLL;

  std::bitset<NUMBER_OF_SCA> bitstmp(rollMask);
  std::bitset<NUMBER_OF_SCA> bits;
  for( size_t i=0; i<NUMBER_OF_SCA; i++ )
    bits[i]=bitstmp[12-i];
  
  if(bits.test(0)&&bits.test(12)){
    m_SCA_to_TS[0]=12;
    m_TS_to_SCA[12]=0;
    for(size_t i=1; i<NUMBER_OF_SCA; i++){
      m_SCA_to_TS[i]=i-1;
      m_TS_to_SCA[i-1]=i;
    }
  }
  else{
    int pos_trk1 = -1;
    for(size_t i=0; i<NUMBER_OF_SCA; i++)
      if(bits.test(i)){
	pos_trk1 = i;
	break;
      }
    for(size_t i=0; i<NUMBER_OF_SCA; i++)
      if( (int)i <= pos_trk1 + 1 ){
	m_SCA_to_TS[i]=i + 12 - (pos_trk1 + 1);
	m_TS_to_SCA[i + 12 - (pos_trk1 + 1)]=i;
      }
      else{
	m_SCA_to_TS[i]=i - 1 - (pos_trk1 + 1);
    	m_TS_to_SCA[i - 1 - (pos_trk1 + 1)]=i;
      }
  }

}

uint16_t Skiroc2CMSData::grayToBinary(uint16_t gray) const
{
  gray ^= (gray >> 8);
  gray ^= (gray >> 4);
  gray ^= (gray >> 2);
  gray ^= (gray >> 1);
  return gray;
}

void Skiroc2CMSData::rawDataSanityCheck()
{
  std::bitset<13> rollMask( m_data[1920]&MASK_ROLL );
  if( rollMask.count()!=2 ){
    std::cout << "Problem: wrong roll mask format in chip " << std::dec << m_chipId << "\t rollMask = "<< rollMask << std::endl;
  }
  uint16_t head[64];
  for( int ichan=0; ichan<64; ichan++ ){
    head[ichan]=(m_data.at(ichan+ADCLOW_SHIFT) & MASK_HEAD)>>12;
    if(head[ichan]!=8&&head[ichan]!=9)
      std::cout << "ISSUE in chip,chan " << std::dec<< m_chipId <<","<<ichan << " : we expected 8(1000) or 9(1001) for the adc header and I find " << std::bitset<4>(head[ichan]) << std::endl;
  }
  bool lgflag(false),hgflag(false);
  for( int ichan=0; ichan<64; ichan++ ){
    for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
      uint16_t new_head=( m_data.at(ichan+ADCLOW_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
      if( new_head!=head[ichan] ){
	lgflag=true;
	//std::cout << "We have a major issue in chip,chan " << std::dec<<m_chipId<<","<<ichan << " : (LG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
      }
    }
  }
  for( int ichan=0; ichan<64; ichan++ ){
    for(int isca=0; isca<NUMBER_OF_SCA-1; isca++){
      uint16_t new_head=( m_data.at(ichan+ADCHIGH_SHIFT+SCA_SHIFT*isca) & MASK_HEAD )>>12;
      if( new_head!=head[ichan] ){
	hgflag=true;
	//std::cout << "We have a major issue in chip,chan " << std::dec<<m_chipId<<","<<ichan << " : (HG)-> " << std::bitset<4>(new_head) << " should be the same as " << std::bitset<4>(head[ichan]) << std::endl;
      }
    }
  }
  if( lgflag ) std::cout << "Major issue in ADC headers of chip " << std::dec<<m_chipId<<" (LG)" << std::endl;
  if( hgflag ) std::cout << "Major issue in ADC headers of chip " << std::dec<<m_chipId<<" (HG)" << std::endl;
}

void Skiroc2CMSData::printDecodedRawData()
{
  std::bitset<13> rollMask( m_data[1920]&MASK_ROLL );
  std::cout << std::dec << "chip ID = " << m_chipId << " rollMask = " << (m_data[1920]&MASK_ROLL) << " = " << rollMask << std::endl;
  // Low gain, TOA fall, TOT fast 
  for( int ichan=0; ichan<64; ichan++ ){
    std::cout << "channel " << ichan << "\t LG ADC (time ordered+toafall+totfast) : ";
    for(int isca=0; isca<NUMBER_OF_SCA; isca++)
      std::cout << std::dec << ADCLow(ichan,isca) << " ";
    std::cout << std::dec << TOAFall(ichan) << " " << TOTFast(ichan);
    std::cout << std::endl;
  }
  // High gain, TOA rise, TOT slow 
  for( int ichan=0; ichan<64; ichan++ ){
    std::cout << "channel " << ichan << "\t HG ADC (time ordered+toarise+totslow) : ";
    for(int isca=0; isca<NUMBER_OF_SCA; isca++)
      std::cout << std::dec << ADCHigh(ichan,isca) << " ";
    std::cout << std::dec << TOARise(ichan) << " " << TOTSlow(ichan);
    std::cout << std::endl;
  }
}
