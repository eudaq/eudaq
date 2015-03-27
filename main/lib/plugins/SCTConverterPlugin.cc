#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "SCT";

  // Declare a new class that inherits from DataConverterPlugin
  class SCTConverterPlugin : public DataConverterPlugin {

    public:
		virtual bool isTLU(const Event&){ return false; }
      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event & bore,
          const Configuration & cnf) {
        m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32  //some linux Stuff //$$change
		(void)cnf; // just to suppress a warning about unused parameter cnf
#endif
        
      }
      virtual int IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const {
        // dummy comparator. it is just checking if the event numbers are the same.
        const TLUEvent *tlu = dynamic_cast<const eudaq::TLUEvent*>(&tluEvent);
        unsigned triggerID = ev.GetEventNumber();
        auto tlu_triggerID = tlu->GetEventNumber();
        return compareTLU2DUT(tlu_triggerID, triggerID+1);
      }
      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const Event & ev) const {
        static const unsigned TRIGGER_OFFSET = 8;
        // Make sure the event is of class RawDataEvent
        if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
          // This is just an example, modified it to suit your raw data format
          // Make sure we have at least one block of data, and it is large enough
          if (rev->NumBlocks() > 0 &&
              rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
            // Read a little-endian unsigned short from offset TRIGGER_OFFSET
            return getlittleendian<unsigned short> (&rev->GetBlock(0)[TRIGGER_OFFSET]);
          }
        }
        // If we are unable to extract the Trigger ID, signal with (unsigned)-1
        return (unsigned)-1;
      }

      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev,
          const Event & ev) const {
        // If the event type is used for different sensors
        // they can be differentiated here
        std::string sensortype = "SCT";
        // Create a StandardPlane representing one sensor plane
        int id =10;
        StandardPlane plane(id, EVENT_TYPE, sensortype);
        // Set the number of pixels
        int width = 256, height = 1;
        plane.SetSizeZS(width, height,0);
        

        auto raw = dynamic_cast<const eudaq::RawDataEvent*>(&ev);
        auto data = raw->GetBlock(0);
        int x = 0;
        for (auto &e :data)
        {
          if (e==1)
          {
            plane.PushPixel(x, 1, 1);
          }
          ++x;
        }
        // Set the trigger ID
        plane.SetTLUEvent(GetTriggerID(ev));
        // Add the plane to the StandardEvent
        sev.AddPlane(plane);
        // Indicate that data was successfully converted
        return true;
      }

#if USE_LCIO
      // This is where the conversion to LCIO is done
    virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }
#endif

    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      SCTConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0)
      {}

      // Information extracted in Initialize() can be stored here:
      unsigned m_exampleparam;

      // The single instance of this converter plugin
      static SCTConverterPlugin m_instance;
  }; // class SCTConverterPlugin

  // Instantiate the converter plugin instance
  SCTConverterPlugin SCTConverterPlugin::m_instance;

} // namespace eudaq
