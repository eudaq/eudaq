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
        
        
        double defaultClock_speed_In_ns = 2.603488;
          TLUClockSpeed = static_cast<uint64_t>(tlu.GetTag("TLUClockSpeed", defaultClock_speed_In_ns) *1e6);
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

    DataConverterPlugin::timeStamp_t GetTimeStamp(const Event& ev, size_t index) const
    {
      switch (index)
      {
      case 0:
        return ev.GetTimestamp(0);
      	break;

      case 1:
        return (ev.GetTimestamp(0)*TLUClockSpeed) / 1e6;
        break;
      }
      return ev.GetTimestamp(index);
    }

    size_t GetTimeStamp_size(const Event & ev) const
    {
      return 2;
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
     uint64_t TLUClockSpeed;
  };

  TLUConverterPlugin const TLUConverterPlugin::m_instance;

}//namespace eudaq
