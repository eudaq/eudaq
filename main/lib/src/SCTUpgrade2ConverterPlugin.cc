#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Configuration.hh"
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif
#include <iostream>
#include <string>
#include <queue>


#define STRIPSPERCHIP 128

#define TLU_chlocks_per_mirco_secound 384066



void uchar2bool(const std::vector<unsigned char>& in, std::vector<bool>& out){
	for (auto i=in.begin();i!=in.end();++i)
	{
		for(int j=0;j<8;++j){
			out.push_back((*i)&(1<<j));
		}
	}


}



namespace eudaq {
  void pushData2StandartPlane(const std::vector<unsigned char>& inputVector,const int y,StandardPlane& plane){
    int streamNr=y%2;
    
    std::vector<bool> outputStream0;

    uchar2bool(inputVector,	outputStream0);


    size_t x_pos=0;
    for (size_t i=0; i<outputStream0.size();++i)
    {



      if (outputStream0.at(i))
      {
        if (streamNr==1)
        {
          x_pos=i/STRIPSPERCHIP;

          plane.PushPixel(2*x_pos*STRIPSPERCHIP-i+STRIPSPERCHIP,y+streamNr,1);
        }else{
          plane.PushPixel(i,y+streamNr,1);
        }
       
        break;
      }


    }


  }






  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "SCTupgrade2";

  // Declare a new class that inherits from DataConverterPlugin
  class SCTupgrade2ConverterPlugin : public DataConverterPlugin {

	 

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const Event & bore,
          const Configuration & cnf) {
        m_exampleparam = bore.GetTag("SCTupgrade", 0);
		auto longdelay=cnf.Get("timeDelay","0");
		cnf.SetSection("EventStruct");

		auto configFile_long_time=cnf.Get("LongBusyTime","0");
	//	std::cout<<" longdelay "<< longdelay<<std::endl;
		long int longPause_time_from_command_line=0;
		
		try{

			longPause_time_from_command_line=std::stol(longdelay);
			longPause_time=std::stol(configFile_long_time);

		}
		catch(...)
		{

			std::string errorMsg="error in SCT Upgrade plugin \n unable to convert " + to_string(longdelay) +"to unsigned long long";
			EUDAQ_THROW(errorMsg);
		}
		if (longPause_time_from_command_line>0)
		{
			
		}
		longPause_time=longPause_time_from_command_line;
		std::cout <<"longPause_time: "<<longPause_time<<std::endl;
#ifndef WIN32  //some linux Stuff //$$change
		(void)cnf; // just to suppress a warning about unused parameter cnf
#endif
        
      }

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.
      virtual unsigned GetTriggerID(const Event & ev) const {
        static const unsigned TRIGGER_OFFSET = 8;
        // Make sure the event is of class RawDataEvent
        if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
          // This is just an example, modified it to suit your raw data format
          // Make sure we have at least one block of data, and it is large enough
			return rev->GetEventNumber();
            //return GetTriggerCounter(*rev);   
          
        }
		
        // If we are unable to extract the Trigger ID, signal with (unsigned)-1
        return (unsigned)-1;
      }

	   virtual int IsSyncWithTLU(eudaq::Event const & ev,eudaq::TLUEvent const & tlu) const {
		   int returnValue=Event_IS_Sync;

		   unsigned long long tluTime=tlu.GetTimestamp();
	         long int tluEv=(long int)tlu.GetEventNumber();
		   	 const RawDataEvent & rawev = dynamic_cast<const RawDataEvent &>(ev);

			 long int trigger_id=ev.GetEventNumber();
			 if (oldDUTid>trigger_id)
			 {
				 std::cout<<" if (oldDUTid>trigger_id)"<<std::endl;
			 }
		  returnValue=compareTLU2DUT(tluEv,trigger_id);


		   return returnValue;
	   
	   
	   }
      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      virtual bool GetStandardSubEvent(StandardEvent & sev,
          const Event & ev) const {
        // If the event type is used for different sensors
        // they can be differentiated here
			
       

 
       
  
		 const RawDataEvent & rawev = dynamic_cast<const RawDataEvent &>(ev);
		 
		 sev.SetTag("DUT_time",rawev.GetTimestamp());
		 int id = 8;
		 std::string sensortype = "SCT";
             
     // Create a StandardPlane representing one sensor plane
		 StandardPlane plane(id, EVENT_TYPE, sensortype);

           
     // Set the number of pixels
     int width =rawev.GetBlock(0).size()*8, height = rawev.NumBlocks();
     plane.SetSizeRaw(width, height);



		 for (size_t i=0;i< rawev.NumBlocks();++i){
      pushData2StandartPlane(rawev.GetBlock(i),i,plane);

     }
	 

		 // Set the trigger ID
	  plane.SetTLUEvent(GetTriggerID(ev));
		  
		//  std::cout<<"length of plane " <<numberOfEvents_inplane<<std::endl;
		 // Add the plane to the StandardEvent	
		 sev.AddPlane(plane);
       // Indicate that data was successfully converted
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
     SCTupgrade2ConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0),last_TLU_time(0),Last_DUT_Time(0),longPause_time(0)
      {oldDUTid=0;}

      // Information extracted in Initialize() can be stored here:
      unsigned m_exampleparam;
	  long int oldDUTid;
	 mutable  unsigned long long last_TLU_time,Last_DUT_Time;
		long int longPause_time;
		mutable std::vector<std::vector<unsigned char>> m_event_queue;
      // The single instance of this converter plugin
      static SCTupgrade2ConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
 SCTupgrade2ConverterPlugin SCTupgrade2ConverterPlugin::m_instance;

} // namespace eudaq
