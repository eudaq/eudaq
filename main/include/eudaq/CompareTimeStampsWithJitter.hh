#ifndef CompareTimeStampsWithJitter_h__
#define CompareTimeStampsWithJitter_h__
#include "eudaq/Utils.hh"
#include <functional>


#define PARAMETER_NOT_SET -1000000000000


namespace eudaq{

  class Event;


  class CompareTimeStampsWithJitter{

    using timeStamp_t = Eudaq_types::timeStamp_t;
  public:


    typedef bool(*isSyncEvent_F)(eudaq::Event const & ev);

    CompareTimeStampsWithJitter(timeStamp_t jitter_denominator, timeStamp_t jitter_offset, timeStamp_t default_delta_timestamps, timeStamp_t DUT_active_time, isSyncEvent_F isSyncEvent);

    CompareTimeStampsWithJitter() = default;



    void set_Jitter_offset(timeStamp_t jitter_offset);

    void set_default_delta_timestamp(timeStamp_t deltaTimestamp);

    void set_DUT_active_time(timeStamp_t DUT_active_time);
    void set_Clock_diff(double clock_diff){ m_Clock_diff = clock_diff; }
    double get_CLock_diff() const { return m_Clock_diff; }
    timeStamp_t get_DUT_begin() const { return m_dut_begin; }
    template <typename T>
    void set_isSyncEventFunction(T isSyncEvent){
      f_isSync_event = isSyncEvent;
    }
    int compareDUT2TLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent)const;
    timeStamp_t calc_Corrected_DUT_TIME(timeStamp_t DUT_time) const;
  private:
    bool isSetup() const;
    void resync_jitter(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const;
    int compareDUT2TLU_normal_Event(eudaq::Event const & ev, const eudaq::Event  & tluEvent)const;

    int compareDUT2TLU_sync_event(eudaq::Event const & ev, const eudaq::Event  & tluEvent)const;

    timeStamp_t calc_Corrected_TLU_TIME_sync_event(timeStamp_t TLU_time) const;
    timeStamp_t calc_Corrected_TLU_TIME_normal_event(timeStamp_t TLU_time) const;
  private:

    mutable  timeStamp_t m_dut_begin = 0, m_tlu_begin = 0, m_last_tlu = 0, m_dut_begin_const = 0, m_event = 1;
    mutable double m_Clock_diff = 0;
    timeStamp_t m_jitter_offset = PARAMETER_NOT_SET, m_default_delta_timestamps = PARAMETER_NOT_SET, m_DUT_active_time = PARAMETER_NOT_SET;
    //    bool m_useSyncEvents = false;
    std::function<bool(eudaq::Event const &)> f_isSync_event;
    mutable bool firstEvent = true;

  };
}
#endif // CompareTimeStampsWithJitter_h__
