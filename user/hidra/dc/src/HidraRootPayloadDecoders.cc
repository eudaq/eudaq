#include "HidraRootPayloadDecoders.hh"
#include "HidraUtils.hh"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hidra {

namespace {

template <typename T> T ReadLE(const std::vector<std::uint8_t>& buffer, std::size_t offset) {
  T value = 0;
  std::memcpy(&value, buffer.data() + offset, sizeof(T));
  return value;
}

template <typename T> T GetBits(T value, unsigned first_bit, unsigned last_bit) {
  static_assert(std::is_unsigned<T>::value, "GetBits expects an unsigned integer type");

  constexpr unsigned bit_count = std::numeric_limits<T>::digits;
  if (first_bit > last_bit || last_bit >= bit_count) {
    return 0;
  }

  const unsigned width = last_bit - first_bit + 1;
  const T mask = (width == bit_count) ? std::numeric_limits<T>::max() : ((T{1} << width) - T{1});
  return (value >> first_bit) & mask;
}

void AddQuantity(std::vector<RootQuantity>& quantities, std::string name, double value, std::string unit = "") {
  quantities.push_back(RootQuantity{std::move(name), value, std::move(unit)});
}

void AddBranchValue(RootBranchValues& branches, const std::string& name, double value) {
  branches[name].push_back(value);
}

void AddBranchValues(RootBranchValues& branches, const std::string& name, const std::vector<double>& values) {
  auto& branch = branches[name];
  branch.insert(branch.end(), values.begin(), values.end());
}

void AddBranchValuesUINT(RootBranchValues& branches, const std::string& name, const std::vector<uint64_t>& values) {
  auto& branch = branches[name];
  branch.insert(branch.end(), values.begin(), values.end());
}

void AddFersBranchNames(std::vector<std::string>& names, const std::string& prefix) {
  names.push_back(prefix + "tsamp_us");
  names.push_back(prefix + "rel_tsamp_us");
  names.push_back(prefix + "trigger_id");
  names.push_back(prefix + "board_id");
  names.push_back(prefix + "hg");
  names.push_back(prefix + "lg");
  names.push_back(prefix + "toa");
  names.push_back(prefix + "tot");
}

void AddFersEventBranches(RootBranchValues& branches, const std::string& prefix, const HidraFersEvent& event) {
  AddBranchValues(branches, prefix + "tsamp_us", event.FERStsamp_us);
  AddBranchValues(branches, prefix + "rel_tsamp_us", event.FERSrel_tsamp_us);
  AddBranchValues(branches, prefix + "trigger_id", event.FERStrigger_id);
  AddBranchValues(branches, prefix + "board_id", event.FERSboard_id);
  AddBranchValues(branches, prefix + "hg", event.FERShg);
  AddBranchValues(branches, prefix + "lg", event.FERSlg);
  AddBranchValues(branches, prefix + "toa", event.FERStoa);
  AddBranchValues(branches, prefix + "tot", event.FERStot);
}

} // namespace

std::vector<std::string> RootPayloadDecoder::BranchNames() const {
  return {};
}

bool HidraGenericPayloadDecoder::Matches(const RootDetectorPayload&) const {
  return true;
}

HidraXdcPayloadDecoder::HidraXdcPayloadDecoder(std::map<int, std::string> vme_geo_map, uint8_t log_level)
    : m_xdc_decoder(std::move(vme_geo_map), log_level) {}

std::vector<std::string> HidraGenericPayloadDecoder::BranchNames() const {
  return {"payload_bytes"};
}

void HidraGenericPayloadDecoder::Decode(const RootDetectorPayload& detector,
                                        std::vector<RootQuantity>& quantities,
                                        RootBranchValues& branches) const {
  AddQuantity(quantities, "payload_bytes", static_cast<double>(detector.payload.size()), "B");

  AddBranchValue(branches, "payload_bytes", static_cast<double>(detector.payload.size()));
  
}

bool HidraXdcPayloadDecoder::Matches(const RootDetectorPayload& detector) const {
  return detector.det_id == 1 || detector.det_id == 6;
}

std::vector<std::string> HidraXdcPayloadDecoder::BranchNames() const {
  auto names = HidraGenericPayloadDecoder{}.BranchNames();
  names.push_back("ADCs");
  names.push_back("ADCFlags");
  names.push_back("TDCs");
  names.push_back("TDCFlags");
  //names.push_back("XDCTriggers");
  return names;
}

void HidraXdcPayloadDecoder::Decode(const RootDetectorPayload& detector,
                                    std::vector<RootQuantity>& quantities,
                                    RootBranchValues& branches) const {
  HidraGenericPayloadDecoder{}.Decode(detector, quantities, branches);

  HidraXdcEvent xdc_event;
  m_xdc_decoder.decode(detector.payload, xdc_event, detector.trigger_n);

  if (xdc_event.ADCvalues.empty()) {
    return;
  }

  AddBranchValues(branches, "ADCs", xdc_event.ADCvalues);
  AddBranchValues(branches, "ADCFlags", xdc_event.ADCflags);
  AddBranchValues(branches, "TDCs", xdc_event.TDCvalues);
  AddBranchValues(branches, "TDCFlags", xdc_event.TDCflags);
  //AddBranchValues(branches, "XDCTriggers", xdc_event.XDCTriggers);
}

bool HidraFersPayloadDecoder::Matches(const RootDetectorPayload& detector) const {
  // TODO: temporary enabling only det_id 2 to abvoid using current decoder for Dry runs on 2025 data
  // return detector.det_id == 2 || detector.det_id == 7 || detector.producer.find("FERS") != std::string::npos;
  return detector.det_id == 2;
}

std::vector<std::string> HidraFersPayloadDecoder::BranchNames() const {
  auto names = HidraGenericPayloadDecoder{}.BranchNames();
  AddFersBranchNames(names, "FERS");
  return names;
}

void HidraFersPayloadDecoder::Decode(const RootDetectorPayload& detector,
                                     std::vector<RootQuantity>& quantities,
                                     RootBranchValues& branches) const {
  HidraGenericPayloadDecoder{}.Decode(detector, quantities, branches);

  HidraFersEvent fers_event;
  m_fers_decoder.decode(detector.payload, fers_event);

  AddFersEventBranches(branches, "FERS", fers_event);
}

HidraMaxiccPayloadDecoder::HidraMaxiccPayloadDecoder()
    : m_fers_decoder(3) {}

bool HidraMaxiccPayloadDecoder::Matches(const RootDetectorPayload& detector) const {
  return (detector.producer == "MAXICCProducer" || detector.det_id == 4);
}

std::vector<std::string> HidraMaxiccPayloadDecoder::BranchNames() const {
  auto names = HidraGenericPayloadDecoder{}.BranchNames();
  AddFersBranchNames(names, "MAXICC");
  return names;
}

void HidraMaxiccPayloadDecoder::Decode(const RootDetectorPayload& detector,
                                       std::vector<RootQuantity>& quantities,
                                       RootBranchValues& branches) const {
  HidraGenericPayloadDecoder{}.Decode(detector, quantities, branches);

  HidraFersEvent fers_event;
  m_fers_decoder.decode(detector.payload, fers_event);

  AddFersEventBranches(branches, "MAXICC", fers_event);
}

bool HidraTrackerPayloadDecoder::Matches(const RootDetectorPayload& detector) const {
  return detector.det_id == 3 || detector.producer.find("Tracker") != std::string::npos;
}

std::vector<std::string> HidraTrackerPayloadDecoder::BranchNames() const {
  auto names = HidraGenericPayloadDecoder{}.BranchNames();
  names.push_back("TrackerX");
  names.push_back("TrackerY");
  return names;
}

void HidraTrackerPayloadDecoder::Decode(const RootDetectorPayload& detector,
                                        std::vector<RootQuantity>& quantities,
                                        RootBranchValues& branches) const {
  HidraGenericPayloadDecoder{}.Decode(detector, quantities, branches);

  HidraTrackerEvent tracker_event;
  m_tracker_decoder.decode(detector.payload, tracker_event, detector.trigger_n);

  if (tracker_event.X.empty()) {
    return;
  }

  AddBranchValues(branches, "TrackerX", tracker_event.X);
  AddBranchValues(branches, "TrackerY", tracker_event.Y);
}

} // namespace hidra
