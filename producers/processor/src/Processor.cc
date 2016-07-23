#include"Processor.hh"

#include"Utils.hh"
#include"ProcessorManager.hh"

#include <string>
#include <chrono>
#include <thread>
#include <sstream>

using namespace eudaq;

namespace{
  void DUMMY_FUNCTION_DO_NOT_USE_PROCESSOR(){
    std::string pstype = "_DUMMY_";
    uint32_t psid = 0;
    eudaq::ClassFactory<Processor, typename std::string, uint32_t>::Create(pstype, psid);
    eudaq::ClassFactory<Processor, typename std::string, uint32_t>::GetTypes();
    eudaq::ClassFactory<Processor, typename std::string, uint32_t>::GetInstance();
  }
}

namespace{
  void DUMMY_FUNCTION_DO_NOT_USE_PROCESSOR_STR(){ //type, cmd
    eudaq::ClassFactory<Processor, typename std::string, typename std::string>::Create("_DUMMY_", "_DUMMY_");
    eudaq::ClassFactory<Processor, typename std::string, typename std::string>::GetTypes();
    eudaq::ClassFactory<Processor, typename std::string, typename std::string>::GetInstance();
  }
}



Processor::Processor(std::string pstype, uint32_t psid)
  :m_pstype(pstype), m_psid(psid), m_state(STATE_READY){
  m_num_upstream = 0;
}


Processor::Processor(std::string pstype, std::string cmd)
  :m_pstype(pstype), m_psid(0), m_state(STATE_READY){
  m_num_upstream = 0;
}


Processor::~Processor() {
  std::cout<<"destructure PSID = "<<m_psid<<std::endl;
};

void Processor::InsertEventType(uint32_t evtype){
  m_evlist_white.insert(evtype);
}

void Processor::EraseEventType(uint32_t evtype){
  m_evlist_white.erase(evtype);
}


void Processor::ProcessUserEvent(EVUP ev){
  std::cout<< "ProcessUserEvent ["<<m_psid<<"] "<<ev->GetSubType()<<std::endl;
  ForwardEvent(std::move(ev));
}


void Processor::ProcessSysEvent(EVUP ev){
  //  TODO config
  uint32_t psid_dst;
  psid_dst = ev->GetTag("PSDST", psid_dst);
  if(psid_dst == m_psid){
    if(ev->HasTag("NEWPS")){
      std::string pstype;
      pstype = ev->GetTag("PSTYPE", pstype);
      uint32_t psid;
      psid = ev->GetTag("PSID", psid);
      CreateNextProcessor(pstype, psid);
    }
    return;
  }
  ForwardEvent(std::move(ev));
}

void Processor::Processing(EVUP ev){
    if(IsAsync()){
      AsyncProcessing(std::move(ev));
    }
    else{
      SyncProcessing(std::move(ev));
    }
}

void Processor::SyncProcessing(EVUP ev){
  auto evtype = ev->get_id();
  if(evtype == 0)//sys
    ProcessSysEvent(std::move(ev));
  else
    ProcessUserEvent(std::move(ev));
}

void Processor::AsyncProcessing(EVUP ev){
  std::unique_lock<std::mutex> lk(m_mtx_fifo);
  m_fifo_events.push(std::move(ev));
  m_cv.notify_all();
}

void Processor::ForwardEvent(EVUP ev) {
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  std::vector<PSSP> pslist;
  uint32_t evid = ev->get_id();
  for(auto &psev: m_pslist_next){
    auto evset = psev.second;
    if(evset.find(evid)!=evset.end()){
      pslist.push_back(psev.first);
    }
  }
  auto psMan = ProcessorManager::GetInstance();
  size_t remain_ps = pslist.size();
  for(auto &ps: pslist){
    if(remain_ps == 1)
      m_ps_hub->RegisterProcessing(ps, std::move(ev));
    else{
      // EVUP evcp(ev->Clone(), ev.get_deleter());
      EVUP evcp; //TODO
      m_ps_hub->RegisterProcessing(ps, std::move(evcp));
    }
    remain_ps--;
  }
}


void Processor::CreateNextProcessor(std::string pstype, uint32_t psid){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  auto psMan = ProcessorManager::GetInstance();
  PSSP ps = psMan->CreateProcessor(pstype, psid);
  if(ps){
    m_pslist_next.push_back(std::make_pair(ps, m_evlist_white));
    m_evlist_white.clear();
    ps->SetPSHub(GetPSHub());
  }
}


void Processor::AddNextProcessor(PSSP ps){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  m_pslist_next.push_back(std::make_pair(ps, m_evlist_white));
  m_evlist_white.clear();
}


void Processor::ProduceEvent(){
}


void Processor::RegisterProcessing(PSSP ps, EVUP ev){
  std::unique_lock<std::mutex> lk_pcs(m_mtx_pcs);
  m_fifo_pcs.push(std::make_pair(ps, std::move(ev)));
  m_cv_pcs.notify_all();
}


void Processor::SetPSHub(PSSP ps){
  m_ps_hub = ps;
  std::cout<<"PS"<<m_psid<<" HUB"<<ps->GetID()<<std::endl;
  for(auto& psev: m_pslist_next){
    auto subps = psev.first;
    if(!subps->IsHub()){
      subps->SetPSHub(m_ps_hub);
    }
  }
}

void Processor::HubProcessing(){
  while(IsHub()){ //TODO: modify STATE enum
    std::unique_lock<std::mutex> lk(m_mtx_pcs);
    bool fifoempty = m_fifo_pcs.empty();
    if(fifoempty){
      std::cout<<"HUB"<<m_psid<<": fifo is empty, waiting"<<std::endl;
      m_cv_pcs.wait(lk);
      std::cout<<"HUB"<<m_psid<<": end of fifo waiting"<<std::endl;
    }
    PSSP ps = m_fifo_pcs.front().first;
    EVUP ev = std::move(m_fifo_pcs.front().second);
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
    EVUP ev = std::move(m_fifo_events.front());
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
  std::thread t(&Processor::HubProcessing, this);
  m_flag = m_flag|FLAG_HUB_RUN;//safe
  t.detach();
}

void Processor::ProcessSysCmd(std::string cmd_name, std::string cmd_par){
  std::cout<<"---------ProcessSysCmd "<<cmd_name<<"="<<cmd_par<<std::endl;

  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("SYS:PD:RUN"):{
    RunProducerThread();
    break;
  }
  case cstr2hash("SYS:CS:RUN"):{
    RunConsumerThread();
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
    std::stringstream ss(cmd_par);
    uint32_t msec;
    ss>>msec;
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    break;
  }
  case cstr2hash("SYS:EVTYPE:ADD"):{
    std::stringstream ss(cmd_par);
    std::string evtype;
    while(getline(ss, evtype, ',')){
      m_evlist_white.insert(Event::str2id(evtype));
    }
    break;
  }
  case cstr2hash("SYS:EVTYPE:DEL"):{
    std::stringstream ss(cmd_par);
    std::string evtype;
    while(getline(ss, evtype, ',')){
      m_evlist_white.erase(Event::str2id(evtype));
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


void Processor::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  std::cout<<"---------default ProcessSysCmd "<<cmd_name<<"="<<cmd_par<<std::endl;
  m_cmdlist_init.push_back(std::make_pair(cmd_name, cmd_par)); //configured
}


PSSP Processor::operator>>(PSSP psr){
  AddNextProcessor(psr);
  uint32_t nup = psr->GetNumUpstream();
  std::cout<< nup<<std::endl;
  if(nup == 0 && !psr->IsHub()){
    psr->SetPSHub(GetPSHub());
  }
  if(nup == 1 && !psr->IsHub()){
    psr->SetPSHub(psr);
    psr->RunHubThread(); //now, it is hub
  }
  psr->IncreaseNumUpstream();

  return psr;
}


Processor& Processor::operator<<(EVUP ev){
  Processing(std::move(ev));
  return *this;
}

Processor& Processor::operator<<(std::string cmd_list){ //TODO: decode
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

  return *this;
}


PSSP operator<<(PSSP psl, std::string cmd_list){
  *psl<<cmd_list;
  return psl;
}


PSSP operator>>(PSSP psl, PSSP psr){
  return *psl>>psr;
}

PSSP operator>>(PSSP psl, std::string stream_str){
  //pstype(par)
  //EV(ADD=ev1,ev2,ev3_)
  std::string cmd_str;
  std::string par_str;
  std::stringstream ss(stream_str);
  getline(ss, cmd_str, '(');
  getline(ss, par_str, ')');
  cmd_str=trim(cmd_str);
  if(cmd_str=="EV"){
    std::stringstream ss_par_str(par_str);
    std::string ev_str;
    while(getline(ss_par_str, ev_str, ';' )){
      std::stringstream ss_ev_str(ev_str);
      std::string ev_cmd, ev_list;
      getline(ss_ev_str, ev_cmd, '=');
      ev_cmd=trim(ev_cmd);
      getline(ss_ev_str, ev_list);
      std::stringstream ss_ev_list(ev_list);
      std::string ev_type;
      while(getline(ss_ev_list, ev_type, ',')){
	ev_type=trim(ev_type);
	if(ev_cmd=="ADD") psl->InsertEventType(Event::str2id(ev_type));
	if(ev_cmd=="DEL") psl->EraseEventType(Event::str2id(ev_type));
      }
    }
    return psl;
  }
  else{
    auto psMan = ProcessorManager::GetInstance();
    std::string ps_type = cmd_str; //TODO
    ps_type=trim(ps_type);
    PSSP psr = psMan->MakePSSP(ps_type, par_str);
    return *psl>>psr;
  }
}

