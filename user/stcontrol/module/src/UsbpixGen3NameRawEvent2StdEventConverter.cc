#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

#include "FEI4Decoder.hh"

#include <cstdlib>
#include <cstring>
#include <exception>


class UsbpixGen3NameRawEvent2StdEventConverter: public eudaq::StdEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("USBPIX_GEN3");
  std::string EVENT_TYPE = "USBPIX_GEN3";
  	
private:
  unsigned GetTriggerID(const eudaq::Event & ev) const;
  
  static std::vector<int> attachedBoards;
  static std::map<int, bool> boardInitialized;
  static std::map<int, std::vector<int>> boardChannels;  

};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<UsbpixGen3NameRawEvent2StdEventConverter>(UsbpixGen3NameRawEvent2StdEventConverter::m_id_factory);
}


bool UsbpixGen3NameRawEvent2StdEventConverter::
Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const {
   std::map<int, eudaq::StandardPlane> StandardPlaneMap;   
   auto evRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
   auto block_n_list = evRaw->GetBlockNumList();   

   //FIXME   
   //if it is a BORE it is the begin of run and thus config could have changed and has in principle to be re-read
   //but assuming here that everything stays the same as otherwise for multiple producers one has to add more intricate logic 
   //if(evRaw->IsBORE()) {
   //  attachedBoards.clear();
   //  boardInitialized.clear();
   //  boardChannels.clear();
   //}

   //In the Gen3 producer we will only have one data block, always!
   auto pixelVec = eudaq::decodeFEI4Data(evRaw->GetBlock(0));
   int boardID = evRaw->GetTag("board", -999);
   auto triggerID = GetTriggerID(*evRaw);
   //std::cout << "TriggerID board " << boardID << " : " << triggerID << std::endl;

   
   if(std::find(attachedBoards.begin(), attachedBoards.end(), boardID) == attachedBoards.end()) {
      attachedBoards.push_back(boardID);
      boardChannels[boardID] = std::vector<int>();
      boardInitialized[boardID] = false;
      //std::cout << "Added USBPix Board: " << boardID << " to list!" << std::endl;
   } 

   if(!boardInitialized.at(boardID)) {
   //getChannels will determine all the channels from a board, making the assumption that every channel (i.e. FrontEnd)
   //wrote date into the data block. This holds true if the FE is responding. Then for every trigger there will be
   //data headers (DHs) in the data stream
      boardChannels.at(boardID) = eudaq::getChannels(evRaw->GetBlock(0));
      if(!boardChannels.at(boardID).empty()) boardInitialized.at(boardID) = true;
   }

    for(auto channel: boardChannels.at(boardID)){
	auto sensorID = 20 + channel;
	std::string planeName = "USBPIX_GEN3_BOARD_" + std::to_string(boardID);
	auto pair = std::make_pair(channel, eudaq::StandardPlane(sensorID, EVENT_TYPE, planeName));
	pair.second.SetSizeZS(80, 336, 0, 16, eudaq::StandardPlane::FLAG_DIFFCOORDS | eudaq::StandardPlane::FLAG_ACCUMULATE);
	StandardPlaneMap.insert(pair);
    }

    int hitDiscConf = 0;

    for(auto& hitPixel: pixelVec) {
	StandardPlaneMap[hitPixel.channel].PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
    }
    for(auto& planePair: StandardPlaneMap) {
	d2->AddPlane(planePair.second);
    }  
    
  return true;
}

unsigned UsbpixGen3NameRawEvent2StdEventConverter::GetTriggerID(const eudaq::Event & ev) const {
	//The trigger id is always the first 4 words in each event's data block
	//we only need the first 24 bit though! (the most significant 8 will be zeroes)
	auto evRaw = dynamic_cast<eudaq::RawEvent const &>(ev);
        std::vector<unsigned char> data;
	try {
		data = evRaw.GetBlock(0);
	}
	catch(std::out_of_range) {
		std::cout << "Block with trigger ID missing for USBpix board" << std::endl;
		return 0;
	}
        uint32_t i =( static_cast<uint32_t>(data[2]) << 16 ) |
                    ( static_cast<uint32_t>(data[1]) << 8 ) |
                    ( static_cast<uint32_t>(data[0]) );
	return i;
}

std::vector<int> UsbpixGen3NameRawEvent2StdEventConverter::attachedBoards ={};
std::map<int, bool> UsbpixGen3NameRawEvent2StdEventConverter::boardInitialized={};
std::map<int, std::vector<int>> UsbpixGen3NameRawEvent2StdEventConverter::boardChannels={};


