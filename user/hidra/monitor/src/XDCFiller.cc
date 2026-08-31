#include "XDCFiller.hh"
#include "HidraUtils.hh"

#include <memory>

namespace {
// Interquartile range of a standard normal = 2·Φ⁻¹(0.75) ≈ 1.349, so
// IQR/1.349 reproduces 1σ for a Gaussian while being robust to the tails
// (a few outlier events barely move the quartiles, unlike the std dev).
constexpr double kNormalIQRtoSigma = 1.349;
} // namespace

XDCFiller::XDCFiller(HistogramRegistry& reg,
                     unsigned int n_adc_channels,
                     unsigned int n_tdc_channels,
                     int saturation_threshold_adc,
                     int saturation_threshold_tdc,
                     int noise_update_interval)
    : IHistogramFiller("XDCFiller"),
      m_noise_update_interval(noise_update_interval),
      m_saturation_threshold_adc(saturation_threshold_adc),
      m_saturation_threshold_tdc(saturation_threshold_tdc) {
  if (m_noise_update_interval < 1) {
    HIDRA_WARN("XDCFiller noise_update_interval={} is invalid, forcing 1.", noise_update_interval);
    m_noise_update_interval = 1;
  }
  if (n_adc_channels == 0) {
    HIDRA_ERROR(
        "XDCFiller constructed with n_adc_channels=0. This may indicate a problem with the VME geo map configuration. "
        "Defaulting to 1 channel to avoid construction failure, but please check the configuration.");
    n_adc_channels = 1;
  }
  if (n_tdc_channels == 0) {
    HIDRA_ERROR("XDCFiller constructed with n_tdc_channels=0.");
    n_tdc_channels = 1;
  }
  // Store the normalized channel count (after the n_adc_channels==0 clamp
  // above) — the same value used to book the histograms, so the bounds checks
  // in Fill()/UpdatePedestalNoise() match the booked binning.
  m_n_adc_channels = n_adc_channels;
  // The ";channel;<y>" axis titles mark these profiles as channel-indexed:
  // the frontend uses the "channel" x-axis title to label the hover with the
  // channel number (rather than assuming every TProfile is per-channel).
  m_profile_adc = reg.Add(std::make_unique<TProfile>(
      "ADC_mean", "Mean of ADC values;channel;mean ADC", n_adc_channels, 0, n_adc_channels));
  m_hist_adc_inclusive = reg.Add(std::make_unique<TH1D>("ADC_inclusive", "Inclusive ADC values", 4096, 0, 4096));
  m_profile_adc_physics = reg.Add(std::make_unique<TProfile>(
      "ADC_mean_physics", "Mean of ADC values (physics);channel;mean ADC", n_adc_channels, 0, n_adc_channels));
  m_profile_adc_pedestal = reg.Add(std::make_unique<TProfile>(
      "ADC_mean_pedestal", "Mean of ADC values (pedestal);channel;mean ADC", n_adc_channels, 0, n_adc_channels));
  m_hist_adc_inclusive_physics =
      reg.Add(std::make_unique<TH1D>("ADC_inclusive_physics", "Inclusive ADC values (physics)", 4096, 0, 4096));
  m_hist_adc_inclusive_pedestal =
      reg.Add(std::make_unique<TH1D>("ADC_inclusive_pedestal", "Inclusive ADC values (pedestal)", 4096, 0, 4096));
  m_profile_adc_saturation = reg.Add(std::make_unique<TProfile>(
      "ADC_saturation", "Saturation fraction per ADC channel;channel;saturation fraction", n_adc_channels, 0,
      n_adc_channels));
  m_profile_adc_saturation_physics = reg.Add(std::make_unique<TProfile>(
      "ADC_saturation_physics", "Saturation fraction per ADC channel (physics);channel;saturation fraction",
      n_adc_channels, 0, n_adc_channels));
  m_profile_adc_saturation_pedestal = reg.Add(std::make_unique<TProfile>(
      "ADC_saturation_pedestal", "Saturation fraction per ADC channel (pedestal);channel;saturation fraction",
      n_adc_channels, 0, n_adc_channels));
  // Per-channel pedestal noise, two estimators. One bin per channel; the
  // contents are set by UpdatePedestalNoise(), not Fill()'d. The "channel"
  // x-axis title lets the frontend label the hover with the channel number.
  m_hist_adc_noise_pedestal = reg.Add(std::make_unique<TH1D>(
      "ADC_noise_pedestal", "Pedestal noise (IQR/1.349);channel;noise [ADC counts]",
      n_adc_channels, 0, n_adc_channels));
  m_hist_adc_noise_std_pedestal = reg.Add(std::make_unique<TH1D>(
      "ADC_noise_std_pedestal", "Pedestal noise (std dev);channel;noise [ADC counts]",
      n_adc_channels, 0, n_adc_channels));
  m_profile_tdc = reg.Add(std::make_unique<TProfile>(
      "TDC_mean", "Mean of TDC values;channel;mean TDC", n_tdc_channels, 0, n_tdc_channels));
  m_hist_tdc_inclusive = reg.Add(std::make_unique<TH1D>("TDC_inclusive", "Inclusive TDC values", 4096, 0, 4096));
  m_profile_tdc_saturation = reg.Add(std::make_unique<TProfile>(
      "TDC_saturation", "Saturation fraction per TDC channel;channel;saturation fraction", n_tdc_channels, 0,
      n_tdc_channels));

  // Per-channel ADC distributions: one TH2I per trigger copy (x = channel,
  // y = ADC), total/physics/pedestal. A single TH2 keeps the registered-object
  // count small; the frontend reads one channel via a server-side ProjectionY
  // (issue #138). The pedestal one also feeds UpdatePedestalNoise.
  auto book_adc_dist = [&](const char* name, const char* title) {
    return reg.Add(std::make_unique<TH2I>(name, title, n_adc_channels, 0, n_adc_channels, 4096, 0, 4096));
  };
  m_adc_dist = book_adc_dist("ADC_dist", "ADC values;channel;ADC");
  m_adc_dist_physics = book_adc_dist("ADC_dist_physics", "ADC values (physics);channel;ADC");
  m_adc_dist_pedestal = book_adc_dist("ADC_dist_pedestal", "ADC values (pedestal);channel;ADC");
  for (unsigned int i = 0; i < n_tdc_channels; ++i) {
    TH1D* hist = reg.Add(std::make_unique<TH1D>(hidra::utils::format("TDC_channel_{}", i).c_str(),
                                                hidra::utils::format("TDC values for channel {}", i).c_str(),
                                                4096,
                                                0,
                                                4096));
    m_hist_tdc_channels.push_back(hist);
  }
}

void XDCFiller::Fill(const HidraEvent& event) {
  // Decide once per event which split copies to fill. The trigger mask is a
  // bitfield (physics = bit 0, pedestal = bit 1), so a "both" event (mask 3)
  // feeds both the physics and pedestal histograms.
  const bool is_physics = event.meta.isPhysics();
  const bool is_pedestal = event.meta.isPedestal();

  for (size_t i = 0; i < event.xdc.ADCvalues.size(); ++i) {
    const double value = event.xdc.ADCvalues[i];
    // The decoder leaves channels with no hit at the -1 sentinel; skip them so
    // they don't drag down ADC_mean or dilute the saturation fraction.
    if (value < 0) {
      continue;
    }
    m_profile_adc->Fill(i, value);
    m_hist_adc_inclusive->Fill(value);
    if (is_physics) {
      m_profile_adc_physics->Fill(i, value);
      m_hist_adc_inclusive_physics->Fill(value);
    }
    if (is_pedestal) {
      m_profile_adc_pedestal->Fill(i, value);
      m_hist_adc_inclusive_pedestal->Fill(value);
    }
    if (i < m_n_adc_channels) {
      m_adc_dist->Fill(i, value);
      if (is_physics) {
        m_adc_dist_physics->Fill(i, value);
      }
      if (is_pedestal) {
        m_adc_dist_pedestal->Fill(i, value);
      }
    } else {
      HIDRA_ERROR("ADC channel index {} is out of bounds for the ADC_dist TH2 x-axis (n_adc_channels={}). Skipping "
                  "filling for this channel.",
                  i,
                  m_n_adc_channels);
    }
    const int saturated = value > m_saturation_threshold_adc ? 1 : 0;
    m_profile_adc_saturation->Fill(i, saturated);
    if (is_physics) {
      m_profile_adc_saturation_physics->Fill(i, saturated);
    }
    if (is_pedestal) {
      m_profile_adc_saturation_pedestal->Fill(i, saturated);
    }
  }

  for (size_t i = 0; i < event.xdc.TDCvalues.size(); ++i) {
    const double value = event.xdc.TDCvalues[i];
    if (value < 0) {
      continue;
    }
    m_profile_tdc->Fill(i, value);
    m_hist_tdc_inclusive->Fill(value);
    if (i < m_hist_tdc_channels.size()) {
      m_hist_tdc_channels[i]->Fill(value);
      m_profile_tdc_saturation->Fill(i, value > m_saturation_threshold_tdc ? 1 : 0);
    }
  }

  // Refresh the noise estimates periodically from the accumulated
  // per-channel pedestal distributions (see UpdatePedestalNoise).
  if (is_pedestal && ++m_pedestal_events_since_noise >= m_noise_update_interval) {
    UpdatePedestalNoise();
    m_pedestal_events_since_noise = 0;
  }
}

void XDCFiller::UpdatePedestalNoise() {
  const double probs[2] = {0.25, 0.75};
  double quants[2] = {0.0, 0.0};
  for (unsigned int c = 0; c < m_n_adc_channels; ++c) {
    // One channel's pedestal distribution is column c+1 of the TH2. Own the
    // projection with a unique_ptr (RAII, freed at end of iteration) and detach
    // it from any ROOT directory (SetDirectory(nullptr)) so gDirectory neither
    // owns nor double-frees it — no reliance on ROOT's gDirectory bookkeeping.
    std::unique_ptr<TH1D> h(m_adc_dist_pedestal->ProjectionY("_adc_ped_col", c + 1, c + 1));
    h->SetDirectory(nullptr);
    double iqr_sigma = 0.0;
    double std_sigma = 0.0;
    // The estimators need a populated distribution; leave the noise at 0 for
    // channels with no pedestal entries yet. GetEntries() is the cheap guard;
    // GetQuantiles (below) reports via nq whether the quantiles are usable.
    if (h->GetEntries() > 0) {
      const int nq = h->GetQuantiles(2, quants, probs);
      if (nq == 2) {
        const double iqr = quants[1] - quants[0];
        iqr_sigma = iqr > 0 ? iqr / kNormalIQRtoSigma : 0.0;
      }
      std_sigma = h->GetStdDev();
    }
    const int bin = static_cast<int>(c) + 1;
    m_hist_adc_noise_pedestal->SetBinContent(bin, iqr_sigma);
    m_hist_adc_noise_std_pedestal->SetBinContent(bin, std_sigma);
  }
}

void XDCFiller::Reset() {
  // The histogram contents are cleared by the registry; just reset the
  // recompute counter so the noise is rebuilt fresh for the new run.
  m_pedestal_events_since_noise = 0;
}
