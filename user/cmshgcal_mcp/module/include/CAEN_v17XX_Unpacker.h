//code adapted from here:
//https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1742.cpp
//Date of access: 08 August 2018

#ifndef CAENv1742Unpacker_H
#define CAENv1742Unpacker_H

#define MAX_DIGI_SAMPLES 100000

#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <algorithm>

#include "eudaq/Logger.hh"


struct digiData {
	unsigned int group ;
	unsigned int frequency ;
	unsigned int channel ;
	unsigned int sampleIndex ;
	float sampleValue ;
};



class CAEN_V1742_Unpacker {
public :

	CAEN_V1742_Unpacker () :
		dig1742channels_ (0),
		isDigiSample_ (0),
		nChannelWords_ (0),
		nSamplesToReadout_ (0),
		nSamplesRead_ (0),
		channelId_ (-1),
		groupId_ (-1)
	{} ;

	int Unpack (std::vector<uint32_t>&, std::vector<digiData>&, unsigned int &triggerTimeTag) ;

private :
	unsigned int dig1742channels_ ;
	bool isDigiSample_ ;
	unsigned int digRawData_ ;
	float digRawSample_ ;

	int nChannelWords_ ;
	int nSamplesToReadout_ ;
	int nSamplesRead_ ;
	int channelId_ ;
	int groupId_ ;
	uint32_t frequency_ ;

};

#endif

