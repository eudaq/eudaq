#ifndef EUDAQ_INCLUDED_DataConverterPlugin
#define EUDAQ_INCLUDED_DataConverterPlugin

#include <memory>
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"



#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
using namespace IMPL;
using namespace UTIL;
#endif
#include "TLUEvent.hh"
#include <functional>

#define NOTIMESTAMPSET (uint64_t)-1
#define NOTIMEDURATIONSET 0

#define PARAMETER_NOT_SET -1000000000000

//////////////////////////////////////////////////////////////////////////
// Compare Time stamps

// DUT_Trigger_number > TLU_Trigger_Number
#define Event_IS_EARLY -1              

// TLU_Trigger_Number > DUT_Trigger_number
#define Event_IS_LATE 1

// DUT_Trigger_number == TLU_Trigger_Number
#define Event_IS_Sync 0





namespace EVENT { class LCEvent; class LCRunHeader; }
namespace lcio { using namespace EVENT; }

#include <string>
#include <algorithm>

namespace eudaq{
  namespace Eudaq_types{
    using timeStamp_t = int64_t;

  }

	  inline int compareTLU2DUT(unsigned TLU_Trigger_Number, unsigned DUT_Trigger_number){
	  if (DUT_Trigger_number==TLU_Trigger_Number)
	  {
		  return Event_IS_Sync;	
	  }else if (DUT_Trigger_number>TLU_Trigger_Number)
	  {
		  return Event_IS_EARLY;
	  }
	  return Event_IS_LATE;



  }
  template <typename T>
  inline int compareTLU2DUT(T TLU_Trigger_Number, T DUT_Trigger_number){
	  if (DUT_Trigger_number==TLU_Trigger_Number)
	  {
		  return Event_IS_Sync;	
	  }else if (DUT_Trigger_number>TLU_Trigger_Number)
	  {
		  return Event_IS_EARLY;
	  }
	  return Event_IS_LATE;



  }

  template <typename T>
  inline int hasTimeOVerlaping(T eventBegin, T EventEnd, T TLUStart,T TLUEnd){



	  if (TLUStart <= EventEnd
					&&
		  eventBegin <= TLUEnd){

		  /*

						  | event start  |event End
		  ----------------------------------------->
		  t

				  | tlu start  | tlu End
		  ------------------------------------------>
		  t

		  */

		  return Event_IS_Sync;

	  }
	  else if (EventEnd<TLUStart){
		  /*

		  | event start  |event End
		  ----------------------------------------->
												  t

								  | tlu start  | tlu End
		  ------------------------------------------>
												  t

		  */
		  return Event_IS_LATE;
	  }else if (eventBegin>TLUEnd)
	  {

		  /*

								| event start  |event End
		  ----------------------------------------->
												t

		  | tlu start  | tlu End
		  ------------------------------------------>
												t

		  */
		  return Event_IS_EARLY;

	  }
		  
	  EUDAQ_THROW("unforseen case this shold not happen");

	  return -666;
  }

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
    f_isSync_event=isSyncEvent;
    }
    int compareDUT2TLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent)const;
    timeStamp_t calc_Corrected_DUT_TIME(timeStamp_t DUT_time) const;
  private:
    bool isSetup() const;
    void resync_jitter(eudaq::Event const & ev,const eudaq::Event  & tluEvent) const;
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

  class Configuration;

  /**
   *  The DataConverterPlugin is the abstract base for all plugins. The
   *  actual impementation provides the GetLCIOEvent() and GetStandardEvent()
   *  functions which
   *  get the eudaq::Event as input parameter.
   *
   *  The implementations should be singleton classes which only can be
   *  accessed via the plugin manager. (See TimepixConverterPlugin as example).
   *  The plugin implementations have to register with the plugin manager.
   */

  class DataConverterPlugin {
    public:
      using timeStamp_t = Eudaq_types::timeStamp_t;
      typedef Event::t_eventid t_eventid;

      virtual void Initialize(eudaq::Event const &, eudaq::Configuration const &) {}

      virtual unsigned GetTriggerID(eudaq::Event const &) const;

      virtual timeStamp_t GetTimeStamp(const Event& ev, size_t index) const{
        return ev.GetTimestamp(index);
      }
      virtual size_t GetTimeStamp_size(const Event & ev) const{
        return ev.GetSizeOfTimeStamps();
      }

	  virtual int IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const {
		  // dummy comparator. it is just checking if the event numbers are the same.
		  const TLUEvent *tlu = dynamic_cast<const eudaq::TLUEvent*>(&tluEvent);
		  unsigned triggerID = ev.GetEventNumber();
	  auto tlu_triggerID=tlu->GetEventNumber();
	return compareTLU2DUT(tlu_triggerID,triggerID);
	  }

	  virtual void setCurrentTLUEvent(eudaq::Event & ev,eudaq::TLUEvent const & tlu){
		  ev.SetTag("tlu_trigger_id",tlu.GetEventNumber());
	  }
	  virtual void GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const {}
	  

      /** Returns the LCIO version of the event.
       */
      virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const { return false; }

      /** Returns the StandardEvent version of the event.
       */
      virtual bool GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const { return false; };

      /** Returns the type of event this plugin can convert to lcio as a pair of Event type id and subtype string.
       */
      virtual t_eventid const & GetEventType() const { return m_eventtype; }


    virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::Event> ev, size_t NumberOfROF) {
      return nullptr;
    }

	  virtual bool isTLU(const Event&){ return false; }

	  virtual unsigned getUniqueIdentifier(const eudaq::Event  & ev){ return m_thisCount; }

    virtual size_t GetNumberOfROF(const eudaq::Event& pac){ return 1; }
      /** The empty destructor. Need to add it to make it virtual.
       */
      virtual ~DataConverterPlugin() {}

    protected:
      /** The string storing the event type this plugin can convert to lcio.
       *  This string has to be set in the constructor of the actual implementations
       *  of the plugin.
       */
      t_eventid m_eventtype;
	  

      /** The protected constructor which automatically registers the plugin
       *  at the pluginManager.
       */
      DataConverterPlugin(std::string subtype);
      DataConverterPlugin(unsigned type, std::string subtype = "");
	  static unsigned m_count;
	  unsigned m_thisCount;
    private:
      /** The private copy constructor and assignment operator. They are not used anywhere, so there is not
       *  even an implementation. Even if the childs default copy constructor is public
       *  the code will not compile if it is called, since it cannot acces this cc, which the
       *  the default cc does.
       */
		DataConverterPlugin(DataConverterPlugin &) = delete;
		DataConverterPlugin & operator = (const DataConverterPlugin &) = delete;
  };





}//namespace eudaq

#endif // EUDAQ_INCLUDED_DataConverterPlugin
