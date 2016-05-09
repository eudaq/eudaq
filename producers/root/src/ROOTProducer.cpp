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

  //client functions
  virtual void OnConfigure(const eudaq::Configuration & config);
  virtual void OnStartRun(unsigned param);
  virtual void OnStopRun();
  virtual void OnTerminate();
  virtual void OnIdle();
  
  //event functions
  void createNewEvent(unsigned nev);
  void createNewEvent(unsigned nev, int dataid, unsigned char* data, size_t size);
  void createBOREvent();
  void createEOREvent();
  void addData2Event(unsigned dataid,const std::vector<unsigned char>& data);
  void addData2Event(unsigned dataid,const unsigned char *data, size_t size);
  void addTag2Event(const char* tag, const char* Value);
  void setTimeStamp2Event( unsigned long long TimeStamp );
  void setTimeStampNow2Event();
  void sendEvent();


  //
  void waitingLockRelease(){std::unique_lock<std::mutex> lck(m_doing);};
  
  eudaq::Configuration& getConfiguration(){return m_config;};  
  const std::string& getName() const{ return m_ProducerName;};
  unsigned getRunNumber(){return m_run;};

  bool timeout(unsigned int tries);

  //
  bool isStateERROR(){return getState() == STATE_ERROR;};
  bool isStateUNCONF(){return getState() == STATE_UNCONF;};
  bool isStateGOTOCONF(){return getState() == STATE_GOTOCONF;};
  bool isStateCONFED(){return getState() == STATE_CONFED;};
  bool isStateGOTORUN(){return getState() == STATE_GOTORUN;};
  bool isStateRUNNING(){return getState() == STATE_RUNNING;};
  bool isStateGOTOSTOP(){return getState() == STATE_GOTOSTOP;};
  bool isStateGOTOTERM(){return getState() == STATE_GOTOTERM;};
  void setStateERROR(){setState(STATE_ERROR);};
  void setStateUNCONF(){setState(STATE_UNCONF);};
  void setStateGOTOCONF(){setState(STATE_GOTOCONF);};
  void setStateCONFED(){setState(STATE_CONFED);};
  void setStateGOTORUN(){setState(STATE_GOTORUN);};
  void setStateRUNNING(){setState(STATE_RUNNING);};
  void setStateGOTOSTOP(){setState(STATE_GOTOSTOP);};
  void setStateGOTOTERM(){setState(STATE_GOTOTERM);};
  //void stateChanged(){Emit("stateChanged()");};


  void setLocalStop(){m_local_stop = 1;};


  std::stringstream m_streamOut;
private:
  eudaq::Configuration  m_config;
  std::unique_ptr<eudaq::RawDataEvent> ev;

  const std::string m_ProducerName;
  std::mutex m_doing;
  std::vector<std::string> m_errors;

  enum FSMState {
    STATE_ERROR,
    STATE_UNCONF,
    STATE_GOTOCONF,
    STATE_CONFED,
    STATE_GOTORUN,
    STATE_RUNNING,
    STATE_GOTOSTOP,
    STATE_GOTOTERM
  } m_fsmstate;


  unsigned m_run, m_ev;
  unsigned int m_Timeout_delay; //milli seconds
  int m_local_stop;

  FSMState getState(){  return m_fsmstate;};
  void setState(FSMState s){  m_fsmstate = s;};
  
};


ROOTProducer::Producer_PImpl::Producer_PImpl(const std::string & name, const std::string & runcontrol) :
  eudaq::Producer(name, runcontrol),m_fsmstate(STATE_UNCONF), m_ProducerName(name){
  streamOut << "hallo from " << name << " producer" << std::endl;

  m_Timeout_delay = 10000; // from itsdaq
  m_local_stop = 0;
}

void ROOTProducer::Producer_PImpl::OnConfigure(const eudaq::Configuration & config){
  std::unique_lock<std::mutex> lck(m_doing);
  m_config = config;
  streamOut << "Configuring: " << m_config.Name() << std::endl;
  m_config.Print(m_streamOut);

  setState(STATE_GOTOCONF);

  ////waiting other thread to finish the config and set STATE to STATE_CONFED
  int j = 0;
  while (getState()==STATE_GOTOCONF && !timeout(++j)){
    eudaq::mSleep(gTimeout_wait);
  }
  ///END waiting
  
  if(getState()==STATE_CONFED){
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }
  else{
    SetStatus(eudaq::Status::LVL_ERROR, "OnConfigure ERROR or timeout(" + config.Name() + ")");
    setState(STATE_ERROR);
  }
}

void ROOTProducer::Producer_PImpl::OnStartRun(unsigned param){
  std::unique_lock<std::mutex> lck(m_doing);
  std::cout<<"OnStartRun()"<<std::endl;
  m_run = param;
  m_ev = 0;
  m_errors.clear();
  setState(STATE_GOTORUN);
  createBOREvent();

  int j = 0;
  while (getState()==STATE_GOTORUN && !timeout(++j)){
    eudaq::mSleep(gTimeout_wait);
  }
  
  if(getState()==STATE_RUNNING){
    //TODO:: add tag to BORE
    sendEvent();
    SetStatus(eudaq::Status::LVL_OK, "Running");
  }
  else{
    SetStatus(eudaq::Status::LVL_ERROR, "OnStartRun ERROR");
    setState(STATE_ERROR);
  }
  std::cout<<"End of OnStartRun()"<<std::endl;
}

void ROOTProducer::Producer_PImpl::OnStopRun(){
  std::unique_lock<std::mutex> lck(m_doing);
  setState(STATE_GOTOSTOP);

  int j = 0;
  while (getState()==STATE_GOTOSTOP && !timeout(++j)){
    eudaq::mSleep(gTimeout_wait);
  }

  if(getState()==STATE_CONFED){
    if(ev){
      if(!ev->IsEORE()){ // last event is not EORE
	sendEvent();
      }
    }
    if(!ev){
      createEOREvent();
    }
    //TODO::tag the m_errors
    ev->SetTag("recorded_messages", m_streamOut.str()); //but cleared by status_check
    sendEvent();
    m_errors.clear();
    EUDAQ_INFO(std::to_string(m_ev) + " Events Processed" );
    EUDAQ_INFO("End of run " + std::to_string(m_run));
  }
  else{
    SetStatus(eudaq::Status::LVL_ERROR, "OnStopRun ERROR");
    setState(STATE_ERROR);
  }  
}

void ROOTProducer::Producer_PImpl::OnTerminate(){
  std::unique_lock<std::mutex> lck(m_doing);
  setState(STATE_GOTOTERM);
  int j = 0;
  while (getState()==STATE_GOTOTERM && !timeout(++j)){
    eudaq::mSleep(gTimeout_wait);
  }
  
  if(getState()==STATE_UNCONF){
    EUDAQ_INFO("Terminated");
  }
  else{
    SetStatus(eudaq::Status::LVL_ERROR, "OnTerminate ERROR");
    setState(STATE_ERROR);
  }
}

void ROOTProducer::Producer_PImpl::OnIdle() {
  // (static_cast<Producer*> this)->OnIdle();
  eudaq::mSleep(500);
  //check when STATE was set out of OnStopRun and OnTerminate
  if(m_local_stop ==1){
    OnStopRun();
    m_local_stop = 0;
  }
  
}

bool ROOTProducer::Producer_PImpl::timeout(unsigned int tries){
  if (tries > (m_Timeout_delay / gTimeout_wait)){
    std::string timeoutWaring;
    timeoutWaring += "[Producer." + m_ProducerName + "] waring: status changed timed out: ";
    if (isStateGOTORUN()){
      timeoutWaring += " onStart timed out";
    }
    if (isStateGOTOCONF()){
      timeoutWaring += " onConfigure timed out";
    }
    if (isStateGOTOSTOP()){
      timeoutWaring += " onStop timed out";
    }
    if (isStateGOTOTERM()){
      timeoutWaring += " OnTerminate timed out";
    }
    streamOut << timeoutWaring << std::endl;
    Producer_warning(timeoutWaring);
    return true;
  }
  return false;
}


void ROOTProducer::Producer_PImpl::createNewEvent(unsigned nev){
  if (m_ev!=nev){
    std::string waring = m_ProducerName + ": Event number mismatch. Expected event: " + std::to_string(m_ev) + "  received event: " + std::to_string(nev);    
    Producer_warning(waring);
    m_ev = nev;
  }
  ev = std::unique_ptr<eudaq::RawDataEvent>(new eudaq::RawDataEvent(m_ProducerName, m_run, m_ev));
  ev->SetTag("eventNr", m_ev);
}


void ROOTProducer::Producer_PImpl::createEOREvent(){
  ev = std::unique_ptr<eudaq::RawDataEvent>(eudaq::RawDataEvent::newEORE(m_ProducerName, m_run, m_ev));
}

void ROOTProducer::Producer_PImpl::createBOREvent(){
  ev = std::unique_ptr<eudaq::RawDataEvent>(eudaq::RawDataEvent::newBORE(m_ProducerName,m_run));
}


void ROOTProducer::Producer_PImpl::setTimeStamp2Event(unsigned long long TimeStamp){
  ev->setTimeStamp(TimeStamp);
}

void ROOTProducer::Producer_PImpl::setTimeStampNow2Event(){
  ev->SetTimeStampToNow();
}

void ROOTProducer::Producer_PImpl::addTag2Event(const char* tag, const char* Value){
  ev->SetTag(tag, Value);
}

void ROOTProducer::Producer_PImpl::addData2Event(unsigned dataid,const std::vector<unsigned char>& data){
  ev->AddBlock(dataid, data);
}


void ROOTProducer::Producer_PImpl::addData2Event(unsigned dataid,const unsigned char *data, size_t size){
  ev->AddBlock(dataid, data, size);
}


void ROOTProducer::Producer_PImpl::sendEvent(){
  SendEvent(*ev);
  if(ev->IsBORE()){
    m_ev = 0;  //Are first data_event and BORE both 0 ??
  }
  else{
    ++m_ev;
  }
  ev.reset(nullptr);
}





///////////////////////////////////////////

ROOTProducer::ROOTProducer(const char* name,const char* runcontrol):m_prod(nullptr) {
  Connect2RunControl(name,runcontrol);
}

ROOTProducer::ROOTProducer():m_prod(nullptr){
  
}

ROOTProducer::~ROOTProducer(){
  if(m_prod != nullptr){
    delete m_prod;
  }
}


void ROOTProducer::Connect2RunControl( const char* name,const char* runcontrol ){
  try {
    std::string n="tcp://"+std::string(runcontrol);
    m_prod=new Producer_PImpl(name,n);
    
    // m_prod->Connect("OnConfigure()",  "ROOTProducer", this, "OnConfigure()");
    // m_prod->Connect("OnStartRun()",  "ROOTProducer", this, "OnStartRun()");
    // m_prod->Connect("OnStopRun()",  "ROOTProducer", this, "OnStopRun()");
    // m_prod->Connect("OnTerminate()",  "ROOTProducer", this, "OnTerminate()");
    // m_prod->Connect("stateChanged()",  "ROOTProducer", this, "checkStatus()");
  }
  catch(...){
    std::cout<<"unable to connect to runcontrol: "<<runcontrol<<std::endl;
  }
}

void ROOTProducer::createEOREvent(){
  try{
    m_prod->createEOREvent();
  }
  catch (...){
    std::cout << "unable to connect create new event" << std::endl;
  }
}

void ROOTProducer::createNewEvent(unsigned nev ){
  // std::cout<<"createNewEvent()"<<std::endl;

  try{
    m_prod->createNewEvent(nev);		
  }
  catch (...){
    std::cout<<"unable to connect create new event"<<std::endl;
  }
}


// void ROOTProducer::createNewEvent(unsigned nev, unsigned dataid, unsigned char* data, size_t size){
//   try{
//     m_prod->createNewEvent(nev);
//     m_prod->addData2Event(dataid, data, size);
//   }
//   catch (...){
//     std::cout<<"unable to connect create new event"<<std::endl;
//   }
// }



void ROOTProducer::setTimeStamp( ULong64_t TimeStamp ){
  try{
    m_prod->setTimeStamp2Event(static_cast<unsigned long long>(TimeStamp));
  }
  catch(...){
    std::cout<<"unable to set time Stamp"<<std::endl;
  }
}

void ROOTProducer::setTimeStamp2Now(){
  m_prod->setTimeStampNow2Event();
}

 
void ROOTProducer::sendEvent(){
  // std::cout<<"sendEvent()"<<std::endl;

  for(size_t i = 0; i< m_vpoint_bool.size(); i++){
    std::vector<unsigned char> out;
    eudaq::bool2uchar(m_vpoint_bool[i], m_vpoint_bool[i] + m_vsize_bool[i], out);
    m_prod->addData2Event(m_vblockid_bool[i], out);
  }
  for(size_t i = 0; i< m_vpoint_uchar.size(); i++){
    m_prod->addData2Event(m_vblockid_uchar[i], m_vpoint_uchar[i], m_vsize_uchar[i]);
  }
  try {
    m_prod->sendEvent();
  }catch (...){
    std::cout<<"unable to send Event"<<std::endl;
  }
  checkStatus();
}

void ROOTProducer::send_OnConfigure(){Emit("send_OnConfigure()");}
void ROOTProducer::send_OnStopRun(){Emit("send_OnStopRun()");}
void ROOTProducer::send_OnStartRun(unsigned nrun){  Emit("send_OnStartRun(unsigned)",nrun);}
void ROOTProducer::send_OnTerminate(){Emit("send_OnTerminate()");}
bool ROOTProducer::isNetConnected(){return !(m_prod==nullptr);}

const char* ROOTProducer::getProducerName(){
  return m_prod->getName().c_str();
}

int ROOTProducer::getConfiguration(const char* tag, int DefaultValue){
  try{
    return m_prod->getConfiguration().Get(tag,DefaultValue);
  }catch(...){
    std::cout<<"unable to getConfiguration"<<std::endl;
    return 0;
  }
}

int ROOTProducer::getConfiguration( const char* tag, const char* defaultValue,char* returnBuffer,Int_t sizeOfReturnBuffer ){
  try{
    std::string dummy(tag);
    std::string ret= m_prod->getConfiguration().Get(dummy,defaultValue );

    if (sizeOfReturnBuffer<static_cast<Int_t>(ret.size()+1)){
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


// void ROOTProducer::getConfiguration( const char* tag ){
//   std::string defaultValue="error";
//   std::string dummy(tag);
//   std::string ret= m_prod->getConfiguration().Get(dummy,defaultValue );
// }


void ROOTProducer::setTag( const char* tag,const char* Value ){
  try{
    m_prod->addTag2Event(tag,Value);
  }catch(...){
    std::cout<<"error in: setTag( "<<tag<< " , "<<Value<<" )" <<std::endl;
  }
}

void ROOTProducer::setTag( const char* tagNameTagValue ){
  std::string dummy(tagNameTagValue);

  size_t equalsymbol=dummy.find_first_of("=");
  if (equalsymbol!=std::string::npos&&equalsymbol>0){
    std::string tagName=dummy.substr(0,equalsymbol);
    std::string tagValue=dummy.substr(equalsymbol+1);
    tagName = eudaq::trim(tagName);
    tagValue = eudaq::trim(tagValue);
    setTag(tagName.c_str(),tagValue.c_str());
  }else{
    std::cout<<"error in: setTag( "<<tagNameTagValue<< ")" <<std::endl;
  }
}



void ROOTProducer::checkStatus(){
  // std::cout<<"checkStatus......"<<std::endl;
  if(m_prod->isStateGOTORUN()){
    std::cout<<"send_OnStartRun"<<std::endl;
    send_OnStartRun(m_prod->getRunNumber());
    m_prod->setStateRUNNING();
    // eudaq::mSleep(gTimeout_statusChanged);  //TODO:: use lock
    m_prod->waitingLockRelease();
  }

  if(m_prod->isStateGOTOCONF()){
    std::cout<<"send_OnConfigure"<<std::endl;
    send_OnConfigure();
    m_prod->setStateCONFED();
    eudaq::mSleep(gTimeout_statusChanged);
    // m_prod->waitingLockRelease();
  }

  if(m_prod->isStateGOTOSTOP()){
    std::cout<<"send_OnStop"<<std::endl;
    send_OnStopRun();
    m_prod->setStateUNCONF();
    eudaq::mSleep(gTimeout_statusChanged);
    // m_prod->waitingLockRelease();
  }

  if(m_prod->isStateGOTOTERM()){
    send_OnTerminate();
    m_prod->setStateUNCONF();
    eudaq::mSleep(gTimeout_statusChanged);
    // m_prod->waitingLockRelease();
  }

  // if (m_prod&&!m_prod->m_streamOut.str().empty()){
  //   std::cout << m_prod->m_streamOut.str() << std::endl;
  //   m_prod->m_streamOut.str("");
  // }
}

void ROOTProducer::setStatusToStopped(){
  m_prod->setLocalStop();
}

void ROOTProducer::addDataPointer_bool(unsigned bid, bool* data, size_t size){
  m_vpoint_bool.push_back(data);
  m_vsize_bool.push_back(size);
  m_vblockid_bool.push_back(bid);
}
void ROOTProducer::addDataPointer_uchar(unsigned bid, unsigned char* data, size_t size){
  m_vpoint_uchar.push_back(data);
  m_vsize_uchar.push_back(size);
  m_vblockid_uchar.push_back(bid);
}

void ROOTProducer::addDataPointer_ULong64_t(unsigned bid, ULong64_t* data, size_t size){
  addDataPointer_uchar(bid, reinterpret_cast<unsigned char*>(data), sizeof(ULong64_t)*size);
}


void ROOTProducer::addData2Event(unsigned dataid,const std::vector<unsigned char>& data){
  m_prod->addData2Event(dataid,data);
}
void ROOTProducer::addData2Event(unsigned dataid,const unsigned char * data, size_t size){
  m_prod->addData2Event(dataid,data, size);
}
