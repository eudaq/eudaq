#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/baseReadOutFrameEvent.hh"
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.


namespace eudaq {
  typedef eudaq::AidaPacket containerT;
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "slowP";

  // Declare a new class that inherits from DataConverterPlugin
  class aidaSyncTestConverterPlugin : public DataConverterPlugin {

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
		virtual void Initialize(const DetectorEvent & bore,
          const Configuration & cnf) {
        m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32  //some linux Stuff //$$change
		(void)cnf; // just to suppress a warning about unused parameter cnf
#endif
        
      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
		virtual unsigned GetTriggerID(const Event & ev) const {

        return (unsigned)-1;
      }
		virtual int IsSyncWithTLU(eudaq::Event const & ev, eudaq::Event const & tlu) const {
        // dummy comparator. it is just checking if the event numbers are the same.

        //auto triggerID=ev.GetEventNumber();
			auto ROF = dynamic_cast<const eudaq::baseReadOutFrame*>(&ev);
        auto tlu_triggerID = PluginManager::GetTriggerID(tlu);
        int min_ret = 2;
        int max_ret = -2;
        int ret = 5;
        uint64_t triggerID = 0;

        for (size_t i = m_triggerIndexCounter; i < ROF->GetMultiTimeStampSize();++i)
        {

          triggerID = ROF->GetMultiTimestamp(i);
          ret = compareTLU2DUT(tlu_triggerID, triggerID);
          if (ret == Event_IS_Sync)
          {
            m_triggerIndexCounter = i+1;
            return Event_IS_Sync;
          }

          max_ret = max(max_ret, ret);
          min_ret = min(min_ret, ret);
        }

        m_triggerIndexCounter = 0;
        if (max_ret==Event_IS_EARLY && min_ret==Event_IS_EARLY)
        {
          return Event_IS_EARLY;
        }

        return Event_IS_LATE;  // this value is returned either when min or max is late and non of the triggers is sync. it means either that this trigger was missed or that the event is in general late 



      }


		virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::AidaPacket> ev, size_t NumberOfROF){
			if (m_ev != ev)
			{
				m_ev = ev;
				EventIt++;
				m_triggerIndexCounter = 0;
			}

			if (NumberOfROF >= GetNumberOfROF(*ev))
			{
				
				return nullptr;
			}

			auto ret = std::make_shared<eudaq::baseReadOutFrame>(EVENT_TYPE, 2, ev->GetPacketNumber());
			if (!ev->GetData().empty() && ev->GetData().at(0) == 1)
			{
				ret->SetFlags(Event::FLAG_BORE);
			}
			if (!ev->GetMetaData().getArray().empty())
			{
				for (size_t i = 0; i < ev->GetSizeOFMetaData();++i)
				{
					ret->addTimeStamp(ev->GetMetaData().getCounterAt(i));
				}
				

			}
			else
			{
				ret->addTimeStamp(0);
			}

			ret->SetTag("eventID", EventIt);
			return ret;
		}
      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev,
		  const Event & ev) const {

        return true;
      }

#if USE_LCIO
      // This is where the conversion to LCIO is done
    virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }
#endif

    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      aidaSyncTestConverterPlugin()
		  : DataConverterPlugin(AidaPacket::str2type("_ROF"), EVENT_TYPE), m_exampleparam(0)
      {}

      // Information extracted in Initialize() can be stored here:
      unsigned m_exampleparam;
      mutable size_t m_triggerIndexCounter = 0;
	  size_t EventIt = 0;
	  std::shared_ptr<eudaq::AidaPacket> m_ev;
      // The single instance of this converter plugin
      static aidaSyncTestConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  aidaSyncTestConverterPlugin aidaSyncTestConverterPlugin::m_instance;












    // The event type for which this converter plugin will be registered
    // Modify this to match your actual event type (from the Producer)
    static const char* EVENT_TYPE1 = "fastP";

    // Declare a new class that inherits from DataConverterPlugin
    class aidaSyncTestConverterPlugin1 : public DataConverterPlugin {

    public:
		virtual unsigned GetTriggerID(eudaq::Event const &ev) const{
			return ev.GetTimestamp();
		}
      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const DetectorEvent & bore,
        const Configuration & cnf) {
        m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32  //some Linux Stuff //$$change
        (void)cnf; // just to suppress a warning about unused parameter cnf
#endif

      }

	  virtual bool isTLU(const Event&){ return true; }
	  virtual int IsSyncWithTLU(eudaq::Event const & ev, eudaq::Event const & tlu)  const {
		  // dummy comparator. it is just checking if the event numbers are the same.
		  auto tlu_triggerID = PluginManager::GetTriggerID(tlu);
		  //auto triggerID=ev.GetEventNumber();
		  int maxRet = -10;
		  int minRet = 10;
		  baseReadOutFrame rof = *dynamic_cast<const baseReadOutFrame*>(&ev);
		  for (size_t i = 0; i < rof.GetMultiTimeStampSize(); ++i){
			  auto time_stamp = rof.GetMultiTimestamp(i);
			  auto ret = compareTLU2DUT(tlu_triggerID, time_stamp);
			  if (ret == Event_IS_Sync)
			  {
				  return Event_IS_Sync;
			  }
			  maxRet = max(maxRet, ret);
			  minRet = min(minRet, ret);
		  }

		  if (minRet == maxRet)
		  {
			  return minRet;
		  }
		  else
		  {
			  std::cout << "min != max" << std::endl;
		  }

		  return maxRet;
      }



	  virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::AidaPacket> ev , size_t NumberOfROF){
		  if (m_ev != ev)
		  {
			  m_ev = ev;
			  EventIt++;
		  }
		  if (NumberOfROF >= GetNumberOfROF(*ev))
		  {
			  
			  return nullptr;
		  }
		  auto ret = std::make_shared<eudaq::baseReadOutFrame>(EVENT_TYPE1, 2,ev->GetPacketNumber());
		  if (!ev->GetData().empty()&&  ev->GetData().at(0) == 1)
		  {
			  ret->SetFlags(Event::FLAG_BORE);
		  }
		  if (!ev->GetMetaData().getArray().empty())
		  {
			  ret->addTimeStamp(ev->GetMetaData().getCounterAt(0));

		  }
		  else
		  {
			  ret->addTimeStamp(0);
		  }
		  

		  ret->SetTag("eventID", EventIt);
		  return ret;
	  }

      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev,
        const AidaPacket & ev) const {

        return true;
      }

#if USE_LCIO
      // This is where the conversion to LCIO is done
      virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }
#endif

    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      aidaSyncTestConverterPlugin1()
		  : DataConverterPlugin(AidaPacket::str2type("_ROF"), EVENT_TYPE1), m_exampleparam(0)
      {}

      // Information extracted in Initialize() can be stored here:
      unsigned m_exampleparam;
	  size_t EventIt=0;
	  std::shared_ptr<eudaq::AidaPacket> m_ev;
      // The single instance of this converter plugin
      static aidaSyncTestConverterPlugin1 m_instance;
    }; // class ExampleConverterPlugin

    // Instantiate the converter plugin instance
    aidaSyncTestConverterPlugin1 aidaSyncTestConverterPlugin1::m_instance;



    // The event type for which this converter plugin will be registered
    
    static const char* EVENT_TYPE2 = "multi";

    // Declare a new class that inherits from DataConverterPlugin
    class multiFrameDataConverter : public DataConverterPlugin  {

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const eudaq::Event & bore,
        const Configuration & cnf) {
        m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32  //some Linux Stuff //$$change
        (void)cnf; // just to suppress a warning about unused parameter cnf
#endif

      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const eudaq::Event & ev) const {

        return ev.GetEventNumber();
      }
	  virtual int IsSyncWithTLU(eudaq::Event const & ev, eudaq::Event const & tlu) const {
        // dummy comparator. it is just checking if the event numbers are the same.
		  auto tlu_triggerID = PluginManager::GetTriggerID(tlu);
        //auto triggerID=ev.GetEventNumber();
		  int maxRet = -10;
		  int minRet = 10;
		  baseReadOutFrame rof = *dynamic_cast<const baseReadOutFrame*>(&ev);
		  for (size_t i = 0; i < rof.GetMultiTimeStampSize(); ++i){
			  auto time_stamp=rof.GetMultiTimestamp(i);
			  auto ret= compareTLU2DUT(tlu_triggerID, time_stamp);
			  if (ret == Event_IS_Sync)
			  {
				  return Event_IS_Sync;
			  }
			  maxRet = max(maxRet, ret);
			  minRet = min(minRet, ret);
		  }

        if (minRet==maxRet)
        {
			return minRet;
		}
		else
		{
			std::cout << "min != max" << std::endl;
		}
        
		  return maxRet;
      }

	  virtual size_t GetNumberOfROF(const eudaq::AidaPacket& pac){

		  return pac.GetSizeOFMetaData();
	  }
	  virtual std::shared_ptr<eudaq::Event> ExtractEventN(std::shared_ptr<eudaq::AidaPacket> ev, size_t NumberOfROF){
		  
		  if (m_ev!=ev)
		  {
			  m_ev = ev;
			  EventIt++; 
			  
		  }
		  if (NumberOfROF > GetNumberOfROF(*ev))
		  {
			  
			  return nullptr;
		  }
		  auto ret = std::make_shared<eudaq::baseReadOutFrame>(EVENT_TYPE2, 2, ev->GetPacketNumber());
		  if (!ev->GetData().empty() && ev->GetData().at(0) == 1)
		  {
			  ret->SetFlags(Event::FLAG_BORE);
		  }
		  if (ev->GetSizeOFMetaData() >NumberOfROF)
		  {
			  ret->addTimeStamp(ev->GetMetaData().getCounterAt(NumberOfROF));

		  }
		  else
		  {
			  ret->addTimeStamp(0);
		  }

		  ret->SetTag("eventID", EventIt);
		  return ret;
	  }
      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev,
        const AidaPacket & ev) const {

        return true;
      }
   

#if USE_LCIO
      // This is where the conversion to LCIO is done
      virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
        return 0;
      }
#endif

    private:
		std::shared_ptr<eudaq::AidaPacket> m_ev=nullptr;
		size_t EventIt = 0;
      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      multiFrameDataConverter()
		  : DataConverterPlugin(AidaPacket::str2type("_ROF"), EVENT_TYPE2), m_exampleparam(0)
      {}

      // Information extracted in Initialize() can be stored here:
      unsigned m_exampleparam;

      // The single instance of this converter plugin
      static multiFrameDataConverter m_instance;
    }; // class ExampleConverterPlugin

    // Instantiate the converter plugin instance
    multiFrameDataConverter multiFrameDataConverter::m_instance;


} // namespace eudaq
