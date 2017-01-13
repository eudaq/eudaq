#include "ModuleManager.hh"

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
#else
#include <dlfcn.h>
#endif

namespace eudaq{

  namespace {
    auto dummy = ModuleManager::Instance();
  }
  
  ModuleManager::ModuleManager(){
    char *module_dir_c = std::getenv("EUDAQ_MODULE_DIR");
    if(module_dir_c){
      LoadModuleDir(module_dir_c);
      return;
    }
    
    std::string top_dir;
#ifdef EUDAQ_CXX17_FS
    filesystem::path bin("bin");
    filesystem::path pwd = filesystem::current_path();
    filesystem::path top;
    if(pwd.filename()==bin){
      top = pwd.parent_path();
    }
    else{
      top = pwd;
    }
    top_dir=top.string();
#else
    const std::string bin("/bin");
    char pwd_path[PATH_MAX];
    char pwd_path_ab[PATH_MAX]; 
    if(!getcwd(pwd_path, sizeof(pwd_path))){
      std::cerr<<"ERROR: Can not get path of cwd.";
    }
    if(!realpath(pwd_path, pwd_path_ab)){
      std::cerr<<"ERROR: Can not get realpath of cwd.";
    }
    std::string pwd(pwd_path_ab);
    std::string top;
    if(pwd.rfind(bin) != std::string::npos && pwd.rfind(bin) == pwd.size()-bin.size()){
      top = pwd.substr(0,pwd.rfind(bin));
    }
    else{
      top = pwd;
    }
    top_dir = top;
#endif

#if EUDAQ_PLATFORM_IS(WIN32)
    LoadModuleDir(top_dir+="\\bin");
#else    
    // setenv("EUDAQ_INSTALL_PREFIX", top_dir.c_str(), 0);
    LoadModuleDir(top_dir+="/lib");
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
      std::cerr<<"ModuleManager WARNING: Fail to load module binary.  "<<file<<std::endl;
      return false;
    }
  }

  void ModuleManager::Print(std::ostream & os, size_t offset) const{
    os<< std::string(offset, ' ')<< "<Modules>\n";
    for(auto &e : m_modules){
      os<< std::string(offset+2, ' ')<< "<Module>\n";
      os<< std::string(offset+4, ' ')<< "<Path> " <<e.first << " </Path>";
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
