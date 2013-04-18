#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/USBpix_i3.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "UTIL/CellIDEncoder.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelAPIXSparsePixel.h"
#  include "EUTelAPIXMCDetector.h"
#  include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

namespace eudaq {
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "USBPIX";

  static const unsigned int CHIP_MIN_COL = 0;
  static const unsigned int CHIP_MAX_COL = 17;
  static const unsigned int CHIP_MIN_ROW = 0;
  static const unsigned int CHIP_MAX_ROW = 159;
  static const unsigned int CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;	// Maximum ROW normalized (starting with 0)
  static const unsigned int CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;

  static int chip_id_offset = 10;

  class USBpixConverterBase {
    private:
      unsigned int count_boards;
      std::vector<unsigned int> board_ids;

    public:
      unsigned int consecutive_lvl1;
      int first_sensor_id;

      int getCountBoards() {
        return count_boards;
      }

      std::vector<unsigned int> getBoardIDs () {
        return board_ids;
      }

      unsigned int getBoardID (unsigned int board_index) const {
        if (board_index >= board_ids.size()) return (unsigned) -1;
        return board_ids[board_index];
      }
      void getBOREparameters (const Event & ev) {
        count_boards = ev.GetTag ("boards", -1);
        board_ids.clear();

        consecutive_lvl1 = ev.GetTag ("consecutive_lvl1", 16);
        first_sensor_id = ev.GetTag ("first_sensor_id", 0);

        if (count_boards == (unsigned) -1) return;

        for (unsigned int i=0; i<count_boards; i++) {
          board_ids.push_back(ev.GetTag ("boardid_" + to_string(i), -1));
        }
      }

      bool isEventValid(const std::vector<unsigned char> & data) const {
        // check for error flags
        unsigned int i = data.size() - 4;
        unsigned Trigger_word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
        if (Trigger_word==(unsigned)-1) return false;

        if (TRIGGER_WORD_ERROR_MACRO(Trigger_word) != 0) return false;

        // ceck data consistency
        unsigned int eoe_found = 0;
        for (unsigned int i=0; i < data.size()-4; i += 4) {
          unsigned word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
          if ((TRIGGER_WORD_HEADER_MACRO(word) == 0) && (HEADER_MACRO(word) == 1) && ((FLAG_MACRO(word) & FLAG_WO_STATUS) == FLAG_WO_STATUS))	{
            eoe_found++;
          }
        }

        if (eoe_found != consecutive_lvl1) return false;
        else return true;
      }

      // returns trigger number or (unsigned) -1 if bcid window is not valid
      unsigned getTrigger(const std::vector<unsigned char> & data) const {
        //Get Trigger Number and check for errors
        unsigned int i = data.size() - 4;
        unsigned Trigger_word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];

        unsigned int trigger_number = TRIGGER_NUMBER_MACRO(Trigger_word);

        return trigger_number;
      }

      bool getHitData (unsigned int &Word, unsigned int &Col, unsigned int &Row, unsigned int &ToT) const {

        if ( ( FLAG_MACRO(Word) & FLAG_WO_STATUS ) == FLAG_WO_STATUS ) return false;	// EoE word; skip for the timebeing
        if ( TRIGGER_WORD_HEADER_MACRO(Word) == 1 ) return false;						// trigger word

        ToT = TOT_MACRO(Word);
        unsigned int t_Col = COL_MACRO(Word);
        unsigned int t_Row = ROW_MACRO(Word);

        if (t_Row > CHIP_MAX_ROW /*|| t_Row < CHIP_MIN_ROW*/) {
          std::cout << "Invalid row: " << t_Row << std::endl;
          return false;
        }
        if (t_Col > CHIP_MAX_COL /*|| t_Col < CHIP_MIN_COL*/) {
          std::cout << "Invalid col: " << t_Col << std::endl;
          return false;
        }
        //std::cout << "Event: (" << getBoardID (id) << " - " << getBoardOrientation(id) << ") " << trigger_number << ": " << Row << ":" << Col << ":" << ToT << std::endl;

        // Normalize Pixelpositions
        t_Col -= CHIP_MIN_COL;
        t_Row -= CHIP_MIN_ROW;

        Col = t_Col;
        Row = t_Row;
        return true;
      }

      StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
        StandardPlane plane(id, EVENT_TYPE, "USBPIX");

        plane.SetSizeZS(CHIP_MAX_COL_NORM + 1, CHIP_MAX_ROW_NORM + 1, 0, consecutive_lvl1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

        //Get Trigger Number
        unsigned int trigger_number = getTrigger(data);
        plane.SetTLUEvent(trigger_number);

        // check for consistency
        if (!isEventValid(data)) return plane;

        unsigned int ToT = 0;
        unsigned int Col = 0;
        unsigned int Row = 0;
        unsigned int lvl1 = 0;

        // Get Events
        for (unsigned int i=0; i < data.size()-4; i += 4) {
          unsigned int Word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];

          if ((HEADER_MACRO(Word) == 1) && ((FLAG_MACRO(Word) & FLAG_WO_STATUS) == FLAG_WO_STATUS)) {
            lvl1++;
          } else if (getHitData(Word, Col, Row, ToT)) {
            plane.PushPixel(Col, Row, ToT, false, lvl1);
          }
        }
        return plane;
      }
  };

  // Declare a new class that inherits from DataConverterPlugin
  class USBPixConverterPlugin : public DataConverterPlugin , public USBpixConverterBase {

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event & bore,
          const Configuration & cnf) {
        (void)cnf; // just to suppress a warning about unused parameter cnf
        getBOREparameters (bore);
      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const Event & ev) const {

        // Make sure the event is of class RawDataEvent
        if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
          if (rev->IsBORE() || rev->IsEORE()) return 0;

          if (rev->NumBlocks() > 0) {

            return getTrigger(rev->GetBlock(0));
          } else return (unsigned) -1;
        }

        // If we are unable to extract the Trigger ID, signal with (unsigned)-1
        return (unsigned)-1;
      }

      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {
        if (ev.IsBORE()) {
          return true;
        } else if (ev.IsEORE()) {
          // nothing to do
          return true;
        }

        // If we get here it must be a data event
        const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
        for (size_t i = 0; i < ev_raw.NumBlocks(); ++i) {
          sev.AddPlane(ConvertPlane(ev_raw.GetBlock(i), ev_raw.GetID(i)));
        }

        return true;
      }

#if USE_LCIO && USE_EUTELESCOPE
      // This is where the conversion to LCIO is done
      virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }

      virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const {
        //std::cout << "getlciosubevent (I3) event " << eudaqEvent.GetEventNumber() << " | " << GetTriggerID(eudaqEvent) << std::endl;
        if (eudaqEvent.IsBORE()) {
          // shouldn't happen
          return true;
        } else if (eudaqEvent.IsEORE()) {
          // nothing to do
          return true;
        }

        // set type of the resulting lcio event
        lcioEvent.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );
        // pointer to collection which will store data
        LCCollectionVec * zsDataCollection;

        // it can be already in event or has to be created
        bool zsDataCollectionExists = false;
        try {
          zsDataCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
          zsDataCollectionExists = true;
        } catch ( lcio::DataNotAvailableException& e ) {
          zsDataCollection = new LCCollectionVec( lcio::LCIO::TRACKERDATA );
        }

        //	create cell encoders to set sensorID and pixel type
        CellIDEncoder< TrackerDataImpl > zsDataEncoder   ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection  );

        // this is an event as we sent from Producer
        // needs to be converted to concrete type RawDataEvent
        const RawDataEvent & ev_raw = dynamic_cast <const RawDataEvent &> (eudaqEvent);

        std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;

        for (size_t chip = 0; chip < ev_raw.NumBlocks(); ++chip) {
          const std::vector <unsigned char> & buffer=dynamic_cast<const std::vector<unsigned char> &> (ev_raw.GetBlock(chip));

          if (lcioEvent.getEventNumber() == 0) {
            eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector(1);
            currentDetector->setMode( "ZS" );

            setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
          }

          std::list<eutelescope::EUTelAPIXSparsePixel*> tmphits;

          zsDataEncoder["sensorID"] = ev_raw.GetID(chip) + chip_id_offset + first_sensor_id;
          zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelAPIXSparsePixel;

          // prepare a new TrackerData object for the ZS data
          // it contains all the hits for a particular sensor in one event
          std::auto_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
          // set some values of "Cells" for this object
          zsDataEncoder.setCellID( zsFrame.get() );

          // this is the structure that will host the sparse pixel
          // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
          // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
          std::auto_ptr< eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > >
            sparseFrame( new eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > ( zsFrame.get() ) );

          unsigned int ToT = 0;
          unsigned int Col = 0;
          unsigned int Row = 0;
          unsigned int lvl1 = 0;

          for (unsigned int i=0; i < buffer.size()-4; i += 4) {
            unsigned int Word = (((unsigned int)buffer[i + 3]) << 24) | (((unsigned int)buffer[i + 2]) << 16) | (((unsigned int)buffer[i + 1]) << 8) | (unsigned int)buffer[i];
            if ((HEADER_MACRO(Word) == 1) && ((FLAG_MACRO(Word) & FLAG_WO_STATUS) == FLAG_WO_STATUS)) {
              lvl1++;
            } else if (getHitData(Word, Col, Row, ToT)) {
              eutelescope::EUTelAPIXSparsePixel *thisHit = new eutelescope::EUTelAPIXSparsePixel( Col, Row, ToT, ev_raw.GetID(chip) + chip_id_offset + first_sensor_id, lvl1);
              sparseFrame->addSparsePixel( thisHit );
              tmphits.push_back( thisHit );
            }
          }

          // write TrackerData object that contains info from one sensor to LCIO collection
          zsDataCollection->push_back( zsFrame.release() );

          for( std::list<eutelescope::EUTelAPIXSparsePixel*>::iterator it = tmphits.begin(); it != tmphits.end(); it++ ){
            delete (*it);
          }
        }

        // add this collection to lcio event
        if ( ( !zsDataCollectionExists )  && ( zsDataCollection->size() != 0 ) ) lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );

        if (lcioEvent.getEventNumber() == 0) {
          // do this only in the first event
          LCCollectionVec * apixSetupCollection = NULL;

          bool apixSetupExists = false;
          try {
            apixSetupCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "apix_setup" ) ) ;
            apixSetupExists = true;
          } catch (...) {
            apixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
          }

          for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {
            apixSetupCollection->push_back( setupDescription.at( iPlane ) );
          }

          if (!apixSetupExists) lcioEvent.addCollection( apixSetupCollection, "apix_setup" );
        }

        return true;

      }
#endif

    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      USBPixConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE)
      {
        consecutive_lvl1=16;
      }

      // The single instance of this converter plugin
      static USBPixConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  USBPixConverterPlugin USBPixConverterPlugin::m_instance;

} // namespace eudaq
