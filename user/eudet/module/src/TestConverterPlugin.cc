#include "DataConverterPlugin.hh"

namespace eudaq {
  class TestConverterPlugin;
  namespace{
    auto dummy0 = Factory<DataConverterPlugin>::Register<TestConverterPlugin>(cstr2hash("Test"));
  }
  
  class TestConverterPlugin : public DataConverterPlugin {
  public:
    TestConverterPlugin() : DataConverterPlugin("Test") {}
  };
}
