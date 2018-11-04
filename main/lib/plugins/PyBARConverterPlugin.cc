#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/PyBAR.hh"

#include <stdlib.h>

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
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
#  include "EUTelAPIXMCDetector.h"
#  include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

namespace eudaq {
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "PyBAR";

  static const unsigned int CHIP_MIN_COL = 1;
  static const unsigned int CHIP_MAX_COL = 80;
  static const unsigned int CHIP_MIN_ROW = 1;
  static const unsigned int CHIP_MAX_ROW = 336;
  static const unsigned int CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;	// Maximum ROW normalized (starting with 0)
  static const unsigned int CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;

#if USE_LCIO && USE_EUTELESCOPE
  static const int chip_id_offset = 20;
#endif

  class PyBARConverterBase {
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
        tot_mode = ev.GetTag ("tot_mode", 1);
        //printf("totmode=%d\n");
        //exit(0);
        //tot_mode =1;
        if (tot_mode>2) tot_mode=0;

        if (count_boards == (unsigned) -1) return;

        for (unsigned int i=0; i<count_boards; i++) {
          board_ids.push_back(ev.GetTag ("boardid_" + to_string(i), -1));
        }
      }

      bool isEventValid(const std::vector<unsigned char> & data) const {
        // ceck data consistency
        unsigned int dh_found = 0;
        if(data.size()%4!=0) return false;
        const unsigned int * p=reinterpret_cast<const unsigned int *>(&(data[0]));
        for (unsigned int i=1; i < data.size()/4; ++i)  {
          //printf("isEventValid word=%08x,masked=%08x, %d\n",word,(PYBAR_DATA_HEADER_MASK & word),int(PYBAR_DATA_HEADER_MACRO(word)));
          if (PYBAR_DATA_HEADER_MACRO(p[i]))	{  //direct casting needed for current versions of pybar  - luetticke
            dh_found++;
          }
        }
        //printf("isEventValid db_found=%d\n",dh_found);

        if (dh_found != consecutive_lvl1){
            //exit(0);
            printf("isEventValid INVALID! sdb_found=%d\n",dh_found);
            return false;
        }
        else{
            //printf("isEventValid ================GOOD DATA!!==============\n");
            //exit(0);
            return true;
        }
      }

      unsigned getTrigger(const std::vector<unsigned char> & data) const {
        //Get Trigger Number and check for errors
        //printf("\ngetTrigger() %d: ",data.size());
        //for (int ii=0;ii<data.size();ii++){
        //    printf("%02x ",data[ii]);
        //}
        //printf("\n");
        //unsigned int i = data.size() - 4; // 8->4 hirono //splitted in 2x 32bit words
        const unsigned int * p=reinterpret_cast<const unsigned int *>(&(data[0]));

        //unsigned Trigger_word1 = (((unsigned int)data[0]) << 24) | (((unsigned int)data[1]) << 16) | (((unsigned int)data[2]) << 8) | (unsigned int)data[3];
        //unsigned Trigger_word1 = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
        unsigned int trigger_number = 0x7FFFFFFF & p[0];  //lutticke. Hello Hirono :-)
        //unsigned int trigger_number = 0x7FFFFFFF & Trigger_word1;  //hirono
        //std::cout << "getTrigger(): " << trigger_number << std::endl;
        return trigger_number;
      }

      bool getHitData (const unsigned int &Word, bool second_hit, unsigned int &Col, unsigned int &Row, unsigned int &ToT) const {
        //printf("getHitData word=%08x col_masked=%08x %d\n",Word,PYBAR_DATA_RECORD_COLUMN_MASK & Word,PYBAR_DATA_RECORD_MACRO(Word));
        if ( !PYBAR_DATA_RECORD_MACRO(Word) ) return false;	// No Data Record

        unsigned int t_Col=0;
        unsigned int t_Row=0;
        unsigned int t_ToT=15;

        if (!second_hit) {
          t_ToT = PYBAR_DATA_RECORD_TOT1_MACRO(Word);
          t_Col = PYBAR_DATA_RECORD_COLUMN1_MACRO(Word);
          t_Row = PYBAR_DATA_RECORD_ROW1_MACRO(Word);
        } else {
          t_ToT = PYBAR_DATA_RECORD_TOT2_MACRO(Word);
          t_Col = PYBAR_DATA_RECORD_COLUMN2_MACRO(Word);
          t_Row = PYBAR_DATA_RECORD_ROW2_MACRO(Word);
        }
        //printf("getHitData() t_Col=%d,t_Row=%d,t_ToT=%d\n",t_Col,t_Row,t_ToT);
        //exit(0);


        if(t_ToT== 15 && !second_hit){
            ToT=17;
            std::cout<< "Got FEI4 ToT 15 in first hit! This should never happen. Word" << std::hex<< Word<<std::dec << std::endl;
            std::cout<< " TOT1: " <<PYBAR_DATA_RECORD_TOT1_MACRO(Word)
                                << " COL1: " <<PYBAR_DATA_RECORD_COLUMN1_MACRO(Word)
                                << " ROW1: " <<PYBAR_DATA_RECORD_ROW1_MACRO(Word)
                                << " TOT2: " <<PYBAR_DATA_RECORD_TOT2_MACRO(Word)
                                << " COL2: " <<PYBAR_DATA_RECORD_COLUMN2_MACRO(Word)
                                << " ROW2: " <<PYBAR_DATA_RECORD_ROW2_MACRO(Word)
                                << std::endl;

            return false;
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
        //printf("getHitData col=%d,row=%d,tot=%d\n",Col,Row,ToT);
        //exit(0);
        return true;
      }

      StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
        StandardPlane plane(id, EVENT_TYPE, "PyBAR");

        // check for consistency
        bool valid=isEventValid(data);
        //printf("ConvertPlane() valid=%d\n",valid);
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
        const unsigned int * p=reinterpret_cast<const unsigned int *>(&(data[0]));
        for (unsigned int i=1; i < data.size()/4; ++i) {
          const unsigned int &Word = p[i];  //direct casting needed for current versions of pybar  - luetticke
          //printf("ConvertPlane() word=%08x masked=%08x %d\n",Word,(PYBAR_DATA_HEADER_MASK & Word),PYBAR_DATA_HEADER_MACRO(Word));
          if (PYBAR_DATA_HEADER_MACRO(Word)) {
            lvl1++;
          } else {
            // First Hit
            if (getHitData(Word, false, Col, Row, ToT)) {
              //printf("ConvertPlane() First Hit %d, %d %d %d\n",Col,Row,ToT,lvl1);
              plane.PushPixel(Col, Row, ToT, true, lvl1 - 1);
              eventnr++;
            }
            // Second Hit
            if (getHitData(Word, true, Col, Row, ToT)) {
              //printf("ConvertPlane() Second Hit %08x, %d, %d %d %d\n",Word,Col,Row,ToT,lvl1);
              plane.PushPixel(Col, Row, ToT, true, lvl1 - 1);
              eventnr++;
            }
          }
        }
        //printf("ConvertPlane() eventnr=%d plane=%d\n",eventnr);
        return plane;
      }
  };

  // Declare a new class that inherits from DataConverterPlugin
  class PyBARConverterPlugin : public DataConverterPlugin , public PyBARConverterBase {

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event & bore,
          const Configuration & cnf) {

#ifndef WIN32
			  (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
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
        //printf("PyBARConverterPlugin NumBlocks=%d\n",ev_raw.NumBlocks());
        for (size_t i = 0; i < ev_raw.NumBlocks(); ++i) {
          //printf("GetId=%d",ev_raw.GetID(i));
          sev.AddPlane(ConvertPlane(ev_raw.GetBlock(i), ev_raw.GetID(i)));
        }

        return true;
      }



    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      PyBARConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE)
      {}

      // The single instance of this converter plugin
      static PyBARConverterPlugin m_instance;
  };

  // Instantiate the converter plugin instance
  PyBARConverterPlugin PyBARConverterPlugin::m_instance;

} // namespace eudaq
