#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/TLUEvent.hh"
#include <string>

#if USE_LCIO
#  include <IMPL/LCEventImpl.h>
#  include <lcio.h>
#endif

namespace eudaq{

  class TLUConverterPlugin : public DataConverterPlugin {
    public:
      TLUConverterPlugin() : DataConverterPlugin(Event::str2id("_TLU")) {}

      virtual bool GetStandardSubEvent(eudaq::StandardEvent & result, const eudaq::Event & source) const {
        result.SetTimestamp(source.GetTimestamp());
		result.SetTag("TLU_trigger",source.GetTag("trigger"));
        return true;
      }
      virtual unsigned GetTriggerID(const eudaq::Event & ev) const {
        return ev.GetEventNumber();
      }


#if USE_LCIO
      virtual bool GetLCIOSubEvent(lcio::LCEvent & result, const eudaq::Event & source) const {
        dynamic_cast<lcio::LCEventImpl &>(result).setTimeStamp(source.GetTimestamp());
        dynamic_cast<lcio::LCEventImpl &>(result).setEventNumber(source.GetEventNumber());
        dynamic_cast<lcio::LCEventImpl &>(result).setRunNumber(source.GetRunNumber());
        return true;
      }
#endif

    private:
      static TLUConverterPlugin const m_instance;
  };

  TLUConverterPlugin const TLUConverterPlugin::m_instance;

}//namespace eudaq
