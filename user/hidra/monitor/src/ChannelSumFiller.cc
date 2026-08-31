#include "ChannelSumFiller.hh"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

ChannelSumFiller::ChannelSumFiller(HistogramRegistry& reg, const ChannelSumConfig& config)
    : IHistogramFiller("ChannelSumFiller") {
  const int nbins = std::max(1, config.nbins);

  // Add one output (its three trigger-flavour histograms) to a group. The axis
  // upper edge defaults to group_size * value_max (the saturation ceiling); an
  // explicit override focuses the range so the pedestal-dominated baseline and
  // the signal are better resolved. nbins is shared.
  auto add_output = [&](SumGroup& g, const std::string& base, const std::string& label, Source source, int value_max,
                        int override_max, const std::string& xunit) {
    const long auto_max = static_cast<long>(std::max<std::size_t>(g.indices.size(), 1)) * value_max;
    const double xmax = (override_max > 0) ? static_cast<double>(override_max) : static_cast<double>(auto_max);
    const std::string title_all = label + ";" + xunit + ";events";
    const std::string title_phys = label + " (physics);" + xunit + ";events";
    const std::string title_ped = label + " (pedestal);" + xunit + ";events";
    SumOutput o;
    o.source = source;
    o.h_all = reg.Add(std::make_unique<TH1D>(base.c_str(), title_all.c_str(), nbins, 0.0, xmax));
    o.h_physics = reg.Add(std::make_unique<TH1D>((base + "_physics").c_str(), title_phys.c_str(), nbins, 0.0, xmax));
    o.h_pedestal = reg.Add(std::make_unique<TH1D>((base + "_pedestal").c_str(), title_ped.c_str(), nbins, 0.0, xmax));
    g.outputs.push_back(o);
  };

  // PMTs (ADC): one output each.
  {
    SumGroup g;
    g.indices = config.pmt_s;
    add_output(g, "sum_PMT_S", "Sum over scintillator PMTs", Source::ADC, config.adc_value_max, config.pmt_max,
               "sum ADC");
    m_groups.push_back(std::move(g));
  }
  {
    SumGroup g;
    g.indices = config.pmt_c;
    add_output(g, "sum_PMT_C", "Sum over Cherenkov PMTs", Source::ADC, config.adc_value_max, config.pmt_max, "sum ADC");
    m_groups.push_back(std::move(g));
  }
  // SiPMs (FERS): HG and LG share one index list, summed in a single pass.
  {
    SumGroup g;
    g.indices = config.sipm_s;
    add_output(g, "sum_SiPM_S_HG", "Sum over scintillator SiPMs (HG)", Source::FERS_HG, config.fers_value_max,
               config.sipm_hg_max, "sum HG");
    add_output(g, "sum_SiPM_S_LG", "Sum over scintillator SiPMs (LG)", Source::FERS_LG, config.fers_value_max,
               config.sipm_lg_max, "sum LG");
    m_groups.push_back(std::move(g));
  }
  {
    SumGroup g;
    g.indices = config.sipm_c;
    add_output(g, "sum_SiPM_C_HG", "Sum over Cherenkov SiPMs (HG)", Source::FERS_HG, config.fers_value_max,
               config.sipm_hg_max, "sum HG");
    add_output(g, "sum_SiPM_C_LG", "Sum over Cherenkov SiPMs (LG)", Source::FERS_LG, config.fers_value_max,
               config.sipm_lg_max, "sum LG");
    m_groups.push_back(std::move(g));
  }
}

const std::vector<double>& ChannelSumFiller::SourceVector(const HidraEvent& ev, Source source) {
  switch (source) {
  case Source::FERS_HG:
    return ev.fers.FERShg;
  case Source::FERS_LG:
    return ev.fers.FERSlg;
  case Source::ADC:
  default:
    return ev.xdc.ADCvalues;
  }
}

void ChannelSumFiller::Fill(const HidraEvent& ev) {
  const bool is_physics = ev.meta.isPhysics();
  const bool is_pedestal = ev.meta.isPedestal();

  // Each group has at most two outputs (a SiPM group's HG and LG); accumulate
  // their sums on the stack while walking the shared index list a single time.
  constexpr std::size_t kMaxOutputs = 2;
  for (const SumGroup& g : m_groups) {
    const std::size_t n_out = g.outputs.size();
    std::array<const std::vector<double>*, kMaxOutputs> srcs{};
    std::array<double, kMaxOutputs> sums{};
    std::array<bool, kMaxOutputs> any{};
    for (std::size_t k = 0; k < n_out; ++k) {
      srcs[k] = &SourceVector(ev, g.outputs[k].source);
    }

    for (const int idx : g.indices) {
      if (idx < 0) {
        continue;
      }
      const auto uidx = static_cast<std::size_t>(idx);
      for (std::size_t k = 0; k < n_out; ++k) {
        const std::vector<double>& src = *srcs[k];
        if (uidx < src.size()) {
          const double value = src[uidx];
          if (value >= 0) { // skip -1 sentinels (absent / no-hit channels)
            sums[k] += value;
            any[k] = true;
          }
        }
      }
    }

    for (std::size_t k = 0; k < n_out; ++k) {
      // Don't fill when the whole group is absent this event for that source: it
      // would pile a spurious entry at sum = 0 rather than a real measurement.
      if (!any[k]) {
        continue;
      }
      const SumOutput& o = g.outputs[k];
      o.h_all->Fill(sums[k]);
      if (is_physics) {
        o.h_physics->Fill(sums[k]);
      }
      if (is_pedestal) {
        o.h_pedestal->Fill(sums[k]);
      }
    }
  }
}
