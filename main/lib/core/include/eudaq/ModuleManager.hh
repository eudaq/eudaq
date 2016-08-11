#ifndef MODULEMANAGER_HH
#define MODULEMANAGER_HH
#include "Platform.hh"

#include <vector>
#include <string>
#include <map>

class ModuleManager;

namespace eudaq{
  class ModuleManager{
  public:
    ModuleManager();
    static ModuleManager* Instance();
    uint32_t LoadModuleDir(const std::string dir);
    bool LoadModuleFile(const std::string file);
  private:
    std::map<std::string, void*> m_modules;
  };
}


#endif
