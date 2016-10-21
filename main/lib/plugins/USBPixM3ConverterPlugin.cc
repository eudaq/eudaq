#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"

//#include "eudaq/ATLASFE4IInterpreter.hh"

#include <cstdlib>
#include <cstring>
#include <exception>

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

template<typename T>
T selectBits(T val, int offset, int length) {
	return (val >> offset) & ((1ull << length) - 1);
}

static const std::string USBPIX_M3_NAME = "USBPIX_M3";

const uint8_t header = 0xE8; //0b11101 000

const uint8_t data_header = header | 0x1;		//0b11101 001
const uint8_t address_record = header | 0x2;	//0b11101 010
const uint8_t value_record = header | 0x4;		//0b11101 100
const uint8_t service_record = header | 0x7;	//0b11101 111


class USBPixM3ConverterPlugin: public eudaq::DataConverterPlugin {

  public:
	USBPixM3ConverterPlugin(): DataConverterPlugin(USBPIX_M3_NAME){};

	std::string EVENT_TYPE = USBPIX_M3_NAME;

    /** Returns the LCIO version of the event.
     */
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/,
                                 eudaq::Event const & /*source*/) const {
      return false;
    }

    /** Returns the StandardEvent version of the event.
     */
    virtual bool GetStandardSubEvent(StandardEvent& sev,
                                     eudaq::Event const & /*source*/) const {

		StandardPlane plane(0, EVENT_TYPE, "FEI4A");
		plane.SetSizeZS(50, 70, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		plane.PushPixel(23, 22, 3, false, 0);
		sev.AddPlane(plane);
      return true;
    };

	static USBPixM3ConverterPlugin m_instance;
};

//Instantiate the converter plugin instance
USBPixM3ConverterPlugin USBPixM3ConverterPlugin::m_instance;
} //namespace eudaq
