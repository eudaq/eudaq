#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

#include "api.h"
#include "datasource_evt.h"

/**
 * CMSPixel event converter, converting from raw detector data to EUDAQ StandardEvent format
 */
namespace eudaq {

  class CMSPixelBaseConverter: public eudaq::StdEventConverter {
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
  private:
    void Initialize(eudaq::EventSPC bore, eudaq::ConfigurationSPC conf) const ;

    void GetSinglePlane(eudaq::StandardEventSP d2, unsigned plane_id, pxar::Event *evt) const;
    void GetMultiPlanes(eudaq::StandardEventSP d2, unsigned plane_id, pxar::Event *evt) const;
    static inline uint16_t roc_to_mod_row(uint8_t roc, uint16_t row);
    static inline uint16_t roc_to_mod_col(uint8_t roc, uint16_t col);
    static std::vector<uint16_t> TransformRawData(const std::vector<unsigned char> &block);

    static uint8_t m_roctype, m_tbmtype;
    static size_t m_planeid;
    static size_t m_nplanes;
    static std::string m_detector;
    static bool m_rotated_pcb;
    static std::string m_event_type;

    // The pipeworks:
    mutable pxar::evtSource src;
    mutable pxar::passthroughSplitter splitter;
    mutable pxar::dtbEventDecoder decoder;
    mutable pxar::dataSink<pxar::Event *> Eventpump;
  };

  class CMSPixelDUT2StdEventConverter: public eudaq::CMSPixelBaseConverter {
  public:
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPixelDUT");
  };

  class CMSPixelREF2StdEventConverter: public eudaq::CMSPixelBaseConverter {
  public:
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPixelREF");
  };

  class CMSPixelTRP2StdEventConverter: public eudaq::CMSPixelBaseConverter {
  public:
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPixelTRP");
  };

  class CMSPixelQUAD2StdEventConverter: public eudaq::CMSPixelBaseConverter {
  public:
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPixelQUAD");
  };

} // namespace eudaq
