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
#include <array>
#include <vector>
#include <math.h> // fabs



#include "kpix_left_and_right.h"


//~LoCo 08/08 histogram (TODO: optimize)
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TRandom.h"
#include "TF1.h"
#include "TTree.h"
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

//~LoCo 12/09
std::string timestamp_milli_seconds();

// Real Class start:
class kpixRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	kpixRawEvent2StdEventConverter();
	~kpixRawEvent2StdEventConverter();
	
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvent");
	
	void parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle, bool isSelfTrig ) const;
	std::map<std::pair<int,int>, double> createMap(const char* filename);//~LoCo 05/08

  	//~LoCo 02/08: added third output to parseSample, ADC value. 05/08: fourth, bucket=0. 13/08 ADC value substituted with fC value
  	std::tuple<int, int, double, uint> parseSample( KpixSample* sample, std::vector<double>   vec_ExtTstamp,  bool isSelfTrig) const;
	
	//~LoCo 07/08 ConvertADC2fC: called inside parseFrame. 09/08 inside parseSample
	double ConvertADC2fC( int channelID, int planeID, int hitVal ) const;

	//~LoCo 13/08 FillPedRes(): branch of pedestal tree
	vector<double> FillPedRes(uint kpix, uint channel, uint bucket, double hitCharge, vector<double> _PedRes) const;

private:
	int getStdPlaneID(uint kpix) const;
	int getStdKpixID(uint hitX, int planeID) const;

	TFile* _file;
	TH1F* _histo;
	unordered_map<uint, uint> _lkpix2strip = kpix_left();
	unordered_map<uint, uint> _rkpix2strip = kpix_right();

	//~LoCo 05/08: can call map in monitor with .at
	std::map<std::pair<int,int>, double> Calib_map = createMap("/home/lorenzo/data/real_calib/calib_normalgain.txt");
	bool                      _pivot = false; // for StdPlane class which is designed for Mimosa;

	//~LoCo 13/08: for pedestal
	TFile* _rFile;
	TTree* _pedestal;
	double _pedestal_median, _pedestal_MAD;
	int _kpix_num, _channel_num, _bucket_num;
	int _kpix_checking = 12;
	int _bucket_checking = 1;
	vector<double> _pedestal_results[12][1024][1];
	
};

namespace{
	auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
		Register<kpixRawEvent2StdEventConverter>(kpixRawEvent2StdEventConverter::m_id_factory);
}

kpixRawEvent2StdEventConverter::kpixRawEvent2StdEventConverter() {


	//~LoCo 12/08 Test histogram for testing
	auto outRoot =  timestamp_milli_seconds();
	auto outRoot_ped =  timestamp_milli_seconds();
	outRoot = "./TEST_HISTO/TEST_" + outRoot + "_histo.root";
	outRoot_ped = "./TEST_PEDESTAL/TEST_" + outRoot_ped + "_pedestal.root";

	_file =new TFile(outRoot.c_str(),"recreate");
	_histo = new TH1F("histo","",10e3,0,10e3);
	TH1::AddDirectory(kFALSE);
	_rFile = new TFile(outRoot_ped.c_str(),"recreate");

	//~LoCo 13/08: Pedestal needed to check with Uwe's code
	_pedestal = new TTree("Online_pedestal_tree", "A ROOT Tree");
	_pedestal->Branch("Online_pedestal_median", &_pedestal_median, "Online_pedestal_median/D");
	_pedestal->Branch("Online_kpix_num", &_kpix_num, "Online_kpix_num/I");
	_pedestal->Branch("Online_channel_num", &_channel_num, "Online_channel_num/I:");
	_pedestal->Branch("Online_bucket_num", &_bucket_num, "Online_bucket_num/I");
	_pedestal->Branch("Online_pedestal_MAD", &_pedestal_MAD, "Online_pedestal_MAD/D");
}

kpixRawEvent2StdEventConverter::~kpixRawEvent2StdEventConverter(){

	_file->cd();
	_histo->Write();
	_file->Close();
	_rFile->cd();
	_rFile->Write();
	_rFile->Close();
}


// ~CONVERTING~

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


	  //~LoCo 02/08: Lookup table TEST. 05/08: TODO: do it with EUDAQ_LOGGER

	  //if (Calib_map.empty()) std::cout << "OH NOOOOO" << std::endl;

	  //JUST A TEST
	  //std::cout << "The slope is " << Calib_map.at(std::make_pair(2,1022)) << std::endl;

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
	  auto hitCharge = std::get<2>(hit);
	  if ( hitCharge < 0 ) continue; // ignore charges < 0 sample
	  auto planeID = std::get<0>(hit);
	  auto hitX    = std::get<1>(hit);
	  
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

	  //Fill test array
	  _histo->Fill(hitCharge);

	  }// - sample loop over


	  // debug:
	  auto &plane = d2->GetPlane(0);
	  std::cout << "debug: Num of Hit pixels: " << plane.HitPixels() << std::endl;

	  return;
}

// ~PARSESAMPLE~

//~LoCo 02/08: added third output to parseSample, the ADC value. 05/08: fourth, bucket=0. 13/08: removed third output ADC value, now third output is fC value
std::tuple<int, int, double, uint> kpixRawEvent2StdEventConverter::parseSample(KpixSample* sample,
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
  double       hitCharge=-1;//~LoCo: to get ADC charge in fC
  KpixSample::SampleType type;
  
  if (sample->getEmpty()){ 
    printf ("empty sample\n");
    return std::make_tuple(-1, strip, hitCharge, bucket);
  }
  
  type    = sample->getSampleType();
  if (type != KpixSample::Data) return std::make_tuple(-1, strip, hitCharge, bucket); // if not DATA, return
  
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
    return std::make_tuple(-1, strip, hitCharge, bucket);
  
  if (strip != 9999)
    std::cout<<"\t"
	     << kpix     << "   "
	     << channel  << "   "
	     << strip    << "   "
	     << bucket   << "   "
	     << value    << "   " 
	     << std::endl;


  //~LoCo: to return fC value
  hitCharge=ConvertADC2fC(channel, kpix, value);

  //~LoCo 13/08: Fill _pedestal_resolution
  //_pedestal_results[kpix][channel][bucket].push_back(3.5);
  //if ( hitCharge != -1 ) FillPedRes(kpix, channel, bucket, hitCharge, _pedestal_results[kpix][channel][bucket]);
    //std::cout << "DEBUG: PEDRES" << _pedestal_results[kpix][channel][bucket].at(0) << std::endl;

  
  return std::make_tuple(getStdPlaneID(kpix), strip, hitCharge, bucket);

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

// ~GETSTDKPIXID~

//~LoCo 05/08. Notice: this works only after hitX reversal. TODO: remove this useless function, really just remove it
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

// ~CREATEMAP~

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

	  //Create Map. ~LoCo 07/08: key is <kpix,channel ID>, value is slope
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

			Calib_map[std::make_pair(map_a, map_b)] = map_d;
		
	  }

	  //if ( map_check1 != map_check2 ) return std::make_tuple(map_check1 - map_check2, Calib_map); TODO: do it with EUDAQ_LOGGER

	  myfile_calib.close();

	std::cout<<"TEST DEBUGGING OUTPUT CALIBMAP" << std::endl;

	  return Calib_map;

}

// ~CONVERTADC2FC~

//~LoCo 02/08: convert hitVal (ADC) to fC with Lookup table. 07/08: function to pass array with channel values
//~LoCo 09/08 REWRITTEN: works with channel, not strip
double kpixRawEvent2StdEventConverter::ConvertADC2fC( int channelID, int kpixID, int hitVal ) const{

	  double hitCharge=-1;

	  //TEST
	  //std::cout << kpixID << "AAA" << channelID << "BBB" << Calib_map.at(std::make_pair(kpixID,channelID)) << std::endl;

	  if ( Calib_map.at(std::make_pair(kpixID,channelID)) > 1.0 ) {

	  	hitCharge = hitVal/Calib_map.at(std::make_pair(kpixID,channelID));

		//Check result, might comment it later
	  	//std::cout << "Conversion: kpix #" << kpixID << ", channel #" << channelID << ", hitVal " << hitVal << ", hitCharge " << hitCharge << std::endl;

	  }

	  else {
	  	std::cout << "!!NOTICE: channel excluded from conversion" << std::endl;
	  }

	  return hitCharge;
	  

}

// ~FILLPEDRES~

//~LoCo 13/08: Fill vector double pedestal_results already existent as +++private+++
vector<double> kpixRawEvent2StdEventConverter::FillPedRes(uint kpix, uint channel, uint bucket, double hitCharge, vector<double> _PedRes) const{

	  int kpix_checking = 12;
	  int bucket_checking = 1;

	  if (bucket < bucket_checking){

		if ( kpix < kpix_checking ){
std::cout << "DEBUG: PEDRES1" << _PedRes.at(0) << std::endl;
			_PedRes.push_back(hitCharge);
std::cout << "DEBUG: PEDRES2" << _PedRes.at(0) << std::endl;
		}
	  }

	  return _PedRes;
}

//~LoCo 12/09. Look: only milliseconds are needed. Note down how many milliseconds from one file to another.
//------ code for file timestamps:

std::string timestamp_milli_seconds(){
	
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int milli = curTime.tv_usec / 1000;
	//unsigned long micro = curTime.tv_sec*(uint64_t)1000000+curTime.tv_usec;
	
	char buffer [80];
	
	strftime(buffer, 80, "%Y_%m_%d_%H_%M_%S", localtime(&curTime.tv_sec));
	
	char currentTime[84] = "";
	sprintf(currentTime, "%s_%d", buffer, milli);
	
	// for(int i=0; i < 84; ++i){
	//   outStr[i] = currentTime[i];
	// }

	return std::string(currentTime);
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
