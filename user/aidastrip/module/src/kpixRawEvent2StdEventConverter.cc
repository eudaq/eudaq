/*
 * Author: Mengqing <mengqing.wu@desy.de>
 * Date: 2019 Jun 20
 * Note: based on new KPiX DAQ for Lycoris telescope.
 */

#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
//--> ds
#include "Data.h"
#include "XmlVariables.h"
#include "KpixEvent.h"
#include "KpixSample.h"

#include <map>
#include <tuple>
#include <vector>
#include <math.h> // fabs

#include "TH1.h"

#include "kpix_left_and_right.h"
//~LoCo 08/08 histogram (TODO: optimize)
//#include <TH1.h>
#define maxlength 5000


//~LoCo 08/08 histogram (TODO: optimize)
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TRandom.h"
#include "TF1.h"
#define maxlength 5000

// Template for useful funcs for this class:
double smallest_time_diff( vector<double> ext_list, int int_value);
void timestamp_histo_test(char* outStr);

// Real Class start:
class kpixRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	kpixRawEvent2StdEventConverter();
	
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvent");
	
	void parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle, bool isSelfTrig ) const;
	std::map<std::pair<int,int>, double> createMap(const char* filename);//~LoCo 05/08

  	//~LoCo 02/08: added third output to parseSample, ADC value. 05/08: fourth, bucket=0.
  	std::tuple<int, int, int, uint> parseSample( KpixSample* sample, std::vector<double>   vec_ExtTstamp,  bool isSelfTrig) const;
	
	//~LoCo 07/08 ConvertADC2fC: called inside parseFrame. 08/08 added array as fourth input
	int ConvertADC2fC( int hitX, int planeID, int hitVal, double* array1_169 ) const;

private:
	int getStdPlaneID(uint kpix) const;
	int getStdKpixID(uint hitX, int planeID) const;


	unordered_map<uint, uint> _lkpix2strip = kpix_left();
	unordered_map<uint, uint> _rkpix2strip = kpix_right();

	//~LoCo 05/08: can call map in monitor with .at
	std::map<std::pair<int,int>, double> Calib_map = createMap("/home/lorenzo/data/real_calib/calib_normalgain.txt");
	bool                      _pivot = false; // for StdPlane class which is designed for Mimosa;
	
};

namespace{
	auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
		Register<kpixRawEvent2StdEventConverter>(kpixRawEvent2StdEventConverter::m_id_factory);
}

kpixRawEvent2StdEventConverter::kpixRawEvent2StdEventConverter() {
	createMap("/home/lorenzo/data/real_calib/calib_normalgain.txt");
}

bool kpixRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
	
	/* It loops over [eudaq raw events] == [kpix acq. cycles] */
	auto rawev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
	if (!rawev)
		return false;
	// check eudaq/StdEventConverter.cc: already done with Convert():
	// // Copy from NiRawEvent2StdEventConverter @ Jun 20
	// if(!d2->IsFlagPacket()){
	// 	d2->SetFlag(d1->GetFlag());
	// 	d2->SetRunN(d1->GetRunN());
	// 	d2->SetEventN(d1->GetEventN());
	// 	d2->SetStreamN(d1->GetStreamN());
	// 	d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
	// 	d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
	// }
	
	KpixEvent    cycle;

	string triggermode = d1->GetTag("triggermode", "internal");
	bool isSelfTrig = triggermode == "internal" ? true:false ;
	
	if (isSelfTrig)
	  EUDAQ_INFO("I am using Self Trigger for kpix Event Converter :)");
	
	std::vector<uint8_t> block = rawev->GetBlock(0); // related to SendEvent(ev) @ _EuDataTools
	if (block.size() == 0 ){
		EUDAQ_THROW("empty data");
		return false;
	}
	
	size_t size_of_kpix = block.size()/sizeof(uint32_t);
	std::cout<<"[dev] # of block.size()/sizeof(uint32_t) = " << block.size()/sizeof(uint32_t) << "\n"
	         <<"\t sizeof(uint32_t) = " << sizeof(uint32_t) << std::endl;
	
	uint32_t *kpixEvent = nullptr;
	if (size_of_kpix)
		kpixEvent = reinterpret_cast<uint32_t *>(block.data());
	else{
		EUDAQ_WARN( "Ignoring bad cycle with EventN = " + std::to_string(rawev->GetEventNumber()) );
		return false;
	}

	/*prepare the planes for later parsing frame to work on*/
	// TODO: hard coded to be 6 plane. - MQ
	for (auto id=0; id<6; id++){
		eudaq::StandardPlane plane(id, "lycoris", "lycoris");
		plane.SetSizeZS(1840, // x, width, TODO: strip ID of half sensor
		                1, // y, height
		                0 // not used 
		                ); // TODO: only 1 frame, define for us as bucket, i.e. only bucket 0 is considered
		plane.Print(std::cout);
		d2->AddPlane(plane);
	}


	  //~LoCo 02/08: Lookup table TEST. 05/08:

	  if (Calib_map.empty()) std::cout << "OH NOOOOO" << std::endl;

	  //JUST A TEST
	  std::cout << "The slope is " << Calib_map.at(std::make_pair(2,1117)) << std::endl;
	  //JUST A TEST TO SEE WHEN IT DOES !NOT! WORK
	  //std::cout << Calib_map.at(std::make_pair(1,1079)) << std::endl;

	  // /* read kpix data */
	  cycle.copy(kpixEvent, size_of_kpix);

	  parseFrame(d2, cycle, isSelfTrig);
	
	  // std::cout << "\t Uint32_t  = " << kpixEvent << "\n"
	  //           << "\t evtNum    = " << kpixEvent[0] << std::endl;	
	  // std::cout << "\t kpixEvent.evtNum = " << cycle.eventNumber() <<std::endl;
	  // std::cout << "\t kpixEvent.timestamp = "<< cycle.timestamp() <<std::endl;
	  // std::cout << "\t kpixEvent.count = "<< cycle.count() <<std::endl;
      
  	  return true;
}

// ~PARSEFRAME~

void kpixRawEvent2StdEventConverter::parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle, bool isSelfTrig) const{

	d2->Print(std::cout);
	std::vector<double>   vec_ExtTstamp;

	vec_ExtTstamp.clear();
	/* globally set variable holders*/
	KpixSample   *sample;
	//uint         emptysamples=0;
	uint         is;
	double       bunchClk;
	uint         subCount;
	KpixSample::SampleType type;

	/*
	 * Jul 25, 2019 MQ
	 + *logic idea*: 
	   1. loop all the samples first to get the external timestamp into a vector as Uwe's analysis_newdaq.cxx
	   2. then process it again to read Data.
	 + DONE: x coordinate reversed for Planes 0, 3, 4: now every plane has same axis. ~LoCo 01/08
	 + TODO! add a switch to tell if you are self-trig or ext-trig
	 */
	 for (is = 0; is < cycle.count(); is++){

	  	sample = cycle.sample(is);
	  	bunchClk = sample -> getBunchCount();
	  	subCount = sample -> getSubCount();
	  	type     = sample -> getSampleType();
	  
	  	if (type == 2){ // external timestamp
	    		double time = bunchClk + double(subCount * 0.125); 
	    		vec_ExtTstamp.push_back(time);
	  	}
	  }
	
	/* Jun 20, 2019 MQ
	   + *idea*: once you read one kpixID, use the m_ids to find the plane ID, Get it from d2, and pushPixel().
	   + DONE: one needs to fix how to cheat EUDAQ that we are a huge pixel intead of strips...
	   Jul 4, 2019 MQ
	   + DONE: PushPixel happend here, found Plane with a function.
	*/

	  //~LoCo tests
	  printf("[test] I am a test! --\n");
	  std::cout << "Number of planes : "<< d2->NumPlanes() << std::endl;

	  printf("[dev] kpix ID,  channel, strip,  bucket,   ADC\n");
	  for (is=0; is<cycle.count(); is++){
	  sample = cycle.sample(is);

	  
	  
	  // parseSample: you need to return the Plane ID (based on kpix ID), and the fired strips positions for parseFrame to add to the Plane. TODO
	  auto hit     = parseSample(sample, vec_ExtTstamp, isSelfTrig);
	  if ( std::get<3>(hit) != 0 ) continue; // ignore bucket != 0 sample
	  auto planeID = std::get<0>(hit);
	  auto hitX    = std::get<1>(hit);
	  auto hitVal  = std::get<2>(hit);
	  
	  if ( planeID < 0 ) continue; // ignore non-DATA sample
	  if ( hitX == 9999 ) continue; // ignore not connected strips
	  
	  // PushPixel here. ~LoCo 01/08: changed '>' to '>='. Tested and kept
	  if (planeID >= d2->NumPlanes()){
	    EUDAQ_WARN("Ignoring non-existing plane : " + planeID);
	    continue;
	  }

	  //~LoCo: hitX is changed for planes 0, 3, 4. Now all planes have same axis direction
	  if (planeID==0 | planeID==3 | planeID==4) hitX = 1839 - hitX;

	  std::cout << "[+] plane : "<< planeID << ", hitX at : " << hitX << std::endl;
	  auto &plane = d2->GetPlane(planeID);

	  plane.PushPixel(hitX, // x
		1,    // y, always to be 1 since we are strips
		1     // T pix
		);    // use bucket as input for frame_num

	  //~LoCo 08/08 histogram
	  double hitValues_chan0[maxlength] = {0};//Istogramma per il kpix

	  //~Loco 07/08
	  ConvertADC2fC(hitX, planeID, hitVal, hitValues_chan0);
	  		
	  }// - sample loop over

	  // debug:
	  auto &plane = d2->GetPlane(0);
	  std::cout << "debug: Num of Hit pixels: " << plane.HitPixels() << std::endl;

	  return;
}

//~LoCo 02/08: added third output to parseSample, the ADC value. 05/08: fourth, bucket=0.
std::tuple<int, int, int, uint> kpixRawEvent2StdEventConverter::parseSample(KpixSample* sample,
								 std::vector<double>   vec_ExtTstamp,
								 bool isSelfTrig) 
const{
  /*
    usage: 
    - decode the 2 * 32bit kpix [sample]
    - return tuple with positive kpix id, otherwise kpix id is negative
  */
  uint         kpix;
  uint         channel;
  int          strip = 9999;
  uint         bucket;
  uint         value;
  double       tstamp;
  double       hitCharge=-1;
  KpixSample::SampleType type;
  
  if (sample->getEmpty()){ 
    printf ("empty sample\n");
    return std::make_tuple(-1, strip, value, bucket);
  }
  
  type    = sample->getSampleType();
  if (type != KpixSample::Data) return std::make_tuple(-1, strip, value, bucket); // if not DATA, return
  
  kpix    = sample->getKpixAddress();
  channel = sample->getKpixChannel();
  bucket  = sample->getKpixBucket();
  value   = sample->getSampleValue();
  tstamp  = sample->getSampleTime();
  
  if (kpix%2 == 0) strip = _lkpix2strip.at(channel);
  else strip = _rkpix2strip.at(channel);

  // Cut- selftrig and extTrig time diff:
  double trig_diff = smallest_time_diff(vec_ExtTstamp, tstamp);
  if (  trig_diff < 0.0 && trig_diff > 3.0 )
    return std::make_tuple(-1, strip, value, bucket);
  
  if (strip != 9999)
    std::cout<<"\t"
	     << kpix     << "   "
	     << channel  << "   "
	     << strip    << "   "
	     << bucket   << "   "
	     << value    << "   " 
	     << std::endl;



	  hitCharge=ConvertADC2fC(channel, kpix, value);

  
  return std::make_tuple(getStdPlaneID(kpix), strip, value, bucket);

}

// ~GETSTDPLANEID~

int kpixRawEvent2StdEventConverter::getStdPlaneID(uint kpix) const{

  //TODO: feature to add, read kpix number with plane configuration from external txt file
  if ( kpix == 0  | kpix == 1  )  return 0; 
  if ( kpix == 2  | kpix == 3  )  return 1; 
  if ( kpix == 4  | kpix == 5  )  return 2;
  
  if ( kpix == 6  | kpix == 7  )  return 5;
  if ( kpix == 8  | kpix == 9  )  return 4;
  if ( kpix == 10 | kpix == 11 )  return 3; 
}

//~LoCo 05/08. Notice: this works only after hitX reversal. TODO: add bool variable 'reverse' for each plane, and use that as well
int kpixRawEvent2StdEventConverter::getStdKpixID(uint hitX, int planeID) const{

  if ( ( planeID == 0 ) && ( hitX <= 919 ) ) return 0;
  if ( ( planeID == 0 ) && ( hitX >= 920 ) ) return 1;
  if ( ( planeID == 1 ) && ( hitX <= 919 ) ) return 3;
  if ( ( planeID == 1 ) && ( hitX >= 920 ) ) return 2;
  if ( ( planeID == 2 ) && ( hitX <= 919 ) ) return 5;
  if ( ( planeID == 2 ) && ( hitX >= 920 ) ) return 4;
  if ( ( planeID == 3 ) && ( hitX <= 919 ) ) return 10;
  if ( ( planeID == 3 ) && ( hitX >= 920 ) ) return 11;
  if ( ( planeID == 4 ) && ( hitX <= 919 ) ) return 8;
  if ( ( planeID == 4 ) && ( hitX >= 920 ) ) return 9;
  if ( ( planeID == 5 ) && ( hitX <= 919 ) ) return 7;
  if ( ( planeID == 5 ) && ( hitX >= 920 ) ) return 6;

}

//~LoCo 05/08. File format: kpix\t channel \t bucket=0\t slope
std::map<std::pair<int,int>, double> kpixRawEvent2StdEventConverter::createMap(const char* filename){//~LoCo 02/08: Lookup table
		
	  std::ifstream myfile_calib;
	  myfile_calib.open(filename);
	  std::map<std::pair<int,int>, double> Calib_map;

	  if (! myfile_calib) return Calib_map;  

	  int map_a=-1, map_c, map_tmp;
	  uint map_b;
	  int map_check1=0, map_check2=0;
	  double map_d;

	  //Create Map. ~LoCo 07/08: key is <kpix,strip ID (from channel ID through left and right maps)>, value is slope
	  while ( true ) {
		
		map_tmp=map_a;
		myfile_calib >> map_a;//this is kpix ID

		if( myfile_calib.eof() ) break;

		if ( map_a != map_tmp) map_check1++;

		myfile_calib >> map_b;//this is strip ID, goes from 0 to 919 (left) or 920 to 1839 (right). 9999 doesn't matter at all!
		myfile_calib >> map_c;//this is bucket, has to be 0
		myfile_calib >> map_d;//this is slope

		if ( map_b == 1023 ) map_check2++;

		if( myfile_calib.eof() ) break;
		
		if ( map_c != 0 ) continue;

		//Finally can create map.
		if ( map_a%2 == 0 ) {
			Calib_map[std::make_pair(map_a, _rkpix2strip.at(map_b))] = map_d;
		}
		if ( map_a%2 != 0 ) {
			Calib_map[std::make_pair(map_a, _lkpix2strip.at(map_b))] = map_d;
		}
		
	  }

	  //if ( map_check1 != map_check2 ) return std::make_tuple(map_check1 - map_check2, Calib_map); TODO: do it with EUDAQ_LOGGER

	  myfile_calib.close();

	  return Calib_map;

}

//~LoCo 02/08: convert hitVal (ADC) to fC with Lookup table. 07/08: function to pass array with channel values
int kpixRawEvent2StdEventConverter::ConvertADC2fC( int hitX, int planeID, int hitVal, double* array1_169 ) const{

	  int conv_kpix=getStdKpixID(hitX, planeID);
	  int conv_strip=hitX;
	  int j1_169=0;
	  
	  if (planeID==0 | planeID==3 | planeID==4) conv_strip = 1839 - conv_strip;

	  double hitCharge;
std::cout << conv_kpix << "OHIOIHOA" << conv_strip << Calib_map.at(std::make_pair(conv_kpix,conv_strip)) << std::endl;
	  if ( Calib_map.at(std::make_pair(conv_kpix,conv_strip)) > 1.0 ) {

	  	hitCharge = hitVal/Calib_map.at(std::make_pair(conv_kpix,conv_strip));

		//Check result, might comment it later
	  	std::cout << "Conversion: kpix #" << conv_kpix << ", channel #" << conv_strip << ", hitVal " << hitVal << ", hitCharge " << hitCharge << std::endl;

	  }

	  else {
	  	std::cout << "!!NOTICE: channel excluded from conversion" << std::endl;
	  }
	  if (planeID==1 && conv_strip==169) {array1_169[j1_169]=hitCharge;j1_169++;}

	  return 3;
	  

}

//------ code for time diff:
double smallest_time_diff( vector<double> ext_list, int int_value){
  double trigger_diff = 8200.0;
  for (uint k = 0; k<ext_list.size(); ++k)
    {
      double delta_t = int_value-ext_list[k];
      if (std::fabs(trigger_diff) > std::fabs(delta_t) && delta_t > 0) 
	{
	  trigger_diff = delta_t;
	}
    }
  return trigger_diff;
}
