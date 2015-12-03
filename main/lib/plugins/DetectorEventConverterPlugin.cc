#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Event.hh"
#include "eudaq/PluginManager.hh"
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

namespace eudaq {

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

  virtual event_sp ExtractEventN(event_sp pac, size_t NumberOfROF) override{

    auto det = std::dynamic_pointer_cast<DetectorEvent>(pac);
    if (NumberOfROF < det->NumEvents()) {
      return det->GetEventPtr(NumberOfROF);
    }

    return nullptr;
  }

private:


  DetectorEventConverterPlugin()
    : DataConverterPlugin(Event::str2id(DetectorEventMaintype), "") {
  }



  static DetectorEventConverterPlugin m_instance;
};

// Instantiate the converter plugin instance
DetectorEventConverterPlugin DetectorEventConverterPlugin::m_instance;

} // namespace eudaq
