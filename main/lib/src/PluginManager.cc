#include "eudaq/PluginManager.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Configuration.hh"

#if USE_LCIO
#include "lcio.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCRunHeaderImpl.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

//#include <iostream>
#include <string>
using namespace std;

namespace eudaq {

  PluginManager &PluginManager::GetInstance() {
    // the only one static instance of the plugin manager is in the getInstance
    // function
    // like this it is ensured that the instance is created before it is used
    static PluginManager manager;
    return manager;
  }

  void PluginManager::RegisterPlugin(DataConverterPlugin *plugin) {
    m_pluginmap[plugin->GetEventType()] = plugin;
  }

  DataConverterPlugin &PluginManager::GetPlugin(const Event &event) {
    return GetPlugin(std::make_pair(event.get_id(), event.GetSubType()));
  }

  DataConverterPlugin &
  PluginManager::GetPlugin(PluginManager::t_eventid eventtype) {
    std::map<t_eventid, DataConverterPlugin *>::iterator pluginiter =
        m_pluginmap.find(eventtype);

    if (pluginiter == m_pluginmap.end()) {
      EUDAQ_THROW("PluginManager::GetPlugin(): Unkown event type " +
                  Event::id2str(eventtype.first) + ":" + eventtype.second);
    }

    return *pluginiter->second;
  }

  void PluginManager::Initialize(const DetectorEvent &dev) {
    eudaq::Configuration conf(dev.GetTag("CONFIG"));
    conf.Set("timeDelay", dev.GetTag("longTimeDelay", "0"));
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const eudaq::Event &subev = *dev.GetEvent(i);
      GetInstance().GetPlugin(subev).Initialize(subev, conf);
    }
  }

  unsigned PluginManager::GetTriggerID(const Event &ev) {
    return GetInstance().GetPlugin(ev).GetTriggerID(ev);
  }

  //   uint64_t PluginManager::GetTimeStamp( const Event& ev)
  //   {
  // 	  return GetInstance().GetPlugin(ev).GetTimeStamp(ev);
  //   }
  //
  //   uint64_t PluginManager::GetTimeDuration( const Event& ev )
  //   {
  // 	  return GetInstance().GetPlugin(ev).GetTimeDuration(ev);
  //   }
  int PluginManager::IsSyncWithTLU(eudaq::Event const &ev,
                                   eudaq::TLUEvent const &tlu) {
    return GetInstance().GetPlugin(ev).IsSyncWithTLU(ev, tlu);
  }

  PluginManager::t_eventid PluginManager::getEventId(eudaq::Event const &ev) {
    return GetInstance().GetPlugin(ev).GetEventType();
  }

#if USE_LCIO && USE_EUTELESCOPE
  lcio::LCRunHeader *PluginManager::GetLCRunHeader(const DetectorEvent &bore) {
    IMPL::LCRunHeaderImpl *lcHeader = new IMPL::LCRunHeaderImpl;
    lcHeader->setRunNumber(bore.GetRunNumber());
    lcHeader->setDetectorName("EUTelescope");
    eutelescope::EUTelRunHeaderImpl runHeader(lcHeader);
    runHeader.setDateTime();
    runHeader.setDataType(EUTELESCOPE::DAQDATA);
    runHeader.setDAQSWName(EUTELESCOPE::EUDAQ);

    const eudaq::Configuration conf(bore.GetTag("CONFIG"));
    //Use the .conf to set the GeoID of the runHeader
    runHeader.setGeoID(conf.Get("GeoID", 0));

    for (size_t i = 0; i < bore.NumEvents(); ++i) {
      const eudaq::Event &subev = *bore.GetEvent(i);
      GetInstance().GetPlugin(subev).GetLCIORunHeader(*lcHeader, subev, conf);
    }
    return lcHeader;
  }
#else
  lcio::LCRunHeader *PluginManager::GetLCRunHeader(const DetectorEvent &) {
    return 0;
  }
#endif

  StandardEvent PluginManager::ConvertToStandard(const DetectorEvent &dev) {
    // StandardEvent event(dev.GetRunNumber(), dev.GetEventNumber(),
    // dev.GetTimestamp());
    StandardEvent event(dev);
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const Event *ev = dev.GetEvent(i);
      if (!ev)
        EUDAQ_THROW("Null event!");
      if (ev->GetSubType() == "EUDRB") {
        ConvertStandardSubEvent(event, *ev);
      }
    }
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const Event *ev = dev.GetEvent(i);
      if (!ev)
        EUDAQ_THROW("Null event!");
      if (ev->GetSubType() != "EUDRB") {
        ConvertStandardSubEvent(event, *ev);
      }
    }
    return event;
  }

#if USE_LCIO
  lcio::LCEvent *PluginManager::ConvertToLCIO(const DetectorEvent &dev) {
    lcio::LCEventImpl *event = new lcio::LCEventImpl;
    event->setEventNumber(dev.GetEventNumber());
    event->setRunNumber(dev.GetRunNumber());
    event->setTimeStamp(dev.GetTimestamp());

    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      ConvertLCIOSubEvent(*event, *dev.GetEvent(i));
    }

    return event;
  }
#else
  lcio::LCEvent *PluginManager::ConvertToLCIO(const DetectorEvent &) {
    return 0;
  }
#endif

  void PluginManager::ConvertStandardSubEvent(StandardEvent &dest,
                                              const Event &source) {
    try {
      GetInstance().GetPlugin(source).GetStandardSubEvent(dest, source);
    } catch (const Exception &e) {
      std::cerr << "Error during conversion in "
                   "PluginManager::ConvertStandardSubEvent:\n" << e.what()
                << std::endl;
    }
  }

  void PluginManager::ConvertLCIOSubEvent(lcio::LCEvent &dest,
                                          const Event &source) {
    GetInstance().GetPlugin(source).GetLCIOSubEvent(dest, source);
  }

  void PluginManager::setCurrentTLUEvent(eudaq::Event &ev,
                                         eudaq::TLUEvent const &tlu) {
    GetInstance().GetPlugin(ev).setCurrentTLUEvent(ev, tlu);
  }

} // namespace eudaq
