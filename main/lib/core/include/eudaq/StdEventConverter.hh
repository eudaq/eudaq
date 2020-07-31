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

  // Exceptions to communicate information from conversion process:
  #define STDEVENTCONVERTER_EXCEPTIONS_
  class StdEventConverterException : public Exception {
  public:
    StdEventConverterException(const std::string& msg) : Exception(msg) {};
  };

  class DataInvalid : public StdEventConverterException {
  public:
    DataInvalid(const std::string& msg) : StdEventConverterException(msg) {};
  };

  class DataDiscarded : public StdEventConverterException {
  public:
    DataDiscarded(const std::string& msg) : StdEventConverterException(msg) {};
  };

  class DLLEXPORT StdEventConverter:public DataConverter<Event, StandardEvent>{
  public:
    StdEventConverter() = default;
    StdEventConverter(const StdEventConverter&) = delete;
    StdEventConverter& operator = (const StdEventConverter&) = delete;
    bool Converting(EventSPC d1, StdEventSP d2, ConfigurationSPC conf) const override = 0;
    static bool Convert(EventSPC d1, StdEventSP d2, ConfigurationSPC conf);
  };

}
#endif
