#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#include "jsoncons/json.hpp"

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

class Version
{
    public:
    Version() : m_version_string("0.0.0.0") { };
    Version( std::string v ) : m_version_string(v) { };
    
    friend bool operator <  (Version u, Version v);
    friend bool operator >  (Version u, Version v);
    friend bool operator <= (Version u, Version v);
    friend bool operator >= (Version u, Version v);
    friend bool operator == (Version u, Version v);
    
    private:
    std::string m_version_string;
    
    int version_compare(std::string v1, std::string v2) {
       size_t i=0, j=0;
       while( i < v1.length() || j < v2.length() ) {
           int acc1=0, acc2=0;

           while (i < v1.length() && v1[i] != '.') {  acc1 = acc1 * 10 + (v1[i] - '0');  i++;  }
           while (j < v2.length() && v2[j] != '.') {  acc2 = acc2 * 10 + (v2[j] - '0');  j++;  }

           if (acc1 < acc2)  return -1;
           if (acc1 > acc2)  return +1;

           ++i;
           ++j;
        }
        return 0;
    }    
};

bool operator <  (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) == -1;  }
bool operator >  (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) == +1;  }
bool operator <= (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) != +1;  }
bool operator >= (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) != -1;  }
bool operator == (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) ==  0; }

typedef struct {
    uint16_t col;
    uint16_t row;
    uint16_t tot;
} Fei4Hit;


struct chipInfo {
    std::string name;
    std::string chipId; 
    unsigned int rx;
    std::string moduleSize;
    std::string sensorLayout;
    std::string pcbType;
    unsigned int chipLocationOnModule;
    unsigned int internalModuleIndex;
    std::string moduleName;    
};
                     


namespace eudaq {

    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    static const std::string EVENT_TYPE = "Yarr";

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
                std::string event_version = bore.GetTag("YARREVENTVERSION", "1.0.0");

		// getting the producer ID for event version >=2.0.1
		int prodID = std::stoi(bore.GetTag("PRODID"));
		std::cout << "TAG: " << bore.GetTag("PRODID") << std::endl;
		
                m_EventVersion[prodID] = event_version; // only now we have the producer ID available
                
                std::string DUTTYPE = bore.GetTag("DUTTYPE");
                
                if(DUTTYPE=="RD53A") {
                   m_FrontEndType[prodID] = "Rd53a";
                } else if(DUTTYPE=="RD53B") {
                   m_FrontEndType[prodID] = "Rd53b";
                };
		std::cout << "YarrConverterPlugin: front end type " << m_FrontEndType.at(prodID) << " declared in BORE for producer " << prodID << std::endl;
		
		// getting the module information for event version >=2.1.0
		jsoncons::json moduleChipInfoJson = jsoncons::json::parse(bore.GetTag("MODULECHIPINFO"));
		for(const auto& uid : moduleChipInfoJson["modules"].array_range()) {
                   chipInfo currentlyHandledChip;
		   currentlyHandledChip.name = uid["name"].as<std::string>();
		   currentlyHandledChip.chipId = uid["chipId"].as<std::string>();
		   currentlyHandledChip.rx = uid["rx"].as<unsigned int>();
		   currentlyHandledChip.moduleSize = uid["moduleSize"].as<std::string>();
		   currentlyHandledChip.sensorLayout = uid["sensorLayout"].as<std::string>();
		   currentlyHandledChip.pcbType = uid["pcbType"].as<std::string>();
		   currentlyHandledChip.chipLocationOnModule = uid["chipLocationOnModule"].as<unsigned int>();
		   currentlyHandledChip.internalModuleIndex = uid["internalModuleIndex"].as<unsigned int>();		   
		   currentlyHandledChip.moduleName = uid["moduleName"].as<std::string>();

                   m_chip_info_by_uid[prodID].push_back(currentlyHandledChip);
		};
		
                int base_id = 110 + prodID * 20; 
                // assuming that there will be never more than 20 FEs connected to one SPEC/producer; otherwise a more intricate handling is necessary
                
		for(const auto& uid : m_chip_info_by_uid[prodID]) {
		   m_module_size_by_module_index[prodID][uid.internalModuleIndex] = uid.moduleSize;
		   m_plane_id_by_module_index[prodID][uid.internalModuleIndex] = base_id + uid.internalModuleIndex;
		   m_module_name_by_module_index[prodID][uid.internalModuleIndex] = uid.moduleName;
		} 
		
            }

            // Here, the data from the RawDataEvent is extracted into a StandardEvent.
            // The return value indicates whether the conversion was successful.
            // Again, this is just an example, adapted it for the actual data layout.
            virtual bool GetStandardSubEvent(StandardEvent &sev,
                    const Event &ev) const {
                    
                if(ev.IsEORE()) {
                   std::cout << "EORE has been reached" << std::endl;
                   return true;
                }
                
                // Differentiate between different sensors
		int prodID = std::stoi(ev.GetTag("PRODID"));
		
		std::map<unsigned int, StandardPlane> standard_plane_by_module_index;
		for (std::size_t currentModuleIndex = 0; currentModuleIndex != m_plane_id_by_module_index.size(); ++currentModuleIndex) {
		    std::string local_plane_type;
		    unsigned int width, height;
		    if(m_FrontEndType.at(prodID)=="Rd53a"){
                       width  = 400;
                       height = 192;
                       local_plane_type = m_FrontEndType.at(prodID);
                     } else if ((m_FrontEndType.at(prodID)=="Rd53b")&&(m_chip_info_by_uid[prodID][currentModuleIndex].moduleSize=="single")) {
                       width  = 400;
                       height = 384;                       
                       local_plane_type = m_FrontEndType.at(prodID);                       
                     } else if ((m_FrontEndType.at(prodID)=="Rd53b")&&(m_chip_info_by_uid[prodID][currentModuleIndex].moduleSize=="quad")) {
                       width  = 800;
                       height = 768;
                       local_plane_type = "RD53BQUAD";                       
                     };
                     unsigned int local_id = m_plane_id_by_module_index[prodID][currentModuleIndex];
                     
		   standard_plane_by_module_index[currentModuleIndex] = StandardPlane(local_id, EVENT_TYPE, m_FrontEndType.at(prodID));
                    // Set the number of pixels
                   standard_plane_by_module_index[currentModuleIndex].SetSizeZS(width, height, 0, 32, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);		   
		};


                const RawDataEvent & my_ev = dynamic_cast<const RawDataEvent &>(ev);
                int ev_id = my_ev.GetTag("EventNumber", -1); 
                
                for (unsigned i=0; i<my_ev.NumBlocks(); i++) {
                    eudaq::RawDataEvent::data_t block=my_ev.GetBlock(i);
                                      
                    standard_plane_by_module_index[m_chip_info_by_uid[prodID][i].internalModuleIndex].SetTLUEvent(ev_id);
                         
                    unsigned it = 0;
                    unsigned fragmentCnt = 0;
                    while (it < block.size()) { // Should find as many fragments as trigger per event
                        uint32_t tag = *((uint32_t*)(&block[it])); it+= sizeof(uint32_t);
                        uint32_t l1id = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t bcid = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t nHits = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        for (unsigned i=0; i<nHits; i++) {
                            Fei4Hit hit = *((Fei4Hit*)(&block[it])); it+= sizeof(Fei4Hit);
			    //plane.PushPixel(hit.col,hit.row,hit.tot);
			    //std::cout << "col: " << hit.col  << " row: " << hit.row << " tot: " << hit.tot << " l1id: " << l1id << " tag: " << tag << std::endl;
                            if(m_FrontEndType.at(prodID)=="Rd53a"){
                                standard_plane_by_module_index[m_chip_info_by_uid[prodID][i].internalModuleIndex].PushPixel(hit.col,hit.row,hit.tot,false,l1id);
                            } else if(m_FrontEndType.at(prodID)=="Rd53b"){
                                standard_plane_by_module_index[m_chip_info_by_uid[prodID][i].internalModuleIndex].PushPixel(hit.col,hit.row,hit.tot,false,tag);
                            }
                        }
                        fragmentCnt++;
                    }
                }
		for(auto it = standard_plane_by_module_index.begin(); it != standard_plane_by_module_index.end(); ++it) {
                    sev.AddPlane(it->second);		
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

            // Information extracted in Initialize()
            mutable std::map<int,std::string> m_FrontEndType;
            mutable std::map<int,Version> m_EventVersion;
            mutable std::map<int,std::vector<chipInfo> > m_chip_info_by_uid;
            mutable std::map<int,std::map<unsigned int,std::string> > m_module_size_by_module_index;
            mutable std::map<int,std::map<unsigned int, unsigned int> > m_plane_id_by_module_index;            
            mutable std::map<int,std::map<unsigned int, std::string> > m_module_name_by_module_index;                        
            unsigned m_run;

            // The single instance of this converter plugin
            static YARRConverterPlugin m_instance;
            
    }; // class YarrConverterPlugin

    // Instantiate the converter plugin instance
    YARRConverterPlugin YARRConverterPlugin::m_instance;

} // namespace eudaq
