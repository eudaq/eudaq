#include "CAEN_v17XX_Unpacker.h"
#include <bitset>
#define DEBUG_UNPACKER 0
#define DEBUG_VERBOSE_UNPACKER 0


//          ====================================================
//          |           V1742 Raw Event Data Format            |
//          ====================================================

//                       31  -  28 27  -  16 15   -   0
//            Word[0] = [ 1010  ] [Event Tot #Words  ] //Event Header (5 words)
//            Word[1] = [     Board Id    ] [ Pattern]
//            Word[2] = [      #channels readout     ]
//            Word[3] = [        Event counter       ]
//            Word[4] = [      Trigger Time Tag      ]
//            Word[5] = [ 1000  ] [    Ch0   #Words  ] // Ch0 Data (2 + #samples words)
//            Word[6] = [    Ch0  #Gr    ] [ Ch0 #Ch ]
//            Word[7] = [ Ch0 Corr. samples  (float) ]
//                ..  = [ Ch0 Corr. samples  (float) ]
// Word[5+Ch0 #Words] = [ 1000  ] [    Ch1   #Words  ] // Ch1 Data (2 + #samples words)
// Word[6+Ch0 #Words] = [    Ch1  #Gr    ] [ Ch1 #Ch ]
// Word[7+Ch0 #Words] = [ Ch1 Corr. samples  (float) ]
//              ...   = [          .....             ]

/*
  (CAEN_V1742_eventBuffer)[0]=0xA0000005 ;
  (CAEN_V1742_eventBuffer)[1]=( (eventInfo->BoardId)<<26) +eventInfo->Pattern ;
  (CAEN_V1742_eventBuffer)[2]=0 ;
  (CAEN_V1742_eventBuffer)[3]=eventInfo->EventCounter ;
  (CAEN_V1742_eventBuffer)[4]=eventInfo->TriggerTimeTag ;
 */


int CAEN_V1742_Unpacker::Unpack (std::vector<uint32_t>& Words, std::vector<digiData>& samples, unsigned int &triggerTimeTag) {
	isDigiSample_ = 0 ;


	//PG loop over board words (the external header was already read in SpillUnpacker
	for (unsigned int i = 0 ; i < Words.size() ; ++i) {
		if (!isDigiSample_) digRawData_ = Words[i];
		else std::memcpy(&digRawSample_, &Words[i], sizeof(uint32_t)) ;

		if (i == 0) {
			short dt_type = digRawData_ >> 28 & 0xF; //BOE is 1010 (0xA)
			if (dt_type != 0xA) {
				std::cout << "[CAEN_V1742][Unpack]       | ERROR: DIGI 1742 BLOCK SEEMS CORRUPTED\n" ;
			}
			unsigned int nWords = digRawData_ & 0xFFFFFFF;
			if (DEBUG_UNPACKER) {
				std::cout << "[CAEN_V1742][Unpack]       | number of words " << nWords << "\n" ;
			}

			//std::cout << "[CAEN_V1742][Unpack]       | ERROR: HEY MISMATCH IN DIGI 1742 #WORDS: "
			//         << 4 * nWords << " != " << (dig1742Words_ - 16) << "\n" ;

		} else if (i == 1) {
			if (DEBUG_UNPACKER) {
				std::cout << "[CAEN_V1742][Unpack]       | DIGI 1742: board ID and pattern " << digRawData_ << "\n" ;
			}
		}
		else if (i == 2) {
			dig1742channels_ = digRawData_;
		}
		else if (i == 3) {
			unsigned int digiEvt = digRawData_;
			if (DEBUG_UNPACKER) {
				std::cout << "[CAEN_V1742][Unpack]       | DIGI 1742 BOE: event " << digiEvt + 1 << "\n" ;
			}
		}
		else if (i == 4) {
			triggerTimeTag = digRawData_;
			if (DEBUG_UNPACKER) {
				std::cout << "[CAEN_V1742][Unpack]       | DIGI 1742 BOE: trigger time tag " << triggerTimeTag << "\n" ;
			}
		}
		else if (i > 4) {
			if (!isDigiSample_) {
				if (0 == nSamplesToReadout_) {
					//This is the ChHeader[0]
					short dt_type = digRawData_ >> 28 & 0xF ; //ChHeader is 1000 (0x8)

					frequency_ = digRawData_ >> 26 & 0x3 ;
					// 00 = 5GS/s
					// 01 = 2.5GS/s
					// 10 = 1GS/s
					// 11 = not used
					if (DEBUG_UNPACKER) {
						std::cout << "[CAEN_V1742][Unpack]       | data taking frequency : " << frequency_ << " : " << std::bitset<2> (frequency_) << "\n" ;
					}
					if (dt_type != 0x8) {
						std::cout << "[CAEN_V1742][Unpack]       | ERROR: DIGI 1742 BLOCK SEEMS CORRUPTED\n" ;
						std::cout << "[CAEN_V1742][Unpack]       | ERROR: CH header : " << std::bitset<32> (digRawData_) << "\n" ;
						std::cout << "[CAEN_V1742][Unpack]       | ERROR:             10987654321098765432109876543210\n" ;
						std::cout << "[CAEN_V1742][Unpack]       | ERROR: quitting unpacking\n" ;
						break ;
					}
					unsigned int nChWords = digRawData_ & 0x3FFFFFF ;
					//                  unsigned int nChWords2 = digRawData_ & 0xFFF ;
					//PG FIXME check whether the second is better, when the trigger digitisation is on
					if (DEBUG_UNPACKER) {
						std::cout << "[CAEN_V1742][Unpack]       | NEW CHANNEL size : " << nChWords << "\n" ;
					}
					nSamplesToReadout_ = nChWords ;
					nChannelWords_ = nChWords ;
					nSamplesRead_ = 0 ;
					--nSamplesToReadout_ ;
				}
				else {
					//This is the ChHeader[1]
					int gr = digRawData_ >> 16 & 0xFFFF ;
					int ch = digRawData_ & 0xFFFF ;
					channelId_ = ch ;
					groupId_ = gr ;
					--nSamplesToReadout_ ;
					if (DEBUG_UNPACKER) {
						std::cout << "[CAEN_V1742][Unpack]       | group " << groupId_ << " channel " << channelId_ << "\n" ;
					}
					//Next sample will be a sample and should be read as a float
					isDigiSample_ = 1 ;
				}
			}
			else {
				//This is a sample!
				digiData aDigiSample ;
				aDigiSample.channel = channelId_ ;
				aDigiSample.group = groupId_ ;
				aDigiSample.frequency = frequency_ ;
				aDigiSample.sampleIndex = nSamplesRead_ ;
				aDigiSample.sampleValue = digRawSample_ ;
				if (DEBUG_VERBOSE_UNPACKER) {
					std::cout << "[CAEN_V1742][Unpack]       | sample "
					          << aDigiSample.sampleIndex
					          << " : " << aDigiSample.sampleValue << "\n" ;
				}
				samples.push_back (aDigiSample) ;

				if (1 == nSamplesToReadout_) {
					//Next sample will be a ChHeader and should be read as as a uint
					isDigiSample_ = 0 ;
					if (nSamplesRead_ + 3 != nChannelWords_) {
						std::cout << "[CAEN_V1742][Unpack]       | ERROR: DIGI 1742 BLOCK SEEMS CORRUPTED:"
						          << " NOT ALL THE SAMPLES WERE READOUT FOR CHANNEL "
						          << channelId_ << " IN GROUP " << groupId_ << "\n"
						          << "[CAEN_V1742][Unpack]       |       expected " << nChannelWords_ - 3
						          << ", read " << nSamplesRead_ << "\n" ;
					}
				}
				--nSamplesToReadout_ ;
				++nSamplesRead_ ;
			}
		}
	}

	return 0 ;

} ;
