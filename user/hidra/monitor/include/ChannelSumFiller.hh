#pragma once
#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1D.h>

#include <string>
#include <vector>

/**
 * @brief Per-event SUM distributions over groups of channels, split by trigger.
 *
 * Books 1D distributions of the per-event sum over a channel group, for six
 * groups — scintillator/Cherenkov PMTs (ADC), and scintillator/Cherenkov SiPMs
 * for both FERS gains (HG, LG) — each in three trigger flavours (all / physics /
 * pedestal), i.e. 18 histograms. The raw values are summed (no pedestal
 * subtraction); the physics-vs-pedestal split is what reveals signal over the
 * baseline.
 *
 * The S/C channel classification is not known to the rest of the backend; the
 * monitor parses it from the frontend channel maps (adc_channels.json /
 * sipm_channels.json) and passes the resulting global-channel-index lists here.
 */
struct ChannelSumConfig {
  // Global channel indices per type, matching the event vectors' indexing:
  // PMT indices into HidraXdcEvent::ADCvalues, SiPM indices into
  // HidraFersEvent::FERShg / FERSlg (boardID * 64 + channel).
  std::vector<int> pmt_s;
  std::vector<int> pmt_c;
  std::vector<int> sipm_s;
  std::vector<int> sipm_c;

  int adc_value_max = 4096;  ///< full scale of one ADC channel (V792, 12-bit).
  int fers_value_max = 4096; ///< full scale of one FERS HG/LG channel.
  int nbins = 1024;          ///< bins on every sum axis.

  // Upper edge of the sum axis per category (0 = auto = group size * value_max).
  // S and C of the same detector/gain share a scale so they stay comparable.
  int pmt_max = 0;
  int sipm_hg_max = 0;
  int sipm_lg_max = 0;
};

class ChannelSumFiller : public IHistogramFiller {
public:
  ChannelSumFiller(HistogramRegistry& reg, const ChannelSumConfig& config);
  void Fill(const HidraEvent& ev) override;

private:
  // Which per-event value vector an output sums over.
  enum class Source { ADC, FERS_HG, FERS_LG };

  // One summed quantity (all / physics / pedestal histograms over one source).
  struct SumOutput {
    Source source;
    TH1D* h_all;
    TH1D* h_physics;
    TH1D* h_pedestal;
  };

  // One channel group: a shared index list summed into one or more outputs. The
  // SiPM groups carry two outputs (HG, LG) so the index list is walked once per
  // event instead of once per gain.
  struct SumGroup {
    std::vector<int> indices;
    std::vector<SumOutput> outputs;
  };

  static const std::vector<double>& SourceVector(const HidraEvent& ev, Source source);

  std::vector<SumGroup> m_groups;
};
