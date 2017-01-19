#ifndef DATACONVERTER_HH_ 
#define DATACONVERTER_HH_

#include "eudaq/Factory.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"
#include <memory>

namespace eudaq{
  template <typename T1, typename T2> class DataConverter;
  
  template <typename T1, typename T2>
  class DLLEXPORT DataConverter{
  public:
    using ConverterUP = std::unique_ptr<DataConverter<T1, T2>>;
    using T1SP = std::shared_ptr<T1>;
    using T1SPC = std::shared_ptr<const T1>;
    using T2SP = std::shared_ptr<T2>;
    using T2SPC = std::shared_ptr<const T2>;

    DataConverter() = default;
    DataConverter(const DataConverter &) = delete;
    DataConverter& operator = (const DataConverter &) = delete;
    virtual ~DataConverter(){};
    virtual bool Converting(T1SPC d1, T2SP d2, ConfigurationSPC conf) const = 0;
  };
}
#endif
