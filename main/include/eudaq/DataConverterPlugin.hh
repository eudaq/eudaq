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

namespace eudaq {



template <typename T>
inline int compareTLU2DUT(T TLU_Trigger_Number, T DUT_Trigger_number) {
  if (DUT_Trigger_number == TLU_Trigger_Number) {
    return Event_IS_Sync;
  } else if (DUT_Trigger_number > TLU_Trigger_Number) {
    return Event_IS_EARLY;
  }
  return Event_IS_LATE;



}

template <typename T>
inline int hasTimeOVerlaping(T eventBegin, T EventEnd, T TLUStart, T TLUEnd) {



  if (TLUStart <= EventEnd
      &&
      eventBegin <= TLUEnd) {

    /*

            | event start  |event End
            ----------------------------------------->
            t

            | tlu start  | tlu End
            ------------------------------------------>
            t

            */

    return Event_IS_Sync;

  } else if (EventEnd<TLUStart) {
    /*

    | event start  |event End
    ----------------------------------------->
    t

    | tlu start  | tlu End
    ------------------------------------------>
    t

    */
    return Event_IS_LATE;
  } else if (eventBegin > TLUEnd) {

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
  using t_eventid = Event::t_eventid;
  static const t_eventid& getDefault();
  virtual void Initialize(eudaq::Event const &, eudaq::Configuration const &);

  virtual unsigned GetTriggerID(eudaq::Event const &) const;

  virtual timeStamp_t GetTimeStamp(const Event& ev, size_t index) const;
  virtual size_t GetTimeStamp_size(const Event & ev) const;

  virtual int IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event  & tluEvent) const;

  virtual void setCurrentTLUEvent(eudaq::Event & ev, eudaq::TLUEvent const & tlu);
  virtual void GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const;


  /** Returns the LCIO version of the event.
   */
  virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const;

  /** Returns the StandardEvent version of the event.
   */
  virtual bool GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const;

  /** Returns the type of event this plugin can convert to lcio as a pair of Event type id and subtype string.
   */
  virtual t_eventid const & GetEventType() const;


  virtual event_sp ExtractEventN(event_sp ev, size_t EventNr);
  virtual size_t GetNumberOfEvents(const eudaq::Event& pac);

  virtual bool isTLU(const Event&);

  virtual unsigned getUniqueIdentifier(const eudaq::Event  & ev);

  /** The empty destructor. Need to add it to make it virtual.
   */
  virtual ~DataConverterPlugin();

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
