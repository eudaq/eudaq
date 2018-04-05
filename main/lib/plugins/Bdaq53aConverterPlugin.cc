/* Raw data conversion for the Bdaq53a producer
 *
 *
 * jorge.duarte.campderros@cern.ch
 * 
 */

#define DEBUG_1 1

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Bdaq53a.hh"

#include <stdlib.h>
#include <cstdint>

#ifdef DEBUG_1
#include <bitset>
#include <iomanip>
#endif 

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

namespace eudaq 
{
    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    static const char* EVENT_TYPE   = "bdaq53a";
    static const unsigned int NCOLS = 400;
    static const unsigned int NROWS = 192;

#if USE_LCIO && USE_EUTELESCOPE
    static const int chip_id_offset = 20;
#endif

    // Declare a new class that inherits from DataConverterPlugin
    class Bdaq53aConverterPlugin : public DataConverterPlugin
    {
        public:
            // This is called once at the beginning of each run.
            // You may extract information from the BORE and/or configuration
            // and store it in member variables to use during the decoding later.
            virtual void Initialize(const Event & bore, const Configuration & cnf) 
            {
#ifndef WIN32
                (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
                // XXX: Is there any parameters? 
                _get_bore_parameters(bore);
            }
            
            // This should return the trigger ID (as provided by the TLU)
            // if it was read out, otherwise it can either return (unsigned)-1,
            // or be left undefined as there is already a default version.
            virtual unsigned GetTriggerID(const Event & ev) const 
            {
                // Make sure the event is of class RawDataEvent
                if(const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) 
                {
                    if(rev->IsBORE() || rev->IsEORE())
                    {
                        return 0;
                    }
                    bool is_data_header = false;
                    if(rev->NumBlocks() >0)
                    {
                        // Trigger header always first 32-bit word
                        return BDAQ53A_TRIGGER_NUMBER(_get_32_word(rev->GetBlock(0),0));
                    }
                    else
                    {
                        return -1;
                    }
                }
                // Not proper event or not enable to extract Trigger ID
                return (unsigned) -1;
            }

            // Here, the data from the RawDataEvent is extracted into a StandardEvent.
            // The return value indicates whether the conversion was successful.
            // Again, this is just an example, adapted it for the actual data layout.
            virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const 
            {
                if(ev.IsBORE() || ev.IsEORE())
                {
                    // nothing to do
                    return true;
                }

                uint32_t event_status = 0;
                // If we get here it must be a data event
                const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
                // [JDC] each block corresponds to one potential sensor attach to 
                //       the card?
                for(size_t i = 0; i < ev_raw.NumBlocks(); ++i) 
                {
                    // Create a standard plane representing one sensor plane
                    StandardPlane plane(i, EVENT_TYPE,"rd53a");
                    plane.SetSizeZS(NCOLS,NROWS,0,1);
                    const RawDataEvent::data_t & raw_data = ev_raw.GetBlock(i);
                    // XXX : Missing check of USER_K data 
                    if( ! BDAQ53A_IS_TRIGGER(_get_32_word(raw_data,0)) )
                    {
                        // Malformed data
                        EUDAQ_ERROR("NO TRIGGER WORD PRESENT!");
                        return false;
                    }

                    // First 32-bit word: the trigger number
                    uint32_t trigger_number = BDAQ53A_TRIGGER_NUMBER(_get_32_word(raw_data,0));
                    plane.SetTLUEvent(trigger_number);
                    /*std::cout << " = number of data words:" << raw_data.size() 
                        << ", Event number: " << ev.GetEventNumber() << ")"<< std::endl;
                    std::cout << "TRG:" << std::setw(26) << trigger_number << " " 
                        << std::setw(32) << std::bitset<32>(_get_32_word(raw_data,0)) << std::endl;*/

                    // Reassemble full 32-bit FE data word: 
                    // The word is constructed using two 16b+16b words (High-Low) _
                    for(unsigned int it = sizeof(uint16_t); it < raw_data.size(); it+=2*sizeof(uint16_t))
                    {
                        // Build the 32-bit FE word from [ raw_data[it](16bits)  raw_dat[it+2](16bits) ]
                        // The FE high-part
                        uint32_t data_word = ((_get_16_word(raw_data,it) & 0xFFFF) << 16);
                        // The FE low-part
                        data_word |= (_get_16_word(raw_data,it+sizeof(uint16_t)) & 0xFFFF);

                        if( BDAQ53A_IS_HEADER(_get_16_word(raw_data,it)) )
                        {
                            uint32_t bcid = BDAQ53A_DATA_HEADER_BCID(data_word);
                            uint32_t trig_id  = (data_word >> 20) & 0x1F;
                            uint32_t trig_tag = (data_word >> 15) & 0x1F;
                            /*std::cout << "EH  " << std::setw(8) << bcid << " " 
                                << std::setw(8) << trig_id << " " 
                                << std::setw(8) << trig_tag << " " 
                                << std::setw(32) <<  std::bitset<32>(data_word) << std::endl;*/
                        }
                        else
                        {
                            const uint32_t multicol = (data_word >> 26) & 0x1F;
                            const uint32_t region   = (data_word >> 15) & 0x1F;
                            for(unsigned int pixid=0; pixid < 4; ++pixid)
                            { 
                                uint32_t col = multicol*8 + pixid;
                                if(region % 2 == 1)
                                {
                                    col += 4;
                                }
                                const uint32_t row = int(region/2);
                                const uint32_t tot = (data_word >> (pixid*4)) & 0xF;
                                if( !(col < NCOLS && row < NROWS) )
                                {
                                    event_status |= E_UNKNOWN_WORD;
                                }
                                else
                                {
                                    plane.PushPixel(col,row,tot);
                                }
                            }
                        }
                    }
                    sev.AddPlane(plane);
                }
                return true;
            }

#if USE_LCIO && USE_EUTELESCOPE
            // This is where the conversion to LCIO is done
            virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const 
            {
                return 0;
            }
            
            virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const 
            {
                //std::cout << "getlciosubevent (I4) event " << eudaqEvent.GetEventNumber() << " | " << GetTriggerID(eudaqEvent) << std::endl;
                if (eudaqEvent.IsBORE()) 
                {
                    // shouldn't happen
                    return true;
                } 
                else if (eudaqEvent.IsEORE()) 
                {
                    // nothing to do
                    return true;
                }
                
                // set type of the resulting lcio event
                lcioEvent.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );
                // pointer to collection which will store data
                LCCollectionVec * zsDataCollection;
                // it can be already in event or has to be created
                bool zsDataCollectionExists = false;
                try 
                {
                    zsDataCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
                    zsDataCollectionExists = true;
                } 
                catch( lcio::DataNotAvailableException& e ) 
                {
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

          zsDataEncoder["sensorID"] = ev_raw.GetID(chip) + chip_id_offset + first_sensor_id; // formerly 14
          zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

          // prepare a new TrackerData object for the ZS data
          // it contains all the hits for a particular sensor in one event
          std::unique_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
          // set some values of "Cells" for this object
          zsDataEncoder.setCellID( zsFrame.get() );

          // this is the structure that will host the sparse pixel
          // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
          // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
          std::unique_ptr< eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > >
            sparseFrame( new eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > ( zsFrame.get() ) );

          unsigned int ToT = 0;
          unsigned int Col = 0;
          unsigned int Row = 0;
          unsigned int lvl1 = 0;

          for (unsigned int i=4; i < buffer.size(); i += 4) {
            unsigned int Word = (((unsigned int)buffer[i]) << 24) | (((unsigned int)buffer[i+1]) << 16) | (((unsigned int)buffer[i+2]) << 8) | (unsigned int)buffer[i+3];

            if (BDAQ53A_DATA_HEADER_MACRO(Word)) {
              lvl1++;
            } else {
              // First Hit
              if (getHitData(Word, false, Col, Row, ToT)) {
                sparseFrame->emplace_back( Col, Row, ToT, lvl1-1 );
              }
              // Second Hit
              if (getHitData(Word, true, Col, Row, ToT)) {
                sparseFrame->emplace_back( Col, Row, ToT, lvl1-1 );
              }
            }
          }

          // write TrackerData object that contains info from one sensor to LCIO collection
          zsDataCollection->push_back( zsFrame.release() );
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
            // Helper function: extract the i-esim 32bit word from the rawdata
            // It is assuming blocks of 32 bits.
            uint32_t _get_32_word(const RawDataEvent::data_t & rawdata,unsigned int i) const
            {
                // Will assume blocks of 32-bits, each element 
                // contains 8 (CHAR_INT) bits. 
                return *((uint32_t*)(&rawdata[sizeof(uint32_t)*i]));
            }
            
            // Helper function: extract the i-esim 16bit word from the rawdata
            // It is assuming blocks of 16 bits, therefore
            uint32_t _get_16_word(const RawDataEvent::data_t & rawdata,unsigned int i) const
            {
                // Will assume blocks of 16-bits, each element 
                // contains 8 (CHAR_INT) bits. 
                return *((uint32_t*)(&rawdata[sizeof(uint16_t)*i]));
            }
            
            void _get_bore_parameters(const Event & )
            {
                // XXX Just if need to initialize paremeters from the BORE
                //std::cout << bore << std::endl;
            }

            StandardPlane convert_plane(const RawDataEvent::data_t & rawdata, unsigned int id) const
            {
                // Consistency check
            }


      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      Bdaq53aConverterPlugin() : DataConverterPlugin(EVENT_TYPE) {}

      // The single instance of this converter plugin
      static Bdaq53aConverterPlugin m_instance;
  };

  // Instantiate the converter plugin instance
  Bdaq53aConverterPlugin Bdaq53aConverterPlugin::m_instance;

} // namespace eudaq
