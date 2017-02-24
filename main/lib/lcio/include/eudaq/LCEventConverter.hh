#ifndef LCEVENTCONVERTER_HH_ 
#define LCEVENTCONVERTER_HH_

#include "eudaq/Platform.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/Event.hh"

#include "lcio.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "IMPL/LCCollectionVec.h"


namespace eudaq{
  class LCEventConverter;
#ifndef EUDAQ_CORE_EXPORTS  
  extern template class DLLEXPORT Factory<LCEventConverter>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<LCEventConverter>::UP(*)()>&
  Factory<LCEventConverter>::Instance<>();
#endif
  using LCEventConverterUP = Factory<LCEventConverter>::UP;
  using LCEventSP = std::shared_ptr<lcio::LCEventImpl>;
  using LCEventSPC = std::shared_ptr<const lcio::LCEventImpl>;
  
  class DLLEXPORT LCEventConverter:public DataConverter<Event, lcio::LCEventImpl>{
  public:
    LCEventConverter() = default;
    LCEventConverter(const LCEventConverter&) = delete;
    LCEventConverter& operator = (const LCEventConverter&) = delete;
    bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override = 0;
    static bool Convert(EventSPC d1, LCEventSP d2, ConfigurationSPC conf);
    // static LCEventSP MakeSharedLCEvent(uint32_t run, uint32_t stm);
  };

}
#endif
