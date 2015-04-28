


#include "ROOTProducer.h"

#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"



#include <ostream>
#include <vector>
#include <time.h>
#include <string>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>
#include <sstream>

#define Producer_warning(msg) EUDAQ_WARN_STREAMOUT(msg,m_streamOut,m_streamOut); \
                              m_errors.push_back(msg)
 
const int gTimeout_wait = 20; //milli seconds 
const int gTimeout_statusChanged = gTimeout_wait* 10; //milli seconds 


#define  streamOut m_streamOut <<"[" << m_ProducerName<<"]: "


class ROOTProducer::Producer_PImpl : public eudaq::Producer {
public:
	Producer_PImpl(const std::string & name, const std::string & runcontrol);
	// This gets called whenever the DAQ is configured
	virtual void OnConfigure(const eudaq::Configuration & config);
	eudaq::Configuration& getConfiguration();
  unsigned getRunNumber();
	// This gets called whenever a new run is started
	// It receives the new run number as a parameter
virtual	void OnStartRun(unsigned param);
	// This gets called whenever a run is stopped
bool timeout(int tries);
virtual	void OnStopRun();

	// This gets called when the Run Control is terminating,
	// we should also exit.
virtual	void OnTerminate();


void setTimeOutTime(int timeout);
	void createNewEvent();
	void createNewEvent(int eventNr);
	void setTimeStamp( unsigned long long TimeStamp );

	void setTimeStamp2Now();
	void setTag(const char* tag,const char* Value);

	void AddPlane2Event( unsigned Block_id,const std::vector<unsigned char>& inputVector );

	void sendEvent();
	void sendEvent(int eventNr);
  void setConfStatus(bool newStat);
	bool ConfigurationSatus();

	bool getOnStart();
	void setOnStart(bool newStat);

	bool getOnConfigure();
	void setOnconfigure(bool newStat);

	bool getOnStop();
	void setOnStop(bool newStat);

  bool getDoStop();
  void setDoStop(bool);

	bool getOnTerminate();
	void setOnTerminate(bool newStat);


  const std::string& getName() const;
	bool isCorrectEventNR(int evNummer);

   void addDataPointer(unsigned Block_id,const bool* inputVector,size_t Elements);

   void addDataPointer(unsigned Block_id, const UChar_t* inputVector, size_t Elements);


  struct Data_pointer_bool
  {
	  Data_pointer_bool(unsigned Block_id, const bool* inputVector, size_t Elements);
    void addDataBlock2Event(eudaq::RawDataEvent& rev);
    unsigned m_Block_id;
    const bool* m_inputVector;
    size_t m_Elements;
  };
  
  struct Data_pointer_char
  {
	  Data_pointer_char(unsigned Block_id, const UChar_t* inputVector, size_t Elements);
	  void addDataBlock2Event(eudaq::RawDataEvent& rev);
	  unsigned m_Block_id;
	  const UChar_t* m_inputVector;
	  size_t m_Elements;
   
  };

  std::vector<Data_pointer_bool> m_data_bool;
  std::vector<Data_pointer_char> m_data_char;


	clock_t startTime_;

  std::atomic<unsigned> m_run;
	unsigned  m_ev;
	bool isConfigured;

	std::unique_ptr<eudaq::RawDataEvent> ev;
		
	
	eudaq::Configuration  m_config;
	const std::string m_ProducerName;

	
	
	std::mutex m_stautus_change;


  std::vector<std::string> m_errors;
  
	bool onStart_,
		onConfigure_,
		onStop_,doStop_,
		OnTerminate_;
  int m_Timeout_delay = 1000; //milli seconds
  std::stringstream m_streamOut;
};

ROOTProducer::Producer_PImpl::Data_pointer_char::Data_pointer_char(unsigned Block_id, const UChar_t* inputVector, size_t Elements) :
m_Block_id(Block_id),
m_inputVector(inputVector),
m_Elements(Elements)
{

}

void ROOTProducer::Producer_PImpl::Data_pointer_char::addDataBlock2Event(eudaq::RawDataEvent& rev)
{
  try{
    rev.AddBlock(m_Block_id, m_inputVector, m_Elements);
  }
  catch (...){
    std::cout << "[Data_pointer_char] unable to Add plane to Event" << std::endl;
  }
}

ROOTProducer::Producer_PImpl::Data_pointer_bool::Data_pointer_bool(unsigned Block_id, const bool* inputVector, size_t Elements) :
m_Block_id(Block_id),
m_inputVector(inputVector),
m_Elements(Elements)
{

}

void ROOTProducer::Producer_PImpl::Data_pointer_bool::addDataBlock2Event(eudaq::RawDataEvent& rev)
{
  try{
    std::vector<unsigned char> out;
    eudaq::bool2uchar(m_inputVector, m_inputVector + m_Elements, out);
    rev.AddBlock(m_Block_id, out);

  }
  catch (...){
    std::cout << "[Data_pointer_bool] unable to Add plane to Event" << std::endl;
  }
}

ROOTProducer::Producer_PImpl::Producer_PImpl(const std::string & name, const std::string & runcontrol) : eudaq::Producer(name, runcontrol),
m_run(0), m_ev(0), isConfigured(false), m_ProducerName(name), onConfigure_(false), onStart_(false), onStop_(false), OnTerminate_(false)
{
  streamOut << "hallo from " << name << " producer" << std::endl;
}

void ROOTProducer::Producer_PImpl::OnConfigure(const eudaq::Configuration & config)
{
  m_config = config;

  setConfStatus(true);
  streamOut << "Configuring: " << getConfiguration().Name() << std::endl;


  config.Print(m_streamOut);
  //m_interface->send_onConfigure();

  setOnconfigure(true);
  int j = 0;
  while (getOnConfigure() && !timeout(++j))
  {
    eudaq::mSleep(gTimeout_wait);
  }
  setOnconfigure(false);
  // Do any configuration of the hardware here
  // Configuration file values are accessible as config.Get(name, default)


  // At the end, set the status that will be displayed in the Run Control.
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
}

eudaq::Configuration& ROOTProducer::Producer_PImpl::getConfiguration()
{
  return m_config;
}

unsigned ROOTProducer::Producer_PImpl::getRunNumber()
{
  return m_run;
}

void ROOTProducer::Producer_PImpl::OnStartRun(unsigned param)
{
  // version 0.1 Susanne from LatencyScan.cpp
  //	streamOut<<"virtual void OnStartRun(unsigned param)"<<std::endl;
  streamOut << "starting Run:" << m_run;
  m_run = param;
  m_ev = 0;
  m_errors.clear();



  startTime_ = clock();



  // It must send a BORE to the Data Collector
  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(m_ProducerName, m_run));



  // Send the event to the Data Collector
  SendEvent(bore);


  setOnStart(true);
  int j = 0;
  while (getOnStart() && !timeout(++j))
  {
    eudaq::mSleep(gTimeout_wait);
  }
  setOnStart(false);
  // At the end, set the status that will be displayed in the Run Control.
  SetStatus(eudaq::Status::LVL_OK, "Running");
}

bool ROOTProducer::Producer_PImpl::timeout(int tries)
{
  if (tries > (m_Timeout_delay / gTimeout_wait))
  {

    std::string timeoutWaring;
    timeoutWaring += "[Producer." + m_ProducerName + "] waring: status changed timed out: ";
    if (getOnStart())
    {
      timeoutWaring += " onStart timed out";
    }
    if (getOnConfigure())
    {
      timeoutWaring += " onConfigure timed out";
    }
    if (getOnStop())
    {
      timeoutWaring += " onStop timed out";
    }
    if (getOnTerminate())
    {
      timeoutWaring += " OnTerminate timed out";
    }
    streamOut << timeoutWaring << std::endl;
    Producer_warning(timeoutWaring);
    return true;
  }
  return false;
}

void ROOTProducer::Producer_PImpl::OnStopRun()
{
  streamOut << "virtual void OnStopRun()" << std::endl;
  //	m_interface->send_onStop();
  streamOut << "received at event: "<< m_ev << std::endl;
  setOnStop(true);
  setDoStop(false);
  int j = 0;
  while (!getDoStop() && !timeout(++j))
  {
    eudaq::mSleep(gTimeout_wait);
  }
  setOnStop(false);
  // Set a flag to signal to the polling loop that the run is over



  streamOut << m_ev << " Events Processed" << std::endl;


  auto EORE = eudaq::RawDataEvent::EORE(m_ProducerName, m_run, ++m_ev);
  if (!m_errors.empty())
  {
    streamOut << "warnings recorded:" << std::endl;
    for (auto& e : m_errors)
    {
      streamOut << e << std::endl;
    }
    EORE.SetTag("recorded_messages", m_streamOut.str());
  }
  // Send an EORE after all the real events have been sent
  // You can also set tags on it (as with the BORE) if necessary
  SendEvent(EORE);
}

void ROOTProducer::Producer_PImpl::OnTerminate()
{
  streamOut << "virtual void OnTerminate()" << std::endl;
  //m_interface->send_OnTerminate();
  setOnTerminate(true);
  int j = 0;
  while (getOnTerminate() && !timeout(++j))
  {
    eudaq::mSleep(gTimeout_wait);
  }
  setOnTerminate(false);
}

void ROOTProducer::Producer_PImpl::createNewEvent()
{
  ev = std::unique_ptr<eudaq::RawDataEvent>(new eudaq::RawDataEvent(m_ProducerName, m_run, m_ev));
}

void ROOTProducer::Producer_PImpl::createNewEvent(int eventNr)
{
  if (!isCorrectEventNR(eventNr))
  {

    std::string waring = m_ProducerName + ": Event number mismatch. Expected event: " + std::to_string(m_ev) + "  received event: " + std::to_string(eventNr);

    Producer_warning(waring);
    m_ev = eventNr;
  }
  createNewEvent();
  ev->SetTag("eventNr", eventNr);
}

void ROOTProducer::Producer_PImpl::setTimeStamp(unsigned long long TimeStamp)
{
  if (ev == nullptr)
  {
    createNewEvent();
  }
  ev->setTimeStamp(TimeStamp);
}

void ROOTProducer::Producer_PImpl::setTimeStamp2Now()
{
  if (ev == nullptr)
  {
    createNewEvent();
  }
  ev->SetTimeStampToNow();
}

void ROOTProducer::Producer_PImpl::setTag(const char* tag, const char* Value)
{
  if (ev == nullptr)
  {
    createNewEvent();
  }

  ev->SetTag(tag, Value);
}

void ROOTProducer::Producer_PImpl::AddPlane2Event(unsigned Block_id, const std::vector<unsigned char>& inputVector)
{
  if (ev == nullptr)
  {
    createNewEvent();
  }

  ev->AddBlock(Block_id, inputVector);
}

void ROOTProducer::Producer_PImpl::sendEvent()
{
  // Send the event to the Data Collector     

  if (ev == nullptr)
  {
    if (!m_data_bool.empty()
      ||
      !m_data_char.empty())
    {
      createNewEvent();
    }
    else{
      streamOut << " you have to create the an event before you can send it" << std::endl;
      return;
    }
  }

  for (auto& e : m_data_bool){
    e.addDataBlock2Event(*ev);
  }
  for (auto& e : m_data_char){
    e.addDataBlock2Event(*ev);
  }

  SendEvent(*ev);

  // clean up 
  ev.reset(nullptr);

  // Now increment the event number

  ++m_ev;
}

void ROOTProducer::Producer_PImpl::sendEvent(int eventNr)
{
  if (!isCorrectEventNR(eventNr))
  {
    streamOut << "void ROOTProducer::sendEvent(int eventNr) " << std::endl;
    streamOut << "event nr mismatch. expected event " << m_ev << " received event " << eventNr << std::endl;
  }
  sendEvent();
}

void ROOTProducer::Producer_PImpl::setConfStatus(bool newStat)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  isConfigured = newStat;
}

bool ROOTProducer::Producer_PImpl::ConfigurationSatus()
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return isConfigured;
}

bool ROOTProducer::Producer_PImpl::getOnStart()
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return onStart_;
}

void ROOTProducer::Producer_PImpl::setOnStart(bool newStat)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  onStart_ = newStat;
}

bool ROOTProducer::Producer_PImpl::getOnConfigure()
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return onConfigure_;
}

void ROOTProducer::Producer_PImpl::setOnconfigure(bool newStat)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  onConfigure_ = newStat;
}

bool ROOTProducer::Producer_PImpl::getOnStop()
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return onStop_;
}

void ROOTProducer::Producer_PImpl::setOnStop(bool newStat)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  onStop_ = newStat;
}

bool ROOTProducer::Producer_PImpl::getDoStop() 
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return doStop_;
}


void ROOTProducer::Producer_PImpl::setDoStop(bool newStatus)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  doStop_ = newStatus;
}

bool ROOTProducer::Producer_PImpl::getOnTerminate()
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  return OnTerminate_;
}

void ROOTProducer::Producer_PImpl::setOnTerminate(bool newStat)
{
  std::unique_lock<std::mutex> lck(m_stautus_change);
  OnTerminate_ = newStat;
}

bool ROOTProducer::Producer_PImpl::isCorrectEventNR(int evNummer)
{
  return m_ev == static_cast<unsigned>(evNummer);
}

void ROOTProducer::Producer_PImpl::addDataPointer(unsigned Block_id, const bool* inputVector, size_t Elements)
{
  m_data_bool.emplace_back(Block_id, inputVector, Elements);
}

void ROOTProducer::Producer_PImpl::addDataPointer(unsigned Block_id, const UChar_t* inputVector, size_t Elements)
{
  m_data_char.emplace_back(Block_id, inputVector, Elements);
}

const std::string& ROOTProducer::Producer_PImpl::getName() const
{
  return m_ProducerName;
}

void ROOTProducer::Producer_PImpl::setTimeOutTime(int timeOut)
{
  if (timeOut > 0){
    m_Timeout_delay = timeOut;
  }
  else
  {
    m_streamOut << "unable to set timeout to " << timeOut << std::endl;
  }
}

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

void ROOTProducer::createNewEvent( int eventNR )
{
	try
	{
		m_prod->createNewEvent(eventNR);
		
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





void ROOTProducer::AddPlane2Event( unsigned Block_id,const std::vector<unsigned char>& inputVector )
{
	try{
	m_prod->AddPlane2Event(Block_id, inputVector);
	}
	catch(...){
		std::cout<<"unable to Add plane to Event"<<std::endl;
	}


}


 void ROOTProducer::AddPlane2Event(unsigned Block_id,const bool* inputVector,size_t Elements){

	 try{
		 std::vector<unsigned char> out;
		 eudaq::bool2uchar(inputVector ,inputVector+Elements,out);
     m_prod->AddPlane2Event(Block_id, out);
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

void ROOTProducer::sendEvent( int eventNR )
{
	try {
		m_prod->sendEvent(eventNR);
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


void ROOTProducer::send_onStart(int RunNumber)
{
	Emit("send_onStart(int)",RunNumber);
}


void ROOTProducer::send_OnTerminate()
{
	Emit("send_OnTerminate()");
}



void ROOTProducer::send_statusChanged()
{
  Emit("send_statusChanged()");
}

bool ROOTProducer::getConnectionStatus()
{
	return !(m_prod==nullptr);
}


const char* ROOTProducer::getProducerName()
{
 return m_prod->getName().c_str();
}

int ROOTProducer::getConfiguration(const char* tag, int DefaultValue)
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

		std::cout<<"error in: setTag( "<<tag<< " , "<<Value<<" )" <<std::endl;
	}

}

void ROOTProducer::setTag( const char* tagNameTagValue )
{
  std::string dummy(tagNameTagValue);

  size_t equalsymbol=dummy.find_first_of("=");
  if (equalsymbol!=std::string::npos&&equalsymbol>0)
  {
    std::string tagName=dummy.substr(0,equalsymbol-1);
    std::string tagValue=dummy.substr(equalsymbol+1);
    setTag(tagName.c_str(),tagValue.c_str());

  }else{

    std::cout<<"error in: setTag( "<<tagNameTagValue<< ")" <<std::endl;
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


void ROOTProducer::setStatusToStopped()
{
  std::cout << "void ROOTProducer::setStatusToStopped()" << std::endl;
 m_prod->setDoStop(true);
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
		
		send_onStart(m_prod->getRunNumber());
		setOnStart(false);
    
    eudaq::mSleep(gTimeout_statusChanged);
    send_statusChanged();
	}

	if(getOnConfigure()){
		
		send_onConfigure();
		setOnconfigure(false);
    eudaq::mSleep(gTimeout_statusChanged);
    send_statusChanged();
	}

	if(getOnStop()){
		send_onStop();
    setOnStop(false);
    eudaq::mSleep(gTimeout_statusChanged);
    send_statusChanged();
	}

	if(getOnTerminate()){
		send_OnTerminate();
		setOnTerminate(false);
    eudaq::mSleep(gTimeout_statusChanged);
    send_statusChanged();
	}

  if (m_prod&&!m_prod->m_streamOut.str().empty())
  {
    std::cout << m_prod->m_streamOut.str() << std::endl;
    m_prod->m_streamOut.str("");
    send_statusChanged();
  }
}


void ROOTProducer::setTimeOut(int timeOut)
{
  if (m_prod)
  {
    m_prod->setTimeOutTime(timeOut);
  }
}

void ROOTProducer::addDataPointer_bool(unsigned Block_id, const bool* inputVector, size_t Elements)
{
  m_prod->addDataPointer(Block_id, inputVector, Elements);
}

void ROOTProducer::addDataPointer_UChar_t(unsigned Block_id, const UChar_t* inputVector, size_t Elements)
{
	m_prod->addDataPointer(Block_id, inputVector, Elements);
}

void ROOTProducer::addDataPointer_Uint_t(unsigned Block_id, const UInt_t* inputVector, size_t Elements)
{
	m_prod->addDataPointer(Block_id, reinterpret_cast<const UChar_t*>(inputVector), Elements * sizeof(UInt_t));
}

void ROOTProducer::addDataPointer_ULong64_t(unsigned Block_id, const ULong64_t* inputVector, size_t Elements){

	m_prod->addDataPointer(Block_id, reinterpret_cast<const UChar_t*>(inputVector), Elements * sizeof(ULong64_t));
}

