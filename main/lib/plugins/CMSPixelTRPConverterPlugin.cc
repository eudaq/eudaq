#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

#include "eudaq/CMSPixelHelper.hh"

#include "iostream"
#include "bitset"

namespace eudaq {

  static const char* EVENT_TYPE = "CMSPixelTRP";

  class CMSPixelTRPConverterPlugin : public DataConverterPlugin {
  public:
    // virtual unsigned GetTriggerID(const Event & ev) const{}; 

    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      m_converter.Initialize(bore,cnf);
    }

    virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {
      return m_converter.GetStandardSubEvent(out,in);
    }

#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
      return m_converter.GetLCIORunHeader(header, bore, conf);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const {
      return m_converter.GetLCIOSubEvent(result, source);
    }
#endif

  private:
    CMSPixelTRPConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
				   m_converter(EVENT_TYPE) {}

    CMSPixelHelper m_converter;
    static CMSPixelTRPConverterPlugin m_instance;
  };
  CMSPixelTRPConverterPlugin CMSPixelTRPConverterPlugin::m_instance;
}
