#include "eudaq/DataConverterPlugin.hh"

namespace eudaq {

  static const char* EVENT_TYPE = "MVD";

  // Test Events have nothing to convert
  // This class is only here to prevent a runtime errors
  class MVDConverterPlugin : public DataConverterPlugin {
    MVDConverterPlugin() : DataConverterPlugin(EVENT_TYPE) {}
    static MVDConverterPlugin m_instance;
  };

  MVDConverterPlugin MVDConverterPlugin::m_instance;

}
