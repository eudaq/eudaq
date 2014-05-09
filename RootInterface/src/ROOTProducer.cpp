


#include "../inc/ROOTProducer.h"

#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"



#include <ostream>
#include <vector>
#include <time.h>
#include <string>
#include <condition_variable>
#include <memory>
#include <chrono>




inline	void bool2uchar1(const bool* inBegin,const bool* inEnd,std::vector<unsigned char>& out){

  int j=0;
  unsigned char dummy=0;
  //bool* d1=&in[0];
  size_t size=(inEnd-inBegin);
  if (size%8)
  {
    size+=8;
  }
  size/=8;
  out.reserve(size);
  for (auto i=inBegin;i<inEnd;++i)
  {
    dummy+=(unsigned char)(*i)<<(j%8);

    if ((j%8)==7)
    {
      out.push_back(dummy);
      dummy=0;
    }
    ++j;
  }
}


//static const std::string EVENT_TYPE = "SCTupgrade";

class ROOTProducer::Producer_PImpl : public eudaq::Producer {
public:
	Producer_PImpl(const std::string & name, const std::string & runcontrol): eudaq::Producer(name, runcontrol),
		m_run(0), m_ev(0), isConfigured(false),m_ProducerName(name),onConfigure_(false),onStart_(false),onStop_(false),OnTerminate_(false) {
			std::cout<< "hallo from "<<name<<" producer"<<std::endl;
		
	}
	// This gets called whenever the DAQ is configured
	virtual void OnConfigure(const eudaq::Configuration & config)  {
		m_config=config;
		
    setConfStatus(true);
		std::cout << "Configuring: " << getConfiguration().Name() << std::endl;

		//m_interface->send_onConfigure();

		setOnconfigure(true);
		int j=0;
		while (getOnConfigure()&&++j<500)
		{
			eudaq::mSleep(20);
		}
		setOnconfigure(false);
		// Do any configuration of the hardware here
		// Configuration file values are accessible as config.Get(name, default)


		// At the end, set the status that will be displayed in the Run Control.
		SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
	}
	eudaq::Configuration& getConfiguration(){
		return m_config;
	}
	// This gets called whenever a new run is started
	// It receives the new run number as a parameter
virtual	void OnStartRun(unsigned param) {
		// version 0.1 Susanne from LatencyScan.cpp
	//	std::cout<<"virtual void OnStartRun(unsigned param)"<<std::endl;

		m_run =param;
		m_ev=0;



		startTime_=clock();

		

		// It must send a BORE to the Data Collector
		eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(m_ProducerName, m_run));



		// Send the event to the Data Collector
		SendEvent(bore);

		// At the end, set the status that will be displayed in the Run Control.
		SetStatus(eudaq::Status::LVL_OK, "Running");

    setOnStart(true);
    int j=0;
    while (getOnStart()&&++j<500)
    {
      eudaq::mSleep(20);
    }
    setOnStart(false);

	}
	// This gets called whenever a run is stopped
virtual	void OnStopRun() {
		std::cout << "virtual void OnStopRun()" << std::endl;
	//	m_interface->send_onStop();

		setOnStop(true);
		int j=0;
		while (getOnStop()&&++j<500)
		{
			eudaq::mSleep(20);
		}
		setOnStop(false);
		// Set a flag to signal to the polling loop that the run is over
		


		std::cout<<m_ev << " Events Processed" << std::endl;
		// Send an EORE after all the real events have been sent
		// You can also set tags on it (as with the BORE) if necessary
		SendEvent(eudaq::RawDataEvent::EORE(m_ProducerName, m_run, ++m_ev));
	}

	// This gets called when the Run Control is terminating,
	// we should also exit.
virtual	void OnTerminate() {
		std::cout << "virtual void OnTerminate()" << std::endl;
		//m_interface->send_OnTerminate();
		setOnTerminate(true);
		int j=0;
		while (getOnTerminate()&&++j<500)
		{
			eudaq::mSleep(20);
		}
		setOnTerminate(false);
	}

	void createNewEvent()
	{
		ev= std::unique_ptr<eudaq::RawDataEvent>(new eudaq::RawDataEvent(m_ProducerName, m_run, m_ev));
	}

	void setTimeStamp( unsigned long long TimeStamp )
	{
		if (ev==nullptr)
		{
			createNewEvent();
		}
		ev->setTimeStamp(TimeStamp);
	}

	void setTimeStamp2Now(){
		if (ev==nullptr)
		{
			createNewEvent();
		}
		ev->SetTimeStampToNow();
	}
	void setTag(const char* tag,const char* Value){
		if (ev==nullptr)
		{
			createNewEvent();
		}

		ev->SetTag(tag,Value);
	}

	void AddPlane2Event( unsigned plane,const std::vector<unsigned char>& inputVector )
	{
		if (ev==nullptr)
		{
			createNewEvent();
		}
		
		ev->AddBlock(plane, inputVector);
	}

	void sendEvent()
	{
		// Send the event to the Data Collector     
		if (ev==nullptr)
		{
      if (!m_data.empty())
      {
        createNewEvent();
      }else{
			std::cout<< " you have to create the an event before you can send it"<<std::endl;
			return;		
      }
		}

    for(auto& e:m_data){

    e.addDataBlock2Event(*ev);
   // ev->AddBlock(e.m_plane,e.m_inputVector,e.m_Elements);
    }

		SendEvent(*ev);
		
		// clean up 
		ev.reset(nullptr);
		
		// Now increment the event number

		++m_ev;
	}
  void setConfStatus(bool newStat){
    std::unique_lock<std::mutex> lck (m_stautus_change);
    isConfigured=newStat;
  }
	bool ConfigurationSatus(){
    std::unique_lock<std::mutex> lck (m_stautus_change);
		return isConfigured;
	}

	bool getOnStart(){
		 std::unique_lock<std::mutex> lck (m_stautus_change);
		 return onStart_;
	}
	void setOnStart(bool newStat){
		 std::unique_lock<std::mutex> lck (m_stautus_change);
		 onStart_=newStat;
	}

	bool getOnConfigure(){
		 std::unique_lock<std::mutex> lck (m_stautus_change);
		 return onConfigure_;
	}
	void setOnconfigure(bool newStat){
		 std::unique_lock<std::mutex> lck (m_stautus_change);
		onConfigure_=newStat;
	}

	bool getOnStop(){
		std::unique_lock<std::mutex> lck (m_stautus_change);
		 return onStop_;

	}
	void setOnStop(bool newStat){
		std::unique_lock<std::mutex> lck (m_stautus_change);
		onStop_=newStat;
	}

	bool getOnTerminate(){
		std::unique_lock<std::mutex> lck (m_stautus_change);
		return OnTerminate_;
	}
	void setOnTerminate(bool newStat){
		std::unique_lock<std::mutex> lck (m_stautus_change);
		OnTerminate_=newStat;
	}



   void addDataPointer(unsigned plane,const bool* inputVector,size_t Elements){
   //  std::cout<<"<m_data.emplace_back(plane,inputVector,Elements);> \n";
     m_data.emplace_back(plane,inputVector,Elements);
   //  std::cout<<"</m_data.emplace_back(plane,inputVector,Elements);> \n";
   }

  struct Data_pointer
  {
    Data_pointer(unsigned plane,const bool* inputVector,size_t Elements):
      m_plane(plane),
      m_inputVector(inputVector),
      m_Elements(Elements)
    {}
    void addDataBlock2Event(eudaq::RawDataEvent& rev){
      try{
        std::vector<unsigned char> out;
        bool2uchar1(m_inputVector ,m_inputVector+m_Elements,out);
        rev.AddBlock(m_plane,out);
        
      }
      catch(...){
        std::cout<<"unable to Add plane to Event"<<std::endl;
      }
    }
    unsigned m_plane;
    const bool* m_inputVector;
    size_t m_Elements;
  };
  

  std::vector<Data_pointer> m_data;

	clock_t startTime_;

	unsigned m_run, m_ev;
	bool isConfigured;

	std::unique_ptr<eudaq::RawDataEvent> ev;
		
	
	eudaq::Configuration  m_config;
	const std::string m_ProducerName;

	
	
	std::mutex m_stautus_change;



	bool onStart_,
		onConfigure_,
		onStop_,
		OnTerminate_;

};










// The constructor must call the eudaq::Producer constructor with the name
// and the runcontrol connection string, and initialize any member variables.
ROOTProducer::ROOTProducer(const char* name,const char* runcontrol):m_prod(nullptr) {
	//		std::cout<< "hallo from sct producer"<<std::endl;

	Connect2RunControl(name,runcontrol);
}

ROOTProducer::ROOTProducer():m_prod(nullptr)
{

}

ROOTProducer::~ROOTProducer()
{
	delete m_prod;
}










void ROOTProducer::Connect2RunControl( const char* name,const char* runcontrol )
{  try {
	std::string n="tcp://"+std::string(runcontrol);
	m_prod=new Producer_PImpl(name,n);

	}
	catch(...){

		std::cout<<"unable to connect to runcontrol: "<<runcontrol<<std::endl;
	}
}

void ROOTProducer::createNewEvent()
{
	try
	{
		m_prod->createNewEvent();
	}
	catch (...)
	{
		std::cout<<"unable to connect create new event"<<std::endl;
	}
	
}

void ROOTProducer::setTimeStamp( ULong64_t TimeStamp )
{
	try{
	m_prod->setTimeStamp(static_cast<unsigned long long>(TimeStamp));
	}
	catch(...){
		std::cout<<"unable to set time Stamp"<<std::endl;
	}
}

void ROOTProducer::setTimeStamp2Now()
{
	m_prod->setTimeStamp2Now();
}





void ROOTProducer::AddPlane2Event( unsigned plane,const std::vector<unsigned char>& inputVector )
{
	try{
	m_prod->AddPlane2Event(plane, inputVector);
	}
	catch(...){
		std::cout<<"unable to Add plane to Event"<<std::endl;
	}


}


 void ROOTProducer::AddPlane2Event(unsigned plane,const bool* inputVector,size_t Elements){

	 try{
		 std::vector<unsigned char> out;
		 bool2uchar1(inputVector ,inputVector+Elements,out);
		 m_prod->AddPlane2Event(plane, out);
	 }
	 catch(...){
		 std::cout<<"unable to Add plane to Event"<<std::endl;
	 }
 }

 void ROOTProducer::AddPlane2Event(unsigned MODULE_NR, int ST_STRIPS_PER_LINK , bool* evtr_strm0,bool* evtr_strm1){
   AddPlane2Event((MODULE_NR*2),evtr_strm0,ST_STRIPS_PER_LINK);
   AddPlane2Event(MODULE_NR*2+1,evtr_strm1,ST_STRIPS_PER_LINK);

 }

 
void ROOTProducer::sendEvent()
{

	try {
	m_prod->sendEvent();
	}catch (...)
	{
		std::cout<<"unable to send Event"<<std::endl;
	}
  checkStatus();
}

void ROOTProducer::send_onConfigure()
{
	Emit("send_onConfigure()");
}

void ROOTProducer::send_onStop()
{
	Emit("send_onStop()");
}


void ROOTProducer::send_onStart()
{
	Emit("send_onStart()");
}


void ROOTProducer::send_OnTerminate()
{
	Emit("send_OnTerminate()");
}



bool ROOTProducer::getConnectionStatus()
{
	return !(m_prod==nullptr);
}

int ROOTProducer::getConfiguration( const char* tag, int DefaultValue )
{
	try{
	return m_prod->getConfiguration().Get(tag,DefaultValue);
	}catch(...){
	std::cout<<"unable to getConfiguration"<<std::endl;
	return 0;
	}


}



int ROOTProducer::getConfiguration( const char* tag, const char* defaultValue,char* returnBuffer,Int_t sizeOfReturnBuffer )
{
	try{
	std::string dummy(tag);
	std::string ret= m_prod->getConfiguration().Get(dummy,defaultValue );

	if (sizeOfReturnBuffer<ret.size()+1)
	{
		return 0;
	}


	strncpy(returnBuffer, ret.c_str(), ret.size());
	returnBuffer[ret.size()]=0;
	return ret.size();
	}catch(...){
	std::cout<<"unable to getConfiguration"<<std::endl;
		return 0;
	}
}

void ROOTProducer::getConfiguration( const char* tag )
{
  std::string defaultValue="error";
  std::string dummy(tag);
  std::string ret= m_prod->getConfiguration().Get(dummy,defaultValue );
  emitConfiguration(ret.c_str());
}

void ROOTProducer::emitConfiguration( const char* answer )
{
  Emit("emitConfiguration(const char*)",answer);
}

// 	TString SCTProducer::getConfiguration_TString( const char* tag, const char* defaultValue )
// 	{
// 		TString ReturnValue(m_prod->getConfiguration().Get(tag,defaultValue));
// 		std::cout<<ReturnValue.Data()<<std::endl;
// 		return ReturnValue;
// 	}

bool ROOTProducer::ConfigurationSatus()
{
	try{
	return	m_prod->ConfigurationSatus();
	}catch(...){
		std::cout<<"unable to get ConfigurationSatus"<<std::endl;
		return false;
	}
}

void ROOTProducer::setTag( const char* tag,const char* Value )
{
	try{
	m_prod->setTag(tag,Value);
	}catch(...){

		std::cout<<"unable setTag( "<<tag<< " , "<<Value<<" )" <<std::endl;
	}

}

bool ROOTProducer::getOnStart()
{
	return m_prod->getOnStart();
}



void ROOTProducer::setOnStart( bool newStat )
{
	m_prod->setOnStart(newStat);
}

bool ROOTProducer::getOnConfigure()
{
	return m_prod->getOnConfigure();
}



void ROOTProducer::setOnconfigure( bool newStat )
{
		m_prod->setOnconfigure(newStat);
}

bool ROOTProducer::getOnStop()
{
	return m_prod->getOnStop();
}



void ROOTProducer::setOnStop( bool newStat )
{
	m_prod->setOnStop(newStat);
}

bool ROOTProducer::getOnTerminate()
{
	return m_prod->getOnTerminate();
}



void ROOTProducer::setOnTerminate( bool newStat )
{
	m_prod->setOnTerminate(newStat);
}

void ROOTProducer::checkStatus()
{
	if(getOnStart()){
		
		send_onStart();
		setOnStart(false);
		eudaq::mSleep(20);
	}

	if(getOnConfigure()){
		
		send_onConfigure();
		setOnconfigure(false);
		eudaq::mSleep(20);
	}

	if(getOnStop()){
		send_onStop();
		setOnStop(false);
		eudaq::mSleep(20);
	}

	if(getOnTerminate()){
		send_OnTerminate();
		setOnTerminate(false);
		eudaq::mSleep(20);
	}
}

void ROOTProducer::addDataPointer( unsigned plane,const bool* inputVector,size_t Elements )
{
  m_prod->addDataPointer(plane,inputVector,Elements);
}

// The main function that will create a Producer instance and run it
bool StartTestbeamProducer(const char* nameIn,const char* IP_AdresseIn) {
	std::string name(nameIn);//"SCTProducer";
	std::string rctrl(IP_AdresseIn);//"tcp://127.0.0.1:44000";

	try {
		// This will look through the command-line arguments and set the options

		// Set the Log level for displaying messages based on command-line
		//		EUDAQ_LOG_LEVEL(level.Value());
		// Create a producer
		ROOTProducer producer(name.c_str(), rctrl.c_str());
		// And set it running...
		//	producer.ReadoutLoop();
		// When the readout loop terminates, it is time to go
		std::cout << "Quitting" << std::endl;
	} catch (...) {
		// This does some basic error handling of common exceptions
		std::cout<<"an error had occurred"<<std::endl;
		return false;
	}
	return true;
}
