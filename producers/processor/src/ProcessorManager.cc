#include"ProcessorManager.hh"

#include"Processor.hh"

using namespace eudaq;
using std::shared_ptr;
using std::unique_ptr;

ProcessorManager* ProcessorManager::GetInstance(){
  static ProcessorManager singleton;
  return &singleton;
}

ProcessorManager::ProcessorManager(){
  InitProcessorPlugins();
}


void ProcessorManager::InitProcessorPlugins(){
  //TODO:: search shared lib, get PS creater and destroyer, prepare  PSplugin map
};

PSSP ProcessorManager::CreateProcessor(std::string pstype, uint32_t psid){
  PSSP ps(std::move(ProcessorClassFactory::Create(pstype, psid)));
  return ps;
}


PSSP ProcessorManager::MakePSSP(std::string pstype, std::string cmd){
  PSSP ps(std::move(PSFactory::Create(pstype, cmd)));
  return ps;
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

PSSP ProcessorManager::operator>>(PSSP psr){
  //TODO: CP shared ptr to somewhere to prevent the destrure
  psr->SetPSHub(psr);
  psr->RunHubThread();
  return psr;
}
