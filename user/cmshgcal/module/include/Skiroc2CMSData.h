#include <iostream>
#include <vector>
#include <array>

static const int MASK_ADC = 0x0FFF;
static const int MASK_ROLL = 0x1FFF;
static const int MASK_GTS_MSB = 0x2FFF;
static const int MASK_GTS_LSB = 0x1FFF;
static const int MASK_ID = 0xFF;
static const int MASK_HEAD = 0xF000;
static const int NUMBER_OF_SCA = 13;
static const int ADCLOW_SHIFT = 0;
static const int ADCHIGH_SHIFT = 64;
static const int SCA_SHIFT = 128;
static const int CHIP_DATA_SIZE = 1924; //number of 16 bits words
static const int N_CHIPS_PER_HEXABOARD = 4;
static const int N_CHANNELS_PER_CHIP = 64;

typedef std::array<uint16_t,CHIP_DATA_SIZE> Skiroc2CMSRawData ;

class Skiroc2CMSData{
 public:
  Skiroc2CMSData(const Skiroc2CMSRawData &data, int chipId);
  ~Skiroc2CMSData(){;}
  
  uint16_t ADCLow(int chan,int TS)  const {chan=N_CHANNELS_PER_CHIP-1-chan; int sca=NUMBER_OF_SCA-1-m_TS_to_SCA[TS]; return (sca>=0 && sca<NUMBER_OF_SCA) ? grayToBinary( m_data.at( chan+ADCLOW_SHIFT+SCA_SHIFT*sca ) & MASK_ADC ) : 10000;}
  uint16_t ADCHigh(int chan,int TS) const {chan=N_CHANNELS_PER_CHIP-1-chan; int sca=NUMBER_OF_SCA-1-m_TS_to_SCA[TS]; return (sca>=0 && sca<NUMBER_OF_SCA) ? grayToBinary( m_data.at(chan+ADCHIGH_SHIFT+SCA_SHIFT*sca ) & MASK_ADC ) : 10000;}
  uint16_t TOTFast(int chan) const {chan=N_CHANNELS_PER_CHIP-1-chan; return grayToBinary( m_data.at( chan+ADCLOW_SHIFT+SCA_SHIFT*(NUMBER_OF_SCA+1)) & MASK_ADC );}
  uint16_t TOTSlow(int chan) const {chan=N_CHANNELS_PER_CHIP-1-chan; return grayToBinary( m_data.at(chan+ADCHIGH_SHIFT+SCA_SHIFT*(NUMBER_OF_SCA+1)) & MASK_ADC );}
  uint16_t TOAFall(int chan) const {chan=N_CHANNELS_PER_CHIP-1-chan; return grayToBinary( m_data.at( chan+ADCLOW_SHIFT+SCA_SHIFT*NUMBER_OF_SCA) & MASK_ADC );}
  uint16_t TOARise(int chan) const {chan=N_CHANNELS_PER_CHIP-1-chan; return grayToBinary( m_data.at(chan+ADCHIGH_SHIFT+SCA_SHIFT*NUMBER_OF_SCA) & MASK_ADC );}

  void printDecodedRawData();
  void rawDataSanityCheck();
  int chipID() const{ return m_chipId; }
 private:
  Skiroc2CMSRawData m_data;
  int m_TS_to_SCA[NUMBER_OF_SCA];
  int m_SCA_to_TS[NUMBER_OF_SCA];
  int m_chipId;
  uint16_t grayToBinary(uint16_t gray) const;
  
};
