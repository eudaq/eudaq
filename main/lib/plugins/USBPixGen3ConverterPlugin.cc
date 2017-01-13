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
  //Will be set as the EVENT_TYPE for the different versions of this producer

static const std::string USBPIX_GEN3_NAME = "USBPIX_GEN3";

class USBPixGen3ConverterPlugin: public eudaq::DataConverterPlugin {

  public:
	USBPixGen3ConverterPlugin(): DataConverterPlugin(USBPIX_GEN3_NAME){};

	std::string EVENT_TYPE = USBPIX_GEN3_NAME;

    /** Returns the LCIO version of the event.
     */
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/,
                                 eudaq::Event const & /*source*/) const {
      return false;
    }

    /** Returns the StandardEvent version of the event.
     */
    virtual bool GetStandardSubEvent(StandardEvent& sev, eudaq::Event const & ev) const {

		StandardPlane plane1(1, EVENT_TYPE, "USBPIX_GEN3");
    StandardPlane plane2(2, EVENT_TYPE, "USBPIX_GEN3");
    StandardPlane plane3(3, EVENT_TYPE, "USBPIX_GEN3");
		plane1.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		plane2.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
    plane3.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

		//If we get here it must be a data event
		auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
	  auto pixelVec = decodeFEI4Data(evRaw.GetBlock(0));
    //auto& dataVec = evRaw.GetBlock(0);

 //std::cout << "Data length: " << dataVec.size() << std::endl;
/*
 size_t x = 0;
 for(auto entry: dataVec) {
   if((x++)%4 == 0) std::cout << "----------\n";
   std::cout << "Data element: " << std::bitset<8>(entry) << std::endl;
 }*/
 /*
		for(size_t index = 0;  index < dataVec.size(); index+=4) {
      uint32_t data = ( static_cast<uint32_t>(dataVec[index+3]) << 24 ) | 
                      ( static_cast<uint32_t>(dataVec[index+2]) << 16 ) | 
                      ( static_cast<uint32_t>(dataVec[index+1]) << 8 ) | 
                      ( static_cast<uint32_t>(dataVec[index+0]) );
      std::cout << "Data element: " << std::bitset<32>(data) << std::endl;
      }
*/
		for(auto& hitPixel: pixelVec) {
      //std::cout << "Got a pixel!" << std::endl;
      switch(hitPixel.channel) {
        case 1:
          plane1.PushPixel(hitPixel.x, hitPixel.y, 3, false, 0);
          break;
        case 2:
          plane2.PushPixel(hitPixel.x, hitPixel.y, 3, false, 0);
          break;
        case 3:
          plane3.PushPixel(hitPixel.x, hitPixel.y, 3, false, 0);
          break;
      }
		}

		sev.AddPlane(plane1);
    sev.AddPlane(plane2);
    sev.AddPlane(plane3);
      return true;
    }


	static USBPixGen3ConverterPlugin m_instance;
};

//Instantiate the converter plugin instance
USBPixGen3ConverterPlugin USBPixGen3ConverterPlugin::m_instance;
} //namespace eudaq
