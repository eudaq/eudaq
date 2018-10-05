#include "CAEN_v1290_Unpacker.h"

//#define DEBUG_UNPACKER

#include <string>

//implementation adapted from H4DQM: https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1290.cpp
//CAEN v1290 manual found in: http://www.tunl.duke.edu/documents/public/electronics/CAEN/caen_v1290.pdf
int CAENv1290Unpacker::Unpack (std::vector<uint32_t>& Words, tdcData* currentData) {
  currentData->extended_trigger_timestamp = 0;

  for (size_t i = 0; i < Words.size(); i++) {
    uint32_t currentWord = Words[i];

#ifdef DEBUG_UNPACKER
    std::cout << "TDC WORD: " << currentWord << std::endl;
    std::cout << "TDC first 5 bits: " << (currentWord >> 27) << std::endl;
#endif
    //bits 27-31 (starting at 0) contain the header, changed on 09th August by T.Q.
    if (currentWord >> 27 == 10 ) { //TDC BOE
      unsigned int tdcEvent = (currentWord) & 0xFFFFFF;
      currentData->event = tdcEvent;
#ifdef DEBUG_UNPACKER
      std::cout << "[CAEN_V12490][Unpack] | TDC 1290 BOE: event " << tdcEvent + 1 << std::endl;
#endif
      continue;
    }

    else if (currentWord >> 27 == 0) { //TDC DATUM
      unsigned int channel = (currentWord >> 21) & 0x1f; //looks at the bits 21 - 26
      unsigned int measurement = currentWord & 0x1fffff;  //looks at bits 0 - 20

#ifdef DEBUG_UNPACKER
      std::cout << "[CAEN_V12490][Unpack] | tdc 1290 board channel " << channel + " measurement " << measurement << std::endl;
#endif

      //check if channel exists, if not create it
      if (timeStamps.find(channel) == timeStamps.end()) {
        timeStamps[channel] = std::vector<unsigned int>();
      }

      timeStamps[channel].push_back(measurement);
      continue;
    }

    else if (currentWord >> 27 == 0x11) { //TDC extended trigger time tag: CAEN_V1290_GLBTRTIMETAG=0x11
#ifdef DEBUG_UNPACKER
      std::cout << "[CAEN_V12490][Unpack] | TDC 1290 extended trigger time tag " << std::endl;
#endif
      currentData->extended_trigger_timestamp = currentWord & 0x7ffffff;  //looks at bits 0 - 26
    }

    else if (currentWord >> 27 == 16) { //TDC EOE
#ifdef DEBUG_UNPACKER
      std::cout << "[CAEN_V12490][Unpack] | TDC 1290 BOE: end of event " << std::endl;
#endif
      break;
    }

    else if (currentWord >> 27 == 4) {
      unsigned int errorFlag = currentWord & 0x7FFF;
      std::cout << "TDC has error with flag: " << errorFlag << std::endl;
      break;
    }

    else if (currentWord >> 27 == 7) {
      unsigned int errorFlag = currentWord & 0x7FFF;
      std::cout << "Unknown word type: " << errorFlag << std::endl;
      break;
    }

  }

  return 0 ;
}


tdcData* CAENv1290Unpacker::ConvertTDCData(std::vector<uint32_t>& Words) {

  for (std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    channelTimeStamps->second.clear();
  }
  timeStamps.clear();

  tdcData* currentData = new tdcData;

  //unpack the stream
  Unpack(Words, currentData);

  for (int ch = 0; ch < N_channels; ch++) {
    currentData->timeOfArrivals[ch] = 0;  //fill all 16 channels with a value indicating that no hit has been registered
    currentData->hits[ch].clear();
  }

  for (std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    if (channelTimeStamps->second.size() == 0) continue;

    unsigned int this_channel = channelTimeStamps->first;
    currentData->timeOfArrivals[this_channel] = *min_element(channelTimeStamps->second.begin(), channelTimeStamps->second.end());
    currentData->hits[this_channel] = channelTimeStamps->second;
#ifdef DEBUG_UNPACKER
    std::cout << "Number of hits in channel: " << this_channel << " :  " << currentData->hits[this_channel].size() << std::endl;
#endif
  }

  return currentData;
}
