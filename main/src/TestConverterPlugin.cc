#include "eudaq/DataConverterPlugin.hh"

namespace eudaq {

  // Test Events have nothing to convert
  // This class is only here to prevent a runtime errors
  class TestConverterPlugin : public DataConverterPlugin {
    TestConverterPlugin() : DataConverterPlugin("Test") {}
    static TestConverterPlugin m_instance;
  };

  TestConverterPlugin TestConverterPlugin::m_instance;

}
