#include"Processor.hh"
#include"Utils.hh"

#include <string>
#include <chrono>
#include <thread>
#include <sstream>

using namespace eudaq;

template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>& Factory<Processor>::Instance<std::string&>();
template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>& Factory<Processor>::Instance<std::string&&>();
template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)()>& Factory<Processor>::Instance<>();

ProcessorSP Processor::MakeShared(std::string pstype, std::string cmd){
  std::string cmd_no;
  ProcessorSP ps(std::move(Factory<Processor>::Create(str2hash(pstype), cmd_no)));
  ps->ProcessCmd(cmd);
  return ps;
}

Processor::Processor(std::string pstype, std::string cmd)
  :m_pstype(pstype), m_psid(0){
}

Processor::~Processor() {
  std::cout<<m_pstype<<" destructure PSID = "<<m_psid<<std::endl;
  if(m_th_hub.joinable()){
    m_hub_go_stop = true;
    m_th_hub.join();
  }
  if(m_th_csm.joinable()){
    m_csm_go_stop = true;
    m_th_csm.join();
  }
};

void Processor::ProcessUserEvent(EventSPC ev){
  ForwardEvent(ev);
}

void Processor::ProcessSysEvent(EventSPC ev){
  ForwardEvent(ev);
}

void Processor::Processing(EventSPC ev){
  if(m_th_csm.joinable()){
    AsyncProcessing(ev);
  }
  else{
    SyncProcessing(ev);
  }
}

void Processor::SyncProcessing(EventSPC ev){
  auto evtype = ev->GetEventID();
  if(evtype == 0)//sys
    ProcessSysEvent(ev);
  else
    ProcessUserEvent(ev);
}

void Processor::AsyncProcessing(EventSPC ev){
  std::unique_lock<std::mutex> lk(m_mtx_csm);
  if(m_csm_go_stop){
    lk.unlock();
    SyncProcessing(ev);
    return;
  }
  m_que_csm.push_back(ev);
  m_cv_csm.notify_all();
}

void Processor::ForwardEvent(EventSPC ev) {
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  auto ps_hub = m_ps_hub.lock();
  //if(!ps_hub) error
  uint32_t evid = ev->GetEventID();
  for(auto &psev: m_pslist_next){
    auto evset = psev.second;
    if(evset.find(evid)!=evset.end()){
      ps_hub->RegisterProcessing(psev.first, ev);
    }
  }
}

void Processor::AddNextProcessor(ProcessorSP ps){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  m_pslist_next.push_back(std::make_pair(ps, m_evlist_white));
  m_evlist_white.clear();
  
  ProcessorSP ps_this = shared_from_this();
  
  if(!m_ps_hub.lock())
    m_ps_hub = ps_this;
  
  std::cout<<"append PS "<< ps->GetID()<< "to PS "<<GetID()<<std::endl; 
  ps->AddUpstream(ps_this);
  ps->UpdatePSHub(m_ps_hub);

}

void Processor::AddUpstream(PSWP ps){
  m_ps_upstr.push_back(ps);
}

void Processor::UpdatePSHub(PSWP ps){
  if(m_ps_upstr.size() < 2){
    if(m_ps_hub.lock() == ps.lock())
      return; //break circle
    std::cout<<"Update PSHUB of PS"<<m_psid<<" to "<<ps.lock()->GetID()<<std::endl; 
    m_ps_hub = ps;
    for(auto &e: m_pslist_next){
      e.first->UpdatePSHub(m_ps_hub);
    }
  }
  else{
    if(m_ps_hub.lock() == shared_from_this())
      return; //break circle
    m_ps_hub = shared_from_this();
    RunHubThread();
    std::cout<<"Update PSHUB of PS"<<m_psid<<" to itself"<<std::endl;     
    for(auto &e: m_pslist_next){
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
    SyncProcessing(ev);
  }
  std::unique_lock<std::mutex> lk(m_mtx_csm);
  for(auto &ev: m_que_csm){
    SyncProcessing(ev);
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

void Processor::ProcessSysCmd(const std::string& cmd_name, const std::string& cmd_par){
  switch(str2hash(cmd_name)){
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
  // case cstr2hash("SYS:HB:STOP"):{ //HB is managed automaticly
  //   if(m_th_hub.joinable()){
  //     m_hub_go_stop = true;
  //     m_th_hub.join();
  //   }
  //   break;
  // }

  case cstr2hash("SYS:PD:STOP"):{
    //TODO, setflag
    break;
  }
  case cstr2hash("SYS:CS:STOP"):{
    //TODO, setflag
    break;
  }
  case cstr2hash("SYS:SLEEP"):{
    uint32_t msec = std::stoul(cmd_par);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    break;
  }
  case cstr2hash("SYS:EVTYPE:ADD"):{
    std::stringstream ss(cmd_par);
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
    std::stringstream ss(cmd_par);
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
    std::stringstream ss(cmd_par);
    ss>>m_psid;
    break;
  }
  }
}

void Processor::ProcessCmd(const std::string& cmd_list){
  //val1=1,2,3;val2=xx,yy;SYS:val3=abc
  std::stringstream ss(cmd_list);
  std::string cmd;
  while(getline(ss, cmd, ';')){//TODO: ProcessXXCmd(name_str,val_str)
    std::stringstream ss_cmd(cmd);
    std::string name_str, val_str;
    getline(ss_cmd, name_str, '=');
    getline(ss_cmd, val_str);
    name_str=trim(name_str);
    val_str=trim(val_str);
    if(!name_str.empty()){
      name_str=ucase(name_str);
      if(!cmd.compare(0,3,"SYS")){
	ProcessSysCmd(name_str, val_str);
      }
      else
	ProcessUsrCmd(name_str, val_str);
    }
  }
}


void Processor::Print(std::ostream &os, uint32_t offset) const{
  os << std::string(offset, ' ') << "<Processor>\n";
  os << std::string(offset + 2, ' ') << "<Type> " << m_pstype <<" </Type>\n";
  os << std::string(offset + 2, ' ') << "<ID> " << m_psid << " </ID>\n";
  os << std::string(offset + 2, ' ') << "<HubID> " << m_ps_hub.lock()->GetID() << " </HubID>\n";
  if(!m_ps_upstr.empty()){
    os << std::string(offset + 2, ' ') << "<Upstreams> \n";
    for (auto &pswp: m_ps_upstr){
      os << std::string(offset+4, ' ') << "<Processor> "<< pswp.lock()->GetType() << "=" << pswp.lock()->GetID() << " </Processor>\n";
    }
    os << std::string(offset + 2, ' ') << "</Upstreams> \n";
  }
  if(!m_pslist_next.empty()){
    os << std::string(offset + 2, ' ') << "<Downstreams> \n";
    for (auto &psev: m_pslist_next){
      os << std::string(offset+4, ' ') << "<Processor> "<< psev.first->GetType() << "=" << psev.first->GetID() << " </Processor>\n";
    }
    os << std::string(offset + 2, ' ') << "</Downstreams> \n";
  }
  os << std::string(offset, ' ') << "</Processor>\n";
}


ProcessorSP Processor::operator>>(ProcessorSP psr){
  AddNextProcessor(psr);
  return psr;
}

ProcessorSP Processor::operator>>(const std::string& stream_str){
  std::string cmd_str;
  std::string par_str;
  std::stringstream ss(stream_str);
  getline(ss, cmd_str, '(');
  getline(ss, par_str, ')');
  cmd_str=trim(cmd_str);
  std::string ps_type = cmd_str; //TODO
  ps_type=trim(ps_type);
  ProcessorSP psr(Factory<Processor>::Create(str2hash(ps_type), par_str));
  AddNextProcessor(psr);
  return psr;
}

ProcessorSP Processor::operator<<(const std::string& cmd_list){
  ProcessCmd(cmd_list);
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
