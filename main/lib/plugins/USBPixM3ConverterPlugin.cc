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

struct APIXPix {
	int x, y, tot1, tot2, channel, lv1;
	APIXPix(int x, int y, int tot1, int tot2, int lv1, int channel): 
		x(x), y(y), tot1(tot1), 
		tot2(tot2), lv1(lv1), channel(channel) {};
};

class USBPixM3ConverterPlugin: public eudaq::DataConverterPlugin {

  public:
	USBPixM3ConverterPlugin(): DataConverterPlugin(USBPIX_M3_NAME){};

	std::string EVENT_TYPE = USBPIX_M3_NAME;



std::vector<APIXPix> decodeData(std::vector<unsigned char> const & data) const {
	std::vector<APIXPix> result;

	for(auto i : data) {

		uint8_t channel = selectBits(i, 24, 8);
		uint8_t data_headers = 0;

		if(channel >> 7) { //Trigger
			uint32_t trigger_number = selectBits(i, 0, 31); //testme
		} else {
			uint8_t type = selectBits(i, 16, 8);

			switch(type) {
				case data_header: {
					int bcid = selectBits(i, 0, 10);
					int lv1id = selectBits(i, 10, 5);
					int flag = selectBits(i, 15, 1);
					data_headers++;

					break;
				}

				case address_record: {
					int type = selectBits(i, 15, 1);
					int address = selectBits(i, 0, 15);
					break;
				}

				case value_record: {
					int value = selectBits(i, 0, 16);
					break;
				}

				case service_record: {
					int code = selectBits(i, 10, 6);
					int count = selectBits(i, 0, 10);
					break;
				}

				default: { //data record
					unsigned tot2 = selectBits(i, 0, 4);
					unsigned tot1 = selectBits(i, 4, 4);
					unsigned row = selectBits(i, 8, 9) - 1;
					unsigned column = selectBits(i, 17, 7) - 1;

					if(column < 80 && ((tot2 == 15 && row < 336) || (tot2 < 15 && row < 335) )) {
						result.emplace_back(row, column, tot1, tot2, 0, channel);
					} else { // invalid data record
						//invalid_dr ++;
					}
					break;
				}
			}
		}
	}
	return result;
}

    /** Returns the LCIO version of the event.
     */
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/,
                                 eudaq::Event const & /*source*/) const {
      return false;
    }

    /** Returns the StandardEvent version of the event.
     */
    virtual bool GetStandardSubEvent(StandardEvent& sev,
                                     eudaq::Event const & ev) const {

		StandardPlane plane(0, EVENT_TYPE, "FEI4A");
		plane.SetSizeZS(80, 336, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

		//If we get here it must be a data event
		auto evRaw = dynamic_cast<RawDataEvent const &>(ev);
		auto pixelVec = decodeData(evRaw.GetBlock(0));

		for(auto& hitPixel: pixelVec) {
			plane.PushPixel(hitPixel.x, hitPixel.y, 3, false, 0);
		}
		sev.AddPlane(plane);
      return true;
    }


	static USBPixM3ConverterPlugin m_instance;
};

//Instantiate the converter plugin instance
USBPixM3ConverterPlugin USBPixM3ConverterPlugin::m_instance;
} //namespace eudaq
