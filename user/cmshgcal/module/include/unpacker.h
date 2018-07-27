#include <iostream>
#include <vector>
#include <map>

class unpacker{
 public:
 unpacker(int nskiroc_per_hexaboard=4) : m_nskiroc_per_hexaboard(m_nskiroc_per_hexaboard) {;}
  ~unpacker(){;}

  std::vector<uint16_t>& getDecodedData(){return m_decoded_raw_data;}
  
  void unpack_and_fill(std::vector<uint32_t> &raw_data);
  void printDecodedRawData();
  void rawDataSanityCheck();
  void checkTimingSync();
  uint64_t lastTimeStamp(){return m_lastTimeStamp;}
  int lastTriggerId(){return m_lastTriggerId;}
 private:
  std::vector<uint16_t> m_decoded_raw_data;
  std::map< uint32_t,uint64_t > m_timestampmap;
  uint64_t m_lastTimeStamp;
  int m_lastTriggerId;
  
 private:
  uint16_t gray_to_binary(uint16_t gray) const;
  int m_nskiroc_per_hexaboard;

  static const int MASK_ADC = 0x0FFF;
  static const int MASK_ROLL = 0x1FFF;
  static const int MASK_GTS_MSB = 0x3FFF;
  static const int MASK_GTS_LSB = 0x1FFF;
  static const int MASK_ID = 0xFF;
  static const int MASK_HEAD = 0xF000;
  
  static const int NUMBER_OF_SCA = 15;       // NUMBER_OF_SCA = 13 SCA + TOT and TOA
  static const int ADCLOW_SHIFT = 0;
  static const int ADCHIGH_SHIFT = 64;
  static const int SCA_SHIFT = 128;
};
