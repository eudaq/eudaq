#ifndef EUDAQ_INCLUDED_PluginManager
#define EUDAQ_INCLUDED_PluginManager

#include "DataConverterPlugin.hh"
#include "DetectorEvent.hh"

#include <string>
#include <map>

namespace eudaq {

  class DLLEXPORT PluginManager {

  public:
    static PluginManager &GetInstance();
    static void Initialize(const DetectorEvent &);
    static lcio::LCRunHeader* GetLCRunHeader(const DetectorEvent &);
    static StandardEvent ConvertToStandard(const DetectorEvent &);
    static lcio::LCEvent* ConvertToLCIO(const DetectorEvent &);
    static void ConvertStandardSubEvent(StandardEvent &, const Event &);
    static void ConvertLCIOSubEvent(lcio::LCEvent &, const Event &);

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator = (const PluginManager&) = delete;
    DataConverterPlugin &GetPlugin(const Event &event);

  private:
    std::map<uint32_t, DataConverterUP> m_datacvts;
    PluginManager() = default;
  };
  
}

#endif
