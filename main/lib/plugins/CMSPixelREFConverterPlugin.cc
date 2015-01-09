#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

#include "eudaq/CMSPixelDecoder.h"
#include "eudaq/CMSPixelHelper.hh"

#include "iostream"
#include "bitset"

namespace eudaq {

  static const char* EVENT_TYPE = "CMSPixelREF";

  class CMSPixelREFConverterPlugin : public DataConverterPlugin {
  public:
    // virtual unsigned GetTriggerID(const Event & ev) const{}; 

    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      m_converter.Initialize(bore,cnf);
    }

    virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {
      return m_converter.GetStandardSubEvent(out,in);
    }

  private:
    CMSPixelREFConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
				   m_converter(EVENT_TYPE) {}

    CMSPixelHelper m_converter;
    static CMSPixelREFConverterPlugin m_instance;
  };
  CMSPixelREFConverterPlugin CMSPixelREFConverterPlugin::m_instance;
}
