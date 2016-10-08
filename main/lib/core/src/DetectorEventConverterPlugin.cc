#include "DataConverterPlugin.hh"
#include "StandardEvent.hh"
#include "Utils.hh"
#include "DetectorEvent.hh"
#include "Event.hh"
#include "PluginManager.hh"
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

namespace eudaq {
  class DetectorEventConverterPlugin;
  namespace{
    auto dummy0 = Factory<DataConverterPlugin>::Register<DetectorEventConverterPlugin>(Event::str2id("_DET"));
  }

class DetectorEventConverterPlugin : public DataConverterPlugin {
public:

  virtual bool GetStandardSubEvent(StandardEvent &,
                                   const Event &) const  override{
    EUDAQ_THROW("a detector Event can't be converted to standart event");
  }

  virtual size_t GetNumberOfEvents(const eudaq::Event& pac) override{
    auto det = dynamic_cast<const DetectorEvent*>(&pac);
    return  det->NumEvents();
  }

  virtual EventSP ExtractEventN(EventSP pac, size_t NumberOfROF) override{

    auto det = std::dynamic_pointer_cast<DetectorEvent>(pac);
    if (NumberOfROF < det->NumEvents()) {
      return det->GetEventPtr(NumberOfROF);
    }

    return nullptr;
  }

  DetectorEventConverterPlugin()
    : DataConverterPlugin(Event::str2id("_DET"), "") {
  }

};


} // namespace eudaq
