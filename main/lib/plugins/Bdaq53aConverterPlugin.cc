/* Raw data conversion for the Bdaq53a producer
 *
 *
 * jorge.duarte.campderros@cern.ch
 * 
 */

#define DEBUG_1 0

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Bdaq53a.hh"

#include <stdlib.h>
#include <cstdint>
#include <map>
#include <vector>
#include <array>


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
// Eutelescope
#  include "EUTELESCOPE.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
// Eudaq
#  include "EUTelAPIXMCDetector.h"
#  include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

namespace eudaq 
{
    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    static const char* EVENT_TYPE   = "bdaq53a"; // XXX to be changed: rd53a
    static const unsigned int NCOLS = 400;
    static const unsigned int NROWS = 192;
    
    // Allowing to convert non-triggered data (DEBUG)
    static int PSEUDO_TRIGGER_ID = 0;

    // Number of sub-triggers for each trigger data frame 
    // (stored in the Trigger Table)
    static unsigned int N_SUB_TRIGGERS = 32;

    // [XXX- JDC] Initial byte number (just in case there is no trigger, 
    // PROV-DEBUG)
    static unsigned int start_byte = sizeof(uint32_t);

#if USE_LCIO && USE_EUTELESCOPE
    static const int chip_id_offset = 30;
#endif
    
    //XXX TO BE PROMOTED?
    class RD53A_DecodedData
    {
        public:
            RD53A_DecodedData() = delete;
            RD53A_DecodedData(const RawDataEvent::data_t & raw_data) :
                _bcid(),
                _trig_id(),
                _trig_tag(),
                _n_event_headers(0),
                _hits()
            {
                _bcid.reserve(N_SUB_TRIGGERS);
                _trig_id.reserve(N_SUB_TRIGGERS);
                _trig_tag.reserve(N_SUB_TRIGGERS);

                // Assuming proper format DH - HITS 
                for(unsigned int it = start_byte; it < raw_data.size()-1; it+=8*sizeof(uint8_t))
                {
                    // XXX PROVISIONAL, allowing not well formed data, to be 
                    // removed eventually
                    if(PSEUDO_TRIGGER_ID != 0 )
                    {
                        start_byte = 0;
                    }

                    // Build the 32-bit FE word 
                    uint32_t data_word = _reassemble_word(raw_data,it);
                    if( BDAQ53A_IS_HEADER(data_word) )
                    {
                        _bcid.push_back(BDAQ53A_BCID(data_word));
                        _trig_id.push_back(BDAQ53A_TRG_ID(data_word));
                        _trig_tag.push_back(BDAQ53A_TRG_TAG(data_word));
                        ++_n_event_headers; // XXX Not NEEDED
                    }
                    else
                    {
                        _hits[_n_event_headers] = std::vector<std::array<uint32_t,3> >();
                        // [MulticolCol 6b][Row 9b][col side: 1b][ 4x(4bits ToT ]
                        const uint32_t multicol = (data_word >> 26) & 0x3F;
                        const uint32_t row      = (data_word >> 17) & 0x1FF;
                        const uint32_t side     = (data_word >> 16) & 0x1;
                        
                        for(unsigned int pixid=0; pixid < 4; ++pixid)
                        { 
                            uint32_t col = (multicol*8+pixid)+4*side;
                            const uint32_t tot = (data_word >> (pixid*4)) & 0xF;
                            if( (col < NCOLS && row < NROWS) \
                                    && tot != 255 && tot != 15)
                            {
                                _hits[_n_event_headers].push_back({ {col,row,tot} });
                            }
                        }
                    }
                }
            }
            
            // Getters
            const std::vector<uint32_t> & bcid() const { return _bcid; }
            const std::vector<uint32_t> & trig_id() const { return _trig_id; }
            const std::vector<uint32_t> & trig_tag() const { return _trig_tag; }
            unsigned int n_event_headers() const { return _n_event_headers; } 
            const std::vector<std::array<uint32_t,3> > & hits(unsigned int i) const { return _hits.at(i); }
            
        private:
            std::vector<uint32_t> _bcid;
            std::vector<uint32_t> _trig_id;
            std::vector<uint32_t> _trig_tag;
            unsigned int _n_event_headers;

            // XXX reading status and so..
            
            // data-header - { col, row, ToT }
            std::map<unsigned int,std::vector<std::array<uint32_t,3> > > _hits;
            
            // Re-assemble the 32bit data word from the 2-32bits Front end words 
            uint32_t _reassemble_word(const RawDataEvent::data_t & rawdata,unsigned int init) const
            {
                // FIXME -- No check in size of the rawdata!
                //
                // Contruct the FE High word from the first 4bytes [0-3] in little endian, 
                // i.e  [0][1][2][3] --> [3][2][1][0][16b Low word]
                // And add the FE Low word from the second set of 4 bytes [4-7] in little endian
                // i.e [4][5][6][7] ->  [3][2][1][0][7][6][5][4] 
                return ( ((getlittleendian<uint32_t>(&(rawdata[init])) & 0xFFFF) << 16) 
                    | (getlittleendian<uint32_t>(&(rawdata[init+4*sizeof(uint8_t)])) & 0xFFFF));
            }
    };


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
                    else if(rev->NumBlocks()<1)
                    {
                        return (unsigned)-1;
                    }
                    else if(rev->GetBlock(0).size() == 0)
                    {
                        return (unsigned)-1;
                    }
                    unsigned int trigger_number =_get_trigger(rev->GetBlock(0));
                    if( trigger_number == static_cast<unsigned>(-1) )
                    {
                        // [XXX WARNING or ERROR?]
                        EUDAQ_WARN("No trigger word found in the first 32-block.");
                        ++PSEUDO_TRIGGER_ID;
                        return PSEUDO_TRIGGER_ID;
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
            virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) 
            {
std::cout << "HOLAAAAAAAAAA" << std::endl;
                if(ev.IsBORE() || ev.IsEORE())
                {
                    // nothing to do
                    return true;
                }
                
                // Clear memory
                _data_map.clear();
                
                // Number of event headers: must be, at maximum, the number of
                // subtriggers of the Trigger Table (TT)
                unsigned int n_event_headers = 0;
                
                uint32_t event_status = 0;

                // If we get here it must be a data event
                const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
                // Trigger number is the same for all blocks (sensors)
                uint32_t const trigger_number = GetTriggerID(ev);
                if(static_cast<uint32_t>(trigger_number) == -1)
                {
                    // Dummy plane
                    StandardPlane plane(0, EVENT_TYPE,"rd53a");
                    plane.SetSizeZS(NCOLS,NROWS,0,1);
                    sev.AddPlane(plane);
                    return false;
                }
                else if(trigger_number != 0)
                {
                    // Got a trigger word
                    event_status |= E_EXT_TRG;
                }

                //unsigned int start_byte = sizeof(uint32_t);
                // [XXX - DEBUG] Allowing to read data without any trigger word
                if(PSEUDO_TRIGGER_ID != 0 )
                {
                    start_byte = 0;
                }

                // [JDC] each block could corresponds to one sensor attach to 
                //       the card. So far, we are using only 1-sensor, but it 
                //       could be extended at some point
                for(size_t i = 0; i < ev_raw.NumBlocks(); ++i) 
                {
                    // Create a standard plane representing one sensor plane
                    StandardPlane plane(i, EVENT_TYPE,"rd53a");
                    plane.SetSizeZS(NCOLS,NROWS,0,1);

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
                            // hits[i] = {col, row, ToT}
                            plane.PushPixel(hits[0],hits[1],hits[2]);
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
            
            virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent)
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
                _data_map.clear();
                
                // set type of the resulting lcio event
                lcioEvent.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);
                // pointer to collection which will store data
                LCCollectionVec * zsDataCollection = nullptr;
                // it can be already in event or has to be created
                bool zsDataCollectionExists = false;
                try 
                {
                    zsDataCollection = static_cast<LCCollectionVec*>(lcioEvent.getCollection("zsdata_apix"));
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
                
                // [JDC - XXX: Just in case we're able to readout more chips
                for(size_t chip = 0; chip < ev_raw.NumBlocks(); ++chip) 
                {
                    //const std::vector <unsigned char> & buffer=dynamic_cast<const std::vector<unsigned char> &>(ev_raw.GetBlock(chip));
                    const RawDataEvent::data_t & raw_data = ev_raw.GetBlock(chip);
                    if (lcioEvent.getEventNumber() == 0) 
                    {
                        // [JDC - XXX : Provisional, needed a dedicated one?
                        eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector(3);
                        currentDetector->setMode("ZS");
                        setupDescription.push_back( new eutelescope::EUTelSetupDescription(currentDetector)) ;
                    }
                    zsDataEncoder["sensorID"] = ev_raw.GetID(chip)+chip_id_offset;  
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
                            sparseFrame->emplace_back(hits[0],hits[0],hits[2],decoded_data.bcid()[dh]);
                        }
                    }
                    // write TrackerData object that contains info from one sensor to LCIO collection
                    zsDataCollection->push_back(zsFrame.release());
                }
                // add this collection to lcio event
                if( (!zsDataCollectionExists )  && (zsDataCollection->size() != 0) )
                {
                    lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );
                }
                if (lcioEvent.getEventNumber() == 0) 
                {
                    // do this only in the first event
                    LCCollectionVec * apixSetupCollection = nullptr;
                    bool apixSetupExists = false;
                    try 
                    {
                        apixSetupCollection=static_cast<LCCollectionVec*>(lcioEvent.getCollection("apix_setup"));
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
                        lcioEvent.addCollection(apixSetupCollection, "apix_setup");
                    }
                }
                return true;
            }
#endif

        private:

            void _get_bore_parameters(const Event & )
            {
                // XXX Just if need to initialize paremeters from the BORE
                //std::cout << bore << std::endl;
            }


            unsigned int _get_trigger(const RawDataEvent::data_t & rawdata) const 
            {
                // Check if this is a trigger header
                // Trigger header always first 32-bit word
                if(BDAQ53A_IS_TRIGGER(getlittleendian<uint32_t>(&(rawdata[0]))))
                {
                    return BDAQ53A_TRIGGER_NUMBER(getlittleendian<uint32_t>(&(rawdata[0])));
                }
                return (unsigned)-1;
            }

            const RD53A_DecodedData & get_decoded_data(const RawDataEvent::data_t & raw_data,const int & chip)
            {
                // memorize if not there
                if(_data_map.find(chip) == _data_map.end())
                {
                    _data_map.emplace(chip,RD53A_DecodedData(raw_data));
                }
                return _data_map.at(chip);
            }
            
            // The decoded data memory
            std::map<int,RD53A_DecodedData> _data_map;

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
