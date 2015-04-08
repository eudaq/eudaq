#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>


#ifdef  USE_LCIO
#include "EUTelTrackerDataInterfacerImpl.h"
#endif //  USE_LCIO

namespace eudaq {

  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype)
    : m_eventtype(make_pair(Event::str2id("_RAW"), subtype))
  {
    //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
    : m_eventtype(make_pair(type, subtype))
  {
    //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

  void ConvertPlaneToLCIOGenericPixel(StandardPlane & plane, lcio::TrackerDataImpl& zsFrame)
  {
     // helper object to fill the TrakerDater object 
    auto sparseFrame =eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(&zsFrame);

    for (size_t iPixel = 0; iPixel < plane.HitPixels(); ++iPixel) {
      eutelescope::EUTelGenericSparsePixel thisHit1(plane.GetX(iPixel), plane.GetY(iPixel), plane.GetPixel(iPixel), 0);
      sparseFrame.addSparsePixel(&thisHit1);
    }
  
  }

}//namespace eudaq
