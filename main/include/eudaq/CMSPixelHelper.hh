#include "dictionaries.h"
#include "constants.h"

using namespace pxar;

namespace eudaq {

  class CMSPixelHelper {
  public:
    CMSPixelHelper(std::string event_type) : m_event_type(event_type) {};

    void Initialize(const Event & bore, const Configuration & cnf) {
      DeviceDictionary* devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      m_detector = bore.GetTag("DETECTOR","");

      m_roctype = devDict->getInstance()->getDevCode(roctype);
      if (m_roctype == 0x0)
	EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin");

      std::cout<<"CMSPixel Converter initialized with detector " << m_detector << ", Event Type " << m_event_type << ", ROC type " << static_cast<int>(m_roctype) << std::endl;
    }

    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {

      // FIXME: use "sensor" to distinguish DUT and REF?
      StandardPlane plane(id, m_event_type, m_detector);

      // Initialize the plane size (zero suppressed), set the number of pixels
      plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0);

      // Set trigger id:
      plane.SetTLUEvent(0);

      // Transform data of from EUDAQ data format to int16_t vector for processing:
      std::vector<uint16_t> rawdata = TransformRawData(data);
      std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
      if(rawdata.empty()) { return plane; }

      // Enable lower debugging verbosity level for the decoder module (very verbose!)
      //CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("DEBUG4");
      CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);
      int status = evtDecoder.get_event(rawdata, evt);
      EUDAQ_DEBUG("Decoding status: " + status);

      // Print the decoder statistics (very verbose!)
      //evtDecoder.statistics.print();

      // Check for serious decoding problems and return empty plane:
      if(status <= DEC_ERROR_NO_TBM_HEADER) { return plane; }

      // Store all decoded pixels:
      for(std::vector<CMSPixel::pixel>::iterator it = evt->begin(); it != evt->end(); ++it){
	plane.PushPixel(it->col, it->row, it->raw);
      }

      delete evt;
      return plane;
    }

    bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {
          
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
    }

  private:
    uint8_t m_roctype;
    size_t m_planeid;
    std::string m_detector;
    std::string m_event_type;

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
  };

}
