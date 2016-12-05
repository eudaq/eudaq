#include "DataConverterPlugin.hh"
#include <string>

#if USE_LCIO
#include <IMPL/LCEventImpl.h>
#include <lcio.h>
#endif

namespace eudaq {
  class TLUConverterPlugin;
  
  namespace{
    auto dummy0 = Factory<DataConverterPlugin>::Register<TLUConverterPlugin>(cstr2hash("TLU"));
  }
  
  class TLUConverterPlugin : public DataConverterPlugin {
  public:
    TLUConverterPlugin() : DataConverterPlugin("TLU") {}

    virtual bool GetStandardSubEvent(eudaq::StandardEvent &result,
                                     const eudaq::Event &source) const {
      result.SetTimestampBegin(source.GetTimestampBegin());
      result.SetTimestampEnd(source.GetTimestampEnd());
      result.SetTag("TLU_trigger", source.GetTag("trigger"));
      return true;
    }
    virtual unsigned GetTriggerID(const eudaq::Event &ev) const {
      return ev.GetEventNumber();
    }

#if USE_LCIO
    virtual bool GetLCIOSubEvent(lcio::LCEvent &result,
                                 const eudaq::Event &source) const {
      dynamic_cast<lcio::LCEventImpl &>(result)
          .setTimeStamp(source.GetTimestamp());
      dynamic_cast<lcio::LCEventImpl &>(result)
          .setEventNumber(source.GetEventNumber());
      dynamic_cast<lcio::LCEventImpl &>(result)
          .setRunNumber(source.GetRunNumber());
      return true;
    }
#endif

  };

} // namespace eudaq
