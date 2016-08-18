#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>

#ifdef USE_EUTELESCOPE
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#endif //  USE_EUTELESCOPE

namespace eudaq {
#if USE_LCIO
  bool Collection_createIfNotExist(lcio::LCCollectionVec **zsDataCollection,
                                   const lcio::LCEvent &lcioEvent,
                                   const char *name) {

    bool zsDataCollectionExists = false;
    try {
      *zsDataCollection =
          static_cast<lcio::LCCollectionVec *>(lcioEvent.getCollection(name));
      zsDataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      *zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    }

    return zsDataCollectionExists;
  }
#endif
  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype)
      : m_eventtype(make_pair(Event::str2id("_RAW"), subtype)) {
    // std::cout << "DEBUG: Registering DataConverterPlugin: " <<
    // Event::id2str(m_eventtype.first) << ":" << m_eventtype.second <<
    // std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
      : m_eventtype(make_pair(type, subtype)) {
    // std::cout << "DEBUG: Registering DataConverterPlugin: " <<
    // Event::id2str(m_eventtype.first) << ":" << m_eventtype.second <<
    // std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

#ifdef USE_EUTELESCOPE
  void ConvertPlaneToLCIOGenericPixel(StandardPlane &plane,
                                      lcio::TrackerDataImpl &zsFrame) {
    // helper object to fill the TrakerDater object
    auto sparseFrame = eutelescope::EUTelTrackerDataInterfacerImpl<
        eutelescope::EUTelGenericSparsePixel>(&zsFrame);

    for (size_t iPixel = 0; iPixel < plane.HitPixels(); ++iPixel) {
      sparseFrame.emplace_back( plane.GetX(iPixel), plane.GetY(iPixel), plane.GetPixel(iPixel), 0 );
    }
  }
#endif // USE_EUTELESCOPE
} // namespace eudaq
