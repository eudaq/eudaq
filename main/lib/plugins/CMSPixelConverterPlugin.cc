#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"

#include "eudaq/CMSPixelDecoder.h"
#include "dictionaries.h"
#include "constants.h"

#include "iostream"
#include "bitset"

using namespace pxar;

namespace eudaq {

  static const char* EVENT_TYPE = "CMSPixel";

  // Events are not converted so far
  // This class is only here to prevent runtime errors
  class CMSPixelConverterPlugin : public DataConverterPlugin {
  public:
    // virtual unsigned GetTriggerID(const Event & ev) const{}; 

    virtual void Initialize(const Event & bore, const Configuration & cnf)
    {
      std::cout<<"CMSPixelConverterPlugin::Initialize" << std::endl;
        
      DeviceDictionary* devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      m_roctype = devDict->getInstance()->getDevCode(roctype);
      if (m_roctype == 0x0)
	EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin");
       
    }

    virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {

      // transform data of form char* to vector<int16_t>
      EUDAQ_DEBUG("m_roctype " + eudaq::to_string((int) m_roctype));
      if(m_roctype != 0x0){
	//CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("DEBUG3");
	CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);

	StandardPlane plane(7, EVENT_TYPE);
	// Set the number of pixels
	plane.SetSizeRaw(ROC_NUMCOLS, ROC_NUMROWS);

	std::vector<uint16_t> rawdata = TransformRawData(in);
	std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
	int status = evtDecoder.get_event(rawdata, evt);
	EUDAQ_DEBUG("Decoding status: " + status);

	// Store all decoded pixels:
	for(std::vector<CMSPixel::pixel>::iterator it = evt->begin(); it != evt->end(); ++it){
	  plane.PushPixel(it->col, it->row, it->raw);
	}

	plane.SetTLUEvent(GetTriggerID(in));
	out.AddPlane(plane);
	delete evt;
	return true;
      }
      else{
	EUDAQ_ERROR("Invalid ROC type\n");
	return false;
      }
    }

  private:
    CMSPixelConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
				m_roctype(0)
    {
      std::cout<<"CMSPixelConverterPlugin Event_Type: "<<EVENT_TYPE << std::endl;

    }

    static std::vector<uint16_t> TransformRawData(const RawDataEvent & in) {
      // transform data of form char* to vector<int16_t>
      std::vector<uint16_t> rawData;

      if(in.NumBlocks() != 1){
	EUDAQ_WARN("event " + to_string(in.GetEventNumber())
		   + ": invalid number of data blocks ");        
	return rawData;
      }

      const std::vector<uint8_t> & block = in.GetBlock(0);

      int size = block.size();
      if(size < 2) {
	EUDAQ_WARN("event " + to_string(in.GetEventNumber())
		   + ": data block is too small");
	return rawData;
      }
      int i = 0;
      while(i < size-1) {
	uint16_t temp = ((uint16_t)block.data()[i+1] << 8) | block.data()[i];
	rawData.push_back(temp);
	i+=2;
      }
      return rawData;
    }

    static std::vector<uint16_t> TransformRawData(const Event & in)
    {
      return TransformRawData(dynamic_cast<const RawDataEvent &>(in));
    }

    static CMSPixelConverterPlugin m_instance;
    uint8_t m_roctype;
  };

  CMSPixelConverterPlugin CMSPixelConverterPlugin::m_instance;

}
