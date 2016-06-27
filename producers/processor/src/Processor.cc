#include"Processor.hh"

#include"ProcessorManager.hh"

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



Processor::Processor(std::string pstype, uint32_t psid)
  :m_pstype(pstype), m_psid(psid), m_state(STATE_READY){
  m_num_upstream = 0;
  m_evlist_white.insert(0);
}

Processor::~Processor() {
  std::cout<<"destructure PSID = "<<m_psid<<std::endl;
};

void Processor::InsertEventType(uint32_t evtype){
  m_evlist_white.insert(evtype);
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
  //todo: check white list
  // if(m_evlist_white.find(ev->get_id())!=m_evlist_white.end()){
    if(IsAsync()){
      AsyncProcessing(std::move(ev));
    }
    else{
      SyncProcessing(std::move(ev));
    }
  // }
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
  size_t remain_ps = m_pslist_next.size();
  auto psMan = ProcessorManager::GetInstance();
  for(auto &ps: m_pslist_next){
    if(remain_ps == 1)
      m_ps_hub->RegisterProcessing(ps, std::move(ev));
    else{
      // EVUP evcp(ev->Clone(), ev.get_deleter());
      EVUP evcp;
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
    m_pslist_next.push_back(ps);  
    ps->SetPSHub(GetPSHub());
  }
}


std::set<uint32_t> Processor::UpdateEventWhiteList(){
  for(auto ps: m_pslist_next){
    auto evtype_list = ps->UpdateEventWhiteList();
    m_evlist_white.insert(evtype_list.begin(), evtype_list.end());    
  }
  return m_evlist_white;
}

void Processor::AddNextProcessor(PSSP ps){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  m_pslist_next.push_back(ps);  
}


void Processor::ProduceEvent(){
}


void Processor::RegisterProcessing(PSSP ps, EVUP ev){
  // std::cout<<"locking register  ";
  std::unique_lock<std::mutex> lk_pcs(m_mtx_pcs);
  // std::cout<<"locked register  ";

  m_fifo_pcs.push(std::make_pair(ps, std::move(ev)));
  m_cv_pcs.notify_all();
  // std::cout<<"unlock register  "<<std::endl;
}


void Processor::SetPSHub(PSSP ps){
  m_ps_hub = ps;
  std::cout<<"PS"<<m_psid<<" HUB"<<ps->GetID()<<std::endl;
  for(auto subps: m_pslist_next){
    if(!subps->IsHub()){
      subps->SetPSHub(m_ps_hub);
    }
  }
}

void Processor::HubProcessing(){
  while(IsHub()){ //TODO: modify STATE enum
    // std::cout<<"locking pcs  ";
    std::unique_lock<std::mutex> lk(m_mtx_pcs);
    // std::cout<<"locked pcs  ";
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




PSSP Processor::operator>>(PSSP psr){
  AddNextProcessor(psr);
  uint32_t nup = psr->GetNumUpstream();
  std::cout<< nup<<std::endl;
  if(nup == 0 && !psr->IsHub()){
    psr->SetPSHub(GetPSHub());
  }
  if(nup == 1 && !psr->IsHub()){
    psr->SetPSHub(psr);
    // psr->UpdateDownstreamHub();//TODO
    psr->RunHubThread(); //now, it is hub
  }
  psr->IncreaseNumUpstream();

  return psr;
}


Processor& Processor::operator<<(EVUP ev){
  Processing(std::move(ev));
  return *this;
}


std::set<uint32_t> SysEventBlockerPS::UpdateEventWhiteList(){
  auto evlist = Processor::UpdateEventWhiteList();
  auto it = evlist.find(0); //0 sys
  evlist.erase(it);
  return evlist;
}



PSSP operator>>(PSSP psl, PSSP psr){
  return *psl>>psr;
}


PSSP operator>>(PSSP psl, std::string psr_str){
  std::string pstype;
  uint32_t psid;
  std::stringstream ss(psr_str);
  ss>>pstype>>psid;
  auto psMan = ProcessorManager::GetInstance();
  PSSP psr = psMan->CreateProcessor(pstype, psid);
  return *psl>>psr;
}
