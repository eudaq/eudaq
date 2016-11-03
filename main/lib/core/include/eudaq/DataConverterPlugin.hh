#ifndef EUDAQ_INCLUDED_DataConverterPlugin
#define EUDAQ_INCLUDED_DataConverterPlugin

#include "StandardEvent.hh"
#include "Factory.hh"
#include "Configuration.hh"
#include <memory>
#include <string>
#include <algorithm>


#define NOTIMESTAMPSET (uint64_t)-1
#define NOTIMEDURATIONSET 0

namespace EVENT { class LCEvent; class LCRunHeader; }
namespace lcio { using namespace EVENT; }

namespace eudaq {
  class DataConverterPlugin;
  class Configuration;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<DataConverterPlugin>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<DataConverterPlugin>::UP_BASE (*)()>&
  Factory<DataConverterPlugin>::Instance<>();
#endif

  using DataConverterUP = Factory<DataConverterPlugin>::UP;
  
  class DLLEXPORT DataConverterPlugin {
  public:
    virtual void Initialize(eudaq::Event const &, eudaq::Configuration const &);
    virtual unsigned GetTriggerID(eudaq::Event const &) const;
    virtual void GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const;
    virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const;
    virtual bool GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const;
    virtual EventSP GetSubEvent(EventSP ev, size_t EventNr);
    virtual size_t GetNumSubEvent(const eudaq::Event& pac);
    virtual ~DataConverterPlugin(){};

  protected:
    DataConverterPlugin(std::string subtype);
    DataConverterPlugin(unsigned type, std::string subtype = "");

    static unsigned m_count;
    std::pair<uint32_t, std::string> m_eventtype;
    unsigned m_thisCount;
  private:
    DataConverterPlugin(const DataConverterPlugin &) = delete;
    DataConverterPlugin & operator = (const DataConverterPlugin &) = delete;
  };

}//namespace eudaq

#endif
