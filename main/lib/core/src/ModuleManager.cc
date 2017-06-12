#include "eudaq/ModuleManager.hh"

#include <cstdlib>
#include <vector>
#include <iostream>

#if !defined(__GNUC__) || (__GNUC__ > 5) || (__GNUC__ == 5 && (__GNUC_MINOR__ > 2))
#define EUDAQ_CXX17_FS
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#error Minimum Visual Studio 2015 required
#endif

#ifdef EUDAQ_CXX17_FS

//This C++17 filesystem is available in GCC >= 5.3 or VisualStudio >= vs2015 (1900)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#else
#include <libgen.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif

#if EUDAQ_PLATFORM_IS(WIN32)
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#else
#include <dlfcn.h>
#endif

#include "eudaq/Logger.hh"

namespace eudaq{
  namespace {
    auto dummy = ModuleManager::Instance();
  }
  
  ModuleManager::ModuleManager(){
    char *env_module_dir_c = std::getenv("EUDAQ_MODULE_DIR");
    if(env_module_dir_c){
      std::string env_module_dir(env_module_dir_c);
      std::stringstream ss(env_module_dir);
      while(ss.good()){
      	std::string dir;
      	std::getline(ss,dir,':');
      	if(!dir.empty()){
      	  LoadModuleDir(dir);
      	}
      }
    }
    char *env_module_ignore_default = std::getenv("EUDAQ_MODULE_IGNORE_DEFALUT");
    if(env_module_ignore_default){
      std::string env_ignore_default(env_module_ignore_default);
      if(env_ignore_default == "YES" || env_ignore_default == "yes" || env_ignore_default == "1")
	return;
    }

    std::string core_lib_path_str = GetModulePath();
#ifdef EUDAQ_CXX17_FS
    filesystem::path core_lib_dir = filesystem::path(core_lib_path_str).parent_path();
    LoadModuleDir(core_lib_dir.string());
#else
    std::string core_lib_dir_str = core_lib_path_str.substr(0, core_lib_path_str.find_last_of("/\\"));
    LoadModuleDir(core_lib_dir_str);
#endif

  }
  
  ModuleManager* ModuleManager::Instance(){
    static ModuleManager mm;
    return &mm;
  }
  
  uint32_t ModuleManager::LoadModuleDir(const std::string& dir){
    const std::string module_prefix("libeudaq_module_");
#if EUDAQ_PLATFORM_IS(WIN32)
    const std::string module_suffix(".dll");
#elif EUDAQ_PLATFORM_IS(MACOSX)
    const std::string module_suffix(".dylib");	
#else
    const std::string module_suffix(".so");
#endif
    
    uint32_t n=0;
#ifdef EUDAQ_CXX17_FS
    if(!filesystem::is_directory(dir)){
      EUDAQ_INFO("Ignored module path which does not exist: "+dir);
      return 0;
    }
    for(auto& e: filesystem::directory_iterator(filesystem::absolute(dir))){
      filesystem::path file(e);
      std::string fname = file.filename().string();
      if(!fname.compare(0, module_prefix.size(), module_prefix)
	 && (fname.find(module_suffix) != std::string::npos)){
	if(LoadModuleFile(file.string())){
	  n++;
	}
      }
    }
#else    
    DIR *dpath = opendir(dir.c_str());
    struct dirent *dfile;
    while((dfile = readdir(dpath)) != NULL){
      std::string fname(dfile->d_name);
      if(!fname.compare(0, module_prefix.size(), module_prefix)
	 && (fname.find(module_suffix) != std::string::npos)){
	if(LoadModuleFile(fname)){
	  n++;
	}
      }
    }
    closedir(dpath);
#endif
    return n;
  }
  
  bool ModuleManager::LoadModuleFile(const std::string& file){
    void *handle;
#if EUDAQ_PLATFORM_IS(WIN32)
    handle = (void *)LoadLibrary(file.c_str());
#else
    handle = dlopen(file.c_str(), RTLD_NOW);
#endif
    if(handle){
      m_modules[file]=handle;
      return true;
    }
    else{
      EUDAQ_WARN("Fail to load module binary ("+ file+")");
      return false;
    }
  }

  std::string ModuleManager::GetModulePath(){
#if EUDAQ_PLATFORM_IS(WIN32)
    void* address_return = _ReturnAddress();
    HMODULE handle = NULL;
    ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
			|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			static_cast<LPCSTR>(address_return), &handle);
    char modpath[MAX_PATH] = {'\0'};
    ::GetModuleFileNameA(handle, modpath, MAX_PATH);
    return modpath;
#else
    void* address_return = (void*)(__builtin_return_address(0));
    Dl_info dli;
    dli.dli_fname = 0;
    dladdr(address_return, &dli);
    return dli.dli_fname;
#endif
  }
  
  void ModuleManager::Print(std::ostream & os, size_t offset) const{
    os<< std::string(offset, ' ')<< "<Modules>\n";
    for(auto &e : m_modules){
      os<< std::string(offset+2, ' ')<< "<Module>\n";
      os<< std::string(offset+4, ' ')<< "<Path>" <<e.first << "</Path>";
      os<< std::string(offset+4, ' ')<< "<Status> ";
      if(e.second)
	os<< "Loaded";
      else
	os<< "Failed";
      os<< "</Status>\n";
      os<< std::string(offset+2, ' ')<< "</Module>\n";
    }
    os << std::string(offset, ' ')<< "</Modules>\n";
  }
}
