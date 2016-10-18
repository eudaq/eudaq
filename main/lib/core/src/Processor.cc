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
  ProcessorSP ps(std::move(Factory<Processor>::Create(str2hash(pstype), "")));
  ps->ProcessCmd(cmd);
  return ps;
}

Processor::Processor(std::string pstype, std::string cmd)
  :m_pstype(pstype), m_psid(0), m_state(STATE_READY), m_flag(0){
}

Processor::~Processor() {
  std::cout<<m_pstype<<" destructure PSID = "<<m_psid<<std::endl;
};

void Processor::ProcessUserEvent(EventUP ev){
  ForwardEvent(std::move(ev));
}

void Processor::ProcessSysEvent(EventUP ev){
  ForwardEvent(std::move(ev));
}

void Processor::Processing(EventUP ev){
  if(IsAsync()){
    AsyncProcessing(std::move(ev));
  }
  else{
    SyncProcessing(std::move(ev));
  }
}

void Processor::SyncProcessing(EventUP ev){
  auto evtype = ev->GetEventID();
  if(evtype == 0)//sys
    ProcessSysEvent(std::move(ev));
  else
    ProcessUserEvent(std::move(ev));
}

void Processor::AsyncProcessing(EventUP ev){
  std::unique_lock<std::mutex> lk(m_mtx_fifo);
  m_fifo_events.push(std::move(ev));
  m_cv.notify_all();
}

void Processor::ForwardEvent(EventUP ev) {
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  std::vector<ProcessorSP> pslist;
  uint32_t evid = ev->GetEventID();
  for(auto &psev: m_pslist_next){
    auto evset = psev.second;
    if(evset.find(evid)!=evset.end()){
      pslist.push_back(psev.first);
    }
  }
  size_t remain_ps = pslist.size();
  auto ps_hub = m_ps_hub.lock();
  if(ps_hub){
    for(auto &ps: pslist){
      if(remain_ps == 1)
	ps_hub->RegisterProcessing(ps, std::move(ev));
      else{
	ps_hub->RegisterProcessing(ps, ev->Clone());
      }
      remain_ps--;
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

void Processor::ProduceEvent(){
}

void Processor::RegisterProcessing(ProcessorSP ps, EventUP ev){
  std::unique_lock<std::mutex> lk_pcs(m_mtx_pcs);
  m_fifo_pcs.push(std::make_pair(ps, std::move(ev)));
  m_cv_pcs.notify_all();
}


void Processor::HubProcessing(){
  while(IsHub()){ //TODO: modify STATE enum
    std::unique_lock<std::mutex> lk(m_mtx_pcs);
    bool fifoempty = m_fifo_pcs.empty();
    if(fifoempty){
      m_cv_pcs.wait(lk);
    }
    ProcessorSP ps = m_fifo_pcs.front().first;
    EventUP ev = std::move(m_fifo_pcs.front().second);
    m_fifo_pcs.pop();
    lk.unlock();
    ps->Processing(std::move(ev));
  }
}

void Processor::ConsumeEvent(){
  while(IsAsync()){
    std::unique_lock<std::mutex> lk(m_mtx_fifo);
    bool fifoempty = m_fifo_events.empty();
    if(fifoempty)
      m_cv.wait(lk);
    EventUP ev = std::move(m_fifo_events.front());
    m_fifo_events.pop();
    lk.unlock();
    SyncProcessing(std::move(ev));
  }
}

void Processor::RunConsumerThread(){
  std::thread t(&Processor::ConsumeEvent, this);
  m_flag = m_flag|FLAG_CSM_RUN;//safe
  t.detach();
}


void Processor::RunProducerThread(){
  std::thread t(&Processor::ProduceEvent, this);
  m_flag = m_flag|FLAG_PDC_RUN;//safe
  t.detach();
}

void Processor::RunHubThread(){
  if(!m_ps_hub.lock())
    m_ps_hub = shared_from_this();
  std::thread t(&Processor::HubProcessing, this);
  m_flag = m_flag|FLAG_HUB_RUN;//safe
  t.detach();
}

void Processor::ProcessSysCmd(std::string cmd_name, std::string cmd_par){
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

void Processor::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  m_cmdlist_init.push_back(std::make_pair(cmd_name, cmd_par)); //configured
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

ProcessorSP Processor::operator<<=(EventUP ev){
  auto ps_hub = m_ps_hub.lock();
  auto ps_this = shared_from_this();
  if(ps_hub)
     ps_hub->RegisterProcessing(ps_this, std::move(ev));
  return ps_this;
}
