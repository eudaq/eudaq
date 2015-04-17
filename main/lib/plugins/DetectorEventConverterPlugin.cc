#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Event.hh"
#include "eudaq/PluginManager.hh"
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
  //static const char* EVENT_TYPE = "Example";

  // Declare a new class that inherits from DataConverterPlugin
  class DetectorEventConverterPlugin : public DataConverterPlugin {

  public:
    virtual bool isTLU(const Event&){ return false; }
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event & bore,
                            const Configuration & cnf) {
      
#ifndef WIN32  //some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif

    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event & ev) const {
      return ev.GetEventNumber();
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent & sev,
                                     const Event & ev) const {
      EUDAQ_THROW("a detector Event can't be converted to standart event");
    }

#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
      return 0;
    }
#endif

    virtual size_t GetNumberOfROF(const eudaq::Event& pac){ 
      size_t sum = 0;
     auto det = dynamic_cast<const DetectorEvent*>(&pac);
     for (size_t i = 0; i < det->NumEvents();++i)
     {
       sum += PluginManager::GetNumberOfROF(*(det->GetEvent(i)));
     }


      return sum; 
    }

    virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::Event> pac, 
                                                        size_t NumberOfROF) {
      
      size_t Sum = 0;
      
      
      auto det = std::dynamic_pointer_cast<DetectorEvent>(pac);
      for (size_t i = 0; i < det->NumEvents(); ++i)
      {
        auto ev = det->GetEventPtr(i);

        if (ev->IsPacket())
        {

          auto elements_in_current_Event= PluginManager::GetNumberOfROF(*(det->GetEvent(i)));
          if (Sum + elements_in_current_Event > NumberOfROF)
          {
            return PluginManager::ExtractEventN(det->GetEventPtr(i), NumberOfROF - Sum);
          }
          Sum += elements_in_current_Event;

        }
        else
        {

          auto elements_in_current_Event = 1;
          if (Sum + elements_in_current_Event > NumberOfROF)
          {
            return det->GetEventPtr(i);
          }
          Sum += elements_in_current_Event;
        }

      }

      return nullptr;
    }

  private:

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    DetectorEventConverterPlugin()
      : DataConverterPlugin(Event::str2id(DetectorEventMaintype), "")
    {}


    // The single instance of this converter plugin
    static DetectorEventConverterPlugin m_instance;
  }; // class DetectorEventConverterPlugin

  // Instantiate the converter plugin instance
  DetectorEventConverterPlugin DetectorEventConverterPlugin::m_instance;

} // namespace eudaq
