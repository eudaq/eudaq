/**
 * an eudaq converter plugin for the MuPix3 readout
 *
 * this progam is based on the ExampleConverterPlugin provided by the eudaq
 * software.
 *
 * @author      Moritz Kiehn <kiehn@physi.uni-heidelberg.de>
 * @author      Niklaus Berger <nberger@physi.uni-heidelberg.de>
 * @date        2013
 *
 */

#ifndef WIN32
#include <inttypes.h> /* uint32_t */
#endif
#include <memory>

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelTakiDetector.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#endif


// the same identifier as used by associated producer.
// should be uppercase otherwise the data collector throws some exception.
static const char *   MUPIX3_EVENT_TYPE = "MUPIX3";
// use a sensor id that is far away from the telescope layers 0-5
// needs to fit into 5 bit to allow cellid encoding for lcio
static const unsigned MUPIX3_SENSOR_ID = 23;
static const char *   MUPIX3_SENSOR_TYPE = "MUPIX3";
// arbitrary value assigned to the signal value for the binary hits
static const unsigned MUPIX3_FAKE_SIGNAL = 1;
// the name of the lcio output collection for the conversion
static const char *   MUPIX3_COLLECTION_NAME = "zsdata_mupix3";


namespace eudaq {

class Mupix3ConverterPlugin : public DataConverterPlugin {
public:
    
    virtual void Initialize(const eudaq::Event &, eudaq::Configuration const &) {};
    virtual unsigned GetTriggerID(const eudaq::Event &) const;
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;
    
#if USE_LCIO && USE_EUTELESCOPE
    // run header conversion is not implemented since its seems to be not
    // really used anymore by the EUTelescope framework
    virtual bool GetLCIOSubEvent(lcio::LCEvent &, const eudaq::Event &) const;
#endif
    
private:
    
    const unsigned char * GetRawDataChecked(const Event &, unsigned & n_hits) const;
    
    unsigned ExtractTriggerId(const unsigned char *) const;
    void ExtractHit(const unsigned char *, unsigned, unsigned &, unsigned &) const;
    
    // The constructor can be private since only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    Mupix3ConverterPlugin() : DataConverterPlugin(MUPIX3_EVENT_TYPE) {}
    
    // static instance as mentioned above
    static Mupix3ConverterPlugin m_instance;
};

// create the single instance of the converter plugin. this automatically
// registers this converter with the plugin-manager
Mupix3ConverterPlugin Mupix3ConverterPlugin::m_instance;

// return the trigger id (as provided by the TLU).
// should return (unsigned)(-1) if it can not be retrieved.
unsigned Mupix3ConverterPlugin::GetTriggerID(const Event & ev) const
{
    // trigger id is not defined for special events
    if (ev.IsBORE() || ev.IsEORE()) return (unsigned)(-1);

    unsigned unused = 0;
    const unsigned char * data = GetRawDataChecked(ev, unused);
    if (data == NULL) {
        EUDAQ_INFO("cannot extract trigger_id from event "
                   + eudaq::to_string(ev.GetEventNumber()));
        return (unsigned)(-1);
    }

    return ExtractTriggerId(data);
}

// convert the data in the RawDataEvent into a StandardEvent.
// returns true if the conversion was successful; false if not.
bool Mupix3ConverterPlugin::GetStandardSubEvent(
    StandardEvent & dest,
    const Event & source) const
{
    // beginning-of-run / end-of-run should not be converted
    if (source.IsBORE()) {
        // this should never happen. BORE event should be handled
        // specially by the initialize function
        EUDAQ_ERROR("got BORE during conversion");
        return true;
    } else if (source.IsEORE()) {
        // i'm not sure if this can happen
        EUDAQ_WARN("got EORE during conversion");
        return true;
    }
    
    unsigned n_hits = 0;
    const unsigned char * data = GetRawDataChecked(source, n_hits);
    
    // avoid breaking the data collector by adding an empty plane for
    // empty data
    if (data == NULL) {
        EUDAQ_WARN("received event "
                   + eudaq::to_string(source)
                   + " with no data blocks.");
        
        StandardPlane plane(MUPIX3_SENSOR_ID, MUPIX3_EVENT_TYPE, MUPIX3_SENSOR_TYPE);
        plane.SetSizeZS(32, 40, 0);
        plane.SetTLUEvent((unsigned)(-1));
        
        dest.AddPlane(plane);
        
        return false;
        
    } else {
        
        StandardPlane plane(MUPIX3_SENSOR_ID, MUPIX3_EVENT_TYPE, MUPIX3_SENSOR_TYPE);
        plane.SetSizeZS(32, 40, n_hits);
        plane.SetTLUEvent(ExtractTriggerId(data));
        
        unsigned col, row;
        for(unsigned i = 0; i < n_hits; ++i) {
            ExtractHit(data, i, col, row);
            plane.SetPixel(i, col, row, MUPIX3_FAKE_SIGNAL);
        }
        
        dest.AddPlane(plane);
        
        return true;
    }
}

#if USE_LCIO && USE_EUTELESCOPE

bool Mupix3ConverterPlugin::GetLCIOSubEvent(
    lcio::LCEvent & dest,
    const eudaq::Event & source) const
{
    using lcio::CellIDEncoder;
    using lcio::LCEvent;
    using lcio::LCCollectionVec;
    using lcio::TrackerDataImpl;
    using eutelescope::EUTELESCOPE;
    using eutelescope::EUTelTrackerDataInterfacerImpl;
    using eutelescope::EUTelGenericSparsePixel;
    
    // raw input data stream
    unsigned n_hits;
    const unsigned char * raw_buffer;
    // lcio takes ownership over the different data objects when they are
    // added to the event. secure them w/ auto_ptr so they get deleted
    // automatically when something breaks.
    // NOTE to future self: use unique_ptr since auto_ptr are deprecated
    bool collection_exists = false;
    LCCollectionVec * collection;
    std::auto_ptr<TrackerDataImpl> frame;
    std::auto_ptr<EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel> > pixels;
    
    if (source.IsBORE()) {
        // this should never happen. BORE event should be handled
        // specially by the initialize function
        EUDAQ_ERROR("got BORE during lcio conversion");
        return true;
    } else if (source.IsEORE()) {
        // i'm not sure if this can happen
        EUDAQ_WARN("got EORE during lcio conversion");
        return true;
    }
    
    // get the raw input data
    raw_buffer = GetRawDataChecked(source, n_hits);
    if (raw_buffer == NULL) {
        std::cerr << "could not convert " << source << std::endl;
        return false;
    }
    
    // get the lcio output collection
    try {
        collection = static_cast<LCCollectionVec *>(
            dest.getCollection(MUPIX3_COLLECTION_NAME));
        collection_exists = true;
    } catch(lcio::DataNotAvailableException & e) {
        collection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
        collection_exists = false;
    }
    
    // a cell id identifies from which detector component a specific data
    // collection originates.
    // here it is used to encode the telescope / dut plane and the type of
    // data, e.g. zero-suppressed or raw, that is stored in the collection
    CellIDEncoder<TrackerDataImpl>
        cell_encoder(EUTELESCOPE::ZSDATADEFAULTENCODING, collection);
    cell_encoder["sensorID"] = MUPIX3_SENSOR_ID;
    cell_encoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;
    
    // the lcio object that stores the hit data for a single readout frame
    frame.reset(new TrackerDataImpl);
    cell_encoder.setCellID(frame.get());
    // a convenience object that encodes the sparse pixel data into an
    // eutelescope-specific format and stores it in the given readout frame
    pixels.reset(new EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel>(frame.get()));
    
    // extract hits from the raw buffer into the lcio readout frame
    unsigned col;
    unsigned row;
    for (unsigned i = 0; i < n_hits; ++i) {
        ExtractHit(raw_buffer, i, col, row);
        EUTelGenericSparsePixel pixel(col, row, MUPIX3_FAKE_SIGNAL);
        pixels->addSparsePixel(pixel);
    }
    
    // hand over ownership over the readout frame to the lcio collection
    collection->push_back(frame.release());
    
    if (!collection_exists && (collection->size() != 0)) {
        dest.addCollection(collection, MUPIX3_COLLECTION_NAME);
    }
    
    // TODO output detector description for the first event as in all the
    // other converter plugins. not sure why this would be needed.
    
    return true;
}

#endif /* USE_LCIO && USE_EUTELESCOPE */

// get the raw event data and the maximum number of hits for the given event
// and check for consistency.
//
// returns null on error
const unsigned char * Mupix3ConverterPlugin::GetRawDataChecked(
    const Event & ev,
    unsigned & n_hits) const
{
    const RawDataEvent & raw = dynamic_cast<const RawDataEvent &>(ev);

    if (raw.NumBlocks() != 1) {
        EUDAQ_WARN("event "
                   + to_string(raw.GetEventNumber())
                   + ": invalid number of data blocks "
                   + to_string(raw.NumBlocks()));
        return NULL;
    }

    if (raw.GetBlock(0).size() < 16) {
        EUDAQ_WARN("event "
                   + to_string(raw.GetEventNumber())
                   + ": data block is too small");
        return NULL;
    }

    // TODO check for consistency, i.e. not 32bit aligned. should never happen
    n_hits = (raw.GetBlock(0).size() - 16) / 4;
    return raw.GetBlock(0).data();
}

// extract the trigger id from the raw data
unsigned Mupix3ConverterPlugin::ExtractTriggerId(const unsigned char * data) const
{
    return 0xFFFF & eudaq::getlittleendian<uint32_t>(data + 8);
}

// extract a single hit from the raw data
void Mupix3ConverterPlugin::ExtractHit(
    const unsigned char * data,
    unsigned index,
    unsigned & col,
    unsigned & row) const
{
    // 4 bytes for the header
    // 4 bytes for the event number
    // 4 bytes for the trigger number
    unsigned hit_word = eudaq::getlittleendian<uint32_t>(data + 12 + 4 * index);
    unsigned row_old = (hit_word & 0x3F00) >> 8;
    unsigned col_old = (hit_word & 0xFC000) >> 14;
    
    if (row_old < 20) {
        row = 2 * row_old;
        if (col_old % 2) {
            row++;
        }
        col = col_old / 2;
    } else {
        col = 64;
        row = 40;
    }
}

} // namespace eudaq
