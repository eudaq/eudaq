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
	  virtual bool isTLU(const Event&){ return true; }
      virtual bool GetStandardSubEvent(eudaq::StandardEvent & result, const eudaq::Event & source) const {
        result.SetTimestamp(source.GetTimestamp());
		result.SetTag("TLU_trigger",source.GetTag("trigger"));
        return true;
      }
      virtual unsigned GetTriggerID(const eudaq::Event & ev) const {
        return ev.GetEventNumber();
      }
	  virtual int IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const {
		  // dummy comparator. it is just checking if the event numbers are the same.
		  
		  auto DUT_TimeStamp = ev.GetTimestamp() - 10002032121 ;
		  auto TLU_TimeStamp = tluEvent.GetTimestamp() - 10763879393;


		  return hasTimeOVerlaping(DUT_TimeStamp, DUT_TimeStamp + 96000, TLU_TimeStamp, TLU_TimeStamp + 1);
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
