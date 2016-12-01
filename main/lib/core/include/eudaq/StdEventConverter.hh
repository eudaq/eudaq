#ifndef STDEVENTCONVERTER_HH_ 
#define STDEVENTCONVERTER_HH_

#include "eudaq/Platform.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/Event.hh"
#include "eudaq/StandardEvent.hh"

namespace eudaq{
  class StdEventConverter;
#ifndef EUDAQ_CORE_EXPORTS  
  extern template class DLLEXPORT Factory<StdEventConverter>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<StdEventConverter>::UP(*)()>&
  Factory<StdEventConverter>::Instance<>();
#endif
  using StdEventConverterUP = Factory<StdEventConverter>::UP;
  using StandardEventSP = std::shared_ptr<StandardEvent>;
  
  class DLLEXPORT StdEventConverter:public DataConverter<Event, StandardEvent>{
  public:
    StdEventConverter() = default;
    StdEventConverter(const StdEventConverter&) = delete;
    StdEventConverter& operator = (const StdEventConverter&) = delete;
    bool Converting(EventSPC d1, StandardEventSP d2, const Configuration &conf) const override = 0;
    static bool Convert(EventSPC d1, StandardEventSP d2, const Configuration &conf);
    static StandardEventSP MakeSharedStdEvent(uint32_t run, uint32_t stm);
  };

}
#endif
