#ifndef EUDAQ_INCLUDED_PluginManager
#define EUDAQ_INCLUDED_PluginManager

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/DetectorEvent.hh"

#include <string>
#include <map>

namespace eudaq {

  /** The plugin manager has a map of all available plugins.
   *  On creating time every plugin automatically registeres at
   *  the plugin manager, wich adds the event type string and 
   *  a pointer to the according plugin to a map.
   *  A generic event being received contains an identification string,
   *  and the plugin manager can deliver the correct plugin to 
   *  convert it to lcio.
   */
  class DLLEXPORT PluginManager {

    public:
      typedef DataConverterPlugin::t_eventid t_eventid;

      /** Register a new plugin to the plugin manager.
       */
      void RegisterPlugin(DataConverterPlugin * plugin);

      /** Get the instance of the plugin manager. As this is a singleton class with
       *  private constructor and copy constructor, this is the only way to access it.
       */
      static PluginManager & GetInstance();

      static unsigned GetTriggerID(const Event &);

	  static int IsSyncWithTLU(eudaq::Event const & ev,eudaq::TLUEvent const & tlu);
	  static t_eventid getEventId( eudaq::Event const & ev);

	  static void SetCurrentTLUEvent(eudaq::Event & ev,eudaq::TLUEvent const & tlu);
      static void Initialize(const DetectorEvent &);
      static lcio::LCRunHeader * GetLCRunHeader(const DetectorEvent &);
      static StandardEvent ConvertToStandard(const DetectorEvent &);
      static lcio::LCEvent * ConvertToLCIO(const DetectorEvent &);

      static void ConvertStandardSubEvent(StandardEvent &, const Event &);
      static void ConvertLCIOSubEvent(lcio::LCEvent &, const Event &);

      /** Get the correct plugin implementation according to the event type.
       */
      DataConverterPlugin & GetPlugin(t_eventid eventtype);
      DataConverterPlugin & GetPlugin(const Event & event);

    private:
      /** The map that correlates the event type with its converter plugin.
       */
      std::map<t_eventid, DataConverterPlugin *> m_pluginmap;

      PluginManager() {}
      PluginManager(PluginManager const &) {}
      class _dummy;
      friend class _dummy; // Silence superfluous warnings in some gcc versions
  };

}//namespace eudaq

#endif //EUDAQ_INCLUDED_PluginManager
