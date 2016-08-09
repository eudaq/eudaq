#include "api.h"
#include "dictionaries.h"
#include "constants.h"
#include "datasource_evt.h"

#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "UTIL/CellIDEncoder.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelRunHeaderImpl.h"

using eutelescope::EUTELESCOPE;
#endif

using namespace pxar;

namespace eudaq {

  class CMSPixelHelper {
  public:
    CMSPixelHelper(std::string event_type) : m_event_type(event_type){};

    void Initialize(const Event &bore, const Configuration &cnf) {
      DeviceDictionary *devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      std::string tbmtype = bore.GetTag("TBMTYPE", "notbm");

      m_detector = bore.GetTag("DETECTOR", "");
      std::string pcbtype = bore.GetTag("PCBTYPE", "");
      m_rotated_pcb =
          (pcbtype.find("-rot") != std::string::npos ? true : false);

      // Get the number of planes:
      m_nplanes = bore.GetTag("PLANES", 1);

      m_roctype = devDict->getInstance()->getDevCode(roctype);
      m_tbmtype = devDict->getInstance()->getDevCode(tbmtype);

      if (m_roctype == 0x0) {
        EUDAQ_THROW("Data contains invalid ROC type: " + roctype);
      }
      if(m_tbmtype == 0x0) {
	EUDAQ_THROW("Data contains invalid TBM type: " + tbmtype);
      }

      std::cout << "CMSPixel Converter initialized with detector " << m_detector
                << ", Event Type " << m_event_type << ", TBM type " << tbmtype
                << " (" << static_cast<int>(m_tbmtype) << ")"
                << ", ROC type " << roctype << " ("
                << static_cast<int>(m_roctype) << ")" << std::endl;

      if (m_detector == "TRP") {
        m_planeid = 6;
      } // TRP
      else if (m_detector == "DUT") {
        m_planeid = 7;
      } // DUT
      else if (m_detector == "REF") {
        m_planeid = 8;
      } // REF
      else {
        m_planeid = 9;
      } // QUAD

      // Set decoder to reasonable verbosity (still informing about problems:
      pxar::Log::ReportingLevel() = pxar::Log::FromString("WARNING");
      //pxar::Log::ReportingLevel() = pxar::Log::FromString("DEBUGPIPES");

      // Connect the data source and set up the pipe:
      src = evtSource(0, m_nplanes, 0, m_tbmtype, m_roctype, FLAG_DISABLE_EVENTID_CHECK);
      src >> splitter >> decoder >> Eventpump;
    }

    bool GetStandardSubEvent(StandardEvent &out, const Event &in) const {

      // If we receive the EORE print the collected statistics:
      if (in.IsEORE()) {
        // Set decoder to INFO level for statistics printout:
        std::cout << "Decoding statistics for detector " << m_detector
                  << std::endl;
        pxar::Log::ReportingLevel() = pxar::Log::FromString("INFO");
	decoder.getStatistics().dump();
	return true;
      }
      // Check if we have BORE:
      else if (in.IsBORE()) {
	return true;
      }

      const RawDataEvent &in_raw = dynamic_cast<const RawDataEvent &>(in);
      // Check of we have more than one data block:
      if (in_raw.NumBlocks() > 1) {
        EUDAQ_ERROR("Only one data block is expected!");
        return false;
      }

      // Transform from EUDAQ data, add it to the datasource:
      src.AddData(TransformRawData(in_raw.GetBlock(0)));
      // ...and pull it out at the other end:
      pxar::Event *evt = Eventpump.Get();

      // If we have no TBM or a TBM Emulator, assume this is
      // some sort of multi-plane telescope:
      if (m_tbmtype <= TBM_EMU)
        GetMultiPlanes(out, m_planeid, evt);
      // If we have a real TBM attached, this is probably
      // a module with just one sensor plane:
      else
        GetSinglePlane(out, m_planeid, evt);
    }

#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader &header,
                                  eudaq::Event const & /*bore*/,
                                  eudaq::Configuration const &conf) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      // Type of data: real.
      runHeader.setDAQHWName(EUTELESCOPE::DAQDATA);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent &result,
                                 const Event &source) const {

      if (source.IsBORE()) {
        std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent BORE " << source
                  << std::endl;
        return true;
      } else if (source.IsEORE()) {
        std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent EORE " << source
                  << std::endl;
        return true;
      }

      // Set event type Data Event (kDE):
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                   eutelescope::kDE);

      // Prepare the data collection and check for its existence:
      LCCollectionVec *zsDataCollection;
      bool zsDataCollectionExists = false;
      try {
        zsDataCollection =
            static_cast<LCCollectionVec *>(result.getCollection(m_event_type));
        zsDataCollectionExists = true;
      } catch (lcio::DataNotAvailableException &e) {
        zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      // Set the proper cell encoder
      CellIDEncoder<TrackerDataImpl> zsDataEncoder(
          eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);

      // Decode the raw data and retrieve the eudaq StandardEvent:
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);

      // Loop over all planes available in the data stream:
      for (size_t iPlane = 0; iPlane < tmp_evt.NumPlanes(); ++iPlane) {
        StandardPlane plane =
            static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

        zsDataEncoder["sensorID"] = plane.ID();
        zsDataEncoder["sparsePixelType"] =
            eutelescope::kEUTelGenericSparsePixel;

        // Get the total number of pixels
        size_t nPixel = plane.HitPixels();

        // Prepare a new TrackerData for the ZS data
        std::auto_ptr<lcio::TrackerDataImpl> zsFrame(new lcio::TrackerDataImpl);
        zsDataEncoder.setCellID(zsFrame.get());

        // This is the structure that will host the sparse pixels
        std::auto_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<
            eutelescope::EUTelGenericSparsePixel>>
            sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<
                eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

        // Prepare a sparse pixel to be added to the sparse data:
        std::auto_ptr<eutelescope::EUTelGenericSparsePixel> sparsePixel(
            new eutelescope::EUTelGenericSparsePixel);
        for (size_t iPixel = 0; iPixel < nPixel; ++iPixel) {

          // Fill the sparse pixel coordinates with decoded data:
          sparsePixel->setXCoord((size_t)plane.GetX(iPixel));
          sparsePixel->setYCoord((size_t)plane.GetY(iPixel));
          // Fill the pixel charge:
          sparsePixel->setSignal((int32_t)plane.GetPixel(iPixel));

          // Add the pixel to the readout frame:
          sparseFrame->addSparsePixel(sparsePixel.get());
        }

        // Now add the TrackerData to the collection
        zsDataCollection->push_back(zsFrame.release());

      } // loop over all planes

      // Add the collection to the event only if not empty and not yet there
      if (!zsDataCollectionExists) {
        if (zsDataCollection->size() != 0) {
          result.addCollection(zsDataCollection, m_event_type);
        } else {
          delete zsDataCollection; // clean up if not storing the collection
                                   // here
        }
      }

      return true;
    }
#endif

  private:
    uint8_t m_roctype, m_tbmtype;
    size_t m_planeid;
    size_t m_nplanes;
    std::string m_detector;
    bool m_rotated_pcb;
    std::string m_event_type;
    // The pipeworks:
    mutable evtSource src;
    mutable passthroughSplitter splitter;
    mutable dtbEventDecoder decoder;
    mutable dataSink<pxar::Event *> Eventpump;

    void GetMultiPlanes(StandardEvent &out, unsigned plane_id,
                        pxar::Event *evt) const {
      // Iterate over all planes and check for pixel hits:
      for (size_t roc = 0; roc < m_nplanes; roc++) {

	// Create a new StandardPlane object:
        StandardPlane plane(plane_id + roc, m_event_type, m_detector);

        // Initialize the plane size (zero suppressed), set the number of pixels
        // Check which carrier PCB has been used and book planes accordingly:
        if (m_rotated_pcb) {
          plane.SetSizeZS(ROC_NUMROWS, ROC_NUMCOLS, 0);
        } else {
          plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0);
        }
        plane.SetTLUEvent(0);

        // Store all decoded pixels belonging to this plane:
        for (std::vector<pxar::pixel>::iterator it = evt->pixels.begin();
             it != evt->pixels.end(); ++it) {
          // Check if current pixel belongs on this plane:
          if (it->roc() == roc) {
            if (m_rotated_pcb) {
              plane.PushPixel(it->row(), it->column(), it->value());
            } else {
              plane.PushPixel(it->column(), it->row(), it->value());
            }
          }
        }

        // Add plane to the output event:
        out.AddPlane(plane);
      }
    };

    void GetSinglePlane(StandardEvent &out, unsigned plane_id,
                        pxar::Event *evt) const {

      // Create a new StandardPlane object:
      StandardPlane plane(plane_id, m_event_type, m_detector);

      // More than 16 ROCs on one module?! Impossible!
      if (m_nplanes > 16)
        return;

      // Initialize the plane size (zero suppressed), set the number of pixels
      // Check which carrier PCB has been used and book planes accordingly:
      if (m_rotated_pcb) {
        plane.SetSizeZS(2 * ROC_NUMROWS, 8 * ROC_NUMCOLS, 0);
      } else {
        plane.SetSizeZS(8 * ROC_NUMCOLS, 2 * ROC_NUMROWS, 0);
      }
      plane.SetTLUEvent(0);

      // Iterate over all pixels and place them in the module plane:
      for (std::vector<pxar::pixel>::iterator it = evt->pixels.begin();
           it != evt->pixels.end(); ++it) {

        if (m_rotated_pcb) {
          plane.PushPixel(roc_to_mod_row(it->roc(), it->row()),
                          roc_to_mod_col(it->roc(), it->column()), it->value());
        } else {
          plane.PushPixel(roc_to_mod_col(it->roc(), it->column()),
                          roc_to_mod_row(it->roc(), it->row()), it->value());
        }
      }

      // Add plane to the output event:
      out.AddPlane(plane);
    };

    static inline uint16_t roc_to_mod_row(uint8_t roc, uint16_t row) {
      if (roc < 8)
        return row;
      else
        return (2 * ROC_NUMROWS - row - 1);
    };

    static inline uint16_t roc_to_mod_col(uint8_t roc, uint16_t col) {
      if (roc < 8)
        return (col + roc * ROC_NUMCOLS);
      else
        return ((16 - roc) * ROC_NUMCOLS - col - 1);
    };

    static std::vector<uint16_t>
    TransformRawData(const std::vector<unsigned char> &block) {

      // Transform data of form char* to vector<int16_t>
      std::vector<uint16_t> rawData;

      int size = block.size();
      if (size < 2) {
        return rawData;
      }

      int i = 0;
      while (i < size - 1) {
        uint16_t temp = ((uint16_t)block.data()[i + 1] << 8) | block.data()[i];
        rawData.push_back(temp);
        i += 2;
      }
      return rawData;
    }
  };
}
