#include"Processor.hh"

#include"ProcessorManager.hh"

using namespace eudaq;

namespace{
  // static eudaq::RegisterDerived<Processor, typename std::string, DERIVED, uint32_t, uint32_t> reg##DERIVED(DERIVED_ID);
  void DUMMY_FUNCTION_DO_NOT_USE_PROCESSOR(){
    std::string pstype = "_DUMMY_";
    uint32_t psid = 0;
    uint32_t psid_mother = 0;
    eudaq::ClassFactory<Processor, typename std::string, uint32_t, uint32_t>::Create(pstype, psid, psid_mother );
    eudaq::ClassFactory<Processor, typename std::string, uint32_t, uint32_t>::GetTypes();
    eudaq::ClassFactory<Processor, typename std::string, uint32_t, uint32_t>::GetInstance();
  }
}



Processor::Processor(std::string pstype, uint32_t psid, uint32_t psid_mother)
  :m_pstype(pstype), m_psid(psid), m_psid_mother(psid_mother), m_state(STATE_READY){
}

void Processor::ProcessUserEvent(EVUP ev){
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

void Processor::ProcessCmdEvent(EVUP ev){
  //  TODO config
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
  else if(evtype == "COMMAND")
    ProcessCmdEvent(std::move(ev));
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
  size_t remain_ps;
  auto evtype = ev->GetSubType();
  if(evtype == "SYSTEM" || evtype == "COMMAND")
    remain_ps = m_pslist_next.size();
  else
    remain_ps = m_pslist_next.size()+m_pslist_aux.size();
  
  auto psMan = ProcessorManager::GetInstance();
  for(auto &ps: m_pslist_next){
    if(remain_ps == 1)
      psMan->RegisterProcessing(ps, std::move(ev));
    else{
      // EVUP evcp(ev->Clone(), ev.get_deleter());
      EVUP evcp;
      psMan->RegisterProcessing(ps, std::move(evcp));
    }
    remain_ps--;
  }
  if(!remain_ps)
    return;
  for(auto &psid: m_pslist_aux){
    if(remain_ps == 1)
      psMan->RegisterProcessing(psid, std::move(ev));
    else{
      // EVUP evcp(ev->Clone(), ev.get_deleter());
      EVUP evcp;
      psMan->RegisterProcessing(psid, std::move(evcp));
    }
    remain_ps--;
  }
}


void Processor::CreateNextProcessor(std::string pstype, uint32_t psid){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  auto psMan = ProcessorManager::GetInstance();
  PSSP ps = psMan->CreateProcessor(pstype, psid, m_psid);
  if(ps){
    m_pslist_next.push_back(ps);
  }
}


void Processor::AddAuxProcessor(uint32_t psid){
  std::lock_guard<std::mutex> lk_list(m_mtx_list);
  m_pslist_aux.push_back(psid);
}

void Processor::ProduceEvent(){
  //TODO::
  //producer event from hardware or network
  //call Processing
}

void Processor::operator()(){
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

// static Processor ps("__base",std::numeric_limits<uint32_t>::max(), 0)
