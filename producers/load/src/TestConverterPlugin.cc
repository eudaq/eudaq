#include "DataConverterPlugin.hh"

namespace eudaq {
  class TestConverterPlugin;
  namespace{
    auto dummy0 = Factory<DataConverterPlugin>::Register<TestConverterPlugin>(Event::str2id("_RAW")+eudaq::cstr2hash("Test"));
  }
  
  class TestConverterPlugin : public DataConverterPlugin {
  public:
    TestConverterPlugin() : DataConverterPlugin("Test") {}
  };
}
