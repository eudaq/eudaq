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
        result.SetTag("TLU_event_nr", source.GetEventNumber());
        return true;
      }
      virtual void Initialize(eudaq::Event const & tlu, eudaq::Configuration const &) {
      
        m_tlu_begin = tlu.GetTimestamp();
      }
      virtual unsigned GetTriggerID(const eudaq::Event & ev) const {
        return ev.GetEventNumber();
      }
	  virtual int IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const {
		  // dummy comparator. it is just checking if the event numbers are the same.
		  
      if (m_tlu_begin==0&&m_dut_begin==0)
      {
        m_dut_begin = ev.GetTimestamp();
        m_tlu_begin = tluEvent.GetTimestamp();
      }
		  auto DUT_TimeStamp = ev.GetTimestamp() - m_dut_begin ;
		  auto TLU_TimeStamp = tluEvent.GetTimestamp() - m_tlu_begin;


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
      
     mutable uint64_t m_tlu_begin=0,m_dut_begin=0;
  };

  TLUConverterPlugin const TLUConverterPlugin::m_instance;

}//namespace eudaq
