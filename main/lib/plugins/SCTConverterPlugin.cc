#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
static const int dbg = 0; // 0=off, 1=structure, 2=structure+data
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO && USE_EUTELESCOPE
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelSetupDescription.h"
#include "EUTelTrackerDataInterfacerImpl.h"
using eutelescope::EUTELESCOPE;
using eutelescope::EUTelTrackerDataInterfacerImpl;
using eutelescope::EUTelGenericSparsePixel;
#endif


#include "eudaq/PluginManager.hh"

#include "eudaq/SCT_defs.hh"
#include "eudaq/Configuration.hh"

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE_ITS_ABC = "ITS_ABC";
  static const char *LCIO_collection_name = "zsdata_strip";
  static const char *LCIO_collection_name_TTC = "zsdata_strip_TTC";
  static const int PlaneID = 8;
  namespace sct {
    std::string TDC_L0ID() { return "TDC.L0ID"; }
    std::string TLU_TLUID() { return "TLU.TLUID"; }
    std::string TDC_data() { return "TDC.data"; }
    std::string TLU_L0ID() { return "TLU.L0ID"; }
    std::string Timestamp_data() { return "Timestamp.data"; }
    std::string Timestamp_L0ID() { return "Timestamp.L0ID"; }
    std::string Event_L0ID() { return "Event.L0ID"; }
    std::string Event_BCID() { return "Event.BCID"; }
  void ConvertPlaneToLCIOGenericPixel(StandardPlane &plane,
                                      lcio::TrackerDataImpl &zsFrame) {
    // helper object to fill the TrakerDater object
    auto sparseFrame = eutelescope::EUTelTrackerDataInterfacerImpl<
        eutelescope::EUTelGenericSparsePixel>(&zsFrame);

    for (size_t iPixel = 0; iPixel < plane.HitPixels(); ++iPixel) {
      eutelescope::EUTelGenericSparsePixel thisHit1(
          plane.GetX(iPixel), plane.GetY(iPixel), plane.GetPixel(iPixel), 0);
      sparseFrame.addSparsePixel(&thisHit1);
    }
  }
  bool Collection_createIfNotExist(lcio::LCCollectionVec **zsDataCollection,
                                   const lcio::LCEvent &lcioEvent,
                                   const char *name) {

    bool zsDataCollectionExists = false;
    try {
      *zsDataCollection =
          static_cast<lcio::LCCollectionVec *>(lcioEvent.getCollection(name));
      zsDataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      *zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    }

    return zsDataCollectionExists;
  }
  }

  using namespace sct;
#if USE_LCIO && USE_EUTELESCOPE

  void add_data(lcio::LCEvent &result, int ID, double value) {
    if (dbg > 0)
      std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent data "
                << std::endl;
    result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                 eutelescope::kDE);

    LCCollectionVec *zsDataCollection = nullptr;
    auto zsDataCollectionExists = Collection_createIfNotExist(
        &zsDataCollection, result, LCIO_collection_name_TTC);

    // set the proper cell encoder
    auto zsDataEncoder = CellIDEncoder<TrackerDataImpl>(
        eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
    zsDataEncoder["sensorID"] = ID;
    zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

    // prepare a new TrackerData for the ZS data
    auto zsFrame =
        std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl());
    zsDataEncoder.setCellID(zsFrame.get());

    zsFrame->chargeValues().resize(4);
    zsFrame->chargeValues()[0] = value;
    zsFrame->chargeValues()[1] = 1;
    zsFrame->chargeValues()[2] = 1;
    zsFrame->chargeValues()[3] = 0;

    // perfect! Now add the TrackerData to the collection
    zsDataCollection->push_back(zsFrame.release());

    if (!zsDataCollectionExists) {
      if (zsDataCollection->size() != 0)
        result.addCollection(zsDataCollection, LCIO_collection_name_TTC);
      else
        delete zsDataCollection; // clean up if not storing the collection here
    }
  }
  void addTTC_data(lcio::LCEvent &result, const StandardEvent &tmp_evt) {

    add_data(result, 9, tmp_evt.GetTag(TDC_data(), -1));
  }

  void add_Timestamp_data(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, 10, tmp_evt.GetTag(Timestamp_data(), 0ULL));
  }
  void Timestamp_L0ID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, 11, tmp_evt.GetTag(Timestamp_L0ID(), -1));
  }

  void add_TDC_L0ID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, 12, tmp_evt.GetTag(TDC_L0ID(), -1));
  }
  void add_TLU_TLUID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, 13, tmp_evt.GetTag(TLU_TLUID(), -1));
  }

#endif

  // Declare a new class that inherits from DataConverterPlugin
  class SCTConverterPlugin_ITS_ABC : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {}

    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              const eudaq::Event &tluEvent) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tluEvent.GetEventNumber();
      return compareTLU2DUT(tlu_triggerID, triggerID + 1);
    }

    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      if (ev.IsBORE()) {
        return true;
      }

      auto raw = dynamic_cast<const RawDataEvent *>(&ev);
      if (!raw) {
        return false;
      }

      auto block = raw->GetBlock(0);

      std::vector<bool> channels;
      eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);

      StandardPlane plane(PlaneID, EVENT_TYPE_ITS_ABC);
      plane.SetSizeZS(channels.size(), 1, 0);
      unsigned x = 0;
      unsigned y = 0;

      for (size_t i = 0; i < channels.size(); ++i) {
        ++x;
        if (channels[i] == true) {
          plane.PushPixel(x, 1, 1);
        }
      }
      sev.AddPlane(plane);

      return true;
    }
#if USE_LCIO && USE_EUTELESCOPE
    // This is where the conversion to LCIO is done
    void GetLCIORunHeader(lcio::LCRunHeader &header,
                          eudaq::Event const & /*bore*/,
                          eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      runHeader.setDAQHWName(EUTELESCOPE::EUDRB); // should be:
      // runHeader.setDAQHWName(EUTELESCOPE::NI);

      // the information below was used by EUTelescope before the
      // introduction of the BUI. Now all these parameters shouldn't be
      // used anymore but they are left here for backward compatibility.

      runHeader.setEUDRBMode("ZS");
      runHeader.setEUDRBDet("SCT");
      runHeader.setNoOfDetector(m_boards);
      std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1280),
          yMin(m_boards, 0), yMax(m_boards, 4);
      runHeader.setMinX(xMin);
      runHeader.setMaxX(xMax);
      runHeader.setMinY(yMin);
      runHeader.setMaxY(yMax);
    }

    bool GetLCIOSubEvent(lcio::LCEvent &result, const Event &source) const {

      if (source.IsBORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent BORE "
                    << std::endl;
        // shouldn't happen
        return true;
      } else if (source.IsEORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent EORE "
                    << std::endl;
        // nothing to do
        return true;
      }
      // If we get here it must be a data event

      if (dbg > 0)
        std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent data "
                  << std::endl;
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                   eutelescope::kDE);

      LCCollectionVec *zsDataCollection = nullptr;
      auto zsDataCollectionExists = Collection_createIfNotExist(
          &zsDataCollection, result, LCIO_collection_name);

      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);
      auto plane = tmp_evt.GetPlane(0);

      // set the proper cell encoder
      auto zsDataEncoder = CellIDEncoder<TrackerDataImpl>(
          eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
      zsDataEncoder["sensorID"] = 8;
      zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

      // prepare a new TrackerData for the ZS data
      auto zsFrame =
          std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl());
      zsDataEncoder.setCellID(zsFrame.get());

      ConvertPlaneToLCIOGenericPixel(plane, *zsFrame);

      // perfect! Now add the TrackerData to the collection
      zsDataCollection->push_back(zsFrame.release());

      if (!zsDataCollectionExists) {
        if (zsDataCollection->size() != 0)
          result.addCollection(zsDataCollection, LCIO_collection_name);
        else
          delete zsDataCollection; // clean up if not storing the collection
                                   // here
      }

      return true;
    }
#endif

    static SCTConverterPlugin_ITS_ABC m_instance;

  private:
    SCTConverterPlugin_ITS_ABC() : DataConverterPlugin(EVENT_TYPE_ITS_ABC) {}

    unsigned m_boards = 1;

    // The single instance of this converter plugin
  }; // class SCTConverterPlugin

  // Instantiate the converter plugin instance
  SCTConverterPlugin_ITS_ABC SCTConverterPlugin_ITS_ABC::m_instance;

  using TriggerData_t = uint64_t;

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE_ITS_TTC = "ITS_TTC";

  // Declare a new class that inherits from DataConverterPlugin
  class SCTConverterPlugin_ITS_TTC : public DataConverterPlugin {

  public:
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      cnf.SetSection("Producer.ITS_ABC");
      cnf.Print();
      m_threshold = cnf.Get("ST_VDET", -1);
      m_Voltage = cnf.Get("ST_VTHR", -1);
    }
    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              const eudaq::Event &tluEvent) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tluEvent.GetEventNumber();
      return compareTLU2DUT(tlu_triggerID, triggerID + 1);
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {

      if (ev.IsBORE()) {
        return true;
      }

      auto raw = dynamic_cast<const RawDataEvent *>(&ev);
      if (!raw) {
        return false;
      }

      auto block = raw->GetBlock(0);

      ProcessTTC(block, sev);

      return true;
    }
    template <typename T>
    static void ProcessTTC(const std::vector<unsigned char> &block, T &sev) {

      size_t size_of_TTC = block.size() / sizeof(TriggerData_t);
      const TriggerData_t *TTC = nullptr;
      if (size_of_TTC) {
        TTC = reinterpret_cast<const TriggerData_t *>(block.data());
      }
      size_t j = 0;
      // sev.SetTag(std::string("triggerData_") + to_string(j), to_hex(TTC[j],
      // 16));
      // sev.SetTag("triggerData_0", to_hex(TTC[0], 16));
      for (size_t i = 0; i < size_of_TTC; ++i) {

        uint64_t data = TTC[i];
        switch (data >> 60) {
        case 0xd:
          ProcessTLU_data(data, sev);
          break;
        case 0xe:
          ProcessTDC_data(data, sev);
          break;

        case 0xf:
          ProcessTimeStamp_data(data, sev);
          break;
        default:
          EUDAQ_THROW("unknown data type");
          break;
        }
      }
    }

    template <typename T> static void ProcessTLU_data(uint64_t data, T &sev) {
      uint32_t hsioID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TLUID = data & 0xffff;

      sev.SetTag(TLU_TLUID(), TLUID);
      sev.SetTag(TLU_L0ID(), hsioID);
    }
    template <typename T> static void ProcessTDC_data(uint64_t data, T &sev) {

      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TDC = data & 0xfffff;
      sev.SetTag(TDC_data(), TDC);
      sev.SetTag(TDC_L0ID(), L0ID);
    }
    template <typename T>
    static void ProcessTimeStamp_data(uint64_t data, T &sev) {

      uint64_t timestamp = data & 0x000000ffffffffffULL;
      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      sev.SetTag(Timestamp_data(), timestamp);
      sev.SetTag(Timestamp_L0ID(), L0ID);
    }
#if USE_LCIO && USE_EUTELESCOPE

    // This is where the conversion to LCIO is done
    void GetLCIORunHeader(lcio::LCRunHeader &header,
                          eudaq::Event const & /*bore*/,
                          eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      runHeader.setDAQHWName(EUTELESCOPE::EUDRB); // should be:
      // runHeader.setDAQHWName(EUTELESCOPE::NI);

      // the information below was used by EUTelescope before the
      // introduction of the BUI. Now all these parameters shouldn't be
      // used anymore but they are left here for backward compatibility.

      runHeader.setEUDRBMode("ZS");
      runHeader.setEUDRBDet("SCT_TTC");
      runHeader.setNoOfDetector(m_boards);
      std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1280),
          yMin(m_boards, 0), yMax(m_boards, 4);
      runHeader.setMinX(xMin);
      runHeader.setMaxX(xMax);
      runHeader.setMinY(yMin);
      runHeader.setMaxY(yMax);
    }

    bool GetLCIOSubEvent(lcio::LCEvent &result, const Event &source) const {

      if (source.IsBORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent BORE "
                    << std::endl;
        // shouldn't happen
        return true;
      } else if (source.IsEORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent EORE "
                    << std::endl;
        // nothing to do
        return true;
      }
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);

      addTTC_data(result, tmp_evt);

      add_Timestamp_data(result, tmp_evt);
      Timestamp_L0ID(result, tmp_evt);

      add_TDC_L0ID(result, tmp_evt);
      add_TLU_TLUID(result, tmp_evt);

      add_data(result, 14, m_threshold);
      add_data(result, 15, m_Voltage);
      return true;
    }

#endif
    static SCTConverterPlugin_ITS_TTC m_instance;

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    SCTConverterPlugin_ITS_TTC() : DataConverterPlugin(EVENT_TYPE_ITS_TTC) {}
    unsigned m_boards = 1;
    int m_threshold;
    int m_Voltage;
    // Information extracted in Initialize() can be stored here:

    // The single instance of this converter plugin
  }; // class SCTConverterPlugin

  // Instantiate the converter plugin instance
  SCTConverterPlugin_ITS_TTC SCTConverterPlugin_ITS_TTC::m_instance;

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE_SCT = "SCT";

  // Declare a new class that inherits from DataConverterPlugin
  class SCTConverterPlugin : public DataConverterPlugin {

  public:
    virtual void Initialize(const Event &bore, const Configuration &cnf) {}
    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              const eudaq::Event &tluEvent) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tluEvent.GetEventNumber();
      return compareTLU2DUT(tlu_triggerID, triggerID + 1);
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {

      if (ev.IsBORE()) {
        return true;
      }

      auto raw = dynamic_cast<const RawDataEvent *>(&ev);
      if (!raw) {
        return false;
      }

      StandardPlane plane(PlaneID, EVENT_TYPE_ITS_ABC);
      auto size_x = raw->GetBlock(0).size() * 8;

      plane.SetSizeZS(size_x, raw->NumBlocks(), 0);
      for (size_t i = 0; i < raw->NumBlocks(); ++i) {

        auto block = raw->GetBlock(i);

        std::vector<bool> channels;
        eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);

        unsigned x = 0;
        unsigned y = 0;
        for (size_t i = 0; i < channels.size(); ++i) {
          ++x;
          if (channels[i] == true) {
            plane.PushPixel(x, y, 1);
          }
        }
      }
      sev.AddPlane(plane);

      sev.SetTag(TDC_data(), ev.GetTag(TDC_data(), ""));
      sev.SetTag(TDC_L0ID(), ev.GetTag(TDC_L0ID(), ""));
      sev.SetTag(TLU_TLUID(), ev.GetTag(TLU_TLUID(), ""));
      sev.SetTag(TDC_data(), ev.GetTag(TDC_data(), ""));

      sev.SetTag(Timestamp_data(), ev.GetTag(Timestamp_data(), ""));
      sev.SetTag(Timestamp_L0ID(), ev.GetTag(Timestamp_L0ID(), ""));

      return true;
    }
#if USE_LCIO && USE_EUTELESCOPE
    void GetLCIORunHeader(lcio::LCRunHeader &header,
                          eudaq::Event const & /*bore*/,
                          eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      runHeader.setDAQHWName(EUTELESCOPE::EUDRB); // should be:
      // runHeader.setDAQHWName(EUTELESCOPE::NI);

      // the information below was used by EUTelescope before the
      // introduction of the BUI. Now all these parameters shouldn't be
      // used anymore but they are left here for backward compatibility.

      runHeader.setEUDRBMode("ZS");
      runHeader.setEUDRBDet("SCT");
      runHeader.setNoOfDetector(m_boards);
      std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1280),
          yMin(m_boards, 0), yMax(m_boards, 4);
      runHeader.setMinX(xMin);
      runHeader.setMaxX(xMax);
      runHeader.setMinY(yMin);
      runHeader.setMaxY(yMax);
    }

    bool GetLCIOSubEvent(lcio::LCEvent &result, const Event &source) const {

      if (source.IsBORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent BORE "
                    << std::endl;
        // shouldn't happen
        return true;
      } else if (source.IsEORE()) {
        if (dbg > 0)
          std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent EORE "
                    << std::endl;
        // nothing to do
        return true;
      }
      // If we get here it must be a data event

      if (dbg > 0)
        std::cout << "SCTUpgradeConverterPlugin::GetLCIOSubEvent data "
                  << std::endl;
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                   eutelescope::kDE);

      LCCollectionVec *zsDataCollection = nullptr;
      auto zsDataCollectionExists = Collection_createIfNotExist(
          &zsDataCollection, result, LCIO_collection_name);

      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);
      auto plane = tmp_evt.GetPlane(0);

      // set the proper cell encoder
      auto zsDataEncoder = CellIDEncoder<TrackerDataImpl>(
          eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
      zsDataEncoder["sensorID"] = plane.ID();
      zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

      // prepare a new TrackerData for the ZS data
      auto zsFrame =
          std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl());
      zsDataEncoder.setCellID(zsFrame.get());

      ConvertPlaneToLCIOGenericPixel(plane, *zsFrame);

      // perfect! Now add the TrackerData to the collection
      zsDataCollection->push_back(zsFrame.release());

      std::cout << zsDataCollection->size() << std::endl;
      if (!zsDataCollectionExists) {
        if (zsDataCollection->size() != 0)
          result.addCollection(zsDataCollection, LCIO_collection_name);
        else
          delete zsDataCollection; // clean up if not storing the collection
                                   // here
      }

      return true;
    }
#endif

    static SCTConverterPlugin m_instance;

  private:
    unsigned m_boards = 1;
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    SCTConverterPlugin() : DataConverterPlugin(EVENT_TYPE_SCT) {}

    // Information extracted in Initialize() can be stored here:

    // The single instance of this converter plugin
  }; // class SCTConverterPlugin

  // Instantiate the converter plugin instance
  SCTConverterPlugin SCTConverterPlugin::m_instance;


} // namespace eudaq
