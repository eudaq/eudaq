#include"ProcessorManager.hh"

#include"Processor.hh"

using namespace eudaq;
using std::shared_ptr;
using std::unique_ptr;

shared_ptr<ProcessorManager> ProcessorManager::GetInstance(){
  static shared_ptr<ProcessorManager>  singleton;
  if(!singleton){
    singleton = std::make_shared<ProcessorManager> ();  
  }
  shared_ptr<ProcessorManager> rt = singleton;
  return rt;
}

ProcessorManager::ProcessorManager(){
  InitProcessorPlugins();
}

void ProcessorManager::InitProcessorPlugins(){
  //TODO:: search shared lib, get PS creater and destroyer, prepare  PSplugin map
};

PSSP ProcessorManager::CreateProcessor(std::string pstype, uint32_t psid, uint32_t psid_mother){
  auto it = m_pslist.find(pstype);
  if(it!=m_pslist.end()){
    auto creater= it->second.first;
    auto destroyer= it->second.second;
    // PSSP ps((*creater)(), destroyer);
    PSSP ps((*creater)(psid, psid_mother));
    m_pslist_instance[psid]=ps;
    return ps;
  }
  else{
    std::cout<<"no pstype "<<pstype<<std::endl;
    PSSP ps;
    return ps;
  }
}


EVUP ProcessorManager::CreateEvent(std::string evtype){
  auto it = m_evlist.find(evtype);
  if(it!=m_evlist.end()){
    auto creater= it->second.first;
    auto destroyer= it->second.second;
    // EVUP ev((*creater)(), destroyer);
    EVUP ev((*creater)());
    return ev;
  }
  else{
    std::cout<<"no evtype "<<evtype<<std::endl;
    EVUP ev;
    return ev;
  }
}

void ProcessorManager::RegisterProcessorType(std::string pstype, CreatePS c, DestroyPS d ){
  m_pslist[pstype]=std::make_pair(c, d);
}

void ProcessorManager::RegisterEventType(std::string evtype, CreateEV c, DestroyEV d ){
  m_evlist[evtype]=std::make_pair(c, d);
}

void ProcessorManager::RegisterProcessing(PSSP ps, EVUP ev){
  std::lock_guard<std::mutex> lk(m_mtx_fifo);
  m_fifo_events.push(std::make_pair(ps, std::move(ev)));
}

void ProcessorManager::RegisterProcessing(uint32_t psid, EVUP ev){
  std::lock_guard<std::mutex> lk(m_mtx_fifo);
  auto it = m_pslist_instance.find(psid);
  if(it!=m_pslist_instance.end()){
    PSSP ps=it->second;
    m_fifo_events.push(std::make_pair(ps, std::move(ev)));
  }
  else{
    std::cout<<"no psid "<<psid<<std::endl;
  }
}

void ProcessorManager::EventLoop(){
  while(1){
    if(!m_fifo_events.empty()){
      m_fifo_events.front().first->Processing(std::move(m_fifo_events.front().second));
      m_fifo_events.pop();
    }
  }
}
