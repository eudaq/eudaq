#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"
#include "EventTime.hh"
#include <set>

/**
 * Caribou event converter, converting from raw detector data to EUDAQ StandardEvent format
 * @WARNING Each Caribou device needs to register its own converter, as Peary does not force a specific data format!
 */
namespace eudaq {

  /** Return the binary representation of the parameter as std::string
   */
  template <typename T> std::string to_bit_string(const T data, int length = -1, bool baseprefix = false) {
    std::ostringstream stream;
    // if length is not defined, use a standard (full) one
    if(length < 0) {
      length = std::numeric_limits<T>::digits;
      // std::numeric_limits<T>::digits does not include sign bits
      if(std::numeric_limits<T>::is_signed) {
        length++;
      }
    }
    if(baseprefix) {
      stream << "0b";
    }
    while(--length >= 0) {
      stream << ((data >> length) & 1);
    }
    return stream.str();
  }

  class CLICTDEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouCLICTDEvent");
  private:
    static size_t t0_seen_;
    static bool t0_is_high_;
    static uint64_t last_shutter_open_;
  };

  class CLICpix2Event2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouCLICpix2Event");
  private:
    static size_t t0_seen_;
    static uint64_t last_shutter_open_;
  };

  class DSO9254AEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouDSO9254AEvent");
  private:
    static bool m_configured;
    static int64_t m_runStartTime;
    static double m_pedStartTime;
    static double m_pedEndTime;
    static double m_ampStartTime;
    static double m_ampEndTime;
    static double m_chargeScale;
    static double m_chargeCut;
    static bool m_generateRoot;
    static std::string m_fileNameEventTimes;
    // container for time sorted pairs of alpide event numbers and time stamps
    static std::set<EventTime> m_eventTimes;
    // covert scope ascii time stamps
    static uint64_t timeConverter(std::string date, std::string time);
    // parse event number, time stamp pairs from file to EventTime set
    static void readEventTimeFile(std::string filename, std::set<EventTime>* eventtime);
  };

  class ATLASPixEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouATLASPixEvent");

private:
    uint32_t gray_decode(uint32_t gray) const;

    static uint64_t readout_ts_;
    static double clockcycle_;
    static uint32_t clkdivend2M_;
    static uint64_t fpga_ts_;
    static uint64_t fpga_ts1_;
    static uint64_t fpga_ts2_;
    static uint64_t fpga_ts3_;
    static bool new_ts1_;
    static bool new_ts2_;
    static size_t t0_seen_;
  };

} // namespace eudaq
