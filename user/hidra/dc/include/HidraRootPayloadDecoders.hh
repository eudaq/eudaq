#pragma once

#include "HidraFersDecoder.hh"
#include "HidraTrackerDecoder.hh"
#include "HidraXdcDecoder.hh"

#include <eudaq/Event.hh>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hidra {

struct RootQuantity {
  std::string name;
  double value = 0.0;
  std::string unit;
};

using RootBranchValues = std::map<std::string, std::vector<double>>;

struct RootDetectorPayload {
  int det_id = -1;
  std::string producer;
  std::uint64_t trigger_n = 0;
  std::uint64_t event_time_begin = 0;
  std::uint64_t event_time_end = 0;
  std::vector<std::uint8_t> payload;
  std::vector<RootQuantity> quantities;
  RootBranchValues branches;
};

class RootPayloadDecoder {
public:
  virtual ~RootPayloadDecoder() = default;
  virtual bool Matches(const RootDetectorPayload& detector) const = 0;
  virtual void Decode(const RootDetectorPayload& detector,
                      std::vector<RootQuantity>& quantities,
                      RootBranchValues& branches) const = 0;
  virtual std::vector<std::string> BranchNames() const;
};

class HidraXdcPayloadDecoder final : public RootPayloadDecoder {
public:
  explicit HidraXdcPayloadDecoder(std::map<int, std::string> vme_geo_map = {}, uint8_t log_level = 1);
  bool Matches(const RootDetectorPayload& detector) const override;
  void Decode(const RootDetectorPayload& detector,
              std::vector<RootQuantity>& quantities,
              RootBranchValues& branches) const override;
  std::vector<std::string> BranchNames() const override;

private:
  HidraXdcDecoder m_xdc_decoder;
};

class HidraFersPayloadDecoder final : public RootPayloadDecoder {
public:
  bool Matches(const RootDetectorPayload& detector) const override;
  void Decode(const RootDetectorPayload& detector,
              std::vector<RootQuantity>& quantities,
              RootBranchValues& branches) const override;
  std::vector<std::string> BranchNames() const override;

private:
  HidraFersDecoder m_fers_decoder;
};

class HidraMaxiccPayloadDecoder final : public RootPayloadDecoder {
public:
  HidraMaxiccPayloadDecoder();
  bool Matches(const RootDetectorPayload& detector) const override;
  void Decode(const RootDetectorPayload& detector,
              std::vector<RootQuantity>& quantities,
              RootBranchValues& branches) const override;
  std::vector<std::string> BranchNames() const override;

private:
  HidraFersDecoder m_fers_decoder;
};

class HidraTrackerPayloadDecoder final : public RootPayloadDecoder {
public:
  bool Matches(const RootDetectorPayload& detector) const override;
  void Decode(const RootDetectorPayload& detector,
              std::vector<RootQuantity>& quantities,
              RootBranchValues& branches) const override;
  std::vector<std::string> BranchNames() const override;

private:
  HidraTrackerDecoder m_tracker_decoder;
};

class HidraGenericPayloadDecoder final : public RootPayloadDecoder {
public:
  bool Matches(const RootDetectorPayload& detector) const override;
  void Decode(const RootDetectorPayload& detector,
              std::vector<RootQuantity>& quantities,
              RootBranchValues& branches) const override;
  std::vector<std::string> BranchNames() const override;
};

} // namespace hidra
