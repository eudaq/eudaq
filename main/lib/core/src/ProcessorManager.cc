#include"ProcessorManager.hh"
#include"Processor.hh"

using namespace eudaq;

ProcessorManager* ProcessorManager::GetInstance(){
  static ProcessorManager singleton;
  return &singleton;
}

PSSP ProcessorManager::operator>>(PSSP psr){
  m_pslist_root.push_back(psr);
  psr->UpdatePSHub(psr);
  psr->SetThisPtr(psr);
  psr->RunHubThread();
  return psr;
}

PSSP ProcessorManager::operator>>(std::string psr_str){
  PSSP ps = Processor::MakePSSP("ExamplePS", "SYS:PSID=0");//DUMMY, TODO: network command recevier PS
  return *this >> ps >> psr_str;
}
