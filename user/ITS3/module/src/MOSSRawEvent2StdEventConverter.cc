#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

// This converter/decoder supplies decoding functionality for MOSS events
// through overload of the `::Converting( ... )` function
class MOSSRawEvent2StdEventConverter final : public eudaq::StdEventConverter {
public:
  [[nodiscard]] bool Converting(eudaq::EventSPC rawev, eudaq::StdEventSP stdev,
                                eudaq::ConfigSPC conf) const final override;

private:
  // Event tag reveals if the half-unit is on top/bottom which then determines
  // the pixel plane dimensions from these constants.
  const uint8_t REGION_COUNT = 4;
  const uint16_t TOP_PIXEL_REGION_SIZE = 256;    // 256x256
  const uint16_t BOTTOM_PIXEL_REGION_SIZE = 320; // 320x320
};

#define REGISTER_CONVERTER(name)                                               \
  namespace {                                                                  \
    auto dummy##name = eudaq::Factory<eudaq::StdEventConverter>::Register<     \
        MOSSRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));              \
  }
REGISTER_CONVERTER(MOSS_0)

namespace { // Anonymous namespace with utility to simplify MOSS decoding

  // Tags for `if constexpr` tag dispatching
  struct DATA_0;
  struct DATA_1;
  struct DATA_2;

  // Helper for constexpr type checks (in C++20 this should be a `concept`)
  template <typename> inline constexpr bool dependent_false_v = false;

  // C++20 would be nice here! std::format("0x{:x}", 255); // "0xff"
  [[nodiscard]] std::string byte_to_hex(uint8_t b) {
    std::stringstream stream;
    stream << "0x" << std::hex << static_cast<int>(b) << std::dec;
    return stream.str(); // "0x{arg}"
  }

  // Represents MOSS Words. Use with `word_from_byte`
  enum class MossWord : uint8_t {
    frame_header = 0xD0,
    frame_trailer = 0xE0,
    region_header = 0xC0,
    data_0 = 0x00, // 00_<hit_row_pos[8:3]>
    data_1 = 0x40, // 01_<hit_row_pos[2:0]>_<hit_col_pos[8:6]>
    data_2 = 0x80, // 10_<hit_col_pos[5:0]>
    idle = 0xFF,
    unknown = 0xFB,
  };

  [[nodiscard]] MossWord word_from_byte(uint8_t raw_byte) noexcept {

    if (static_cast<MossWord>(raw_byte) == MossWord::idle) {
      EUDAQ_DEBUG("Got Moss::idle");
      return MossWord::idle;
    } else if (static_cast<MossWord>(raw_byte & 0xF0) ==
               MossWord::frame_header) {
      EUDAQ_DEBUG("Got Moss::frame_header");
      return MossWord::frame_header;
    } else if (static_cast<MossWord>(raw_byte) == MossWord::frame_trailer) {
      EUDAQ_DEBUG("Got Moss::frame_trailer");
      return MossWord::frame_trailer;
    } else if (static_cast<MossWord>(raw_byte & 0xFC) ==
               MossWord::region_header) {
      EUDAQ_DEBUG("Got Moss::region_header");
      return MossWord::region_header;
    } else if (static_cast<MossWord>(raw_byte & 0xC0) == MossWord::data_0) {
      EUDAQ_DEBUG("Got Moss::data_0");
      return MossWord::data_0;
    } else if (static_cast<MossWord>(raw_byte & 0xC0) == MossWord::data_1) {
      EUDAQ_DEBUG("Got Moss::data_1");
      return MossWord::data_1;
    } else if (static_cast<MossWord>(raw_byte & 0xC0) == MossWord::data_2) {
      EUDAQ_DEBUG("Got Moss::data_2");
      return MossWord::data_2;
    }

    EUDAQ_DEBUG("Got MossWord::unknown: " + ::byte_to_hex(raw_byte));
    return MossWord::unknown;
  }

  struct MossHit {
    [[nodiscard]] constexpr explicit MossHit(uint8_t region) noexcept {
      _region = region & 0x03;
    }
    [[nodiscard]] constexpr explicit MossHit() noexcept {}
    [[nodiscard]] constexpr explicit MossHit(const MossHit &other) noexcept
        : _hit_done{other._hit_done}, _region{other._region}, _col{other._col},
          _row{other._row} {}

    template <typename DATA_WORD> constexpr void add_hit_data(uint8_t byte) {
      if (_hit_done) {
        EUDAQ_ERROR("ERROR - Attemted to add hit data to a registered hit that "
                    "was already complete!");
      }

      if constexpr (std::is_same_v<DATA_0, DATA_WORD>) {
        _row = ((byte & 0x3F) << 3);
      } else if constexpr (std::is_same_v<DATA_1, DATA_WORD>) {
        _row |= ((byte & 0x38) >> 3);
        _col = ((byte & 0x07) << 6);
      } else if constexpr (std::is_same_v<DATA_2, DATA_WORD>) {
        _col |= ((byte & 0x3F));
        _hit_done = true;
      } else {
        static_assert(dependent_false_v<DATA_WORD>,
                      "Function has to be called with a MOSS dataword");
      }
    }

    bool _hit_done = false;
    uint8_t _region{0};
    uint16_t _col{0};
    uint16_t _row{0};
  };

  [[nodiscard]] bool packet_sanity_check(const std::vector<uint8_t> &packet) {
    if (!(::word_from_byte(packet.front()) == MossWord::frame_header)) {
      EUDAQ_ERROR("INVALID MOSS PACKET. Skipping event.");
      return false;
    }

    return true;
  }

} // namespace

[[nodiscard]] bool MOSSRawEvent2StdEventConverter::Converting(
    eudaq::EventSPC in, eudaq::StdEventSP out, eudaq::ConfigSPC conf) const {

  auto raw_event = std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  std::string half_unit_location = raw_event->GetTag(
      "VerticalLocation",
      "NOT_FOUND"); // If the tag is not found the second argument is returned

  std::vector<uint8_t> packet = raw_event->GetBlock(0);

  EUDAQ_DEBUG("MOSS Event Packet size =  " + std::to_string(packet.size()));

  if (!::packet_sanity_check(std::cref(packet))) {
    return false;
  }

  EUDAQ_DEBUG("Attempting decoding raw MOSS event from a single half unit");

  eudaq::StandardPlane plane_reg0(raw_event->GetDeviceN(), "ITS3DAQ",
                                  "MOSS_reg0");
  eudaq::StandardPlane plane_reg1(raw_event->GetDeviceN(), "ITS3DAQ",
                                  "MOSS_reg1");
  eudaq::StandardPlane plane_reg2(raw_event->GetDeviceN(), "ITS3DAQ",
                                  "MOSS_reg2");
  eudaq::StandardPlane plane_reg3(raw_event->GetDeviceN(), "ITS3DAQ",
                                  "MOSS_reg3");

  // A MOSS half-unit is actually made up of four segments with pixel matrixes
  /// Top    : Region 0 | Region 1  | Region 2  | Region 3
  //         [ 256x256 ]|[ 256x256 ]|[ 256x256 ]|[ 256x256 ]
  /// Bottom : Region 0 | Region 1  | Region 2  | Region 3
  //         [ 320x320 ]|[ 320x320 ]|[ 320x320 ]|[ 320x320 ]

  uint16_t current_event_region_size =
      0; // Used to calcuate offsets for regions
  if (half_unit_location == "TOP") {

    plane_reg0.SetSizeZS(TOP_PIXEL_REGION_SIZE, TOP_PIXEL_REGION_SIZE, 0);
    plane_reg1.SetSizeZS(TOP_PIXEL_REGION_SIZE, TOP_PIXEL_REGION_SIZE, 0);
    plane_reg2.SetSizeZS(TOP_PIXEL_REGION_SIZE, TOP_PIXEL_REGION_SIZE, 0);
    plane_reg3.SetSizeZS(TOP_PIXEL_REGION_SIZE, TOP_PIXEL_REGION_SIZE, 0);
    current_event_region_size = TOP_PIXEL_REGION_SIZE;
  } else {
    plane_reg1.SetSizeZS(BOTTOM_PIXEL_REGION_SIZE, BOTTOM_PIXEL_REGION_SIZE, 0);
    plane_reg0.SetSizeZS(BOTTOM_PIXEL_REGION_SIZE, BOTTOM_PIXEL_REGION_SIZE, 0);
    plane_reg2.SetSizeZS(BOTTOM_PIXEL_REGION_SIZE, BOTTOM_PIXEL_REGION_SIZE, 0);
    plane_reg3.SetSizeZS(BOTTOM_PIXEL_REGION_SIZE, BOTTOM_PIXEL_REGION_SIZE, 0);
    current_event_region_size = BOTTOM_PIXEL_REGION_SIZE;
    if (half_unit_location != "BOTTOM") {
      // NOT_FOUND
      EUDAQ_ERROR("VerticalLocation tag for MOSS half unit event not found, "
                  "defaulting to BOTTOM pixel matrix dimensions");
    }
  }

  EUDAQ_DEBUG("DEBUG - first byte: " + ::byte_to_hex(packet.front()));

  std::vector<MossHit> hits{};

  bool trailer_seen = false;
  uint8_t current_region = 0xFF;

  for (uint8_t raw_byte : packet) {
    if (trailer_seen) {
      EUDAQ_DEBUG("TRAILER SEEN");
      break;
    }

    switch (::word_from_byte(raw_byte)) {
    case MossWord::frame_header:
      break;

    case MossWord::region_header:
      current_region = raw_byte;
      break;

    case MossWord::data_0:
      hits.emplace_back(MossHit{current_region});
      hits.back().add_hit_data<DATA_0>(raw_byte);
      break;

    case MossWord::data_1:
      hits.back().add_hit_data<DATA_1>(raw_byte);
      break;

    case MossWord::data_2:
      hits.back().add_hit_data<DATA_2>(raw_byte);
      break;

    case MossWord::frame_trailer:
      trailer_seen = true;
      break;

    case MossWord::idle:
      break;

    case MossWord::unknown:
      EUDAQ_ERROR("Invalid word in MOSS Packet" + ::byte_to_hex(raw_byte));
      break;
    }
  }

  if (!trailer_seen) {
    EUDAQ_ERROR("Decoded full event but found no packet trailer");
  }

  EUDAQ_DEBUG("Got: " + std::to_string(hits.size()) + " hits");

  EUDAQ_DEBUG("Last byte is: " + ::byte_to_hex(packet.back()));

  for (MossHit &hit : hits) {
    EUDAQ_DEBUG("Hit - Region: " + ::byte_to_hex(hit._region) + " row/col: (" +
                ::byte_to_hex(hit._row) + ", " + ::byte_to_hex(hit._col) + ')');
    switch (hit._region) {
    case 0:
      plane_reg0.PushPixel(hit._col, hit._row, 1);
      break;

    case 1:
      plane_reg1.PushPixel(hit._col, hit._row, 1);
      break;

    case 2:
      plane_reg2.PushPixel(hit._col, hit._row, 1);
      break;

    case 3:
      plane_reg3.PushPixel(hit._col, hit._row, 1);
      break;

    default:
      // Unreachable because the region is calculated (MossHit Constructor) with
      // a mask on the 2 LSB making it impossible to be >3
      EUDAQ_ERROR("[Unreachable] ERROR - Unknown region: " +
                  ::byte_to_hex(hit._region));
    }
  }

  out->AddPlane(plane_reg0);
  out->AddPlane(plane_reg1);
  out->AddPlane(plane_reg2);
  out->AddPlane(plane_reg3);
  return true;
}
