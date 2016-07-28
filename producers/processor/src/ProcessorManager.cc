#include"ProcessorManager.hh"

#include"Processor.hh"

#include <experimental/filesystem>

#ifndef WIN32
#include <dlfcn.h>
#include <dirent.h> 

#endif

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
  std::vector<std::string> libs;
  std::vector<std::string> libpath;
  libpath.emplace_back(".");
  libpath.emplace_back("../lib");
  for(auto &e: libpath){
    DIR *d = opendir(e.c_str());
    struct dirent *dir;
    while((dir = readdir(d)) != NULL){
      std::string fname(dir->d_name);      
      if(!fname.compare(0, 12, "libeudaq_ps_")
	 && fname.rfind(".so") != std::string::npos
	 && fname.rfind(".pcm") == std::string::npos
	 && fname.rfind(".rootmap") == std::string::npos){
	fname.insert(0, e+"/");
	libs.push_back(fname);
      }
    }      
    closedir(d);
  }

  for(auto &e: libs){
    void *handle = dlopen(e.c_str(), RTLD_NOW); //RTLD_LOCAL RTLD_GLOBAL
    if(handle)
      std::cout<<e<<"  load succ"<<std::endl;
    else
      std::cout<<e<<"  load fail"<<std::endl;
  }
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
  m_pslist_root.push_back(psr);
  psr->SetPSHub(psr);
  psr->RunHubThread();
  return psr;
}

PSSP ProcessorManager::operator>>(std::string psr_str){
  PSSP ps = MakePSSP("ExamplePS", "SYS:PSID=0");//DUMMY, TODO: network command recevier PS
  return *this >> ps >> psr_str;
}
