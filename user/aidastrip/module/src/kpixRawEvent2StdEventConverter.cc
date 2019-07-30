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

#include "kpix_left_and_right.h"

// Template for useful funcs for this class:
double smallest_time_diff( vector<double> ext_list, int int_value);

// Real Class start:
class kpixRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvent");
	
	void parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle, bool isSelfTrig ) const;
	std::tuple<int, int> parseSample( KpixSample* sample, std::vector<double>   vec_ExtTstamp,  bool isSelfTrig) const;
	
private:
	int getStdPlaneID(uint kpix) const;
	
	unordered_map<uint, uint> _lkpix2strip = kpix_left();
	unordered_map<uint, uint> _rkpix2strip = kpix_right();
	bool                      _pivot = false; // for StdPlane class which is designed for Mimosa;
	
};

namespace{
	auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
		Register<kpixRawEvent2StdEventConverter>(kpixRawEvent2StdEventConverter::m_id_factory);
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
	   1. loop al the samples first to get the external timestamp into a vector as Uwe's analysis_newdaq.cxx
	   2. then process it again to read Data.
	 + TODO! add a switch to tell if you are self-trig or ext-trig
	 */
	for (is=0; is<cycle.count(); is++){
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
	printf("[dev] kpix ID,  channel, strip,  bucket,   ADC\n");
	for (is=0; is<cycle.count(); is++){
	  sample = cycle.sample(is);
	  
	  // parseSample: you need to return the Plane ID (based on kpix ID), and the fired strips positions for parseFrame to add to the Plane. TODO
	  auto hit     = parseSample(sample, vec_ExtTstamp, isSelfTrig);
	  auto planeID = std::get<0>(hit);
	  auto hitX    = std::get<1>(hit);
	  
	  if ( planeID < 0 ) continue; // ignore non-DATA sample
	  if ( hitX == 9999 ) continue; // ignore not connected strips
	  
	  // PushPixel here
	  if (planeID > d2->NumPlanes()){
	    EUDAQ_WARN("Ignoring non-existing plane : " + planeID);
	    continue;
	  }
	  
	  std::cout << "[+] plane : "<< planeID << ", hitX at : " << hitX << std::endl;
	  auto &plane = d2->GetPlane(planeID);
	  plane.PushPixel(hitX, // x
			  1,    // y, always to be 1 since we are strips
			  1     // T pix
			  );    // use bucket as input for frame_num
	}// - sample loop over
	
	// debug:
	auto &plane = d2->GetPlane(0);
	std::cout << "debug: Num of Hit pixels: " << plane.HitPixels() << std::endl;
	return;
}

std::tuple<int, int> kpixRawEvent2StdEventConverter::parseSample(KpixSample* sample,
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
  KpixSample::SampleType type;
  
  if (sample->getEmpty()){ 
    printf ("empty sample\n");
    return std::make_tuple(-1, strip);
  }
  
  type    = sample->getSampleType();
  if (type != KpixSample::Data) return std::make_tuple(-1, strip); // if not DATA, return
  
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
    return std::make_tuple(-1, strip);
  
  if (strip != 9999)
    std::cout<<"\t"
	     << kpix     << "   "
	     << channel  << "   "
	     << strip    << "   "
	     << bucket   << "   "
	     << value    << "   " 
	     << std::endl;
  
  
  return std::make_tuple(getStdPlaneID(kpix), strip);
  //return getStdPlaneID(kpix);
  
  
}

int kpixRawEvent2StdEventConverter::getStdPlaneID(uint kpix) const{

  //TODO: feature to add, read kpix number with plane configuration from external txt file
  if (kpix == 0  | kpix == 1  )  return 0; 
  if (kpix == 2  | kpix == 3  )  return 1; 
  if (kpix == 4  | kpix == 5 )   return 2;
  
  if (kpix == 6  | kpix == 7 )   return 5;
  if (kpix == 8  | kpix == 9 )   return 4;
  if (kpix == 10 | kpix == 11 )  return 3; 
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
