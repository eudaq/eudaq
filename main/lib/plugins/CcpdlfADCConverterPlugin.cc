#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include <iostream>
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
  static const char* EVENT_TYPE = "ccpdlf_adc";

#if USE_LCIO && USE_EUTELESCOPE
  static const int chip_id_offset = 30;
#endif
  // Declare a new class that inherits from DataConverterPlugin
  class CcpdlfADCConverterPlugin : public DataConverterPlugin {

    public:
      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event & bore, const Configuration & cnf) {

#ifndef WIN32
			  (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const Event & ev) const {
        // Make sure the event is of class RawDataEvent
        if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
            if (rev->IsBORE() || rev->IsEORE()) return 0;
            if (rev->NumBlocks() <= 0)  return (unsigned) -1;
            const std::vector <unsigned char> & data=dynamic_cast<const std::vector<unsigned char> &> (rev->GetBlock(0));
            unsigned int tmp;
            for (unsigned int b=0; b < data.size(); b += 4) {
                tmp=(((unsigned int)data[b+3]) << 24) |(((unsigned int)data[b+2]) << 16) |(((unsigned int)data[b+1]) << 8) | (unsigned int)data[b+0];
                if ((tmp & 0xF0000000)==0x80000000){
                    return tmp & 0x0FFFFFFF;
                }
            }
            return (unsigned)-1;
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
        unsigned int cnt_max=0;
        
        for (size_t i = 0; i < ev_raw.NumBlocks(); ++i) {
          StandardPlane plane(ev_raw.GetID(i), EVENT_TYPE, "CcpdlfAdc");
          plane.SetSizeZS(114, 24, 0, 1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE); //
          //Get Trigger Number
          const std::vector <unsigned char> & data=dynamic_cast<const std::vector<unsigned char> &> (ev_raw.GetBlock(i));
          // Get Events
          unsigned int tmp,trigger_number=0xFFFFFFFF,wave_start=0xFFFFFFFF;
          unsigned int t_max=0,cnt_max=0,cnt_base=0,cnt;
          for (unsigned int b=0; b < data.size(); b += 4) {
            tmp=(((unsigned int)data[b+3]) << 24) |(((unsigned int)data[b+2]) << 16) |(((unsigned int)data[b+1]) << 8) | (unsigned int)data[b+0];
            if ((tmp & 0xF0000000)==0x80000000 && trigger_number==0xFFFFFFFF){
              trigger_number=tmp & 0x0FFFFFFF;
            }else if ((tmp & 0xF0000000)==0xF0000000 && trigger_number!=0xFFFFFFFF && wave_start==0xFFFFFFFF){
              wave_start=b;
              cnt_max=0x00003FFF & tmp;
              t_max=0;
            }else if ((tmp & 0xF0000000)==0xe0000000 && trigger_number!=0xFFFFFFFF && wave_start!=0xFFFFFFFF){
              cnt=(0x00003FFF & tmp);
              if (cnt>cnt_max){
                cnt_max=cnt;
                t_max=b-wave_start;
              }
              if (wave_start+20==b){
                cnt_base=cnt;
              }
            }else if ((tmp & 0xF0000000)==0x80000000 && trigger_number!=0xFFFFFFFF){
                break;
            }
          }
          plane.SetTLUEvent(trigger_number);
          plane.PushPixel(53,19,cnt_max-cnt_base, true, 0);
          plane.PushPixel(53,18,t_max, true, 0);
          sev.AddPlane(plane);
        }
        return true;
      }

#if USE_LCIO && USE_EUTELESCOPE
      // This is where the conversion to LCIO is done
      virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }

      virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const {
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
          const std::vector <unsigned char> & data=dynamic_cast<const std::vector<unsigned char> &> (ev_raw.GetBlock(chip));

          if (lcioEvent.getEventNumber() == 0) {
            eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector(2);
            currentDetector->setMode( "ZS" );

            setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
          }

          zsDataEncoder["sensorID"] = ev_raw.GetID(chip) + chip_id_offset; // 30
          zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

          // prepare a new TrackerData object for the ZS data
          // it contains all the hits for a particular sensor in one event
          std::auto_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
          // set some values of "Cells" for this object
          zsDataEncoder.setCellID( zsFrame.get() );

          // this is the structure that will host the sparse pixel
          // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
          // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
          std::auto_ptr< eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > >
            sparseFrame( new eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > ( zsFrame.get() ) );

          unsigned int cnt,trigger_number;
          unsigned int cnt_max=0,t_max=0,cnt_base=0;
          unsigned int flg_tlu=0xFFFFFFFF,flg_adc=0xFFFFFFFF;
          for (unsigned int b=0; b <data.size(); b += 4) {
              if (((unsigned int)data[b+3] & 0xF0)==0x80 && flg_tlu==0xFFFFFFFF){
                  flg_tlu=b;
                  trigger_number=(((unsigned int)data[3] & 0x0F) << 24) |(((unsigned int)data[2]) << 16) |(((unsigned int)data[1]) << 8) | (unsigned int)data[0];
              }else if (((unsigned int)data[b+3] & 0xF0)==0xF0 && flg_tlu+4==b){
                  flg_adc=b;
                  cnt = ((((unsigned int)data[b+1]) << 8) | ((unsigned int)data[b]) ) & 0x3FFF;  // frame count recorded by spi_rx
              }else if (((unsigned int)data[b+3] & 0xF0)==0xe0 &&flg_adc!=0xFFFFFFFF){
                  cnt = ((((unsigned int)data[b+1]) << 8) | ((unsigned int)data[b]) ) & 0x3FFF;  // frame count recorded by spi_rx
                  if (cnt>cnt_max){
                     cnt_max=cnt;
                     t_max=(b-flg_adc);
                  }
                  if (b==flg_adc+20){
                     cnt_base=cnt;
                  }
             }
          }
          if (flg_tlu==0xFFFFFFFF || flg_adc==0xFFFFFFFF){
               std::cout<< "GetStandardSubEvent() broken data"<<std::endl;
               return false;
          }
          sparseFrame->emplace_back( 53, 19, cnt_max-cnt_base, trigger_number );
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

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      CcpdlfADCConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE)
      {}

      // The single instance of this converter plugin
      static CcpdlfADCConverterPlugin m_instance;
  };

  // Instantiate the converter plugin instance
  CcpdlfADCConverterPlugin CcpdlfADCConverterPlugin::m_instance;

} // namespace eudaq
