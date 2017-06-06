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
    static ModuleManager* Instance();
    static std::string GetModulePath();
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    uint32_t LoadModuleDir(const std::string& dir);
    bool LoadModuleFile(const std::string& file);
    void Print(std::ostream& os, size_t offset) const;
  private:
    ModuleManager();
    std::map<std::string, void*> m_modules;
  };
}

#endif
