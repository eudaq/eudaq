#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

/**
* CMS Phase2 event converter, converting from raw detector data to EUDAQ StandardEvent format
*/
namespace eudaq {
  class CMSPhase2RawEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    void AddFrameToPlane(eudaq::StandardPlane *pPlane, const std::vector<uint8_t> &data, uint32_t pFrame, uint32_t pNFrames) const;
    uint16_t getHitVal(const std::vector<uint8_t> &data, size_t index, size_t value_id) const; 
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPhase2RawEvent");
  private:
  };

} // namespace eudaq
