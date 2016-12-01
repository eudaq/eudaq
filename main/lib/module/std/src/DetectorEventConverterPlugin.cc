#include "DataConverterPlugin.hh"
#include "StandardEvent.hh"
#include "Utils.hh"
#include "DetectorEvent.hh"
#include "Event.hh"
#include "PluginManager.hh"

namespace eudaq {
  class DetectorEventConverterPlugin;
  namespace{
    auto dummy0 = Factory<DataConverterPlugin>::Register<DetectorEventConverterPlugin>(cstr2hash("DetectorEvent"));
  }

  class DetectorEventConverterPlugin : public DataConverterPlugin {
public:

  virtual bool GetStandardSubEvent(StandardEvent &,
                                   const Event &) const  override{
    EUDAQ_THROW("a detector Event can't be converted to standart event");
  }

  virtual size_t GetNumSubEvent(const eudaq::Event& pac) override{
    auto det = dynamic_cast<const DetectorEvent*>(&pac);
    return  det->GetNumSubEvent();
  }

  virtual EventSP GetSubEvent(EventSP pac, size_t NumberOfROF) override{

    auto det = std::dynamic_pointer_cast<DetectorEvent>(pac);
    if (NumberOfROF < det->GetNumSubEvent()) {
      return std::const_pointer_cast<Event>(det->GetSubEvent(NumberOfROF));      
    }

    return nullptr;
  }

  DetectorEventConverterPlugin()
    : DataConverterPlugin("DetectorEvent") {
  }
    
};


} // namespace eudaq
