#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Configuration.hh"
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
#include <iostream>
#include <string>
#include <queue>

#define EVENTHEADERSIZE 14
#define MODULEHEADERSIZE 3
#define STREAMHEADERSIZE 3
#define STREAMESIZE 160
#define TOTALHEADERSIZE (EVENTHEADERSIZE + MODULEHEADERSIZE + STREAMHEADERSIZE)
#define STARTSTREAM0 TOTALHEADERSIZE
#define ENDSTREAM0 (STARTSTREAM0 + STREAMESIZE)
#define STARTSTREAM1 (TOTALHEADERSIZE + STREAMHEADERSIZE + STREAMESIZE)
#define ENDSTREAM1 (STARTSTREAM1 + STREAMESIZE)
#define TOTALMODULSIZE                                                         \
  (MODULEHEADERSIZE + 2 * STREAMHEADERSIZE + 2 * STREAMESIZE)
#define STREAMSTART(streamNr)                                                  \
  (TOTALHEADERSIZE + streamNr * (STREAMHEADERSIZE + STREAMESIZE))
#define STREAMEND(streamNr) (STREAMSTART(streamNr) + STREAMESIZE)

#define STRIPSPERCHIP 128

#define TLU_chlocks_per_mirco_secound 384066

inline void helper_uchar2bool(const std::vector<unsigned char> &in, int lOffset,
                              int hOffset, std::vector<bool> &out) {

  eudaq::uchar2bool(in.data() + lOffset, in.data() + hOffset, out);

  //   for (auto i = in.begin() + lOffset; i != in.begin() + hOffset; ++i)
  //   {
  //     for (int j = 0; j < 8; ++j){
  //       out.push_back((*i)&(1 << j));
  //     }
  //   }
}

int numberOfEvents_inplane;

namespace eudaq {
  static const int dbg = 0; // 0=off, 1=structure, 2=structure+data

  void puschDataInStandartPlane(const std::vector<unsigned char> &inputVector,
                                int moduleNr, StandardPlane &plane) {
    int y_pos = (moduleNr - 1) * 2 + 1;

    for (int streamNr = 0; streamNr < 2; ++streamNr) {

      std::vector<bool> outputStream0;

      helper_uchar2bool(
          inputVector, STREAMSTART(streamNr) + (moduleNr - 1) * TOTALMODULSIZE,
          STREAMEND(streamNr) + (moduleNr - 1) * TOTALMODULSIZE, outputStream0);

      size_t x_pos = 0;
      for (size_t i = 0; i < outputStream0.size(); ++i) {

        if (outputStream0.at(i)) {
          if (streamNr == 1) {
            x_pos = i / STRIPSPERCHIP;

            plane.PushPixel(2 * x_pos * STRIPSPERCHIP - i + STRIPSPERCHIP,
                            y_pos + streamNr, 1);
          } else {
            plane.PushPixel(i, y_pos + streamNr, 1);
          }
          ++numberOfEvents_inplane;
          break;
        }
      }
    }
  }
  void pushHeaderInStundartplane(const std::vector<unsigned char> &inputVector,
                                 StandardPlane &plane) {

    int trigger_id = eudaq::getlittleendian<int>(&inputVector[0]);
    short scan_type = eudaq::getlittleendian<short>(&inputVector[4]);
    float scan_current =
        static_cast<float>(eudaq::getlittleendian<int>(&inputVector[6])) / 1000;
  }
  int GetTriggerCounter(const eudaq::RawDataEvent &ev) {
    int trigger_id = eudaq::getlittleendian<int>(&(ev.GetBlock(0).at(0)));
    return trigger_id;
  }
  void processImputVector(const std::vector<unsigned char> &inputVector,
                          StandardPlane &plane) {

    int noModules = (inputVector.size() - EVENTHEADERSIZE) / TOTALMODULSIZE;
    int y_pos = 0;

    int width = 1280, height = noModules * 2;
    plane.SetSizeZS(width, height, 0);

    for (size_t k = 1; k <= static_cast<size_t>(noModules); ++k) {

      puschDataInStandartPlane(inputVector, k, plane);
    }
  }
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "SCTupgrade";
  static const char *LCIO_collection_name = "zsdata_strip";

  // Declare a new class that inherits from DataConverterPlugin
  class SCTupgradeConverterPlugin : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      m_exampleparam = bore.GetTag("SCTupgrade", 0);
      auto longdelay = cnf.Get("timeDelay", "0");
      cnf.SetSection("EventStruct");

      auto configFile_long_time = cnf.Get("LongBusyTime", "0");
      //	std::cout<<" longdelay "<< longdelay<<std::endl;
      int32_t longPause_time_from_command_line = 0;

      try {

        longPause_time_from_command_line = std::stol(longdelay);
        longPause_time = std::stol(configFile_long_time);

      } catch (...) {

        std::string errorMsg =
            "error in SCT Upgrade plugin \n unable to convert " +
            to_string(longdelay) + "to uint64_t";
        EUDAQ_THROW(errorMsg);
      }
      if (longPause_time_from_command_line > 0) {
      }
      longPause_time = longPause_time_from_command_line;
// std::cout << "longPause_time: " << longPause_time << std::endl;
#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event &ev) const {
      static const unsigned TRIGGER_OFFSET = 8;
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
        // This is just an example, modified it to suit your raw data format
        // Make sure we have at least one block of data, and it is large enough

        return GetTriggerCounter(*rev);
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              eudaq::TLUEvent const &tlu) const {
      int returnValue = Event_IS_Sync;

      uint64_t tluTime = tlu.GetTimestamp();
      int32_t tluEv = (int32_t)tlu.GetEventNumber();
      const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(ev);
      // int32_t trigger_id=(int32_t)GetTriggerCounter(rawev );
      int32_t trigger_id = ev.GetEventNumber();
      if (oldDUTid > trigger_id) {
        std::cout << " if (oldDUTid>trigger_id)" << std::endl;
      }
      returnValue = compareTLU2DUT(tluEv, trigger_id);

      // 		  if (returnValue==Event_IS_EARLY)
      // 		  {
      // 			  std::cout<<"SCT event is Early Event ID=
      // "<<trigger_id<<std::endl;
      // 		  }else if (returnValue==Event_IS_LATE)
      // 		  {
      // 			  std::cout<<"SCT event is Late Event ID=
      // "<<trigger_id<<std::endl;
      // 		  }

      return returnValue;
    }
    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here

      numberOfEvents_inplane = 0;
      // Create a StandardPlane representing one sensor plane

      // Set the number of pixels
      const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(ev);

      sev.SetTag("DUT_time", rawev.GetTimestamp());

      std::string sensortype = "SCT";
      StandardPlane plane(m_plane_id, EVENT_TYPE, sensortype);

      std::vector<unsigned char> inputVector = rawev.GetBlock(0);

      processImputVector(inputVector, plane);

      // Set the trigger ID

      pushHeaderInStundartplane(inputVector, plane);

      plane.SetTLUEvent(GetTriggerID(ev));

      //  std::cout<<"length of plane " <<numberOfEvents_inplane<<std::endl;
      // Add the plane to the StandardEvent
      sev.AddPlane(plane);
      // Indicate that data was successfully converted
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
      CellIDEncoder<TrackerDataImpl> zsDataEncoder (eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
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

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    SCTupgradeConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0), last_TLU_time(0),
          Last_DUT_Time(0), longPause_time(0) {
      oldDUTid = 0;
    }

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;
    int32_t oldDUTid;
    mutable uint64_t last_TLU_time, Last_DUT_Time;
    int32_t longPause_time;
    unsigned m_boards = 1;

    const int m_plane_id = 8;
    mutable std::vector<std::vector<unsigned char>> m_event_queue;
    // The single instance of this converter plugin
    static SCTupgradeConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  SCTupgradeConverterPlugin SCTupgradeConverterPlugin::m_instance;

} // namespace eudaq
