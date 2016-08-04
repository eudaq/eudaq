#include"ProcessorManager.hh"
#include"Processor.hh"
// #include"ExamplePS.hh"

#include <experimental/filesystem>

#if EUDAQ_PLATFORM_IS(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>
#endif

using namespace eudaq;
using std::shared_ptr;
using std::unique_ptr;


template class __declspec(dllexport) Factory<Processor>;
template __declspec(dllexport) std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>& Factory<Processor>::GetInstance<std::string&>();
template __declspec(dllexport) std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>& Factory<Processor>::GetInstance<std::string&&>();


// template std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>& Factory<Processor>::GetInstance<std::string&>();
// template std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>& Factory<Processor>::GetInstance<std::string&&>();


// namespace{
//   // static auto dummy0 = Factory<Processor>::Register<ExamplePS, std::string&>(eudaq::cstr2hash("ExamplePS"));
//   // static auto dummy1 = Factory<Processor>::Register<ExamplePS, std::string&&>(eudaq::cstr2hash("ExamplePS"));
  // static auto dummy2 = Factory<Processor>::Register<ExamplePS, std::string&&>(0);

// }
// static ExamplePS *p;

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
  char* env_ps_path = getenv("EUDAQ_PS_PATH");
  if(env_ps_path){
    libpath.emplace_back(env_ps_path);
  }
  for(auto &e: libpath){
    std::vector<std::string> dir_files; 
#if EUDAQ_PLATFORM_IS(WIN32)
    
#else
    DIR *d = opendir(e.c_str());
    struct dirent *dir;
    while((dir = readdir(d)) != NULL){
      dir_files.embrace_back(dir->d_name);
    }
    closedir(d);
#endif
    for(auto &fname: dir_files){
      std::string ps_module_prefix("libeudaq_module_ps_");
      if(!fname.compare(0, ps_module_prefix.size(), ps_module_prefix)
	 && (fname.rfind(".so") != std::string::npos
	     || fname.rfind(".dll") != std::string::npos)
	 && fname.rfind(".pcm") == std::string::npos
	 && fname.rfind(".rootmap") == std::string::npos){
	fname.insert(0, e+"/");
	libs.push_back(fname);
      }
    }      
  }

#if EUDAQ_PLATFORM_IS(WIN32)
  // HMODULE handle = LoadLibrary(e.c_str());
  std::cout<<">>>>>>>>>>====load.... "<< std::endl;
  HMODULE handle = LoadLibrary("../lib/libeudaq_module_ps_example.dll");
  if(handle)
    std::cout<<"  load succ"<<std::endl;
  else
    std::cout<<"  load fail"<<std::endl;
#endif
  
  for(auto &e: libs){
#if EUDAQ_PLATFORM_IS(WIN32)
    // HMODULE handle = LoadLibrary(e.c_str());
    std::cout<<">>>>>>>>>>load..."<<std::endl;
    HMODULE handle = LoadLibrary("libeudaq_module_ps_example.dll");
#else
    void *handle = dlopen(e.c_str(), RTLD_NOW); //RTLD_LOCAL RTLD_GLOBAL
#endif
    if(handle)
      std::cout<<e<<"  load succ"<<std::endl;
    else
      std::cout<<e<<"  load fail"<<std::endl;
  }
}


PSSP ProcessorManager::MakePSSP(std::string pstype, std::string cmd){
  PSSP ps(std::move(Factory<Processor>::Create(cstr2hash(pstype.c_str()), cmd)));
  // std::string cmd_mv = cmd;
  // Factory<Processor>::Create(cstr2hash(pstype.c_str()), std::move(cmd_mv));
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
