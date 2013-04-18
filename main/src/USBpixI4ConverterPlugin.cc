#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/USBpix_i4.hh"

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
  static const char* EVENT_TYPE = "USBPIXI4";

  static const unsigned int CHIP_MIN_COL = 1;
  static const unsigned int CHIP_MAX_COL = 80;
  static const unsigned int CHIP_MIN_ROW = 1;
  static const unsigned int CHIP_MAX_ROW = 336;
  static const unsigned int CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;	// Maximum ROW normalized (starting with 0)
  static const unsigned int CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;

  static int chip_id_offset = 20;

  class USBpixI4ConverterBase {
    private:
      unsigned int count_boards;
      std::vector<unsigned int> board_ids;

    public:
      unsigned int consecutive_lvl1;
      unsigned int tot_mode;
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
        if (consecutive_lvl1>16) consecutive_lvl1=16;
        first_sensor_id = ev.GetTag ("first_sensor_id", 0);
        tot_mode = ev.GetTag ("tot_mode", 0);
        if (tot_mode>2) tot_mode=0;

        if (count_boards == (unsigned) -1) return;

        for (unsigned int i=0; i<count_boards; i++) {
          board_ids.push_back(ev.GetTag ("boardid_" + to_string(i), -1));
        }
      }

      bool isEventValid(const std::vector<unsigned char> & data) const {
        // ceck data consistency
        unsigned int dh_found = 0;
        for (unsigned int i=0; i < data.size()-8; i += 4) {
          unsigned word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
          if (DATA_HEADER_MACRO(word))	{
            dh_found++;
          }
        }

        if (dh_found != consecutive_lvl1) return false;
        else return true;
      }

      unsigned getTrigger(const std::vector<unsigned char> & data) const {
        //Get Trigger Number and check for errors
        unsigned int i = data.size() - 8; // splitted in 2x 32bit words
        unsigned Trigger_word1 = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
        if (Trigger_word1==(unsigned)-1) return (unsigned)-1;
        unsigned Trigger_word2 = (((unsigned int)data[i + 7]) << 24) | (((unsigned int)data[i + 6]) << 16) | (((unsigned int)data[i + 5]) << 8) | (unsigned int)data[i + 4];

        unsigned int trigger_number = TRIGGER_NUMBER_MACRO2(Trigger_word1, Trigger_word2);
        //std::cout << "Trigger: " << trigger_number << std::endl;
        return trigger_number;
      }

      bool getHitData (unsigned int &Word, bool second_hit, unsigned int &Col, unsigned int &Row, unsigned int &ToT) const {

        if ( !DATA_RECORD_MACRO(Word) ) return false;	// No Data Record

        unsigned int t_Col=0;
        unsigned int t_Row=0;
        unsigned int t_ToT=15;

        if (!second_hit) {
          t_ToT = DATA_RECORD_TOT1_MACRO(Word);
          t_Col = DATA_RECORD_COLUMN1_MACRO(Word);
          t_Row = DATA_RECORD_ROW1_MACRO(Word);
        } else {
          t_ToT = DATA_RECORD_TOT2_MACRO(Word);
          t_Col = DATA_RECORD_COLUMN2_MACRO(Word);
          t_Row = DATA_RECORD_ROW2_MACRO(Word);
        }

        // translate FE-I4 ToT code into tot
        if (tot_mode==1) {
          if (t_ToT==15) return false;
          if (t_ToT==14) ToT = 1;
          else ToT = t_ToT + 2;
        } else if (tot_mode==2) {
          // No tot = 2 ?
          if (t_ToT==15) return false;
          if (t_ToT==14) ToT = 1;
          else ToT = t_ToT + 3;
        } else {
          // 0
          if (t_ToT==14 || t_ToT==15) return false;
          ToT = t_ToT + 1;
        }

        if (t_Row > CHIP_MAX_ROW || t_Row < CHIP_MIN_ROW) {
          std::cout << "Invalid row: " << t_Row << std::endl;
          return false;
        }
        if (t_Col > CHIP_MAX_COL || t_Col < CHIP_MIN_COL) {
          std::cout << "Invalid col: " << t_Col << std::endl;
          return false;
        }

        // Normalize Pixelpositions
        t_Col -= CHIP_MIN_COL;
        t_Row -= CHIP_MIN_ROW;
        Col = t_Col;
        Row = t_Row;
        return true;
      }

      StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
        StandardPlane plane(id, EVENT_TYPE, "USBPIXI4");

        // check for consistency
        bool valid=isEventValid(data);

        unsigned int ToT = 0;
        unsigned int Col = 0;
        unsigned int Row = 0;
        // FE-I4: DH with lv1 before Data Record
        unsigned int lvl1 = 0;

        plane.SetSizeZS(CHIP_MAX_COL_NORM + 1, CHIP_MAX_ROW_NORM + 1, 0, consecutive_lvl1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE); //
        //Get Trigger Number
        unsigned int trigger_number = getTrigger(data);
        plane.SetTLUEvent(trigger_number);

        if (!valid) return plane;
        unsigned int eventnr=0;

        // Get Events
        for (unsigned int i=0; i < data.size()-8; i += 4) {
          unsigned int Word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];

          if (DATA_HEADER_MACRO(Word)) {
            lvl1++;
          } else {
            // First Hit
            if (getHitData(Word, false, Col, Row, ToT)) {
              plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
              eventnr++;
            }
            // Second Hit
            if (getHitData(Word, true, Col, Row, ToT)) {
              plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
              eventnr++;
            }
          }
        }
        return plane;
      }
  };

  // Declare a new class that inherits from DataConverterPlugin
  class USBPixI4ConverterPlugin : public DataConverterPlugin , public USBpixI4ConverterBase {

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
        //std::cout << "getlciosubevent (I4) event " << eudaqEvent.GetEventNumber() << " | " << GetTriggerID(eudaqEvent) << std::endl;
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
            eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector(2);
            currentDetector->setMode( "ZS" );

            setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
          }

          std::list<eutelescope::EUTelAPIXSparsePixel*> tmphits;

          zsDataEncoder["sensorID"] = ev_raw.GetID(chip) + chip_id_offset + first_sensor_id; // formerly 14
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

            if (DATA_HEADER_MACRO(Word)) {
              lvl1++;
            } else {
              // First Hit
              if (getHitData(Word, false, Col, Row, ToT)) {
                eutelescope::EUTelAPIXSparsePixel *thisHit = new eutelescope::EUTelAPIXSparsePixel( Col, Row, ToT, ev_raw.GetID(chip) + chip_id_offset + first_sensor_id, lvl1-1);
                sparseFrame->addSparsePixel( thisHit );
                tmphits.push_back( thisHit );
              }
              // Second Hit
              if (getHitData(Word, true, Col, Row, ToT)) {
                eutelescope::EUTelAPIXSparsePixel *thisHit = new eutelescope::EUTelAPIXSparsePixel( Col, Row, ToT, ev_raw.GetID(chip) + chip_id_offset + first_sensor_id, lvl1-1);
                sparseFrame->addSparsePixel( thisHit );
                tmphits.push_back( thisHit );
              }
            }
          }

          // write TrackerData object that contains info from one sensor to LCIO collection
          zsDataCollection->push_back( zsFrame.release() );

          // clean up
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
      USBPixI4ConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE)
      {}

      // The single instance of this converter plugin
      static USBPixI4ConverterPlugin m_instance;
  };

  // Instantiate the converter plugin instance
  USBPixI4ConverterPlugin USBPixI4ConverterPlugin::m_instance;

} // namespace eudaq
