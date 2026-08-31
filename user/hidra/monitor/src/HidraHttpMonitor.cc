#include "HidraHttpMonitor.hh"

#include "HidraEvent.hh"

#include "SummaryFiller.hh"
#include "XDCFiller.hh"
#include "FERSFiller.hh"
#include "TrackerFiller.hh"
#include "MetaFiller.hh"
#include "ChannelSumFiller.hh"
#include "HidraUtils.hh"
#include "ScopedTimer.hh"

#include <HidraFersDecoder.hh>
#include <HidraFersRandomDecoder.hh>

#include <eudaq/Event.hh>
#include <eudaq/Factory.hh>
#include <eudaq/FileNamer.hh>

#include <nlohmann/json.hpp>

#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

const uint32_t HidraHttpMonitor::m_id_factory = eudaq::cstr2hash("HidraHttpMonitor");

namespace {
auto _reg = eudaq::Factory<eudaq::Monitor>::Register<HidraHttpMonitor, const std::string&, const std::string&>(
    HidraHttpMonitor::m_id_factory);

// Strip leading/trailing whitespace; returns "" for an all-whitespace string.
std::string Trim(const std::string& s) {
  const auto first = s.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = s.find_last_not_of(" \t\r\n");
  return s.substr(first, last - first + 1);
}

// Parse a JSON object key as a non-negative channel index. Returns nullopt when
// the key is not all digits or is out of int range, so one malformed key skips
// its entry instead of std::stoi throwing and aborting the whole map load.
std::optional<int> ParseChannelKey(const std::string& key) {
  if (key.empty() || key.find_first_not_of("0123456789") != std::string::npos) {
    return std::nullopt;
  }
  try {
    return std::stoi(key);
  } catch (const std::exception&) {
    return std::nullopt; // out of int range
  }
}

// Parse the PMT (ADC) channel map adc_channels.json: {"<chan>":"M105S", ...}.
// A name shaped M<digits>(S|C) is a PMT; its last character is the type. Other
// names (Muon, Cher*, T*, ...) are not PMTs and are skipped. Appends the integer
// channel keys to s_channels / c_channels by type.
void LoadPmtTypeChannels(const std::string& path, std::vector<int>& s_channels, std::vector<int>& c_channels) {
  std::ifstream f(path);
  if (!f) {
    HIDRA_WARN("ADC channel map '{}' could not be opened; PMT sum histograms will stay empty.", path);
    return;
  }
  nlohmann::json j;
  f >> j;
  for (auto it = j.begin(); it != j.end(); ++it) {
    if (!it.value().is_string()) {
      continue;
    }
    const std::string name = it.value().get<std::string>();
    if (name.size() < 3 || name.front() != 'M' || (name.back() != 'S' && name.back() != 'C')) {
      continue;
    }
    bool all_digits = true;
    for (std::size_t k = 1; k + 1 < name.size(); ++k) {
      if (std::isdigit(static_cast<unsigned char>(name[k])) == 0) {
        all_digits = false;
        break;
      }
    }
    if (!all_digits) {
      continue;
    }
    if (const auto chan = ParseChannelKey(it.key())) {
      (name.back() == 'S' ? s_channels : c_channels).push_back(*chan);
    }
  }
}

// Parse the SiPM (FERS) channel map sipm_channels.json: {"<chan>":[col,row,"S",module], ...}.
// Element [2] is the type string. Appends the integer channel keys by type.
void LoadSipmTypeChannels(const std::string& path, std::vector<int>& s_channels, std::vector<int>& c_channels) {
  std::ifstream f(path);
  if (!f) {
    HIDRA_WARN("SiPM channel map '{}' could not be opened; SiPM sum histograms will stay empty.", path);
    return;
  }
  nlohmann::json j;
  f >> j;
  for (auto it = j.begin(); it != j.end(); ++it) {
    const auto& entry = it.value();
    if (!entry.is_array() || entry.size() < 3 || !entry[2].is_string()) {
      continue;
    }
    const std::string type = entry[2].get<std::string>();
    if (type != "S" && type != "C") {
      continue;
    }
    if (const auto chan = ParseChannelKey(it.key())) {
      (type == "S" ? s_channels : c_channels).push_back(*chan);
    }
  }
}
} // namespace

// ── MonitorContext ──────────────────────────────────────────────────────────

HidraHttpMonitor::MonitorContext::MonitorContext(
    int port,
    int pump_interval_ms,
    int prescale,
    hidra::HidraXdcDecoder xdc_dec,
    std::unique_ptr<hidra::IFersDecoder> fers_dec,
    std::unique_ptr<hidra::IFersDecoder> maxicc_dec,
    int n_adc_channels,
    int noise_update_interval,
    int fers_nboards,
    int fers_value_max,
    int fers_channel_nbins,
    int fers_saturation_threshold,
    bool fers_per_channel_distributions,
    int maxicc_nboards,
    std::vector<TrackerStationConfig> tracker_stations,
    std::string http_output_dir,
    int trigger_strip_length,
    int trigger_gap_max,
    ChannelSumConfig sum_config)
    : publisher(registry, port, pump_interval_ms, std::move(http_output_dir)),
      chain(publisher.Mutex()),
      xdc_decoder(std::move(xdc_dec)),
      fers_decoder(std::move(fers_dec)),
      maxicc_decoder(std::move(maxicc_dec)),
      fers_nboards(fers_nboards),
      fers_value_max(fers_value_max),
      maxicc_nboards(maxicc_nboards),
      trigger_strip_length(trigger_strip_length),
      trigger_gap_max(trigger_gap_max),
      event_prescale(prescale) {

  chain.Add(std::make_unique<SummaryFiller>(registry, prescale));
  chain.Add(std::make_unique<XDCFiller>(registry, n_adc_channels, 100, 3800, 3800, noise_update_interval));
  chain.Add(std::make_unique<FERSFiller>(registry, static_cast<unsigned int>(fers_nboards), 64u, fers_value_max,
                                         fers_channel_nbins, fers_saturation_threshold,
                                         fers_per_channel_distributions));
  // MAXICC: same FERS filler, its own sub-event (HidraEvent::maxicc) and `MAXICC_*`
  // histograms, sized for the MAXICC board count (3).
  chain.Add(std::make_unique<FERSFiller>(registry, static_cast<unsigned int>(maxicc_nboards), 64u, fers_value_max,
                                         fers_channel_nbins, fers_saturation_threshold, fers_per_channel_distributions,
                                         "MAXICC", &HidraEvent::maxicc));
  chain.Add(std::make_unique<TrackerFiller>(registry, tracker_stations));
  chain.Add(std::make_unique<MetaFiller>(registry, trigger_strip_length, trigger_gap_max));
  chain.Add(std::make_unique<ChannelSumFiller>(registry, sum_config));

  // Start the HTTP server only after all fillers are constructed, so THttpServer sees the complete set of histograms
  // from the start.
  publisher.Start();
}

HidraHttpMonitor::MonitorContext::~MonitorContext() noexcept {
  publisher.Stop();
}

void HidraHttpMonitor::MonitorContext::ResetTelemetry() {
  // Caller holds publisher.Mutex(): ProcessRequestsTimer is written by the pump thread inside that same lock, and the
  // decode/fill timers are written by DoReceive which is not running at a run boundary.
  duration_xdc_decode.Reset();
  duration_fers_decode.Reset();
  duration_maxicc_decode.Reset();
  duration_tracker_decode.Reset();
  chain.LockWaitTimer().Reset();
  for (const auto& filler : chain.Fillers()) {
    filler->Timer().Reset();
  }
  publisher.ProcessRequestsTimer().Reset();
}

void HidraHttpMonitor::MonitorContext::LogTelemetry() {
  HIDRA_INFO("=== Monitor Telemetry ===");

  HIDRA_INFO("  " + duration_xdc_decode.Summary());
  HIDRA_INFO("  " + duration_fers_decode.Summary());
  HIDRA_INFO("  " + duration_maxicc_decode.Summary());
  HIDRA_INFO("  " + duration_tracker_decode.Summary());

  // Lock wait — this is the time spent waiting for the histogram lock in FillerChain::Fill(). If this is high, it means
  // the pump thread is contending with DoReceive for the lock, which may indicate that the pump frequency is too high
  // or that the fillers are doing too much work inside the critical section.
  HIDRA_INFO("  " + chain.LockWaitTimer().Summary());

  for (const auto& filler : chain.Fillers()) {
    HIDRA_INFO("  " + filler->Timer().Summary());
  }

  HIDRA_INFO("  " + publisher.ProcessRequestsTimer().Summary());
}

// ── HttpMonitor ───────────────────────────────────────────────────────────

HidraHttpMonitor::HidraHttpMonitor(const std::string& name, const std::string& runcontrol)
    : eudaq::Monitor(name, runcontrol) {}

void HidraHttpMonitor::DoInitialise() {
  HIDRA_INFO("Initializing HidraHttpMonitor");
  if (auto ini = GetInitConfiguration()) {
    m_port = ini->Get("HTTP_PORT", 9090);
    m_pump_interval_ms = ini->Get("PUMP_INTERVAL_MS", 20);
    m_event_prescale = ini->Get("EVENT_PRESCALE", 1);
    if (m_event_prescale == 0) {
      HIDRA_WARN("EVENT_PRESCALE=0 is invalid, forcing EVENT_PRESCALE=1");
      m_event_prescale = 1;
    }
    m_noise_update_interval = ini->Get("PEDESTAL_NOISE_UPDATE_EVENTS", 200);
    if (m_noise_update_interval < 1) {
      HIDRA_WARN("PEDESTAL_NOISE_UPDATE_EVENTS={} is invalid, forcing 1", m_noise_update_interval);
      m_noise_update_interval = 1;
    }
    // FileNamer pattern for the ROOT file written at end-of-run. Set empty to disable saving.
    m_histo_output_pattern = ini->Get("HISTO_OUTPUT_PATTERN", m_histo_output_pattern);
  }
}

std::string HidraHttpMonitor::MakeHistoOutputFile() const {
  std::time_t time_now = std::time(nullptr);
  // Thread-safe local-time conversion (std::localtime uses shared static storage). localtime_r is POSIX, localtime_s is
  // the Windows equivalent (note the reversed argument order).
  std::tm tm_buf{};
#ifdef _WIN32
  localtime_s(&tm_buf, &time_now);
#else
  localtime_r(&time_now, &tm_buf);
#endif
  // strftime null-terminates on success; zero-initialise so the buffer is a valid (empty) string if it ever fails.
  char time_buff[13] = {};
  std::string date_str;
  if (std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S", &tm_buf) != 0) {
    date_str = time_buff;
  } else {
    // Fall back to a non-empty, unique value so the $D field cannot collapse to empty and make two runs clobber the
    // same output file.
    date_str = std::to_string(static_cast<long long>(time_now));
    HIDRA_WARN("strftime failed to format the timestamp; using epoch seconds ({}) in the monitor output file name",
               date_str);
  }

  return eudaq::FileNamer(m_histo_output_pattern)
      .Set('X', ".root")
      .Set('R', GetRunNumber())
      .Set('D', date_str);
}

void HidraHttpMonitor::DoConfigure() {
  HIDRA_INFO("Configuring HidraHttpMonitor");
  auto conf = GetConfiguration();

  std::map<int, std::string> vme_geo_map;
  std::string vmecrateconfig = conf->Get("VME_CRATE_1", "");
  if (vmecrateconfig != "") {
    std::map<std::string, std::string> tempvme = hidra::utils::parseConfigMap(vmecrateconfig);
    for (const auto& kv : tempvme) {
      const std::string& geo = kv.first;
      const std::string& modname = kv.second;
      int geoaddr = std::stoi(geo);
      vme_geo_map[geoaddr] = modname;
      HIDRA_INFO("VME module at geo address {} is {}", geoaddr, modname);
    }
  }

  hidra::HidraXdcDecoder xdc_decoder(vme_geo_map);
  // Capture the channel count before moving the decoder: the XDCFiller histograms are sized from it. As the fillers are
  // built once (the histogram set is fixed for the life of the server), this is only consumed on the first configure.
  const int n_adc_channels = xdc_decoder.NADCChannels();

  // FERS decoder selection and histogram sizing live in the run config, read and passed through here just like the XDC
  // VME_CRATE_1 → n_adc_channels above (so the monitor holds no FERS-specific state). FERS_DECODER picks the real decoder
  // or the random test generator; FERS_NBOARDS / FERS_VALUE_MAX / FERS_CHANNEL_NBINS / FERS_SATURATION_THRESHOLD /
  // FERS_PER_CHANNEL_DISTRIBUTIONS size and shape the FERS histograms (only used on the first configure, when the filler
  // set is built).
  const std::string fers_decoder_kind = conf->Get("FERS_DECODER", std::string("real"));
  const bool fers_random = (fers_decoder_kind == "random");
  if (!fers_random && fers_decoder_kind != "real") {
    HIDRA_WARN("FERS_DECODER='{}' is unknown (expected 'real' or 'random'); using the real decoder.", fers_decoder_kind);
  }
  int cfg_nboards = conf->Get("FERS_NBOARDS", 20);
  const int cfg_value_max = conf->Get("FERS_VALUE_MAX", 4096);
  const int fers_channel_nbins = conf->Get("FERS_CHANNEL_NBINS", 1024);
  const int fers_saturation_threshold = conf->Get("FERS_SATURATION_THRESHOLD", 3800);
  const bool fers_per_channel = conf->Get("FERS_PER_CHANNEL_DISTRIBUTIONS", 1) != 0;
  if (cfg_nboards < 1) {
    HIDRA_WARN("FERS_NBOARDS={} is invalid, forcing 20.", cfg_nboards);
    cfg_nboards = 20;
  }
  // MAXICC is a separate FERS-format sub-event (det_id 4) with its own boards
  // (3). It reuses the FERS value range / binning above; only the board count
  // differs. Like the FERS sizing, this is consumed once on the first configure.
  int cfg_maxicc_nboards = conf->Get("MAXICC_NBOARDS", 3);
  if (cfg_maxicc_nboards < 1) {
    HIDRA_WARN("MAXICC_NBOARDS={} is invalid, forcing 3.", cfg_maxicc_nboards);
    cfg_maxicc_nboards = 3;
  }

  // Tracker 2D hit maps: one TH2 per station, sized from the run config (like the
  // FERS sizing above, only consumed on the first configure). `TRACKER_NSTATIONS`
  // stations all default to the global `TRACKER_X_*` / `TRACKER_Y_*` range and
  // binning; an optional per-station `TRACKER_STATION<i> = xmin,xmax,ymin,ymax`
  // overrides the range of that station.
  std::vector<TrackerStationConfig> tracker_stations;
  {
    int n_stations = conf->Get("TRACKER_NSTATIONS", 3);
    if (n_stations < 0) {
      HIDRA_WARN("TRACKER_NSTATIONS={} is invalid, forcing 0.", n_stations);
      n_stations = 0;
    }
    TrackerStationConfig def;
    // Coordinates are in cm (official format). Range [0, 11] with 110 bins gives
    // a clean 0.1 cm/bin (0 and 10 land on bin edges); the upper margin to 11
    // leaves room above the ~10 cm detector. Use doubles for every default so a
    // fractional override isn't truncated by the int Get() overload.
    def.x_nbins = conf->Get("TRACKER_X_NBINS", 55);
    def.x_min = conf->Get("TRACKER_X_MIN", 0.0);
    def.x_max = conf->Get("TRACKER_X_MAX", 11.0);
    def.y_nbins = conf->Get("TRACKER_Y_NBINS", 55);
    def.y_min = conf->Get("TRACKER_Y_MIN", 0.0);
    def.y_max = conf->Get("TRACKER_Y_MAX", 11.0);

    for (int i = 0; i < n_stations; ++i) {
      TrackerStationConfig station = def;
      const std::string override_str = conf->Get(hidra::utils::format("TRACKER_STATION{}", i), std::string(""));
      // The EUDAQ config parser keeps inline comments on the value (it only
      // drops whole lines starting with '#'/';'), so strip a trailing "# ..."
      // or "; ..." before parsing — the repo's .conf files use inline comments.
      std::string spec = override_str;
      const auto comment = spec.find_first_of("#;");
      if (comment != std::string::npos) {
        spec = spec.substr(0, comment);
      }
      if (!Trim(spec).empty()) {
        // Expect "xmin,xmax,ymin,ymax"; binning stays the global default. Each
        // token is parsed strictly: trimmed, and the whole token must be a
        // number (no trailing garbage like "50abc"), else the override is
        // rejected and the global range kept.
        std::vector<double> vals;
        std::stringstream ss(spec);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
          const std::string trimmed = Trim(tok);
          if (trimmed.empty()) {
            vals.clear();
            break;
          }
          std::size_t pos = 0;
          try {
            const double v = std::stod(trimmed, &pos);
            if (pos != trimmed.size()) {
              throw std::invalid_argument("trailing characters");
            }
            vals.push_back(v);
          } catch (const std::exception&) {
            vals.clear();
            break;
          }
        }
        if (vals.size() == 4) {
          station.x_min = vals[0];
          station.x_max = vals[1];
          station.y_min = vals[2];
          station.y_max = vals[3];
        } else {
          HIDRA_WARN("TRACKER_STATION{}='{}' is malformed (expected 'xmin,xmax,ymin,ymax'); using global range.", i,
                     override_str);
        }
      }
      tracker_stations.push_back(station);
    }
  }

  // Trigger-mask pattern monitoring (MetaFiller). Sized once on the first configure,
  // like the FERS/tracker histograms above. The strip shows the last N raw trigger-mask
  // values; the gap histogram the number of non-pedestal events between pedestals.
  int trigger_strip_length = conf->Get("TRIGGER_MASK_STRIP_LENGTH", 200);
  if (trigger_strip_length < 1) {
    HIDRA_WARN("TRIGGER_MASK_STRIP_LENGTH={} is invalid, forcing 200.", trigger_strip_length);
    trigger_strip_length = 200;
  }
  int trigger_gap_max = conf->Get("TRIGGER_PATTERN_GAP_MAX", 30);
  if (trigger_gap_max < 1) {
    HIDRA_WARN("TRIGGER_PATTERN_GAP_MAX={} is invalid, forcing 30.", trigger_gap_max);
    trigger_gap_max = 30;
  }

  // Per-event channel SUM distributions (ChannelSumFiller). The S/C classification
  // is parsed from the frontend channel maps (the backend has no type info). The
  // map paths default to the in-repo frontend maps via ${REPO_ROOT} (exported by
  // misc/setup.sh), so no run-config entry is needed; ${VAR} is expanded from the
  // environment. A missing map / env var / parse error disables the affected groups
  // (the histograms book but stay empty) rather than failing the whole configure.
  // The SiPM axis maxima default to 3M (the auto group_size*full_scale ~2.6M is too
  // coarse to resolve the baseline). Sized once on the first configure.
  ChannelSumConfig sum_config;
  sum_config.adc_value_max = 4096; // V792 full scale (matches XDCFiller's ADC range)
  sum_config.fers_value_max = cfg_value_max;
  sum_config.nbins = conf->Get("SUM_NBINS", 1024);
  sum_config.pmt_max = conf->Get("SUM_PMT_MAX", 0);
  sum_config.sipm_hg_max = conf->Get("SUM_SIPM_HG_MAX", 3000000);
  sum_config.sipm_lg_max = conf->Get("SUM_SIPM_LG_MAX", 3000000);
  const std::string adc_map_default = "${REPO_ROOT}/user/hidra/monitor/frontend/hidra_frontend/mapping/adc_channels.json";
  const std::string sipm_map_default =
      "${REPO_ROOT}/user/hidra/monitor/frontend/hidra_frontend/mapping/sipm_channels.json";
  try {
    const std::string adc_map = hidra::utils::ExpandEnv(conf->Get("ADC_CHANNEL_MAP_JSON", adc_map_default));
    if (adc_map.empty()) {
      HIDRA_WARN("ADC_CHANNEL_MAP_JSON is empty; PMT sum histograms will stay empty.");
    } else {
      LoadPmtTypeChannels(adc_map, sum_config.pmt_s, sum_config.pmt_c);
    }
  } catch (const std::exception& e) {
    HIDRA_WARN("ADC channel map not loaded ({}); PMT sum histograms will stay empty.", e.what());
  }
  try {
    const std::string sipm_map = hidra::utils::ExpandEnv(conf->Get("SIPM_CHANNEL_MAP_JSON", sipm_map_default));
    if (sipm_map.empty()) {
      HIDRA_WARN("SIPM_CHANNEL_MAP_JSON is empty; SiPM sum histograms will stay empty.");
    } else {
      LoadSipmTypeChannels(sipm_map, sum_config.sipm_s, sum_config.sipm_c);
    }
  } catch (const std::exception& e) {
    HIDRA_WARN("SiPM channel map not loaded ({}); SiPM sum histograms will stay empty.", e.what());
  }
  HIDRA_INFO("Channel sums: PMT S={} C={}, SiPM S={} C={}", sum_config.pmt_s.size(), sum_config.pmt_c.size(),
             sum_config.sipm_s.size(), sum_config.sipm_c.size());

  std::unique_lock<std::shared_mutex> lock(m_state_mutex);

  // Like the FERS sizing, the trigger-pattern histograms are sized once; warn if a
  // reconfigure changes a value, since the already-built histograms keep the old one.
  if (m_ctx && (trigger_strip_length != m_ctx->trigger_strip_length || trigger_gap_max != m_ctx->trigger_gap_max)) {
    HIDRA_WARN("Trigger-pattern sizing change ignored on reconfigure (histograms already sized); keeping "
               "strip_length={}, gap_max={}.",
               m_ctx->trigger_strip_length, m_ctx->trigger_gap_max);
  }

  // The FERS histograms are sized once (first configure). On a reconfigure reuse the sizing the histograms were built
  // with, so the rebuilt decoder stays consistent with them (re-reading changed FERS_NBOARDS / FERS_VALUE_MAX would
  // otherwise make the random decoder produce a channel count / range the booked histograms can't hold).
  const int eff_nboards = m_ctx ? m_ctx->fers_nboards : cfg_nboards;
  const int eff_value_max = m_ctx ? m_ctx->fers_value_max : cfg_value_max;
  if (m_ctx && (cfg_nboards != eff_nboards || cfg_value_max != eff_value_max)) {
    HIDRA_WARN("FERS sizing change ignored on reconfigure (histograms already sized); keeping nboards={}, value_max={}.",
               eff_nboards, eff_value_max);
  }
  std::unique_ptr<hidra::IFersDecoder> fers_decoder;
  if (fers_random) {
    HIDRA_WARN("FERS_DECODER=random: the monitor will histogram synthetic FERS data (TEST ONLY).");
    fers_decoder =
        std::make_unique<hidra::HidraFersRandomDecoder>(static_cast<unsigned int>(eff_nboards), 64u, eff_value_max);
  } else {
    fers_decoder = std::make_unique<hidra::HidraFersDecoder>();
  }

  // MAXICC decoder: mirrors the FERS one (same FERS_DECODER selection), but sized
  // for the MAXICC board count. The real decoder caps board_id at max_boards; the
  // random one synthesises that many boards so MAXICC histograms show data in dry.
  const int eff_maxicc_nboards = m_ctx ? m_ctx->maxicc_nboards : cfg_maxicc_nboards;
  std::unique_ptr<hidra::IFersDecoder> maxicc_decoder;
  if (fers_random) {
    maxicc_decoder = std::make_unique<hidra::HidraFersRandomDecoder>(static_cast<unsigned int>(eff_maxicc_nboards), 64u,
                                                                     eff_value_max);
  } else {
    maxicc_decoder = std::make_unique<hidra::HidraFersDecoder>(static_cast<std::size_t>(eff_maxicc_nboards));
  }

  if (!m_ctx) {
    // First configure: build the long-lived monitoring context. This starts the HTTP server with empty histograms,
    // so the GUI is reachable from now on and stays up across run start/stop.
    //
    // Absolute directory of the histogram snapshots (HISTO_OUTPUT_PATTERN's folder), published over HTTP so the
    // frontend's reference-overlay finds the files without per-deployment config. Empty when saving is disabled.
    // The monitor's CWD is the run dir, so a relative pattern resolves there.
    std::string http_output_dir;
    if (!m_histo_output_pattern.empty()) {
      // Non-throwing absolute(): on the rare OS error (e.g. CWD removed) just
      // leave the dir unexposed rather than failing the whole configure.
      std::error_code ec;
      const std::filesystem::path dir =
          std::filesystem::absolute(std::filesystem::path(m_histo_output_pattern).parent_path(), ec);
      if (!ec) {
        http_output_dir = dir.lexically_normal().string();
      }
    }
    m_ctx = std::make_unique<MonitorContext>(m_port, m_pump_interval_ms, m_event_prescale, std::move(xdc_decoder),
                                             std::move(fers_decoder), std::move(maxicc_decoder), n_adc_channels,
                                             m_noise_update_interval, cfg_nboards, cfg_value_max, fers_channel_nbins,
                                             fers_saturation_threshold, fers_per_channel, cfg_maxicc_nboards,
                                             std::move(tracker_stations), std::move(http_output_dir),
                                             trigger_strip_length, trigger_gap_max, std::move(sum_config));
  } else {
    // Reconfigure: keep the server alive, only swap the decoders to the new configuration. Decoder identity and state
    // are protected solely by m_state_mutex (held unique here) and are never touched by the pump thread, so the swap
    // does not need publisher.Mutex(); the unique lock already excludes DoReceive, the only reader.
    m_ctx->xdc_decoder = std::move(xdc_decoder);
    m_ctx->fers_decoder = std::move(fers_decoder);
    m_ctx->maxicc_decoder = std::move(maxicc_decoder);
  }

  // Clear the histogram contents left over from a previous run: a (re)configuration means a fresh setup, so the GUI
  // should not keep showing stale data. We deliberately do NOT reset the fillers' run-relative state here (e.g.
  // SummaryFiller's start-of-run time reference): that belongs to DoStartRun, as a configure may happen well before the
  // run actually starts. On the first configure the histograms are already empty, so this is a no-op.
  std::lock_guard<std::mutex> fill_lock(m_ctx->publisher.Mutex());
  m_ctx->registry.Reset();
}

void HidraHttpMonitor::DoStartRun() {
  HIDRA_INFO("Starting HidraHttpMonitor run");

  std::shared_lock<std::shared_mutex> lock(m_state_mutex);
  if (!m_ctx) {
    EUDAQ_THROW("HidraHttpMonitor started before being configured");
  }

  // Reset the histograms (in place, so the THttpServer keeps pointing at the same objects) and the per-run state, so
  // the new run starts from a clean slate while the server keeps serving.
  m_ctx->event_counter.store(0, std::memory_order_relaxed);
  std::lock_guard<std::mutex> fill_lock(m_ctx->publisher.Mutex());
  m_ctx->chain.Reset();
  m_ctx->registry.Reset();
  m_ctx->ResetTelemetry();
  m_ctx->run_active = true; // re-arm finalization for the new run
}

void HidraHttpMonitor::FinalizeRun() {
  // Caller holds m_state_mutex and guarantees m_ctx is set.
  // The telemetry log and the ROOT save are done while holding publisher.Mutex() on purpose: ROOT is not thread-safe,
  // so writing a TFile concurrently with the pump thread's TBufferJSON serialization would race on global ROOT state.
  // The only cost is briefly pausing HTTP serialization at end-of-run, when no events are arriving anyway.
  std::lock_guard<std::mutex> fill_lock(m_ctx->publisher.Mutex());
  if (!m_ctx->run_active) {
    return; // already finalized (e.g. STOP then TERMINATE), or no run was started
  }

  m_ctx->LogTelemetry();

  if (!m_histo_output_pattern.empty()) {
    const std::string path = MakeHistoOutputFile();
    if (m_ctx->registry.SaveToFile(path)) {
      HIDRA_INFO("Saved monitor histograms to {}", path);
    } else {
      HIDRA_WARN("Failed to save monitor histograms to {}", path);
    }
  }

  m_ctx->run_active = false;
}

void HidraHttpMonitor::DoStopRun() {
  HIDRA_INFO("Stopping HidraHttpMonitor run");
  // The run is over but we deliberately keep m_ctx (and its HTTP server) alive so the histograms of the run just
  // finished stay browsable until the next run starts or the monitor terminates. Here we just finalize the run (log
  // telemetry and snapshot the histograms to a ROOT file).

  std::shared_lock<std::shared_mutex> lock(m_state_mutex);
  if (!m_ctx) {
    return;
  }
  FinalizeRun();
}

void HidraHttpMonitor::DoReset() {
  HIDRA_INFO("Resetting HidraHttpMonitor state");
  std::shared_lock<std::shared_mutex> lock(m_state_mutex);
  if (!m_ctx) {
    return;
  }
  std::lock_guard<std::mutex> fill_lock(m_ctx->publisher.Mutex());
  m_ctx->chain.Reset();
  m_ctx->registry.Reset();
}

void HidraHttpMonitor::DoTerminate() {
  HIDRA_INFO("Terminating HidraHttpMonitor");
  // Final shutdown. The unique lock waits for in-flight DoReceive to finish. If the monitor is terminated while a run
  // is still active (no explicit STOP), finalize it first so telemetry and the ROOT snapshot are not lost; FinalizeRun
  // is a no-op if the run was already finalized. Then destroy the context, which stops the HTTP server.
  std::unique_lock<std::shared_mutex> lock(m_state_mutex);
  if (m_ctx) {
    FinalizeRun();
  }
  m_ctx.reset();
}

void HidraHttpMonitor::DoReceive(eudaq::EventSP ev) {
  // Shared lock: keeps m_ctx alive and the decoders stable for the whole call.
  std::shared_lock<std::shared_mutex> lock(m_state_mutex);
  if (!m_ctx) {
    return;
  }

  const uint64_t event_index = m_ctx->event_counter.fetch_add(1, std::memory_order_relaxed);
  if ((event_index % m_ctx->event_prescale) != 0) {
    return;
  }

  // ── Decoding — outside the histogram lock ───────────────────────────
  // Each decoder writes to its own field of HidraEvent. Decoding is read-only on the payload and doesn't touch
  // histograms, so there's no reason to keep it inside the critical section. The lock only protects the Fill() that
  // follows.

  HidraEvent decoded;

  // Per-event metadata (trigger mask, spill, timestamps, …) comes from the EUDAQ event/tags, not the binary payload.
  m_ctx->meta_decoder.decode(*ev, decoded.meta);

  std::vector<std::uint8_t> fers_payload;
  std::vector<std::uint8_t> maxicc_payload;
  for (size_t index = 0; index < ev->GetNumSubEvent(); ++index) {
    eudaq::EventSPC subevent = ev->GetSubEvent(index); // no copy, just a shared pointer copy of the subevent handle
    if (!subevent) {
      continue;
    }

    const int det_id = hidra::utils::getTagOr<int>(*subevent, "detID", index);
    const auto block_ids = subevent->GetBlockNumList();
    std::size_t total_payload_size = 0;
    for (const auto block_id : block_ids) {
      total_payload_size += subevent->GetBlock(block_id).size();
    }

    std::vector<std::uint8_t> detector_payload;
    detector_payload.reserve(total_payload_size);
    for (const auto block_id : block_ids) {
      const auto block = subevent->GetBlock(block_id);
      detector_payload.insert(detector_payload.end(), block.begin(), block.end());
    }

    if (det_id == 1 || det_id == 6) {
      ScopedTimer t(m_ctx->duration_xdc_decode);
      m_ctx->xdc_decoder.decode(detector_payload, decoded.xdc, subevent->GetTriggerN());
    } else if (det_id == 2) {
      // Defer FERS decoding until after the loop (see below).
      fers_payload = std::move(detector_payload);
    } else if (det_id == 4) {
      // MAXICC: same FERS payload format, its own sub-event. Decoded after the loop.
      maxicc_payload = std::move(detector_payload);
    } else if (det_id == 3) {
      ScopedTimer t(m_ctx->duration_tracker_decode);
      m_ctx->tracker_decoder.decode(detector_payload, decoded.tracker, subevent->GetTriggerN());
    }
  }

  // FERS is decoded once, after the sub-event loop. The real decoder runs on the captured payload (empty when no FERS
  // sub-event was present → all-sentinel vectors, a no-op for the filler), while the random test decoder ignores the
  // payload and always produces data, so the chain is exercised even in the dry chain (no FERS sub-event).
  {
    ScopedTimer t(m_ctx->duration_fers_decode);
    m_ctx->fers_decoder->decode(fers_payload, decoded.fers);
  }

  // MAXICC: decoded the same way as FERS (empty payload → all-sentinel vectors, a
  // no-op for the MAXICC filler; the random decoder ignores the payload).
  {
    ScopedTimer t(m_ctx->duration_maxicc_decode);
    m_ctx->maxicc_decoder->decode(maxicc_payload, decoded.maxicc);
  }

  // --- Dispatch — inside the histogram lock --------------------------
  // FillerChain acquires publisher.Mutex() before calling the fillers.
  m_ctx->chain.Fill(decoded);
}
