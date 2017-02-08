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

    /** Returns the LCIO version of the event.
     */
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/,
                                 eudaq::Event const & /*source*/) const {
      return false;
    }


    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      int boardID = bore.GetTag("board", -999);

      cnf.Print(std::cout);

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

	static USBPixGen3ConverterPlugin m_instance;

};

//Instantiate the converter plugin instance
USBPixGen3ConverterPlugin USBPixGen3ConverterPlugin::m_instance;
} //namespace eudaq
