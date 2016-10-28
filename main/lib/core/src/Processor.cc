#include"Processor.hh"
#include"Utils.hh"

#include <string>
#include <chrono>
#include <thread>
#include <sstream>

using namespace eudaq;

template DLLEXPORT
std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)()>& Factory<Processor>::Instance<>();

ProcessorSP Processor::MakeShared(const std::string& pstype,
				  std::initializer_list
				  <std::pair<const std::string, const std::string>> l){
  ProcessorSP ps = Factory<Processor>::MakeShared(str2hash(pstype));
  for(auto &p: l){
    ps->ProcessSysCmd(p.first, p.second);
  }
  return ps;
}


Processor::Processor(const std::string& dsp)
  :m_description(dsp){
  m_instance_n = static_cast<uint32_t>(reinterpret_cast<uint64_t>(this));
}

Processor::~Processor() {
  std::cout<<m_description<<" destructure PSID = "<<m_instance_n<<std::endl;
  if(m_th_hub.joinable()){
    m_hub_go_stop = true;
    m_th_hub.join();
  }
  if(m_th_csm.joinable()){
    m_csm_go_stop = true;
    m_th_csm.join();
  }
};

void Processor::ProcessEvent(EventSPC ev){
  ForwardEvent(ev);
}

void Processor::Processing(EventSPC ev){
  if(m_th_csm.joinable()){
    AsyncProcessing(ev);
  }
  else{
    ProcessEvent(ev);
  }
}

void Processor::AsyncProcessing(EventSPC ev){
  std::unique_lock<std::mutex> lk(m_mtx_csm);
  if(m_csm_go_stop){
    lk.unlock();
    //TODO, wait for m_que_csm to be empty!
    ProcessEvent(ev);
    return;
  }
  m_que_csm.push_back(ev);
  m_cv_csm.notify_all();
}

void Processor::ForwardEvent(EventSPC ev) {
  std::lock_guard<std::mutex> lk(m_mtx_config);
  auto ps_hub = m_ps_hub.lock();
  //if(!ps_hub) error
  uint32_t evid = ev->GetEventID();
  for(auto &psev: m_ps_downstream){
    auto evset = psev.second;
    if(evset.find(evid)!=evset.end()){
      ps_hub->RegisterProcessing(psev.first, ev);
    }
  }
}

void Processor::RegisterEvent(EventSPC ev) {
  auto ps_hub = m_ps_hub.lock();
  //if(!ps_hub) error
  ps_hub->RegisterProcessing(shared_from_this(), ev);
}

void Processor::AddNextProcessor(ProcessorSP ps){
  std::lock_guard<std::mutex> lk(m_mtx_config);
  m_ps_downstream.push_back(std::make_pair(ps, m_evlist_white));
  m_evlist_white.clear();
  
  ProcessorSP ps_this = shared_from_this();
  
  if(!m_ps_hub.lock())
    m_ps_hub = ps_this;
  
  std::cout<<"append PS "<< ps->m_instance_n<< "to PS "<<m_instance_n<<std::endl; 
  ps->AddUpstream(ps_this);
  ps->UpdatePSHub(m_ps_hub);
}

void Processor::AddUpstream(ProcessorWP ps){
  m_ps_upstream.push_back(ps);
}

void Processor::UpdatePSHub(ProcessorWP ps){
  if(m_ps_upstream.size() < 2){
    if(m_ps_hub.lock() == ps.lock())
      return; //break circle
    std::cout<<"Update PSHUB of PS"<<m_instance_n<<" to "<<ps.lock()->m_instance_n<<std::endl; 
    m_ps_hub = ps;
    for(auto &e: m_ps_downstream){
      e.first->UpdatePSHub(m_ps_hub);
    }
  }
  else{
    if(m_ps_hub.lock() == shared_from_this())
      return; //break circle
    m_ps_hub = shared_from_this();
    RunHubThread();
    std::cout<<"Update PSHUB of PS"<<m_instance_n<<" to itself"<<std::endl;     
    for(auto &e: m_ps_downstream){
      e.first->UpdatePSHub(m_ps_hub);
    }
  }
}

void Processor::RegisterProcessing(ProcessorSP ps, EventSPC ev){ //NOTE:: return ps_hub_wk if not sucess
  std::unique_lock<std::mutex> lk(m_mtx_hub);
  if(m_hub_go_stop){
    lk.unlock();
    return; //register to new hub_ps of that ps
  }
  m_que_hub.push_back(std::make_pair(ps, ev));
  m_cv_hub.notify_all();
}

void Processor::HubProcessing(){
  while(!m_hub_go_stop){
    std::unique_lock<std::mutex> lk(m_mtx_hub);
    if(m_que_hub.empty()){
      if(m_cv_hub.wait_for(lk,std::chrono::microseconds(100))==std::cv_status::timeout)
	continue;
    }
    ProcessorSP ps = m_que_hub.front().first;
    EventSPC ev(m_que_hub.front().second);
    m_que_hub.pop_front();
    lk.unlock();
    ps->Processing(ev);
  }
  std::unique_lock<std::mutex> lk(m_mtx_hub);
  for(auto &ps_ev: m_que_hub){
    ps_ev.first->Processing(ps_ev.second);
  }
  m_que_hub.clear();
}

void Processor::ConsumeEvent(){
  while(!m_csm_go_stop){
    std::unique_lock<std::mutex> lk(m_mtx_csm);
    if(m_que_csm.empty()){
      if(m_cv_csm.wait_for(lk,std::chrono::microseconds(100))==std::cv_status::timeout)
	continue;
    }
    EventSPC ev(m_que_csm.front());
    m_que_csm.pop_front();
    lk.unlock();
    ProcessEvent(ev);
  }
  std::unique_lock<std::mutex> lk(m_mtx_csm);
  for(auto &ev: m_que_csm){
    ProcessEvent(ev);
  }
  m_que_csm.clear();
}

void Processor::RunConsumerThread(){
  m_th_csm = std::thread(&Processor::ConsumeEvent, this);
}


void Processor::RunProducerThread(){
  m_th_pdc = std::thread(&Processor::ProduceEvent, this);
  m_th_pdc.detach();
}

void Processor::RunHubThread(){
  if(!m_ps_hub.lock())
    m_ps_hub = shared_from_this();
  m_th_hub = std::thread(&Processor::HubProcessing, this);
}

void Processor::ProcessSysCmd(const std::string& cmd, const std::string& arg){
  switch(str2hash(cmd)){
  case cstr2hash("SYS:PD:RUN"):{
    RunProducerThread();
    break;
  }
  case cstr2hash("SYS:CS:RUN"):{
    RunConsumerThread();
    break;
  }
  case cstr2hash("SYS:HB:RUN"):{
    RunHubThread();
    break;
  }

  case cstr2hash("SYS:PD:STOP"):{
    //TODO, setflag
    break;
  }
  case cstr2hash("SYS:CS:STOP"):{
    //TODO, setflag
    break;
  }
  case cstr2hash("SYS:SLEEP"):{
    uint32_t msec = std::stoul(arg);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    break;
  }
  case cstr2hash("SYS:EVTYPE:ADD"):{
    std::stringstream ss(arg);
    std::string evtype;
    while(getline(ss, evtype, ',')){
      if(evtype.front()=='_')
	m_evlist_white.insert(Event::str2id(evtype));
      else
	m_evlist_white.insert(str2hash(evtype));
    }
    break;
  }
  case cstr2hash("SYS:EVTYPE:DEL"):{
    std::stringstream ss(arg);
    std::string evtype;
    while(getline(ss, evtype, ',')){
      if(evtype.front()=='_')
	m_evlist_white.erase(Event::str2id(evtype));
      else
	m_evlist_white.erase(str2hash(evtype));
    }
    break;
  }

  case cstr2hash("SYS:PSID"):{
    std::stringstream ss(arg);
    ss>>m_instance_n;
    break;
  }
  default:{
    ProcessCommand(cmd, arg);
    //TODO user
    break;
  }
  }
}

void Processor::Print(std::ostream &os, uint32_t offset) const{
  os << std::string(offset, ' ') << "<Processor>\n";
  os << std::string(offset + 2, ' ') << "<Type> " << m_description <<" </Type>\n";
  os << std::string(offset + 2, ' ') << "<ID> " << m_instance_n << " </ID>\n";
  os << std::string(offset + 2, ' ') << "<HubID> " << m_ps_hub.lock()->m_instance_n << " </HubID>\n";
  if(!m_ps_upstream.empty()){
    os << std::string(offset + 2, ' ') << "<Upstreams> \n";
    for (auto &pswp: m_ps_upstream){
      os << std::string(offset+4, ' ') << "<Processor> "<< pswp.lock()->m_description << "=" << pswp.lock()->m_instance_n << " </Processor>\n";
    }
    os << std::string(offset + 2, ' ') << "</Upstreams> \n";
  }
  if(!m_ps_downstream.empty()){
    os << std::string(offset + 2, ' ') << "<Downstreams> \n";
    for (auto &psev: m_ps_downstream){
      os << std::string(offset+4, ' ') << "<Processor> "<< psev.first->m_description << "=" << psev.first->m_instance_n << " </Processor>\n";
    }
    os << std::string(offset + 2, ' ') << "</Downstreams> \n";
  }
  os << std::string(offset, ' ') << "</Processor>\n";
}


ProcessorSP Processor::operator>>(ProcessorSP psr){
  AddNextProcessor(psr);
  return psr;
}

ProcessorSP Processor::operator<<(const std::string& cmd){
  auto dm = cmd.find_first_of('=');
  if(dm!=std::string::npos&& dm!=cmd.size()-1)
    ProcessSysCmd(cmd.substr(0, dm), cmd.substr(dm+1));
  else
    ProcessSysCmd(cmd.substr(0, dm), "");
  return shared_from_this();
}

ProcessorSP Processor::operator+(const std::string& evtype){
  uint32_t evid; 
  if(evtype.front()=='_')
    evid=Event::str2id(evtype);
  else
    evid=str2hash(evtype);
  m_evlist_white.insert(evid);
  return shared_from_this();
}

ProcessorSP Processor::operator-(const std::string& evtype){
  uint32_t evid; 
  if(evtype.front()=='_')
    evid=Event::str2id(evtype);
  else
    evid=str2hash(evtype);
  m_evlist_white.erase(evid);
  return shared_from_this();
}

ProcessorSP Processor::operator<<=(EventSPC ev){
  auto ps_hub = m_ps_hub.lock();
  auto ps_this = shared_from_this();
  if(ps_hub)
     ps_hub->RegisterProcessing(ps_this, ev);
  return ps_this;
}
