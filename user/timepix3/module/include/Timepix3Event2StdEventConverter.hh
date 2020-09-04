#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

/**
* Timepix3 event converter, converting from raw detector data to EUDAQ StandardEvent format
* SPIDR provides two event types, pixel data and trigger information events.
*/
namespace eudaq {
  class Timepix3RawEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix3RawEvent");
  private:
    static uint64_t m_syncTime;
    static uint64_t m_syncTime_prev;
    static bool m_clearedHeader;
    static bool m_first_time;
    static std::vector<std::vector<float>> vtot;
    static std::vector<std::vector<float>> vtoa;

    void loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat) const;
  };

  class Timepix3TrigEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix3TrigEvent");
    static long long int m_syncTimeTDC;
    static int m_TDCoverflowCounter;
  };

} // namespace eudaq
