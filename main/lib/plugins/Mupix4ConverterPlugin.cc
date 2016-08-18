/**
 * an eudaq converter plugin for the MuPix4 readout
 *
 * @author      Moritz Kiehn <kiehn@physi.uni-heidelberg.de>
 * @date        2013-10-07
 *
 */

#ifndef WIN32
#include <inttypes.h> /* uint32_t */
#endif
#include <memory>
#include <vector>

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Mupix.hh"
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


static const char *     MUPIX4_EVENT_TYPE = "MUPIX4";
static const unsigned   MUPIX4_SENSOR_ID = 24;
static const char *     MUPIX4_SENSOR_TYPE = "MUPIX4";
static const unsigned   MUPIX4_SENSOR_NUM_COLS = 32;
static const unsigned   MUPIX4_SENSOR_NUM_ROWS = 40;
static const unsigned   MUPIX4_SENSOR_BINARY_SIGNAL = 1;
static const char *     MUPIX4_COLLECTION_NAME = "zsdata_mupix4";

using eudaq::mupix::Mupix4DataProxy;

namespace eudaq {

class Mupix4ConverterPlugin : public DataConverterPlugin {
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
    // The constructor can be private since only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    Mupix4ConverterPlugin() : DataConverterPlugin(MUPIX4_EVENT_TYPE) {}

    // static instance as mentioned above
    static Mupix4ConverterPlugin m_instance;
};

/** create the singleton. registers the converter with the plugin-manager */
Mupix4ConverterPlugin Mupix4ConverterPlugin::m_instance;

/** return the trigger id (as provided by the TLU).
 *
 * returns (unsigned)(-1) if it can not be retrieved.
 */
unsigned Mupix4ConverterPlugin::GetTriggerID(const Event & e) const
{
    Mupix4DataProxy data;

    // trigger id is not defined for special events
    if (e.IsBORE() || e.IsEORE()) return (unsigned)(-1);
    if (!(data = Mupix4DataProxy::from_event(e))) return (unsigned)(-1);
    return data.trigger_id();
}

/** convert the data in the RawDataEvent into a StandardEvent
 *
 * should return false on failure but this does not seem to have any effect.
 */
bool Mupix4ConverterPlugin::GetStandardSubEvent(
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

    Mupix4DataProxy data = Mupix4DataProxy::from_event(source);
    if (!data) return true;

    // NOTE
    // during the data taking (oct 2013) were switched so. in Ivans setup the
    // chip was mounted with 90deg rotation and the mapping rows -> x and
    // cols -> y was used to be able to see correlations in the online monitor.
    // this is not needed for the offline analysis.
    StandardPlane plane(MUPIX4_SENSOR_ID, MUPIX4_EVENT_TYPE, MUPIX4_SENSOR_TYPE);
    plane.SetSizeZS(MUPIX4_SENSOR_NUM_COLS, MUPIX4_SENSOR_NUM_ROWS, data.num_hits());
    plane.SetTLUEvent(data.trigger_id());

    // Furthermore we put the timestamp into the signal amplitude
    for (unsigned i = 0; i < data.num_hits(); ++i) {
        plane.SetPixel(i, data.hit_col(i), data.hit_row(i),
                       MUPIX4_SENSOR_BINARY_SIGNAL);
    }
    dest.AddPlane(plane);

    return true;
}

#if USE_LCIO && USE_EUTELESCOPE

bool Mupix4ConverterPlugin::GetLCIOSubEvent(
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

    // beginning-of-run / end-of-run should not be converted
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

    // lcio takes ownership over the different data objects when they are
    // added to the event. secure them w/ auto_ptr so they get deleted
    // automatically when something breaks.
    // NOTE to future self: use unique_ptr since auto_ptr is deprecated
    bool collection_exists = false;
    LCCollectionVec * collection;
    std::auto_ptr<TrackerDataImpl> frame;
    std::auto_ptr<EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel> > pixels;

    // get the lcio output collection
    try {
        collection = static_cast<LCCollectionVec *>(
            dest.getCollection(MUPIX4_COLLECTION_NAME));
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
    cell_encoder["sensorID"] = MUPIX4_SENSOR_ID;
    cell_encoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

    // the lcio object that stores the hit data for a single readout frame
    frame.reset(new TrackerDataImpl);
    cell_encoder.setCellID(frame.get());
    // a convenience object that encodes the sparse pixel data into an
    // eutelescope-specific format and stores it in the given readout frame
    pixels.reset(new EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel>(frame.get()));

    Mupix4DataProxy data = Mupix4DataProxy::from_event(source);
    if (!data) return true;

    // extract hits from the raw buffer into the lcio readout frame
    // WARNING columns and rows are switched. Ivans setup only allows
    // mounting the chip with 90deg rotation and we already rotate here
    // use the analog signal as feedthrough for the per-pixel timestamp (diffs)
    for (unsigned i = 0; i < data.num_hits(); ++i) {
        EUTelGenericSparsePixel pixel(
            data.hit_row(i),
            data.hit_col(i),
            MUPIX4_SENSOR_BINARY_SIGNAL);
        pixels->addSparsePixel(pixel);
    }

    // hand over ownership over the readout frame to the lcio collection
    collection->push_back(frame.release());

    if (!collection_exists && (collection->size() != 0)) {
        dest.addCollection(collection, MUPIX4_COLLECTION_NAME);
    }

    // TODO output detector description for the first event as in all the
    // other converter plugins. not sure why this would be needed.

    return true;
}

#endif /* USE_LCIO && USE_EUTELESCOPE */

} // namespace eudaq
