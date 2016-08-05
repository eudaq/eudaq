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


template class DLLEXPORT Factory<Processor>;
template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>& Factory<Processor>::Instance<std::string&>();
template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>& Factory<Processor>::Instance<std::string&&>();

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
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    std::string name_pattern = e+"/libeudaq_*.dll";
    if((hFind=FindFirstFile(name_pattern.c_str(),&FindFileData)) !=INVALID_HANDLE_VALUE){
      do{
	dir_files.emplace_back(FindFileData.cFileName);
	std::cout<< "Find Moulde  " <<FindFileData.cFileName<<std::endl;
      }while(FindNextFile(hFind, &FindFileData));
      FindClose(hFind);
    }
#else
    DIR *d = opendir(e.c_str());
    struct dirent *dir;
    while((dir = readdir(d)) != NULL){
      dir_files.emplace_back(dir->d_name);
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
  
  for(auto &e: libs){
#if EUDAQ_PLATFORM_IS(WIN32)
    std::cout<<">>>>>>>>>>load..."<<std::endl;
    HMODULE handle = LoadLibrary(e.c_str());
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
