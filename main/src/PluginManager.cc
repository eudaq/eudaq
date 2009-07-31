#include "eudaq/PluginManager.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Configuration.hh"

#if USE_LCIO
#  include "lcio.h"
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/LCRunHeaderImpl.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

//#include <iostream>
#include <string>
using namespace std;

namespace eudaq {

  PluginManager & PluginManager::GetInstance() {
    // the only one static instance of the plugin manager is in the getInstance function
    // like this it is ensured that the instance is created before it is used
    static PluginManager manager;
    return manager;
  }

  void PluginManager::RegisterPlugin(DataConverterPlugin * plugin) {
    m_pluginmap[plugin->GetEventType()] = plugin;
  }

  DataConverterPlugin & PluginManager::GetPlugin(const Event & event) {
    return GetPlugin(std::make_pair(event.get_id(), event.GetSubType()));
  }

  DataConverterPlugin & PluginManager::GetPlugin(PluginManager::t_eventid eventtype) {
    std::map<t_eventid, DataConverterPlugin *>::iterator pluginiter
      = m_pluginmap.find(eventtype);

    if (pluginiter == m_pluginmap.end()) {
      EUDAQ_THROW("PluginManager::GetPlugin(): Unkown event type "+Event::id2str(eventtype.first)+":"+eventtype.second);
    }

    return *pluginiter->second;
  }

  void PluginManager::Initialize(const DetectorEvent & dev) {
    const eudaq::Configuration conf(dev.GetTag("CONFIG"));
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const eudaq::Event & subev = *dev.GetEvent(i);
      GetInstance().GetPlugin(subev).Initialize(subev, conf);
    }
  }

#if USE_LCIO && USE_EUTELESCOPE
  lcio::LCRunHeader * PluginManager::GetLCRunHeader(const DetectorEvent & bore) {
    IMPL::LCRunHeaderImpl * lcHeader = new IMPL::LCRunHeaderImpl;
    lcHeader->setRunNumber(bore.GetRunNumber());
    lcHeader->setDetectorName("EUTelescope");
    eutelescope::EUTelRunHeaderImpl runHeader(lcHeader);
    runHeader.setDateTime();
    runHeader.setDataType(EUTELESCOPE::DAQDATA);
    runHeader.setDAQSWName(EUTELESCOPE::EUDAQ);

    const eudaq::Configuration conf(bore.GetTag("CONFIG"));
    runHeader.setGeoID(conf.Get("GeoID", 0));

    for (size_t i = 0; i < bore.NumEvents(); ++i) {
      const eudaq::Event & subev = *bore.GetEvent(i);
      GetInstance().GetPlugin(subev).GetLCIORunHeader(*lcHeader, subev, conf);
    }
    return lcHeader;
  }
#else
  lcio::LCRunHeader * PluginManager::GetLCRunHeader(const DetectorEvent &) {
    return 0;
  }
#endif

  StandardEvent PluginManager::ConvertToStandard(const DetectorEvent & dev) {
    StandardEvent event(dev.GetRunNumber(), dev.GetEventNumber(), dev.GetTimestamp());
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      if (dev.GetEvent(i)->GetSubType() == "EUDRB") {
        ConvertStandardSubEvent(event, *dev.GetEvent(i));
      }
    }
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      if (dev.GetEvent(i)->GetSubType() != "EUDRB") {
        ConvertStandardSubEvent(event, *dev.GetEvent(i));
      }
    }
    return event;
  }

#if USE_LCIO
  lcio::LCEvent * PluginManager::ConvertToLCIO(const DetectorEvent & dev) {
    lcio::LCEventImpl * event = new lcio::LCEventImpl;
    event->setEventNumber(dev.GetEventNumber());
    event->setRunNumber(dev.GetRunNumber());
    event->setTimeStamp(dev.GetTimestamp());

    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      ConvertLCIOSubEvent(*event, *dev.GetEvent(i));
    }

    return event;
  }
#else
  lcio::LCEvent * PluginManager::ConvertToLCIO(const DetectorEvent &) {
    return 0;
  }
#endif

  void PluginManager::ConvertStandardSubEvent(StandardEvent & dest, const Event & source) {
    GetInstance().GetPlugin(source).GetStandardSubEvent(dest, source);
  }

  void PluginManager::ConvertLCIOSubEvent(lcio::LCEvent & dest, const Event & source) {
    GetInstance().GetPlugin(source).GetLCIOSubEvent(dest, source);
  }

}//namespace eudaq
