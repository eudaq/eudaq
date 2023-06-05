#include "CaribouEvent2StdEventConverter.hh"

#include "eudaq/Exception.hh"

#include "dSiPMFrameDecoder.hpp"
#include "dSiPMPixels.hpp"
#include "utils/log.hpp"

#include <string>
#include <algorithm>

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      dSiPMEvent2StdEventConverter>(dSiPMEvent2StdEventConverter::m_id_factory);
}

constexpr double design_width = 1e6/(32*408);

std::vector<dSiPMEvent2StdEventConverter::PlaneConfiguration> dSiPMEvent2StdEventConverter::m_configuration({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::m_trigger({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::m_frame({});

bool dSiPMEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StandardEventSP d2,
    eudaq::ConfigurationSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // No event
  if (!ev) {
    return false;
  }

  // Set eudaq::StandardPlane::ID for multiple detectors
  uint32_t plane_id = conf->Get("plane_id", 0);
  EUDAQ_DEBUG("Setting eudaq::StandardPlane::ID to " + to_string(plane_id));

  // Check if this is the first encounter of the plane
  if (m_configuration.size() < plane_id + 1) {
    EUDAQ_DEBUG("Resizing static members for new plane");
    m_configuration.push_back({});
    m_trigger.push_back(0);
    m_frame.push_back(0);
  }

  // Shorthand for the configuration of this plane
  auto* plane_conf = &m_configuration[plane_id];

  if (!plane_conf->configured && conf != NULL) {
    plane_conf->zeroSupp = conf->Get("zero_suppression", true);
    plane_conf->discardDuringReset = conf->Get("discard_during_reset", true);
    plane_conf->checkValid = conf->Get("check_valid", false);
    plane_conf->fine_tdc_bin_widths = {
      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q0", "")),
      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q1", "")),
      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q2", "")),
      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q3", ""))};
    plane_conf->frame_start = conf->Get("frame_start", 0);
    plane_conf->frame_stop = conf->Get("frame_stop", 2);

    EUDAQ_INFO("Using configuration for plane ID " + to_string(plane_id) + ":");
    EUDAQ_INFO("  zero_suppression = " + to_string(plane_conf->zeroSupp));
    EUDAQ_INFO("  discard_during_reset = " + to_string(plane_conf->discardDuringReset));
    EUDAQ_INFO("  check_valid = " + to_string(plane_conf->checkValid));
    EUDAQ_INFO("  frame_start = " + to_string(plane_conf->frame_start));
    EUDAQ_INFO("  frame_stop = " + to_string(plane_conf->frame_stop));
    EUDAQ_INFO("  fine_tdc_bin_widths");
    EUDAQ_INFO("    _q0 " + to_string(plane_conf->fine_tdc_bin_widths[0]));
    EUDAQ_INFO("    _q1 " + to_string(plane_conf->fine_tdc_bin_widths[1]));
    EUDAQ_INFO("    _q2 " + to_string(plane_conf->fine_tdc_bin_widths[2]));
    EUDAQ_INFO("    _q3 " + to_string(plane_conf->fine_tdc_bin_widths[3]));

    plane_conf->configured = true;
  }

  // get an instance of the frame decoder
  static caribou::dSiPMFrameDecoder decoder;

  // Data container:
  std::vector<uint32_t> rawdata;

  // Retrieve data from event
  if (ev->NumBlocks() == 1) {

    // contains all data, split it into timestamps and pixel data, returned as
    // std::vector<uint8_t>
    auto datablock = ev->GetBlock(0);

    // get number of words in datablock
    auto data_length = datablock.size();

    // translate data block into pearyRawData format
    rawdata.resize(data_length / sizeof(uint32_t));
    memcpy(&rawdata[0], &datablock[0], data_length);

    // print some info
    EUDAQ_DEBUG("Data block contains " + to_string(data_length) + " words.");

  } // data from good event
  else {
    EUDAQ_WARN("Ignoring bad event " + to_string(ev->GetEventNumber()));
    return false;
  } // bad event

  // call frame decoder from dSiPM device in peary, to translate pearyRawData to
  // pearydata
  auto frame = decoder.decodeFrame(rawdata, plane_conf->zeroSupp);
  EUDAQ_DEBUG("Found " + to_string(rawdata.size()) + " words in frame.");
  EUDAQ_DEBUG("Decoded into " + to_string(frame.size()) + " pixels in frame.");

  // decode trailer with time info from FPGA
  auto fpgadata = decoder.decodeTrailer(rawdata);
  // derive frame counter (inside trigger number)
  m_frame[plane_id] = (fpgadata.trigger_id == m_trigger[plane_id] ? m_frame[plane_id]+1 : 0);
  // store for next frame
  m_trigger[plane_id] = fpgadata.trigger_id;

  EUDAQ_DEBUG("Decoded trigger "  + to_string(m_trigger[plane_id]) + " frame " + to_string(m_frame[plane_id]));
  if (m_frame[plane_id] < plane_conf->frame_start || m_frame[plane_id] > plane_conf->frame_stop) {
    EUDAQ_DEBUG("Skipping frame");
    return false;
  }

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(plane_id, "Caribou", "dSiPM");

  // prepare for info printing
  EUDAQ_DEBUG("\ttrigger frame \tcol \trow \thit \tvalid \tbc \t\tcclck "
              "\tfclck \tts \t\tfs \t\tfe");

  // go through pixels and add info to plane
  plane.SetSizeZS(32, 32, 0);
  uint64_t frameStart = 0;
  uint64_t frameEnd = 0;
  for (const auto &pixel : frame) {

    // get pixel information
    // pearydata is a map of a pair (col,row) and a pointer to a dsipm_pixel
    auto col = pixel.first.first;
    auto row = pixel.first.second;
    auto quad = getQuadrant(col, row);
    // cast into right type of pixel and retrieve stored data
    auto ds_pix = dynamic_cast<caribou::dsipm_pixel *>(pixel.second.get());
    // binary hit information
    auto hitBit = ds_pix->getBit();
    // valid bit
    auto validBit = ds_pix->getValidBit();
    // time info, bunch counter, coarse clock, fine clock
    auto bunchCount = ds_pix->getBunchCounter();
    auto clockCoarse = ds_pix->getCoarseTime();
    auto clockFine = ds_pix->getFineTime();

    // check valid bits if requested
    if (plane_conf->checkValid == true && hitBit == true && validBit == false) {
      EUDAQ_WARN(
        "This pixel is hit, but the valid bit for the quadrant is not set");
      EUDAQ_WARN("  col and row " + to_string(col) + " " + to_string(row));
      return false;
    }

    // assemble time info from bunchCounter and clocks. we have a dead time of
    // 4 * 1 / 204 MHz due between read going low and frame reset going high.
    // 3 * 1 / 408 MHz of that dead time are at the begin of a bunch, the rest
    // is at the end.
    // all three clocks should start with 1, which is subtracted. For the coarse
    // clock 0 should never appear, unless the clocks and frame reset are out
    // of sync or a trigger appears in the frame reset.
    if (clockCoarse == 0) {
      if (plane_conf->discardDuringReset == true) {
        return false;
      }
      else {
        EUDAQ_WARN("Coarse clock == 0. This might screw up timing analysis.");
      }
    }
    // According to Inge this needs to be discarded
    if (clockFine == 0) {
      if (plane_conf->discardDuringReset == true) {
        return false;
      }
      else {
        EUDAQ_WARN("clockFine == 0. This might screw up timing analysis.");
        // 0 comes after 31
        clockFine = 32;
      }
    }

    // frame start
    // bunch counter (starts with 1) runs with 3 MHz clock in ps +
    // a fixed offset for the first part of the dead time, 3 cycles of the 408
    // MHz clock in ps
    uint64_t thisPixFrameStart =
        static_cast<uint64_t>((bunchCount - 1) * 1e6 / 3. + 3. * 1e6 / 408.);

    // frame end
    // frame start + frame length (3 MHz clock - rest of the dead time)
    uint64_t thisPixFrameEnd =
        static_cast<uint64_t>((bunchCount - 0) * 1e6 / 3. - 5. * 1e6 / 408);

    // Calculate fine timestamp by summing over fine bin widths
    // Remember: fine clock starts at 1, so we need to subtract 1 for array access
    double fine_ts = 0.;
    for (size_t n = 0; n < clockFine - 1; ++n) {
      fine_ts += plane_conf->fine_tdc_bin_widths[quad][n];
    }
    // Place timestamp in the middle of the last bin
    fine_ts += plane_conf->fine_tdc_bin_widths[quad][clockFine - 1];

    // Pixel delay due to cable length
    auto pixel_delay = plane_conf->pixel_delays[col][row];

    // timestamp
    // frame start + partial dead time shift
    // coarse clock runs with 408 MHz in ps +
    // fine clock runs with nominally 13056 MHz (408 MHz * nBitEff) in ps +
    // shift by dead time.
    uint64_t timestamp =
        static_cast<uint64_t>((bunchCount - 1) * 1e6 / 3. + 3. * 1e6 / 408. +
                              (clockCoarse - 1) * 1e6 / 408. + fine_ts - pixel_delay);

    // check frame start if we want valid check
    if (plane_conf->checkValid && (frameStart > 0 && frameStart != thisPixFrameStart)) {
      EUDAQ_ERROR("This frame start does not match prev. pixels frame start "
                  "(from same event)");
      EUDAQ_ERROR("  bunch counter ID " + to_string(bunchCount));
      EUDAQ_ERROR("  this frame start " + to_string(thisPixFrameStart));
      EUDAQ_ERROR("  previous frame start " + to_string(frameStart));
      return false;
    }
    // update frame start and end
    frameStart = thisPixFrameStart;
    frameEnd = thisPixFrameEnd;

    EUDAQ_DEBUG(" \t" + to_string(m_trigger[plane_id]) + " \t" + to_string(m_frame[plane_id]) +
                " \t" + to_string(col) + " \t" + to_string(row) + " \t" +
                to_string(hitBit) + " \t" + to_string(validBit) + " \t" +
                to_string(bunchCount) + " \t" +
                to_string(static_cast<uint64_t>(clockCoarse)) + " \t" +
                to_string(static_cast<uint64_t>(clockFine)) + " \t" +
                to_string(timestamp) + " \t" + to_string(frameStart) + " \t" +
                to_string(frameEnd));

    // assemble pixel and add to plane
    plane.PushPixel(col, row, hitBit, timestamp);

  } // pixels in frame

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(frameStart);
  d2->SetTimeEnd(frameEnd);
  d2->SetTriggerN(fpgadata.trigger_id);

  // Identify the detetor type
  d2->SetDetectorType("dSiPM");

  // Copy tags
  for (const auto &tag : d1->GetTags()) {
    d2->SetTag(tag.first, tag.second);
  }

  // Indicate that data was successfully converted
  return true;
}

uint8_t dSiPMEvent2StdEventConverter::getQuadrant(const uint16_t &col,
                                                  const uint16_t &row) {
  if (col < 16 && row < 16)
    return 2;
  if (col < 16)
    return 0;
  if (row < 16)
    return 3;
  return 1;
}

template<size_t N> std::array<double, N> convert_config_to_double_array(std::string& config) {
  auto out = std::array<double, N>();

  // remove whitespaces and quotes
  config.erase(std::remove(config.begin(), config.end(), ' '), config.end());
  config.erase(std::remove(config.begin(), config.end(), '"'), config.end());

  // split by comma
  std::vector<std::string> substrs {};
  size_t pos_start = 0, pos_end = 0;
  while ((pos_end = config.find(',', pos_start)) != std::string::npos) {
    substrs.push_back(config.substr(pos_start, pos_end - pos_start));
    pos_start = pos_end + 1;
  }

  // check size: we need N substrings
  if (substrs.size() != N) {
    throw Exception("Wrong array size in config");
  }

  // convert substrings to doubles
  for (size_t n = 0; n < out.size(); ++n) {
    try {
      out[n] = std::stod(substrs[n]);
    } catch(std::invalid_argument& error) {
      throw Exception("Failed to convert substring to double in config");
    }
  }

  return out;
}

std::array<double, 32> dSiPMEvent2StdEventConverter::getFineTDCWidths(std::string config) {
  // no config, use defaults
  if (config == "") {
    auto out = std::array<double, 32>();
    for (size_t n = 0; n < out.size(); ++n) {
      out[n] = design_width;
    }
    return out;
  }

  return convert_config_to_double_array<32>(config);
}

std::array<std::array<double, 32>, 32> dSiPMEvent2StdEventConverter::getPixelDelays(std::string config) {
  auto out = std::array<std::array<double, 32>, 32>();

  // no config, no delays
  if (config == "") {
    for (size_t n = 0; n < out.size(); ++n) {
      for (size_t m = 0; m < out[n].size(); ++n) {
        out[n][m] = 0.;
      }
    }
    return out;
  }

  auto tmp = convert_config_to_double_array<32*32>(config);
  for (size_t n = 0; n < out.size(); ++n) {
    for (size_t m = 0; m < out[n].size(); ++m) {
      out[n][m] = tmp[n * 32 + m];
    }
  }

  return out;
}
