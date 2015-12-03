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
#include "EUTELESCOPE.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#endif
#include "eudaq/Exception.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"

namespace eudaq {

// The event type for which this converter plugin will be registered
// Modify this to match your actual event type (from the Producer)
static const char* EVENT_TYPE = "Default";


// Declare a new class that inherits from DataConverterPlugin
class DefaultConverterPlugin : public DataConverterPlugin {

public:
  virtual void Initialize(eudaq::Event const &ev, eudaq::Configuration const &) {
    auto id = getID(ev);
  }
  // supported default Functions
  virtual unsigned getUniqueIdentifier(const eudaq::Event  & ev) {
    return m_thisCount + getID(ev);
  }


  // unsupported functions 
  virtual bool GetStandardSubEvent(StandardEvent & sev,
                                   const Event & ev) const {

    EUDAQ_THROW("default data convert cannot convert data to standart events please provide a data convertert for event: " + Event::id2str(ev.get_id()) + "." + ev.GetSubType());
    return false;
  }

  virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::Event> ev, size_t) override {
    EUDAQ_THROW("default data convert cannot extract events please provide a data convertert for event: " + Event::id2str(ev->get_id()) + "." + ev->GetSubType());
    return nullptr;
  }

  virtual size_t GetNumberOfEvents(const eudaq::Event& ev) override {
    EUDAQ_THROW("default data convert cannot extract events please provide a data convertert for event: " + Event::id2str(ev.get_id()) + "." + ev.GetSubType());
    return 0;
  }




private:


  DefaultConverterPlugin()
    : DataConverterPlugin(getDefault().first, getDefault().second) {
  }


  using id_t = unsigned;

  id_t getID(const Event & ev) {

    auto event_type = PluginManager::getEventId(ev);

    auto pluginiter = m_default_events.find(event_type);

    if (pluginiter == m_default_events.end()) {
      EUDAQ_WARN("registering new unknown event type: " + Event::id2str(ev.get_id()) + "." + ev.GetSubType());
      m_default_events[event_type] = ++m_registeredEventTypes;
    }

    return m_default_events[event_type];
  }

  std::map<t_eventid, id_t> m_default_events;
  mutable id_t m_registeredEventTypes = 0;
  // The single instance of this converter plugin
  static DefaultConverterPlugin m_instance;
}; // class ExampleConverterPlugin

// Instantiate the converter plugin instance
DefaultConverterPlugin DefaultConverterPlugin::m_instance;

} // namespace eudaq
