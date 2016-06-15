#include"Processor.hh"

#include"ProcessorManager.hh"

using namespace eudaq;

namespace{
  // static eudaq::RegisterDerived<Processor, typename std::string, DERIVED, uint32_t, uint32_t> reg##DERIVED(DERIVED_ID);
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
  // std::unique_lock<std::mutex> lk_state(m_mtx_state);
  if(m_state >= STATE_THREAD_UNCONF){
    AsyncProcessing(std::move(ev));
  }
  else{
    SyncProcessing(std::move(ev));
  }
}

// void Processor::SyncProcessing(EVUP ev, uint32_t psid_caller){
void Processor::SyncProcessing(EVUP ev){
  auto evtype = ev->GetSubType();
  if(evtype == "SYSTEM")
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



void Processor::AddNextProcessor(PSSP ps){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  m_pslist_next.push_back(ps);  
}


void Processor::ProduceEvent(){
  //TODO::
  //producer event from hardware or network
  //call Processing
}


void Processor::RegisterProcessing(PSSP ps, EVUP ev){
  std::unique_lock<std::mutex> lk_pcs(m_mtx_pcs);
  m_fifo_pcs.push(std::make_pair(ps, std::move(ev)));
  m_cv_pcs.notify_all();
}


void Processor::SetPSHub(PSSP ps){
  m_ps_hub = ps;
}

void Processor::HubThread(){
  
  while(1){ //TODO: modify STATE enum
    std::unique_lock<std::mutex> lk(m_mtx_pcs);
    bool fifoempty = m_fifo_pcs.empty();
    if(fifoempty)
      m_cv_pcs.wait(lk);
    PSSP ps = m_fifo_pcs.front().first;
    EVUP ev = std::move(m_fifo_pcs.front().second);
    m_fifo_pcs.pop();
    ps->Processing(std::move(ev));
  }
}

void Processor::CustomerThread(){
  // std::unique_lock<std::mutex> lk_state(m_mtx_state);
  m_state = STATE_THREAD_READY;
  // lk_state.unlock();
  while(m_state>=STATE_THREAD_UNCONF){
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


void Processor::operator()(){
  CustomerThread();
}


PSSP operator>>(Processor* psl, PSSP psr){
  psl->AddNextProcessor(psr);
  psr->IncreaseNumUpstream();
  uint32_t nup = psr->GetNumUpstream();
  if(nup == 1){
    psr->SetPSHub(psl->GetPSHub());
  }
  if(nup == 2){
    psr->SetPSHub(psr);
    std::thread hub(&Processor::HubThread, psr);
    hub.detach();
  }
  
  return psr;
}


PSSP operator>>(ProcessorManager* pm, PSSP psr){
  psr->SetPSHub(psr);
  std::thread hub(&Processor::HubThread, psr);
  hub.detach();
}


PSSP operator>>(PSSP psl, PSSP psr){
  auto pslp = psl.get();
  //if not null
  return pslp>>psr;
}


PSSP operator>>(PSSP psl, std::string psr_str){
  std::string pstype = psr_str;  //TODO: pst_str contains more info, then decode is needed.
  uint32_t psid = 1; //TODO: get from psr_str
  auto psMan = ProcessorManager::GetInstance();
  PSSP psr = psMan->CreateProcessor(pstype, psid);
  return psl>>psr;
}


// static Processor ps("__base",std::numeric_limits<uint32_t>::max(), 0)
