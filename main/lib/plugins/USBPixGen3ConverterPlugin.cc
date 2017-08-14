#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"

#include "eudaq/FEI4Decoder.hh"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <bitset>

//All LCIO-specific parts are put in conditional compilation blocks
//so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "UTIL/CellIDEncoder.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelRunHeaderImpl.h"
#endif


#include "eudaq/Configuration.hh"

namespace eudaq {

static const std::string USBPIX_GEN3_NAME = "USBPIX_GEN3";

class USBPixGen3ConverterPlugin: public eudaq::DataConverterPlugin {

  private:
    std::vector<int> attachedBoards;
    mutable std::map<int, bool> boardInitialized;
    mutable std::map<int, std::vector<int>> boardChannels;

  public:
  USBPixGen3ConverterPlugin(): DataConverterPlugin(USBPIX_GEN3_NAME) {};

	std::string EVENT_TYPE = USBPIX_GEN3_NAME;

#if USE_LCIO
	/** Returns the LCIO version of the event.*/
virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, eudaq::Event const & eudaqEvent) const {

	if(eudaqEvent.IsBORE() || eudaqEvent.IsEORE()) return true;

	auto evRaw = dynamic_cast<RawDataEvent const &>(eudaqEvent);
	int boardID = evRaw.GetTag("board", int());

	if(!boardInitialized.at(boardID)) {
		//getChannels will determine all the channels from a board, making the assumption that every channel (i.e. FrontEnd)
		//wrote date into the data block. This holds true if the FE is responding. Then for every trigger there will be
		//data headers (DHs) in the data stream
		boardChannels.at(boardID) = getChannels(evRaw.GetBlock(0));
		if(!boardChannels.at(boardID).empty()) boardInitialized.at(boardID) = true;
    	}

	lcioEvent.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );
	LCCollectionVec * dataCollection;

	auto dataCollectionExists = false;

	try {
		dataCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
		dataCollectionExists = true;
	} catch(lcio::DataNotAvailableException& e) {
		dataCollection =  new LCCollectionVec(lcio::LCIO::TRACKERDATA);
	}

	CellIDEncoder<TrackerDataImpl> cellIDEncoder( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, dataCollection );
	//The pixel type is the same for every hit
	cellIDEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

	std::map<int, std::unique_ptr<lcio::TrackerDataImpl>> frameMap;
	std::map<int, std::unique_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>>> frameInterfaceMap;
	

	for(auto channel: boardChannels.at(boardID)){
		auto frame = std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl);
		auto sensorID = 20 + channel;
		cellIDEncoder["sensorID"] = sensorID;
		cellIDEncoder.setCellID(frame.get());
		auto frameInterface = 	std::unique_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>>( 
						new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(frame.get())
					);
		frameInterfaceMap[channel] = std::move(frameInterface);
		frameMap[channel] = std::move(frame);
	}
	
	int hitDiscConf = 0;

	auto pixelVec = decodeFEI4Data(evRaw.GetBlock(0));
	for(auto& hitPixel: pixelVec) {
		frameInterfaceMap[hitPixel.channel]->emplace_back(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, hitPixel.lv1);
	}

	for(auto& framePair: frameMap){
		dataCollection->push_back( framePair.second.release() );
	}

	//add this collection to lcio event
	if( !dataCollectionExists && (dataCollection->size()!=0)) lcioEvent.addCollection(dataCollection, "zsdata_apix" );

	return true;
}

#endif

    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      int boardID = bore.GetTag("board", -999);

      cnf.Print(std::cout);
	
	cnf.SetSection("Producer.MimosaNI");
	std::cout << "Test confi param: " << cnf.Get("NiIPaddr", "failed") << std::endl;


      if(boardID != -999) {
          attachedBoards.emplace_back(boardID);
          boardChannels[boardID] = std::vector<int>();
          boardInitialized[boardID] = false;
          std::cout << "Added USBPix Board: " << boardID << " to list!" << std::endl;
      } 
    }

    /** Returns the StandardEvent version of the event.
     */
virtual bool GetStandardSubEvent(StandardEvent& sev, eudaq::Event const & ev) const {
    
	std::map<int, StandardPlane> StandardPlaneMap;
	auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
	//In the Gen3 producer we will only have one data block, always!
	auto pixelVec = decodeFEI4Data(evRaw.GetBlock(0));
	int boardID = evRaw.GetTag("board", int());

	if(!boardInitialized.at(boardID)) {
		//getChannels will determine all the channels from a board, making the assumption that every channel (i.e. FrontEnd)
		//wrote date into the data block. This holds true if the FE is responding. Then for every trigger there will be
		//data headers (DHs) in the data stream
		boardChannels.at(boardID) = getChannels(evRaw.GetBlock(0));
		if(!boardChannels.at(boardID).empty()) boardInitialized.at(boardID) = true;
    	}

	for(auto channel: boardChannels.at(boardID)){
		std::string planeName = "USBPIX_GEN3_BOARD_" + to_string(boardID);
		auto pair = std::make_pair(channel, StandardPlane(channel, EVENT_TYPE, planeName));
		pair.second.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		StandardPlaneMap.insert(pair);
	}

	int hitDiscConf = 0;

	for(auto& hitPixel: pixelVec) {
		StandardPlaneMap[hitPixel.channel].PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
	}
	for(auto& planePair: StandardPlaneMap) {
		sev.AddPlane(planePair.second);
	}
	return true;
}

virtual unsigned GetTriggerID(const Event & ev) const {
	//The trigger id is always the first 4 words in each event's data block
	//we only need the first 24 bit though! (the most significant 8 will be zeroes)
	auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
	auto data = evRaw.GetBlock(0);
        uint32_t i =( static_cast<uint32_t>(data[2]) << 16 ) |
                    ( static_cast<uint32_t>(data[1]) << 8 ) |
                    ( static_cast<uint32_t>(data[0]) );
	return i;
}

	//Static instance needed for EUDAQ
	static USBPixGen3ConverterPlugin m_instance;
};

//Instantiate the converter plugin instance
USBPixGen3ConverterPlugin USBPixGen3ConverterPlugin::m_instance;
} //namespace eudaq
