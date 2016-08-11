#include"ProcessorManager.hh"
#include"Processor.hh"

using namespace eudaq;

ProcessorManager* ProcessorManager::GetInstance(){
  static ProcessorManager singleton;
  return &singleton;
}

ProcessorManager::ProcessorManager(){
}


PSSP ProcessorManager::MakePSSP(std::string pstype, std::string cmd){
  PSSP ps(std::move(Factory<Processor>::Create(cstr2hash(pstype.c_str()), cmd)));
  // std::string cmd_mv = cmd;
  // Factory<Processor>::Create(cstr2hash(pstype.c_str()), std::move(cmd_mv));
  return ps;
}

EVUP ProcessorManager::CreateEvent(std::string evtype){
  // auto it = m_evlist.find(evtype);
  // if(it!=m_evlist.end()){
  //   auto creater= it->second.first;
  //   auto destroyer= it->second.second;
  //   // EVUP ev((*creater)(), destroyer);
  //   EVUP ev((*creater)());
  //   return ev;
  // }
  // else{
  //   std::cout<<"no evtype "<<evtype<<std::endl;
  //   EVUP ev;
  //   return ev;
  // }
  EVUP ev;
  return ev;
}


PSSP ProcessorManager::operator>>(PSSP psr){
  m_pslist_root.push_back(psr);
  psr->SetPSHub(psr);
  psr->RunHubThread();
  return psr;
}

PSSP ProcessorManager::operator>>(std::string psr_str){
  PSSP ps = MakePSSP("ExamplePS", "SYS:PSID=0");//DUMMY, TODO: network command recevier PS
  return *this >> ps >> psr_str;
}
