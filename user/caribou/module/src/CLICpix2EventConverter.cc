#include "CaribouEvent2StdEventConverter.hh"

#include "CLICpix2/clicpix2_frameDecoder.hpp"
#include "CLICpix2/clicpix2_pixels.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CLICpix2Event2StdEventConverter>(CLICpix2Event2StdEventConverter::m_id_factory);
}

bool CLICpix2Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // Prepare matrix decoder:
  static auto matrix_config = []() {
    std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix;
    for(uint8_t x = 0; x < 128; x++) {
      for(uint8_t y = 0; y < 128; y++) {
        matrix[std::make_pair(y,x)] = caribou::pixelConfig(true, 3, true, false, false);
      }
    }
    return matrix;
  };

  static caribou::clicpix2_frameDecoder decoder(true, true, matrix_config());
  // No event
  if(!ev) {
    return false;
  }

  // Bad event
  if (ev->NumBlocks() != 2) {
    EUDAQ_WARN("Ignoring bad frame " + std::to_string(ev->GetEventNumber()));
    return false;
  }
  EUDAQ_WARN("Decoding good frame " + std::to_string(ev->GetEventNumber()));

  // Block 0 is timestamps:
  std::vector<unsigned char> time = ev->GetBlock(0);

  // Ouch, this hurts:
  std::vector<unsigned int> rawdata;
  auto tmp = ev->GetBlock(1);
  rawdata.resize(tmp.size() / sizeof(unsigned int));
  memcpy(&rawdata[0], &tmp[0],tmp.size());

  // Decode the data:
  decoder.decode(rawdata);
  auto data = decoder.getZerosuppressedFrame();

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "CLICpix2", "CLICpix2");

  int i = 0;
  plane.SetSizeZS(128, 128, data.size());
  for(const auto& px : data) {
    auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
    int col = px.first.first;
    int row = px.first.second;

    // Disentangle data types from pixel:
    int tot = -1;

    // ToT will throw if longcounter is enabled:
    try {
      tot = cp2_pixel->GetTOT();
    } catch(caribou::DataException&) {
      // Set ToT to one if not defined.
      tot = 1;
    }

    plane.SetPixel(i, col, row, tot);
    i++;
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Indicate that data was successfully converted
  return true;
}
