#pragma once
#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1D.h>
#include <TH2.h>
#include <TProfile.h>

class XDCFiller : public IHistogramFiller {
public:
  explicit XDCFiller(HistogramRegistry& reg,
                     unsigned int n_adc_channels,
                      unsigned int n_tdc_channels,
                     int saturation_threshold_adc = 3800,
                     int saturation_threshold_tdc = 3800,
                     int noise_update_interval = 200);
  void Fill(const HidraEvent&) override;
  void Reset() override;

private:
  unsigned int m_n_adc_channels;

  TProfile* m_profile_adc;
  TH1D* m_hist_adc_inclusive;
  TProfile* m_profile_adc_saturation;

  // Physics/pedestal-split copies of the ADC views (mean, inclusive,
  // per-channel and saturation), selected per event from the trigger mask
  // (physics = bit 0, pedestal = bit 1). All TDC histograms stay inclusive.
  TProfile* m_profile_adc_physics;
  TProfile* m_profile_adc_pedestal;
  TH1D* m_hist_adc_inclusive_physics;
  TH1D* m_hist_adc_inclusive_pedestal;
  TProfile* m_profile_adc_saturation_physics;
  TProfile* m_profile_adc_saturation_pedestal;

  // Per-channel distributions as one TH2I per trigger copy (x = channel,
  // y = ADC), total/physics/pedestal. One TH2 instead of one TH1 per channel
  // keeps the registered-object count tiny so THttpServer stays responsive; the
  // frontend reads one channel via a server-side ProjectionY (issue #138). The
  // pedestal one also feeds UpdatePedestalNoise (per-channel column quantiles).
  TH2I* m_adc_dist;
  TH2I* m_adc_dist_physics;
  TH2I* m_adc_dist_pedestal;

  // Per-channel pedestal noise, one bin per channel. Two estimators are
  // published from each channel's pedestal distribution: the robust
  // IQR/1.349 and the (outlier-sensitive) standard deviation, recomputed
  // together every m_noise_update_interval pedestal events (see Fill /
  // UpdatePedestalNoise).
  TH1D* m_hist_adc_noise_pedestal;     ///< IQR/1.349 (robust).
  TH1D* m_hist_adc_noise_std_pedestal; ///< standard deviation.
  int m_noise_update_interval;
  int m_pedestal_events_since_noise{0};

  void UpdatePedestalNoise();

  TProfile* m_profile_tdc;
  TH1D* m_hist_tdc_inclusive;
  std::vector<TH1D*> m_hist_tdc_channels;
  TProfile* m_profile_tdc_saturation;

  double m_saturation_threshold_adc;
  double m_saturation_threshold_tdc;
};
