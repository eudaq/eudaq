/* Raw data conversion for the BDAQ53 producer
 *
 *
 * jorge.duarte.campderros@cern.ch
 * 
 */

#define DEBUG_1 0

#include "eudaq/RD53ADecoder.hh"

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

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
// Eudaq
#  include "EUTelRD53ADetector.h"
#  include "EUTelRunHeaderImpl.h"
// Eutelescope
#  include "EUTELESCOPE.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

#include <stdlib.h>
#include <cstdint>
#include <map>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <istream>

#ifdef DEBUG_1
#include <bitset>
#include <iomanip>
#endif 

namespace eudaq 
{
    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    //static const char* EVENT_TYPE   = "BDAQ53";
    static const char* EVENT_TYPE   = "bdaq53a";
    
#if USE_LCIO && USE_EUTELESCOPE
    static const int chip_id_offset = 30;
#endif


    // Declare a new class that inherits from DataConverterPlugin
    class BDAQ53ConverterPlugin : public DataConverterPlugin 
    {
        public:
            enum LAYOUT_FLAVOUR 
            {
                L100X25 = 0,
                L25x100 = 0,
                L50X50  = 1,
                DEFAULT = 1,
                _UNDF
            };

            // This is called once at the beginning of each run.
            // You may extract information from the BORE and/or configuration
            // and store it in member variables to use during the decoding later.
            virtual void Initialize(const Event & bore, const Configuration & cnf) 
            {
                // -- Obtain the PIXEL LAYOUT
                eudaq::Configuration * theconfig = nullptr;
                bool theconfig_todelete = false;
                // Obtain the offline modifications at layout if needed
                // otherwise from the config file
                std::ifstream flayout("sensor_layouts_offline.cfg");
                if( flayout.good() )
                {
                    // do things as modify the BORE with the new tags
                    theconfig = new eudaq::Configuration("cfg_offline");
                    theconfig->Load(flayout,"BDAQ53A.SensorsLayout");
                    theconfig_todelete=false;
                }
                else
                {
                    // -- Try to obtain it from the config file 
                    theconfig = const_cast<eudaq::Configuration*>(&cnf);
                }
                // Extract board id
                const unsigned int board_id = bore.GetTag("board", -999);
                // Get the pixel layout
                const std::string layout_cfg("layout_DUT"+std::to_string(board_id));
                // Default layout: 50x50
                const std::string sensor_flavour = theconfig->Get(layout_cfg,"50x50");
                if(theconfig_todelete)
                {
                    delete theconfig;
                    theconfig = nullptr;
                }
                    
                
                if(board_id != -999)
                {
                    _boards.emplace_back(board_id);
                    _board_channels[board_id] = std::vector<int>();
                    _board_initialized[board_id] = false;
                    EUDAQ_INFO("[BDAQ53A::New Kintex KC705 board added. Board ID: "+std::to_string(board_id)+"]");
                    // Set the layout
                    set_geometry_layout(sensor_flavour,board_id);
                    EUDAQ_INFO("[BDAQ53A::-------Sensor pitch: "
                            +std::to_string(int(get_x_pitch(board_id)*1e3))+
                            "x"+std::to_string(int(get_y_pitch(board_id)*1e3))+" um]");
                }
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
                    else if(rev->NumBlocks()<1)
                    {
                        return (unsigned)-1;
                    }
                    else if(rev->GetBlock(0).size() == 0)
                    {
                        return (unsigned)-1;
                    }
                    unsigned int trigger_number = RD53ADecoder::get_trigger_number(rev->GetBlock(0));
                    if( trigger_number == static_cast<unsigned>(-1) )
                    {
                        EUDAQ_ERROR("No trigger word found in the first 32-block.");
                        return (unsigned)-1;
                    }
                    else
                    {
                        return trigger_number;
                    }
                }
                // Not proper event or not enable to extract Trigger ID
                return (unsigned)-1;
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
                
                // Clear memory
                _data_map->clear();
                
                // Number of event headers: must be, at maximum, the number of
                // subtriggers of the Trigger Table (TT)
                unsigned int n_event_headers = 0;
                
                // If we get here it must be a data event
                const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
                // Trigger number is the same for all blocks (sensors)
                uint32_t const trigger_number = GetTriggerID(ev);
                const unsigned int board_id = ev_raw.GetTag("board",int());
                // --- Get the pixel pitch X and Y 
                const float x_pitch = get_x_pitch(board_id);
                const float y_pitch = get_y_pitch(board_id);
                
                const std::string plane_name("KC705_"+std::to_string(board_id)+":RD53A");
                if(static_cast<uint32_t>(trigger_number) == -1)
                {
                    // Dummy plane
                    StandardPlane plane(board_id, EVENT_TYPE,plane_name);
                    plane.SetSizeZS(get_ncols(board_id),get_nrows(board_id),0,RD53A_MAX_TRG_ID,
                            StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
                    sev.AddPlane(plane);
                    return false;
                }
                //else if(trigger_number != 0)
                //{
                //    // Got a trigger word
                //    event_status |= E_EXT_TRG;
                //}

                // [JDC] each block could corresponds to one sensor attach to 
                //       the card. So far, we are using only 1-sensor, but it 
                //       could be extended at some point
                for(size_t i = 0; i < ev_raw.NumBlocks(); ++i) 
                {
                    // Create a standard plane representing one sensor plane
                    StandardPlane plane(i, EVENT_TYPE,plane_name);
                    // FIXME: Understand what is the meaning of those flags!! 
                    plane.SetSizeZS(get_ncols(board_id),get_nrows(board_id),0,RD53A_MAX_TRG_ID,
                            StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

                    const auto & decoded_data = get_decoded_data(ev_raw.GetBlock(i),i);
                    plane.SetTLUEvent(trigger_number);
                    // fill data-header (be used?)
                    for(unsigned int dh=0; dh < decoded_data.n_event_headers(); ++dh)
                    {
                        uint32_t bcid = decoded_data.bcid()[dh];
                        uint32_t trig_id = decoded_data.trig_id()[dh];
                        uint32_t trig_tag = decoded_data.trig_tag()[dh];
                        //decoded_data->header_number;
#if DEBUG_1
                        std::cout << "EH  " << std::setw(9) << bcid
                            << std::setw(9) << trig_id << std::setw(9) 
                            << trig_tag << " " << std::bitset<32>(data_word) << std::endl;
#endif
                        for(const auto & hits: decoded_data.hits(dh))
                        {
                            plane.PushPixel(get_pixel_column.at(board_id)(hits[0],hits[1]),
                                    get_pixel_row.at(board_id)(hits[0],hits[1]),
                                    hits[2],false,trig_id);
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
                if(eudaqEvent.IsBORE()) 
                {
                    // shouldn't happen
                    return true;
                } 
                else if(eudaqEvent.IsEORE()) 
                {
                    // nothing to do
                    return true;
                }

                // Clear the memory
                _data_map->clear();
                
                // set type of the resulting lcio event
                lcioEvent.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);
                // pointer to collection which will store data
                LCCollectionVec * zsDataCollection = nullptr;
                // it can be already in event or has to be created
                bool zsDataCollectionExists = false;
                try 
                {
                    zsDataCollection = static_cast<LCCollectionVec*>(lcioEvent.getCollection("zsdata_rd53a"));
                    zsDataCollectionExists = true;
                } 
                catch(lcio::DataNotAvailableException& e) 
                {
                    zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
                }
                //	create cell encoders to set sensorID and pixel type
                CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
                
                // this is an event as we sent from Producer
                // needs to be converted to concrete type RawDataEvent
                const RawDataEvent & ev_raw = dynamic_cast <const RawDataEvent &>(eudaqEvent);
                std::vector<eutelescope::EUTelSetupDescription*> setupDescription;
                // --- Get the board identifier
                const unsigned int board_id = ev_raw.GetTag("board",int());
                
                // [JDC - XXX: Just in case we're able to readout more chips
                for(size_t chip = 0; chip < ev_raw.NumBlocks(); ++chip) 
                {
                    //const std::vector <unsigned char> & buffer=dynamic_cast<const std::vector<unsigned char> &>(ev_raw.GetBlock(chip));
                    const RawDataEvent::data_t & raw_data = ev_raw.GetBlock(chip);
                    if (lcioEvent.getEventNumber() == 0) 
                    {
                        eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelRD53ADetector(get_x_pitch(board_id),get_y_pitch(board_id));
                        currentDetector->setMode("ZS");
                        setupDescription.push_back( new eutelescope::EUTelSetupDescription(currentDetector)) ;
                    }
                    zsDataEncoder["sensorID"] = ev_raw.GetID(chip)+(10*board_id+chip_id_offset);  
                    zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;
                    
                    // prepare a new TrackerData object for the ZS data
                    // it contains all the hits for a particular sensor in one event
                    std::unique_ptr<lcio::TrackerDataImpl > zsFrame(new lcio::TrackerDataImpl);
                    // set some values of "Cells" for this object
                    zsDataEncoder.setCellID(zsFrame.get());
                
                    // this is the structure that will host the sparse pixel
                    // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
                    // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
                    std::unique_ptr< eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel> >
                        sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

                    // Reassemble full 32-bit FE data word: 
                    // The word is constructed using two 32b+32b words (High-Low) 
                    // In order to build the FE high and low words, you need
                    // 4 blocks of uint8 (unsigned char) on little endian for the FE high word
                    // and 4 blocks of uint8 more (on little endian) for the FE low word
                    // (see _reassemble_word)
                    const auto & decoded_data = get_decoded_data(ev_raw.GetBlock(chip),chip);
                    for(unsigned int dh=0; dh < decoded_data.n_event_headers(); ++dh)
                    {
                        for(const auto & hits: decoded_data.hits(dh))
                        {
                            //sparseFrame->emplace_back(hits[0],hits[1],hits[2],decoded_data.trig_id()[dh]);
                            sparseFrame->emplace_back(get_pixel_column.at(board_id)(hits[0],hits[1]),
                                    get_pixel_row.at(board_id)(hits[0],hits[1]),
                                    hits[2],decoded_data.trig_id()[dh]);
                        }
                    }
                    // write TrackerData object that contains info from one sensor to LCIO collection
                    zsDataCollection->push_back(zsFrame.release());
                }
                // add this collection to lcio event
                if( (!zsDataCollectionExists )  && (zsDataCollection->size() != 0) )
                {
                    lcioEvent.addCollection( zsDataCollection, "zsdata_rd53a" );
                }
                if (lcioEvent.getEventNumber() == 0) 
                {
                    // do this only in the first event
                    LCCollectionVec * apixSetupCollection = nullptr;
                    bool apixSetupExists = false;
                    try 
                    {
                        apixSetupCollection=static_cast<LCCollectionVec*>(lcioEvent.getCollection("rd53a_setup"));
                        apixSetupExists = true;
                    }
                    catch(...) 
                    {
                        apixSetupCollection = new LCCollectionVec(lcio::LCIO::LCGENERICOBJECT);
                    }
                    
                    for(auto const & plane: setupDescription) 
                    {
                        apixSetupCollection->push_back(plane);
                    }
                    if(!apixSetupExists)
                    {
                        lcioEvent.addCollection(apixSetupCollection, "rd53a_setup");
                    }
                }
                return true;
            }
#endif

        private:
            const RD53ADecoder & get_decoded_data(const RawDataEvent::data_t & raw_data,const int & chip) const
            {
                // memorize if not there
                if(_data_map->find(chip) == _data_map->end())
                {
                    _data_map->emplace(chip,RD53ADecoder(raw_data));
                }
                return _data_map->at(chip); //_data_map->find(chip)->second;
            }

            // Helper function to deal with diferent pixels geometry
            // -- Figure out the geometry type
            void set_geometry_layout(const std::string & layout, const unsigned int & board_id)
            {
                float xpitch = -1;
                float ypitch = -1;
                // RD53A layout is 50x50, if 100x25 --> 200 columns
                if(layout == "100x25" || layout == "25x100")
                {
                    //_sensor_layout[board_id] = LAYOUT_FLAVOUR::L100X25;
                    // the functors to map the roc to pixels
                    get_pixel_column[board_id] = _column_map_100x50;
                    get_pixel_row[board_id]    = _row_map_100x50;
                    xpitch=0.100;
                    ypitch=0.025;
                }
                else if(layout == "50x50")
                {
                    //_sensor_layout[board_id] = LAYOUT_FLAVOUR::L505X50;
                    // identity (no need to mapping)
                    get_pixel_column[board_id] = _identity_col;
                    get_pixel_row[board_id] = _identity_row;
                    xpitch = ypitch = 0.05;
                }
                else
                {
                    EUDAQ_ERROR("Unexistent layout '"+layout+"'");
                    std::cout << "BDAQ53A ERROR: Unexistent sensor layout read from config '" 
                        << layout << "'" << std::endl;
                    exit(-1);
                }

                // Set the pitch
                _sensor_xypitch[board_id] = std::pair<float,float>(xpitch,ypitch);
                // Set the number of columsn/rows
                _sensor_colrows[board_id] = std::pair<unsigned int,unsigned int>(
                        int(RD53A_XSIZE/xpitch),int(RD53A_YSIZE/ypitch));
            }

            // -- The pixel size
            const float get_x_pitch(unsigned int board_id) const
            {
                return _sensor_xypitch.at(board_id).first;
            }
            const float get_y_pitch(unsigned int board_id) const
            {
                return _sensor_xypitch.at(board_id).second;
            }
            // The total number of rows and columns
            const float get_ncols(unsigned int board_id) const
            {
                return _sensor_colrows.at(board_id).first;
            }
            const float get_nrows(unsigned int board_id) const
            {
                return _sensor_colrows.at(board_id).second;
            }
            // The mapping from ROC to 100x25 pixel
            static int _column_map_100x50(unsigned int column_roc, unsigned int row_roc)
            {
                return int(floor(column_roc/2.0));
            }

            static int _row_map_100x50(unsigned int column_roc, unsigned int row_roc)
            {
                return 2*row_roc+(column_roc%2);
            }
            // No mapping
            static int _identity_col(unsigned int column_roc,unsigned int row_roc)
            {
                return column_roc;
            }
            static int _identity_row(unsigned int column_roc,unsigned int row_roc)
            {
                return row_roc;
            }

            // The mapping function
            std::map<unsigned int,std::function<int(unsigned int,unsigned int)> > get_pixel_column;
            std::map<unsigned int,std::function<int(unsigned int,unsigned int)> > get_pixel_row;
            
            // The decoded data memory (pointer to avoid constantness of the Get** functions)
            std::unique_ptr<std::map<int,RD53ADecoder> >  _data_map;
            
            // The number of BDAQ/Kintex boards present
            std::vector<unsigned int> _boards;
            // I dont' know yet --> TO BE DEPRECATED
            std::map<unsigned int,std::vector<int> > _board_channels;
            // i--- > TO BE DEPRECATED
            std::map<unsigned int,bool> _board_initialized;
            // --- Dealing with different pixel layouts
            // the x_pitch/y_pitch
            std::map<unsigned int,std::pair<float,float> > _sensor_xypitch;
            // the sensor geometry
            std::map<unsigned int,std::pair<unsigned int,unsigned int> > _sensor_colrows;

            // The constructor can be private, only one static instance is created
            // The DataConverterPlugin constructor must be passed the event type
            // in order to register this converter for the corresponding conversions
            // Member variables should also be initialized to default values here.
            BDAQ53ConverterPlugin() : 
                DataConverterPlugin(EVENT_TYPE),
                _data_map(std::unique_ptr<std::map<int,RD53ADecoder>>(new std::map<int,RD53ADecoder>)) 
            {
            }

            // The single instance of this converter plugin
            static BDAQ53ConverterPlugin m_instance;

    };

    // Instantiate the converter plugin instance
    BDAQ53ConverterPlugin BDAQ53ConverterPlugin::m_instance;

} // namespace eudaq
