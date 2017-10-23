#include "ROOTProducer.hh"

#include "eudaq/Producer.hh"
#include "eudaq/RawEvent.hh"

#include <ostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>

class ItsRootProducer : public eudaq::Producer {
public:
  ItsRootProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override{EUDAQ_THROW("NOT implemeted for ITSDAQ");};
  void DoTerminate() override;

  enum STATE{
    ERR,
    UNINIT,
    GOTOINIT,
    UNCONF,
    GOTOCONF,
    CONFED,
    GOTORUN,
    RUNNING,
    GOTOSTOP,
    GOTOTERM
  };
  
  STATE getState(){return m_state;};
  void setState(STATE s){m_state=s;};
  void waitingLockRelease(){std::unique_lock<std::mutex> lck(m_doing);};
  eudaq::EventUP makeRawEvent(){ return eudaq::Event::MakeUnique(m_ev_name);};
private:
  STATE m_state;
  std::mutex m_doing;

  std::string m_ev_name;
};

ItsRootProducer::ItsRootProducer(const std::string & name,
			       const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_state(STATE::UNINIT){
  m_ev_name = name;
}


void ItsRootProducer::DoInitialise(){
  std::unique_lock<std::mutex> lck(m_doing);
  int j = 0;
  setState(STATE::GOTOINIT);
  while (getState()==STATE::GOTOINIT && j<5){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    j++;
  }
  if(getState()!=STATE::UNCONF){
    EUDAQ_THROW("OnInitialise ERROR or timeout");
    setState(STATE::UNCONF);
  }
}

void ItsRootProducer::DoConfigure(){
  std::unique_lock<std::mutex> lck(m_doing);
  auto conf = GetConfiguration();
  m_ev_name = conf->Get("ITSROOT_EVENT_NAME", m_ev_name);
  int j = 0;
  setState(STATE::GOTOCONF);
  while (getState()==STATE::GOTOCONF && j<5){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    j++;
  }
  if(getState()!=STATE::CONFED){
    EUDAQ_THROW("OnConfigure ERROR or timeout");
    setState(STATE::ERR);
  }
}

void ItsRootProducer::DoStartRun(){
  std::unique_lock<std::mutex> lck(m_doing);
  setState(STATE::GOTORUN);
  int j = 0;  
  while (getState()==STATE::GOTORUN && j<5){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    j++;
  }  
  if(getState()!=STATE::RUNNING){
    EUDAQ_THROW("OnStartRun ERROR");
    setState(STATE::ERR);
  }
}

void ItsRootProducer::DoStopRun(){
  std::unique_lock<std::mutex> lck(m_doing);
  setState(STATE::GOTOSTOP);
  int j = 0;
  while (getState()==STATE::GOTOSTOP && j<5){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    j++;
  }
  if(getState()!=STATE::CONFED){
    EUDAQ_THROW("OnStopRun ERROR");
  }  
}

void ItsRootProducer::DoTerminate(){
  std::unique_lock<std::mutex> lck(m_doing);
  setState(STATE::GOTOTERM);
}

///////////////////////////////////////////
ROOTProducer::ROOTProducer(){  

}

ROOTProducer::~ROOTProducer(){
}

void ROOTProducer::Connect2RunControl( const char* name,const char* runcontrol ){
  try {
    std::string addr_rc="tcp://"+std::string(runcontrol);
    m_prod=std::unique_ptr<ItsRootProducer>(new ItsRootProducer(name,addr_rc));
    m_prod->Connect();
  }
  catch(...){
    std::cout<<"unable to connect to runcontrol: "<<runcontrol<<std::endl;
  }
}

bool ROOTProducer::isNetConnected(){
  if(m_prod && m_prod->IsConnected())
    return true;
  else
    return false;
}

void ROOTProducer::createNewEvent(unsigned nev){
  std::unique_lock<std::mutex> lk(m_mtx_ev);
  ev = m_prod->makeRawEvent();
  ev->SetTriggerN(nev);
  if(nev == 0)
    ev->SetBORE();
}

void ROOTProducer::addData2Event(unsigned dataid, UChar_t * data, size_t size){
  std::unique_lock<std::mutex> lk(m_mtx_ev);
  if(ev)
    ev->AddBlock(dataid, data, size);
}

int ROOTProducer::getConfiguration(const char* tag, int defaultValue){
  try{
    return m_prod->GetConfiguration()->Get(tag,defaultValue);
  }catch(...){
    std::cout<<"unable to getConfiguration"<<std::endl;
    return 0;
  }
}

void ROOTProducer::setTag( const char* tagNameTagValue ){
  std::unique_lock<std::mutex> lk(m_mtx_ev);
  std::string dummy(tagNameTagValue);
  size_t equalsymbol=dummy.find_first_of("=");
  if (equalsymbol!=std::string::npos&&equalsymbol>0){
    std::string tagName=dummy.substr(0,equalsymbol);
    std::string tagValue=dummy.substr(equalsymbol+1);
    tagName = eudaq::trim(tagName);
    tagValue = eudaq::trim(tagValue);
    if(ev)
      ev->SetTag(tagName, tagValue);
    else {
      std::cout<<"Skip initial tag: ( "<<tagNameTagValue<< ")" <<std::endl;
    }
  }else{
    std::cout<<"error in: setTag( "<<tagNameTagValue<< ")" <<std::endl;
  }
}

void ROOTProducer::sendEvent(){
  std::unique_lock<std::mutex> lk(m_mtx_ev);
  try {
    if(ev)
      m_prod->SendEvent(std::move(ev));
    checkStatus();
  }catch (...){
    std::cout<<"unable to send Event"<<std::endl;
  }
}

void ROOTProducer::sendLog(const char *msg){
  EUDAQ_USER(std::string(msg));
}

void ROOTProducer::send_OnInilialise(){Emit("send_OnInilialise()");}
void ROOTProducer::send_OnConfigure(){Emit("send_OnConfigure()");}
void ROOTProducer::send_OnStartRun(unsigned nrun){Emit("send_OnStartRun(unsigned)",nrun);}
void ROOTProducer::send_OnStopRun(){Emit("send_OnStopRun()");}
// void ROOTProducer::send_OnReset(){Emit("send_OnReset()");}
void ROOTProducer::send_OnTerminate(){Emit("send_OnTerminate()");}

void ROOTProducer::checkStatus(){
  if(!m_prod)
    return;
  
  if(m_prod->getState()==ItsRootProducer::STATE::GOTOINIT){
    std::cout<<"send_OnInilialise"<<std::endl;
    send_OnInilialise();
    m_prod->setState(ItsRootProducer::STATE::UNCONF);
    m_prod->waitingLockRelease();
  }
  
  if(m_prod->getState()==ItsRootProducer::STATE::GOTOCONF){
    std::cout<<"send_OnConfigure"<<std::endl;
    send_OnConfigure();
    m_prod->setState(ItsRootProducer::STATE::CONFED);
    m_prod->waitingLockRelease();
  }

  if(m_prod->getState()==ItsRootProducer::STATE::GOTORUN){
    std::cout<<"send_OnStartRun"<<std::endl;
    send_OnStartRun(m_prod->GetRunNumber());
    m_prod->setState(ItsRootProducer::STATE::RUNNING);
    m_prod->waitingLockRelease();
  }

  if(m_prod->getState()==ItsRootProducer::STATE::GOTOSTOP){
    std::cout<<"send_OnStop"<<std::endl;
    send_OnStopRun();
    m_prod->setState(ItsRootProducer::STATE::CONFED);
    m_prod->waitingLockRelease();
  }

  if(m_prod->getState()==ItsRootProducer::STATE::GOTOTERM){
    send_OnTerminate();
    m_prod->setState(ItsRootProducer::STATE::UNCONF);
    m_prod->waitingLockRelease();
  }
}
