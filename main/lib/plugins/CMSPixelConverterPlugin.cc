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

    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {

      // FIXME: use "sensor" to distinguish DUT and REF?
      StandardPlane plane(7, EVENT_TYPE, "DUT");

      // Initialize the plane size (zero suppressed), set the number of pixels
      plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0);

      // Set trigger id:
      plane.SetTLUEvent(id);

      // Transform data of from EUDAQ data format to int16_t vector for processing:
      std::vector<uint16_t> rawdata = TransformRawData(data);
      std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
      if(rawdata.empty()) { return plane; }

      CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);
      int status = evtDecoder.get_event(rawdata, evt);

      EUDAQ_DEBUG("Decoding status: " + status);
      // Check for serious decoding problems and return empty plane:
      if(status <= DEC_ERROR_NO_TBM_HEADER) { return plane; }

      // Store all decoded pixels:
      for(std::vector<CMSPixel::pixel>::iterator it = evt->begin(); it != evt->end(); ++it){
	plane.PushPixel(it->col, it->row, it->raw);
      }

      delete evt;
      return plane;
    }

    virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {

      // Check if we have BORE or EORE:
      if (in.IsBORE() || in.IsEORE()) { return true; }

      // Check ROC type from event tags:
      if(m_roctype == 0x0){
	EUDAQ_ERROR("Invalid ROC type\n");
	return false;
      }

      //CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("DEBUG3");
      const RawDataEvent & in_raw = dynamic_cast<const RawDataEvent &>(in);
      for (size_t i = 0; i < in_raw.NumBlocks(); ++i) {
	out.AddPlane(ConvertPlane(in_raw.GetBlock(i), in_raw.GetID(i)));
      }

      return true;
    }

  private:
    CMSPixelConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
				m_roctype(0)
    {
      std::cout<<"CMSPixelConverterPlugin Event_Type: "<<EVENT_TYPE << std::endl;

    }

    static std::vector<uint16_t> TransformRawData(const std::vector<unsigned char> & block) {

      // Transform data of form char* to vector<int16_t>
      std::vector<uint16_t> rawData;

      int size = block.size();
      if(size < 2) { return rawData; }

      int i = 0;
      while(i < size-1) {
	uint16_t temp = ((uint16_t)block.data()[i+1] << 8) | block.data()[i];
	rawData.push_back(temp);
	i+=2;
      }
      return rawData;
    }

    static CMSPixelConverterPlugin m_instance;
    uint8_t m_roctype;
  };

  CMSPixelConverterPlugin CMSPixelConverterPlugin::m_instance;

}
