#ifndef CAENv1290Unpacker_H
#define CAENv1290Unpacker_H

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <algorithm>

#include "eudaq/Logger.hh"

#define WORDSIZE 4

struct tdcData {
  int event;
  int ID;
  std::map<unsigned int, unsigned int> timeOfArrivals;  //like in September 2016 test-beam: minimum of all read timestamps
  std::map<unsigned int, std::vector<unsigned int> >hits;
  unsigned int extended_trigger_timestamp;

} ;

#define DEBUG_BOARD 0

class CAENv1290Unpacker {
public:
  CAENv1290Unpacker(int N) {N_channels = N;}
  tdcData* ConvertTDCData(std::vector<uint32_t>&);

private:
  int Unpack (std::vector<uint32_t>&, tdcData*) ;
  std::map<unsigned int, std::vector<unsigned int> > timeStamps;
  int N_channels;
};

#endif