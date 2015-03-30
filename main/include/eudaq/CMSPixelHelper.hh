#include "dictionaries.h"
#include "constants.h"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
#  include "EUTelRunHeaderImpl.h"

#include "EUTelCMSPixelDetector.h"
using eutelescope::EUTELESCOPE;
#endif

using namespace pxar;

namespace eudaq {

  class CMSPixelHelper {
  public:
    CMSPixelHelper(std::string event_type) : m_event_type(event_type) {};

    void Initialize(const Event & bore, const Configuration & cnf) {
      DeviceDictionary* devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      m_detector = bore.GetTag("DETECTOR","");
      std::string pcbtype = bore.GetTag("PCBTYPE", "");
      m_rotated_pcb = (pcbtype.find("-rot") != std::string::npos ? true : false);  

      m_roctype = devDict->getInstance()->getDevCode(roctype);
      if (m_roctype == 0x0)
	EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin");

      std::cout<<"CMSPixel Converter initialized with detector " << m_detector << ", Event Type " << m_event_type << ", ROC type " << roctype << " (" << static_cast<int>(m_roctype) << ")" << std::endl;
    }

    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {

      // We are using the event's "sensor" (m_detector) to distinguish DUT and REF:
      StandardPlane plane(id, m_event_type, m_detector);

      // Initialize the plane size (zero suppressed), set the number of pixels
      // Check which carrier PCB has been used and book planes accordingly:
      if(m_rotated_pcb) { plane.SetSizeZS(ROC_NUMROWS, ROC_NUMCOLS, 0); }
      else { plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0); }

      // Set trigger id:
      plane.SetTLUEvent(0);

      // Transform data of from EUDAQ data format to int16_t vector for processing:
      std::vector<uint16_t> rawdata = TransformRawData(data);
      std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
      if(rawdata.empty()) { return plane; }

      // Enable lower debugging verbosity level for the decoder module (very verbose!): DEBUG4
      CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("WARNING");
      CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);
      int status = evtDecoder.get_event(rawdata, evt);

      // Print the decoder statistics (very verbose!)
      //evtDecoder.statistics.print();

      // Check for serious decoding problems and return empty plane:
      if(status <= DEC_ERROR_NO_TBM_HEADER) { return plane; }

      // Store all decoded pixels:
      for(std::vector<CMSPixel::pixel>::iterator it = evt->begin(); it != evt->end(); ++it){
	if(m_rotated_pcb) { plane.PushPixel(it->row, it->col, it->raw); }
	else { plane.PushPixel(it->col, it->row, it->raw); }
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

      const RawDataEvent & in_raw = dynamic_cast<const RawDataEvent &>(in);
      unsigned plane_id;
      if(m_detector == "TRP") { plane_id = 6; }      // TRP
      else if(m_detector == "DUT") { plane_id = 7; } // DUT
      else { plane_id = 8; }                         // REF
      for (size_t i = 0; i < in_raw.NumBlocks(); ++i) {
	//out.AddPlane(ConvertPlane(in_raw.GetBlock(i), in_raw.GetID(i)));
	out.AddPlane(ConvertPlane(in_raw.GetBlock(i), plane_id));
      }
    }

    #if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & conf) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      // Type of data: real.
      runHeader.setDAQHWName(EUTELESCOPE::DAQDATA);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const {

      if(source.IsBORE()) {
	std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent BORE " << source << std::endl;
	return true;
      } else if(source.IsEORE()) {
	std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent EORE " << source << std::endl;
	return true;
      }

      // Set event type Data Event (kDE):
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);

      // Prepare the data collection and check for its existence:
      LCCollectionVec * zsDataCollection;
      bool zsDataCollectionExists = false;
      try {
	/// FIXME choose another name for the collection!
	zsDataCollection = static_cast<LCCollectionVec*>(result.getCollection(m_event_type));
	zsDataCollectionExists = true;
      } catch(lcio::DataNotAvailableException& e) {
	zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      // Set the proper cell encoder
      CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);

      // Prepare a description of the setup
      std::vector<eutelescope::EUTelSetupDescription*> setupDescription;

      // Decode the raw data and retrieve the eudaq StandardEvent:
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);

      // Loop over all planes available in the data stream:
      for (size_t iPlane = 0; iPlane < tmp_evt.NumPlanes(); ++iPlane) {
	StandardPlane plane = static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

	// The current detector is ...
	eutelescope::EUTelPixelDetector * currentDetector = 0x0;
	if(plane.Sensor() == "DUT" || plane.Sensor() == "REF" || plane.Sensor() == "TRP") {

	  currentDetector = new eutelescope::EUTelCMSPixelDetector;
	  // FIXME what is that mode used for?
	  std::string mode = "ZS";
	  currentDetector->setMode(mode);
	  if(result.getEventNumber() == 0) {
	    setupDescription.push_back(new eutelescope::EUTelSetupDescription(currentDetector));
	  }
	} else {
	  EUDAQ_ERROR("Unrecognised sensor type in LCIO converter: " + plane.Sensor());
	  return true;
	}

	zsDataEncoder["sensorID"] = plane.ID();
	zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

	// Get the total number of pixels
	size_t nPixel = plane.HitPixels();

	// Prepare a new TrackerData for the ZS data
	std::auto_ptr<lcio::TrackerDataImpl>zsFrame(new lcio::TrackerDataImpl);
	zsDataEncoder.setCellID(zsFrame.get());

	// This is the structure that will host the sparse pixels
	std::auto_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel> >
	  sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

	// Prepare a sparse pixel to be added to the sparse data:
	std::auto_ptr<eutelescope::EUTelGenericSparsePixel> sparsePixel(new eutelescope::EUTelGenericSparsePixel);
	for(size_t iPixel = 0; iPixel < nPixel; ++iPixel) {

	  // Fill the sparse pixel coordinates with decoded data:
	  sparsePixel->setXCoord((size_t)plane.GetX(iPixel));
	  sparsePixel->setYCoord((size_t)plane.GetY(iPixel));
	  // Fill the pixel charge:
	  sparsePixel->setSignal( (size_t)plane.GetPixel(iPixel) );
	  
	  // Add the pixel to the readout frame:
	  sparseFrame->addSparsePixel(sparsePixel.get());
	}

	// FIXME find out about difference between ZS and ZS2!

	// Now add the TrackerData to the collection
	zsDataCollection->push_back(zsFrame.release());
	delete currentDetector;

      } // loop over all planes



      // Add the collection to the event only if not empty and not yet there
      if(!zsDataCollectionExists){
	if(zsDataCollection->size() != 0) {
	  result.addCollection(zsDataCollection, m_event_type);
	} else {
	  delete zsDataCollection; // clean up if not storing the collection here
	}
      }

      return true;
    }
#endif

  private:
    uint8_t m_roctype;
    size_t m_planeid;
    std::string m_detector;
    bool m_rotated_pcb;
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
