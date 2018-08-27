#include <iostream>
#include <vector>
#include <map>

#include "Skiroc2CMSData.h"

class unpacker{
 public:
 unpacker(int nskiroc_per_hexaboard=4) : m_nskiroc_per_hexaboard(m_nskiroc_per_hexaboard) {;}
  ~unpacker(){;}

  std::vector<Skiroc2CMSData>& getChipData(){return m_skiroc2cms_data;}
  
  void unpack_and_fill(std::vector<uint32_t> &raw_data);
  void checkTimingSync();
  uint64_t lastTimeStamp(){return m_lastTimeStamp;}
  int lastTriggerId(){return m_lastTriggerId;}
  uint16_t lastOrmID(){return m_ormID;}
 private:
  std::vector<Skiroc2CMSData> m_skiroc2cms_data;
  std::map< uint32_t,uint64_t > m_timestampmap;
  uint64_t m_lastTimeStamp;
  int m_lastTriggerId;
  uint16_t m_ormID;
 private:
  int m_nskiroc_per_hexaboard;
};
