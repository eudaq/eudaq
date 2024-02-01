#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"
#include <set>

#include <array>
#include <vector>

// Foreward declaration of TF1, so that the header has no root dependecies
class TF1;

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

  class AD9249Event2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouAD9249Event");
  private:
    void decodeChannel(const size_t adc, const std::vector<uint8_t>& data, size_t size, size_t offset, std::vector<std::vector<uint16_t>>& waveforms, uint64_t& timestamp) const;
    static size_t trig_;
    static bool m_configured;
    static std::string m_waveform_filename;
    static std::ofstream m_outfile_waveforms;
    static int m_blStart;
    static int m_blEnd;
    static int m_ampStart;
    static int m_ampEnd;
    static double m_calib_range_min;
    static double m_calib_range_max;
    static std::vector<std::string> m_calib_strings;
    static std::vector<TF1> m_calib_functions;
    // Channels are sorted like ADC0: A1 C1 E1 ...
    //                          ADC1: B1 D1 F1 ...
    // AD9249 channels to pixel matrix map:
    // A2, H2, F2, H1
    // C1, A1, D2, F1
    // C2, E1, B1, B2
    // E2, G1, G2, D1
    const std::vector<std::pair<int, int>> mapping = {
      {1, 2}, {0, 2}, {1, 1}, {1, 0}, {0, 3}, {0, 1}, {0, 0}, {2, 0},
      {2, 1}, {3, 0}, {3, 2}, {3, 3}, {3, 1}, {2, 2}, {2, 3}, {1, 3}};
};


  class CLICTDEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouCLICTDEvent");
  private:
    static size_t t0_seen_;
    static bool t0_is_high_;
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
    // covert scope ascii time stamps
    static uint64_t timeConverter(std::string date, std::string time);
    // parse event number, time stamp pairs from file to EventTime set
  };

  class dSiPMEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CariboudSiPMEvent");
  private:
    struct PlaneConfiguration {
      bool configured {false};
      bool zeroSupp {true};
      bool discardDuringReset {false};
      bool discardDuringResetWarned {false};
      bool checkValid {false};
      std::array<std::array<double, 32>, 4> fine_tdc_bin_widths {};
      std::array<std::array<double, 32>, 32> pixel_delays {};
      uint64_t frame_start {0};
      uint64_t frame_stop {INT8_MAX};
    };
    static uint8_t getQuadrant(const uint16_t& col, const uint16_t& row);
    static std::array<double, 32> getFineTDCWidths(std::string config);
    static std::array<std::array<double, 32>, 32> getPixelDelays(std::string config);
    static std::vector<PlaneConfiguration> m_configuration;
    static std::vector<uint64_t> m_trigger;
    static std::vector<uint64_t> m_frame;
  };

  class CLICpix2Event2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouCLICpix2Event");
  private:
    static size_t t0_seen_;
    static uint64_t last_shutter_open_;
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
  class H2MEvent2StdEventConverter: public eudaq::StdEventConverter{
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouH2MEvent");
  };
} // namespace eudaq
