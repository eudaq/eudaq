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

class kpixRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvent");
	
	void parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle) const;
	void parseSample(KpixSample* sample) const;
	
private:
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
		eudaq::StandardPlane plane(id, "strip", "lycoris");
		d2->AddPlane(plane);
	}
	

	// /* read kpix data */
	cycle.copy(kpixEvent, size_of_kpix);

	parseFrame(d2, cycle);
	
	// std::cout << "\t Uint32_t  = " << kpixEvent << "\n"
	//           << "\t evtNum    = " << kpixEvent[0] << std::endl;
	
	// std::cout << "\t kpixEvent.evtNum = " << cycle.eventNumber() <<std::endl;
	// std::cout << "\t kpixEvent.timestamp = "<< cycle.timestamp() <<std::endl;
	// std::cout << "\t kpixEvent.count = "<< cycle.count() <<std::endl;
	
    
  return true;
}


void kpixRawEvent2StdEventConverter::parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle) const{

	d2->Print(std::cout);

	/* globally set variable holders*/
	KpixSample   *sample;
	uint         emptysamples=0;


		
	/* Jun 20, 2019 MQ
	   + the idea is: once you read one kpixID, use the m_ids to find the plane ID, Get it from d2, and pushPixel().
	   + one needs to fix how to cheat EUDAQ that we are a huge pixel intead of strips...
	*/
	printf("[dev] kpix ID,   channel,  bucket,   ADC\n");
	for (uint is=0; is<cycle.count(); is++){
		sample = cycle.sample(is);

		parseSample(sample);
		// parseSample: you need to return the Plane ID (based on kpix ID), and the fired strips positions for parseFrame to add to the Plane. TODO
		
		
	}// - sample loop over

	return;
}

void kpixRawEvent2StdEventConverter::parseSample(KpixSample* sample) const {

	uint         kpix;
	uint         channel;
	uint         bucket;
	uint         value;
	KpixSample::SampleType type;
	
	if (sample->getEmpty()){ 
		printf ("empty sample\n");
		return;
	}
	
	kpix    = sample->getKpixAddress();
	channel = sample->getKpixChannel();
	bucket  = sample->getKpixBucket();
	type    = sample->getSampleType();
	value   = sample->getSampleValue();

	std::cout<< kpix    << "   "
	         << channel	<< "   "
	         << bucket	<< "   "
	         << value   << "   " 
	         << std::endl;
	
	
}
