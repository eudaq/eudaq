#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <unistd.h>
#include <map>
#include <stdexcept>

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelRunHeaderImpl.h"
#endif

typedef struct {
    uint16_t col;
    uint16_t row;
    uint16_t tot;
} Fei4Hit;

namespace eudaq {

    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    static const std::string EVENT_TYPE = "Yarr";

    // A variable to store the sensor layout
    std::map<unsigned int, std::string> layout_DUT;

    //Set correct width (number of columns) of the sensor, based on the layout
    static int sensorWidth(std::string layout){
      if (layout=="100_25_odd" or layout=="100_25_even")
        return 800;
      else if (layout=="25_100_odd" or layout=="25_100_even")
        return 200;
      else if (layout=="50_50")
        return 400;
      else {
        throw std::invalid_argument( "YARR Converter Plugin Error: '"+layout+"' unexistent sensor layout." );
      }
    }

    //Set correct height (number of rows) of the sensor, based on the layout
    static int sensorHeight(std::string layout){
      if (layout=="100_25_odd" or layout=="100_25_even")
        return 96;
      else if (layout=="25_100_odd" or layout=="25_100_even")
        return 384;
      else if (layout=="50_50")
        return 192;
      else {
        throw std::invalid_argument( "YARR Converter Plugin Error: '"+layout+"' unexistent sensor layout." );
      }
    }

    // Mapping columns from RD53 to Sensor Geometry
    static int _column_RD53toSensorGeometry(std::string layout, unsigned int column_roc, unsigned int row_roc ){
        // Mapping for 100x25 planar pixel sensor with odd connectivity on RD53a
        if (layout=="100_25_odd")
          return 2*column_roc+(row_roc%2);
        // Mapping for 100x25 planar pixel sensor with even connectivity on RD53a
        else if (layout=="100_25_even")
          return 2*column_roc+((row_roc-1)%2);
        // Mapping for 25x100 planar pixel sensor with odd connectivity on RD53a
        else if (layout=="25_100_odd")
          return static_cast<int>(floor(column_roc/2.0));
        // Mapping for 25x100 planar pixel sensor with  even connectivity on RD53a
        else if (layout=="25_100_even")
          return static_cast<int>(floor(column_roc/2.0));
        // DEFAULT 50x50 geometry
        else
          return static_cast<int>(column_roc);
    }

    // Mapping rows from RD53 to Sensor Geometry
    static int _row_RD53toSensorGeometry(std::string layout, unsigned int column_roc, unsigned int row_roc ){
        // Mapping for 100x25 planar pixel sensor with odd connectivity on RD53a
        if (layout=="100_25_odd")
          return static_cast<int>(floor(row_roc/2.0));
        // Mapping for 100x25 planar pixel sensor with even connectivity on RD53a
        else if (layout=="100_25_even")
          return static_cast<int>(floor(row_roc/2.0));
        // Mapping for 25x100 planar pixel sensor with odd connectivity on RD53a
        else if (layout=="25_100_odd")
          return 2*row_roc+(column_roc%2);
        // Mapping for 25x100 planar pixel sensor with  even connectivity on RD53a
        else if (layout=="25_100_even")
          return 2*row_roc+((column_roc-1)%2);
        // DEFAULT 50x50 geometry
        else
          return static_cast<int>(row_roc);
    }


    // Declare a new class that inherits from DataConverterPlugin
    class YARRConverterPlugin : public DataConverterPlugin {

        public:
            // This is called once at the beginning of each run.
            // You may extract information from the BORE and/or configuration
            // and store it in member variables to use during the decoding later.
            virtual void Initialize(const Event &bore, const Configuration &cnf) {
#ifndef WIN32 // some linux Stuff //$$change
                (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
		    
               // Read the sensor layouts from the file
               std::ifstream infile;
               infile.open("./sensor_layout.txt");
               std::string layout;
               int DUT_id;

               while(true){
                  infile >> DUT_id >> layout;
                     if (infile.eof()) {
                        break;
                     }
                  layout_DUT[DUT_id]=layout;
               }
		    
            }

            // Here, the data from the RawDataEvent is extracted into a StandardEvent.
            // The return value indicates whether the conversion was successful.
            // Again, this is just an example, adapted it for the actual data layout.


            virtual bool GetStandardSubEvent(StandardEvent &sev,
                    const Event &ev) const {
                // If the event type is used for different sensors
                // they can be differentiated here
                std::string sensortype = "Rd53a";
                // Create a StandardPlane representing one sensor plane
                int base_id = 110;
		    
                const RawDataEvent & my_ev = dynamic_cast<const RawDataEvent &>(ev);
                int ev_id = my_ev.GetTag("EventNumber", -1);

                for (unsigned i=0; i<my_ev.NumBlocks(); i++) {
                    eudaq::RawDataEvent::data_t block=my_ev.GetBlock(i);
                    int plane_id = base_id + my_ev.GetID(i);

                    if (layout_DUT[plane_id].empty()){
                      throw std::invalid_argument( "Yarr Converter Plugin Error: DUT ID does not exist in the setup." );
                    }

                    StandardPlane plane(plane_id, EVENT_TYPE, sensortype);

                    // Set the number of pixels
                    plane.SetSizeZS(sensorWidth(layout_DUT[plane_id]), sensorHeight(layout_DUT[plane_id]), 0, 32, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
                    plane.SetTLUEvent(ev_id);

                    unsigned it = 0;
                    unsigned fragmentCnt = 0;
                    while (it < block.size()) { // Should find as many fragments as trigger per event
                        uint32_t tag = *((uint32_t*)(&block[it])); it+= sizeof(uint32_t);
                        uint32_t l1id = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t bcid = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t nHits = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        for (unsigned i=0; i<nHits; i++) {
                            Fei4Hit hit = *((Fei4Hit*)(&block[it])); it+= sizeof(Fei4Hit);
			    //                            plane.PushPixel(hit.col,hit.row,hit.tot);
                          //std::cout << "Frame: " << l1id << ", Plane: " << plane_id << std::endl;
                            plane.PushPixel(_column_RD53toSensorGeometry(layout_DUT[plane_id],hit.col-1,hit.row-1),_row_RD53toSensorGeometry(layout_DUT[plane_id],hit.col-1,hit.row-1),hit.tot,l1id);
                        }
                        fragmentCnt++;
                    }
                    sev.AddPlane(plane);
                }

                // Indicate that data was successfully converted
                return true;
            }

#if USE_LCIO && USE_EUTELESCOPE
            // This is where the conversion to LCIO is done
            virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
                return 0;
            }

            virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const {

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

                //create cell encoders to set sensorID and pixel type
                CellIDEncoder<TrackerDataImpl> zsDataEncoder   ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection  );

                //this is an event as we sent from Producer, needs to be converted to concrete type RawDataEvent
                const RawDataEvent & ev_raw = dynamic_cast <const RawDataEvent &> (eudaqEvent);

                std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
                std::list<eutelescope::EUTelGenericSparsePixel*> tmphits;

		//                std::string sensortype = "Rd53a";
                int base_id = 110;
                int width = 400, height = 192;




                const RawDataEvent & my_ev = dynamic_cast<const RawDataEvent &>(ev_raw);
                for (int i=0; i<ev_raw.NumBlocks(); i++) {
                    eudaq::RawDataEvent::data_t block=my_ev.GetBlock(i);

		    zsDataEncoder["sensorID"] = base_id+my_ev.GetID(i);
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

                    unsigned it = 0;
		    unsigned fragmentCnt = 0;
		    while (it < block.size()){
		      uint32_t tag = *((uint32_t*)(&block[it])); it+= sizeof(uint32_t);
		      uint32_t l1id = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
		      uint32_t bcid = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
		      uint32_t nHits = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
		      for (unsigned i=0; i<nHits; i++) {
                        Fei4Hit hit = *((Fei4Hit*)(&block[it])); it+= sizeof(Fei4Hit);
                        eutelescope::EUTelGenericSparsePixel *thisHit = new eutelescope::EUTelGenericSparsePixel( hit.col, hit.row, hit.tot, l1id);
                        //sparseFrame->addSparsePixel( thisHit );
                        sparseFrame->emplace_back( hit.col, hit.row, hit.tot, l1id );
                        tmphits.push_back( thisHit );
		      }
		      fragmentCnt++;
		    }
		    // write TrackerData object that contains info from one sensor to LCIO collection
		    zsDataCollection->push_back( zsFrame.release() );
                }


                for( std::list<eutelescope::EUTelGenericSparsePixel*>::iterator it = tmphits.begin(); it != tmphits.end(); it++ ){
                    delete (*it);
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
            YARRConverterPlugin()
                : DataConverterPlugin(EVENT_TYPE) {}

            // Information extracted in Initialize() can be stored here:
            unsigned m_run;

            // The single instance of this converter plugin
            static YARRConverterPlugin m_instance;
    }; // class ExampleConverterPlugin

    // Instantiate the converter plugin instance
    YARRConverterPlugin YARRConverterPlugin::m_instance;

} // namespace eudaq
