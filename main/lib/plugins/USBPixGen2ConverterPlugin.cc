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

static const std::string USBPIX_GEN2_NAME = "USBPIX_GEN2";

class USBPixGen2ConverterPlugin: public eudaq::DataConverterPlugin {

  private:
    std::vector<int> attachedBoards;
    mutable std::map<int, bool> boardInitialized;
    mutable std::map<int, std::vector<int>> boardChannels;

  public:
  USBPixGen2ConverterPlugin(): DataConverterPlugin(USBPIX_GEN2_NAME) {};

	std::string EVENT_TYPE = USBPIX_GEN2_NAME;

    /** Returns the LCIO version of the event.
     */
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/,
                                 eudaq::Event const & /*source*/) const {
      return false;
    }


    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      int boardID = bore.GetTag("board", -999);

      cnf.Print(std::cout);
      /*
      if(boardID != -999) {
          attachedBoards.emplace_back(boardID);
          boardChannels[boardID] = std::vector<int>();
          boardInitialized[boardID] = false;
          std::cout << "Added USBPix Board: " << boardID << " to list!" << std::endl;
      } */
    }

    /** Returns the StandardEvent version of the event.
     */
    virtual bool GetStandardSubEvent(StandardEvent& sev, eudaq::Event const & ev) const {
      auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
      int boardID = evRaw.GetTag("board", int());
      std::string planeName = "USBPIX_GEN2_BOARD_" + to_string(boardID);

      auto plane1 = StandardPlane(0, EVENT_TYPE, planeName);
      auto plane2 = StandardPlane(1, EVENT_TYPE, planeName);
      auto plane3 = StandardPlane(2, EVENT_TYPE, planeName);
      auto plane4 = StandardPlane(3, EVENT_TYPE, planeName);

      plane1.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      plane2.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      plane3.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      plane4.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);


     
      int hitDiscConf = 0;

      //std::cout << "Data from board 1: " << std::endl;
      auto data1 = evRaw.GetBlock(0);
      auto vec1 = decodeFEI4DataGen2(data1);
      for(auto& hitPixel: vec1) {
        plane1.PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
      }

      //std::cout << "Data from board 2: " << std::endl;
      auto data2 = evRaw.GetBlock(1);
      auto vec2 = decodeFEI4DataGen2(data2);
      for(auto& hitPixel: vec2) {
        plane2.PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
      }      
      //std::cout << "Data from board 3: " << std::endl;
      auto data3 = evRaw.GetBlock(2);
      auto vec3 =decodeFEI4DataGen2(data3);
      for(auto& hitPixel: vec3) {
        plane3.PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
      }
      //std::cout << "Data from board 4: " << std::endl;
      auto data4 = evRaw.GetBlock(3);
      auto vec4 =decodeFEI4DataGen2(data4);
      for(auto& hitPixel: vec4) {
        plane4.PushPixel(hitPixel.x, hitPixel.y, hitPixel.tot+1+hitDiscConf, false, hitPixel.lv1);
      }


      sev.AddPlane(plane1);
      sev.AddPlane(plane2);
      sev.AddPlane(plane3);
      sev.AddPlane(plane4);
/*      
    std::map<int, StandardPlane> StandardPlaneMap;


		auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
    auto pixelVec = decodeFEI4Data(evRaw.GetBlock(0));

    int boardID = evRaw.GetTag("board", int());

      if(!boardInitialized.at(boardID)) {
        boardChannels.at(boardID) = getChannels(evRaw.GetBlock(0));
        if(!boardChannels.at(boardID).empty()) boardInitialized.at(boardID) = true;
        //for(auto channel: channels){
        //  std::cout << channel << ' ';
        //} 
      }
      for(auto channel: boardChannels.at(boardID)){
            std::string planeName = "USBPIX_GEN2_BOARD_" + to_string(boardID);
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
    }*/
      return true;
    }

	static USBPixGen2ConverterPlugin m_instance;

};

//Instantiate the converter plugin instance
USBPixGen2ConverterPlugin USBPixGen2ConverterPlugin::m_instance;
} //namespace eudaq
