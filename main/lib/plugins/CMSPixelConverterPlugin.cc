#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

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
      DeviceDictionary* devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      m_detector = bore.GetTag("DETECTOR","");

      m_roctype = devDict->getInstance()->getDevCode(roctype);
      if (m_roctype == 0x0)
	EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin");

      std::cout<<"CMSPixelConverterPlugin initialized with detector " << m_detector << ", ROC type " << static_cast<int>(m_roctype) << std::endl;       
    }

    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {

      // FIXME: use "sensor" to distinguish DUT and REF?
      StandardPlane plane(id, EVENT_TYPE, m_detector);

      // Initialize the plane size (zero suppressed), set the number of pixels
      plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0);

      // Set trigger id:
      plane.SetTLUEvent(0);

      // Transform data of from EUDAQ data format to int16_t vector for processing:
      std::vector<uint16_t> rawdata = TransformRawData(data);
      std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
      if(rawdata.empty()) { return plane; }

      CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);
      int status = evtDecoder.get_event(rawdata, evt);

      EUDAQ_DEBUG("Decoding status: " + status);
      // Check for serious decoding problems and return empty plane:
      if(status <= DEC_ERROR_NO_TBM_HEADER) { return plane; }

      std::cout << "Decoding plane for " << m_detector << ", " << evt->size() << " px" << std::endl;
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

      CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("INFO");
      const RawDataEvent & in_raw = dynamic_cast<const RawDataEvent &>(in);
      for (size_t i = 0; i < in_raw.NumBlocks(); ++i) {
	//out.AddPlane(ConvertPlane(in_raw.GetBlock(i), in_raw.GetID(i)));
	out.AddPlane(ConvertPlane(in_raw.GetBlock(i), (m_detector == "REF" ? 8 : 7)));
      }

      return true;
    }

  private:
    CMSPixelConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
				m_roctype(0), m_detector("") {}

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
    std::string m_detector;
  };

  CMSPixelConverterPlugin CMSPixelConverterPlugin::m_instance;

}
