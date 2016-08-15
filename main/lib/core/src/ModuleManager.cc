#include "ModuleManager.hh"

#include <vector>
#include <iostream>
#include <experimental/filesystem> //It will be avaliable in C++17. Waiting

#if EUDAQ_PLATFORM_IS(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif


namespace eudaq{

  namespace {
    auto dummy = ModuleManager::Instance();
  }

  namespace filesystem = std::experimental::filesystem;
  
  ModuleManager::ModuleManager(){
    filesystem::path bin("bin");
    filesystem::path lib("lib");
    filesystem::path pwd = filesystem::current_path();
    filesystem::path top;
    if(pwd.filename()==bin){
      top = pwd.parent_path();
    }
    else{
      top = pwd;
    }

    // char* env_ps_path = getenv("EUDAQ_MODULE_PATH");
    filesystem::path module_dir=top/lib;
    LoadModuleDir(module_dir.string());
    module_dir=top/bin;
    LoadModuleDir(module_dir.string());
  }
  
  ModuleManager* ModuleManager::Instance(){
    static ModuleManager mm;
    return &mm;
  }
  
  
  uint32_t ModuleManager::LoadModuleDir(const std::string dir){
    const std::string module_prefix("libeudaq_module_");
    const std::string module_suffix_win(".dll");
    const std::string module_suffix_lnx(".so");
    uint32_t n=0;
    for(auto& e: filesystem::directory_iterator(filesystem::absolute(dir))){
      filesystem::path file(e);
      std::string fname = file.filename().string();
      if(!fname.compare(0, module_prefix.size(), module_prefix)
	 && (fname.find(module_suffix_win) != std::string::npos ||
	     fname.find(module_suffix_lnx) != std::string::npos)){
	if(LoadModuleFile(file.string())){
	  n++;
	}
      }
    }
    return n;
  }
  
  bool ModuleManager::LoadModuleFile(const std::string file){
    std::cout<<">>>>>>>>>>load... "<<file<<std::endl;
    void *handle;
#if EUDAQ_PLATFORM_IS(WIN32)
    handle = (void *)LoadLibrary(file.c_str());
#else
    handle = dlopen(file.c_str(), RTLD_NOW);
#endif
    if(handle){
      std::cout<<file<<"  load succ"<<std::endl;
      m_modules[file]=handle;
      return true;
    }
    else{
      std::cout<<file<<"  load fail"<<std::endl;
      return false;
    }
  }
  
}
