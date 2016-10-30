#include "PluginManager.hh"
#include "Exception.hh"
#include "Configuration.hh"

#include <string>
using namespace std;

namespace eudaq {

  PluginManager &PluginManager::GetInstance() {
    static PluginManager manager;
    return manager;
  }

  DataConverterPlugin & PluginManager::GetPlugin(const Event& ev){
    uint32_t id = ev.GetEventID();
    if (m_datacvts.find(id) == m_datacvts.end()){
      m_datacvts[id] = Factory<DataConverterPlugin>::MakeUnique(id);
    }
    return *(m_datacvts[id].get());
  }

  void PluginManager::Initialize(const DetectorEvent & dev) {
    eudaq::Configuration conf(dev.GetTag("CONFIG"));
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const eudaq::Event & subev = *dev.GetEvent(i);
      GetInstance().GetPlugin(subev).Initialize(subev, conf);
    }
  }

  StandardEvent PluginManager::ConvertToStandard(const DetectorEvent & dev) {
    StandardEvent event(dev.GetRunN(), dev.GetEventN(), dev.GetTimestampBegin());
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const Event * ev = dev.GetEvent(i);
      if (!ev) EUDAQ_THROW("Null event!");
      if (ev->GetSubType() == "EUDRB") {
	ConvertStandardSubEvent(event, *ev);
      }
    }
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const Event * ev = dev.GetEvent(i);
      if (!ev) EUDAQ_THROW("Null event!");
      if (ev->GetSubType() != "EUDRB") {
	ConvertStandardSubEvent(event, *ev);
      }
    }
    return event;
  }

  void PluginManager::ConvertStandardSubEvent(StandardEvent & dest, const Event & source) {
    try {
      GetInstance().GetPlugin(source).GetStandardSubEvent(dest, source);
    }
    catch (const Exception & e) {
      std::cerr << "Error during conversion in PluginManager::ConvertStandardSubEvent:\n"
		<< e.what() << std::endl;
    }
  }

  void PluginManager::ConvertLCIOSubEvent(lcio::LCEvent & dest, const Event & source) {
    GetInstance().GetPlugin(source).GetLCIOSubEvent(dest, source);
  }
  
#if USE_LCIO && USE_EUTELESCOPE
#include "lcio.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCRunHeaderImpl.h"

#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;

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
  lcio::LCRunHeader * PluginManager::GetLCRunHeader(const DetectorEvent &) {return 0;}
  lcio::LCEvent * PluginManager::ConvertToLCIO(const DetectorEvent &) {return 0;}
#endif

}
