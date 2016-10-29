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
  ps->m_ps_hub = ps;
  ps->m_th_hub = std::thread(&Processor::HubProcessing, ps.get());
  for(auto &p: l){
    ps->ProcessSysCommand(p.first, p.second);
  }
  return ps;
}


Processor::Processor(const std::string& dsp)
  :m_description(dsp), m_csm_go_stop(0), m_hub_go_stop(0), m_hub_force(0){
  m_instance_n = static_cast<uint32_t>(reinterpret_cast<uint64_t>(this));
}

Processor::~Processor() {
  std::cout<<m_description<<" destructure PSID = "<<m_instance_n<<std::endl;
  if(m_th_hub.joinable()){
    m_hub_go_stop = true;
    m_th_hub.join();
    m_hub_go_stop = false;
  }
  if(m_th_csm.joinable()){
    m_csm_go_stop = true;
    m_th_csm.join();
    m_csm_go_stop = false;
  }
};

void Processor::ProcessEvent(EventSPC ev){
  ForwardEvent(ev);
}

void Processor::Processing(EventSPC ev){
  if(m_th_csm.joinable()){
    BufferEvent(ev);
  }
  else{
    ProcessEvent(ev);
  }
}

void Processor::BufferEvent(EventSPC ev){
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
  ps_hub->RegisterProcessing(shared_from_this(), ev);
}

void Processor::AddDownstream(ProcessorSP ps){
  std::lock_guard<std::mutex> lk(m_mtx_config);
  m_ps_downstream.push_back(std::make_pair(ps, m_evlist_white));
  m_evlist_white.clear();
  std::cout<<"append PS "<< ps->m_instance_n<< "to PS "<<m_instance_n<<std::endl; 
  ps->m_ps_upstream.push_back(shared_from_this());
  ps->UpdateHub(m_ps_hub);
}


void Processor::UpdateHub(ProcessorWP ps){
  //forced
  if(m_hub_force)
    return;
  //unforced, same
  if(m_ps_hub.lock() == ps.lock())
    return;
  //unforced, diff, single upstream
  if(m_ps_upstream.size() < 2){
    bool to_stop = false;
    //unforced, diff, single upstream, self hub
    if(m_ps_hub.lock() == shared_from_this()){
      to_stop = true;
    }
    //else, unforced, diff, single upstream, other hub
    m_ps_hub = ps;
    for(auto &e: m_ps_downstream){
      e.first->UpdateHub(ps);
    }
    if(to_stop && m_th_hub.joinable()){
	m_hub_go_stop = true;
	m_th_hub.join();
    }
    return;
  }
  //unforced, diff, muilt upstreams, self hub ready
  if(m_ps_hub.lock() == shared_from_this()){
    return;
  }
  //unforced, diff, muilt upstreams, self hub unready  
  m_th_hub = std::thread(&Processor::HubProcessing, this);
  m_ps_hub = shared_from_this();
  for(auto &e: m_ps_downstream){
    e.first->UpdateHub(m_ps_hub);
  }
  return;
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

void Processor::ProcessSysCommand(const std::string& cmd, const std::string& arg){
  switch(str2hash(cmd)){
  case cstr2hash("SYS:PD:RUN"):{
    m_th_pdc = std::thread(&Processor::ProduceEvent, this);
    m_th_pdc.detach();
    break;
  }
  case cstr2hash("SYS:CS:RUN"):{
    m_th_csm = std::thread(&Processor::ConsumeEvent, this);
    break;
  }
  case cstr2hash("SYS:CS:STOP"):{
    if(m_th_csm.joinable()){
      m_csm_go_stop = true;
      m_th_csm.join();
      m_csm_go_stop = false;
    }
    break;
  }
  case cstr2hash("SYS:SLEEP"):{
    uint32_t msec = std::stoul(arg);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    break;
  }
  case cstr2hash("SYS:EV:ADD"):{
    if(arg.front()=='_')
      m_evlist_white.insert(Event::str2id(arg));
    else
      m_evlist_white.insert(str2hash(arg));
    break;
  }
  case cstr2hash("SYS:EV:DEL"):{
    if(arg.front()=='_')
      m_evlist_white.erase(Event::str2id(arg));
    else
      m_evlist_white.erase(str2hash(arg));
    break;
  }
  case cstr2hash("SYS:PSID"):{
    m_instance_n = std::stoul(arg);
    break;
  }
  default:{
    ProcessCommand(cmd, arg);
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
  AddDownstream(psr);
  return psr;
}

ProcessorSP Processor::operator<<(const std::string& cmd){
  auto dm = cmd.find_first_of('=');
  if(dm!=std::string::npos&& dm!=cmd.size()-1)
    ProcessSysCommand(cmd.substr(0, dm), cmd.substr(dm+1));
  else
    ProcessSysCommand(cmd.substr(0, dm), "");
  return shared_from_this();
}

ProcessorSP Processor::operator+(const std::string& evtype){
  ProcessSysCommand("SYS:EV:ADD",evtype);
  return shared_from_this();
}

ProcessorSP Processor::operator-(const std::string& evtype){
  ProcessSysCommand("SYS:EV:DEL",evtype);
  return shared_from_this();
}

ProcessorSP Processor::operator<<=(EventSPC ev){
  auto ps_hub = m_ps_hub.lock();
  auto ps_this = shared_from_this();
  if(ps_hub)
     ps_hub->RegisterProcessing(ps_this, ev);
  return ps_this;
}
