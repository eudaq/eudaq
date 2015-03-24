#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>

namespace eudaq {

  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype) :DataConverterPlugin(Event::str2id("_RAW"),subtype)
  {
    //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;

  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
	  : m_eventtype(make_pair(type, subtype)), m_thisCount(m_count)
  {
  //  std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << " unique identifier "<< m_thisCount << " number of instances " <<m_count << std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
	m_count+=10;
  }
  unsigned DataConverterPlugin::m_count = 0;


  int CompareTimeStampsWithJitter::compareDUT2TLU(eudaq::Event const & ev, const eudaq::Event & tluEvent) const
  {
    if (firstEvent)
    {
      if (!isSetup()){
        EUDAQ_THROW("Parameter for the compare algoritm are not setup correctly");
      }

      m_tlu_begin = tluEvent.GetTimestamp();
      m_dut_begin = ev.GetTimestamp() ;
      m_dut_begin_const = m_dut_begin;
      m_last_tlu = 0; //would be tluEvent.GetTimestamp()- m_tlu_begin;
      firstEvent = false;
    }

    if (f_isSync_event)
    {

      if (f_isSync_event(tluEvent))
      {
        return compareDUT2TLU_sync_event(ev, tluEvent);
      }
      else
      {
        return compareDUT2TLU_normal_Event(ev, tluEvent);
      }


    }
    else{

      auto sync = compareDUT2TLU_normal_Event(ev, tluEvent);
      if (sync == Event_IS_Sync)
      {
        resync_jitter(ev, tluEvent);
      }
      return sync;
    }
  }



  void CompareTimeStampsWithJitter::set_Jitter_offset(timeStamp_t jitter_offset)
  {
    m_jitter_offset = jitter_offset;
  }

  void CompareTimeStampsWithJitter::set_default_delta_timestamp(timeStamp_t deltaTimestamp)
  {
    m_default_delta_timestamps = deltaTimestamp;
  }

  void CompareTimeStampsWithJitter::set_DUT_active_time(timeStamp_t DUT_active_time)
  {
    m_DUT_active_time = DUT_active_time;
  }

  bool CompareTimeStampsWithJitter::isSetup() const
  {
    if (m_jitter_offset == PARAMETER_NOT_SET
        ||
        m_default_delta_timestamps == PARAMETER_NOT_SET
        ||
        m_DUT_active_time == PARAMETER_NOT_SET
        ||
        m_Clock_diff==PARAMETER_NOT_SET

        )
    {
      return false;
    }

    return true;
  }

  void CompareTimeStampsWithJitter::resync_jitter(eudaq::Event const & ev, const eudaq::Event & tluEvent) const
  {
    auto TLU_TimeStamp = calc_Corrected_TLU_TIME_sync_event(tluEvent.GetTimestamp());
    m_last_tlu = TLU_TimeStamp;



     int64_t ev_offset = 10;
     if (m_event>ev_offset)
     {
       int64_t eve = m_event -ev_offset;
       auto dummy = ((double) TLU_TimeStamp / (ev.GetTimestamp() - m_dut_begin_const)) - 1;
       if (std::abs(dummy) > 1){
         dummy = 0;
       }
       
       m_Clock_diff = (eve - 1)*m_Clock_diff / eve + dummy / eve;
     }

     ++m_event;

     auto dumm1 = calc_Corrected_DUT_TIME(ev.GetTimestamp());
     m_dut_begin = dumm1 + m_dut_begin - TLU_TimeStamp;
  }

  int CompareTimeStampsWithJitter::compareDUT2TLU_normal_Event(eudaq::Event const & ev, const eudaq::Event & tluEvent) const
  {
    auto DUT_TimeStamp = calc_Corrected_DUT_TIME(ev.GetTimestamp());
    auto TLU_TimeStamp = calc_Corrected_TLU_TIME_normal_event(tluEvent.GetTimestamp());
#ifdef _DEBUG
 //  std::cout << "normal event : DUT_TimeStamp: " << DUT_TimeStamp << "  TLU_TimeStamp:  " << TLU_TimeStamp << "  diff " << (double) DUT_TimeStamp - TLU_TimeStamp  << " overlap " << hasTimeOVerlaping(DUT_TimeStamp, DUT_TimeStamp + m_DUT_active_time, TLU_TimeStamp, TLU_TimeStamp + m_jitter_offset) << std::endl;

#endif // _DEBUG

    return hasTimeOVerlaping(DUT_TimeStamp, DUT_TimeStamp + m_DUT_active_time, TLU_TimeStamp, TLU_TimeStamp + m_jitter_offset);//96000
  }

  int CompareTimeStampsWithJitter::compareDUT2TLU_sync_event(eudaq::Event const & ev, const eudaq::Event & tluEvent) const
  {
    auto DUT_TimeStamp = calc_Corrected_DUT_TIME(ev.GetTimestamp()) ;
    auto TLU_TimeStamp = calc_Corrected_TLU_TIME_sync_event(tluEvent.GetTimestamp() );



   
#ifdef _DEBUG
  //  std::cout << "sync   event : DUT_TimeStamp: " << DUT_TimeStamp << "  TLU_TimeStamp:  " << TLU_TimeStamp << "  diff " << (double) DUT_TimeStamp - TLU_TimeStamp << " overlap " << hasTimeOVerlaping(DUT_TimeStamp, DUT_TimeStamp + m_jitter_offset, TLU_TimeStamp, TLU_TimeStamp + m_jitter_offset) << std::endl;

#endif // _DEBUG
    auto sync = hasTimeOVerlaping(DUT_TimeStamp, DUT_TimeStamp + m_jitter_offset, TLU_TimeStamp, TLU_TimeStamp + m_jitter_offset);

    if (sync == Event_IS_Sync)
    {
      resync_jitter(ev, tluEvent);
    }
    return sync;
  }

  CompareTimeStampsWithJitter::CompareTimeStampsWithJitter(timeStamp_t jitter_denominator, timeStamp_t jitter_offset, timeStamp_t default_delta_timestamps, timeStamp_t DUT_active_time, isSyncEvent_F isSyncEvent) :
    m_jitter_offset(jitter_offset),
    m_default_delta_timestamps(default_delta_timestamps),
    m_DUT_active_time(DUT_active_time),
    f_isSync_event(isSyncEvent)
  {

  }

  CompareTimeStampsWithJitter::timeStamp_t CompareTimeStampsWithJitter::calc_Corrected_DUT_TIME(timeStamp_t DUT_time) const
  {
    return DUT_time - m_dut_begin + m_Clock_diff*(DUT_time - m_dut_begin);

  }

  CompareTimeStampsWithJitter::timeStamp_t CompareTimeStampsWithJitter::calc_Corrected_TLU_TIME_sync_event(timeStamp_t TLU_time) const
  {
    return TLU_time - m_tlu_begin - m_default_delta_timestamps;
  }

  CompareTimeStampsWithJitter::timeStamp_t CompareTimeStampsWithJitter::calc_Corrected_TLU_TIME_normal_event(timeStamp_t TLU_time) const
  {
    return TLU_time - m_tlu_begin ;
  }

}//namespace eudaq
