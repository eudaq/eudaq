#include "CaribouEvent2StdEventConverter.hh"

#include "dSiPMFrameDecoder.hpp"
#include "dSiPMPixels.hpp"
#include "utils/log.hpp"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      dSiPMEvent2StdEventConverter>(dSiPMEvent2StdEventConverter::m_id_factory);
}

bool dSiPMEvent2StdEventConverter::m_configured(0);
bool dSiPMEvent2StdEventConverter::m_zeroSupp(1);
bool dSiPMEvent2StdEventConverter::m_checkValid(0);
uint64_t dSiPMEvent2StdEventConverter::m_trigger(0);
uint64_t dSiPMEvent2StdEventConverter::m_frame(0);
double dSiPMEvent2StdEventConverter::m_fine_ts_effective_bits[4];

bool dSiPMEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StandardEventSP d2,
    eudaq::ConfigurationSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  if (!m_configured && conf != NULL) {
    m_zeroSupp = conf->Get("zero_suppression", true);
    m_checkValid = conf->Get("check_valid", false);
    m_fine_ts_effective_bits[0] = conf->Get("fine_ts_effective_bits_q0", 32.);
    m_fine_ts_effective_bits[1] = conf->Get("fine_ts_effective_bits_q1", 32.);
    m_fine_ts_effective_bits[2] = conf->Get("fine_ts_effective_bits_q2", 32.);
    m_fine_ts_effective_bits[3] = conf->Get("fine_ts_effective_bits_q3", 32.);

    EUDAQ_INFO("Using configuration:");
    EUDAQ_INFO("  zero_suppression = " + to_string(m_zeroSupp));
    EUDAQ_INFO("  check_valid = " + to_string(m_checkValid));
    EUDAQ_INFO("  fine_ts_effective_bits");
    EUDAQ_INFO("    _q0 " + to_string(m_fine_ts_effective_bits[0]));
    EUDAQ_INFO("    _q1 " + to_string(m_fine_ts_effective_bits[1]));
    EUDAQ_INFO("    _q2 " + to_string(m_fine_ts_effective_bits[2]));
    EUDAQ_INFO("    _q3 " + to_string(m_fine_ts_effective_bits[3]));

    m_configured = true;
  }

  // get an instance of the frame decoder
  static caribou::dSiPMFrameDecoder decoder;

  // No event
  if (!ev) {
    return false;
  }

  // Set eudaq::StandardPlane::ID
  uint32_t plane_id = conf->Get("plane_id", 0);
  EUDAQ_DEBUG("Setting eudaq::StandardPlane::ID to " + to_string(plane_id));

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
  auto frame = decoder.decodeFrame(rawdata, 1);
  //  auto frame = decoder.decodeFrame(rawdata, m_zeroSupp);
  EUDAQ_DEBUG("Found " + to_string(rawdata.size()) + " words in frame.");
  EUDAQ_DEBUG("Decoded into " + to_string(frame.size()) + " pixels in frame.");

  // decode trailer with time info from FPGA
  auto [ts_control_fpga, ts_fine_fpga, ts_readout_fpga, trigger_id_fpga] =
    decoder.decodeTrailer(rawdata);
  // derive frame counter (inside trigger number)
  m_frame = (trigger_id_fpga == m_trigger ? m_frame+1 : 0);
  // store for next frame
  m_trigger = trigger_id_fpga;

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
    if (m_checkValid == true && hitBit == true && validBit == false) {
      EUDAQ_ERROR(
        "This pixel is hit, but the valid bit for the quadrant is not set");
      EUDAQ_ERROR("  col and row " + to_string(col) + " " + to_string(row));
      return false;
    }

    // assemble time info from bunchCounter and clocks. we have a dead time of
    // 4 * 1 / 204 MHz due between read going low and frame reset going high.
    // 3 * 1 / 408 MHz of that dead time are at the begin of a bunch, the rest
    // is at the end.
    // all three clocks should start with 1, which i subtract. For the coarse
    // clock 0 should never appear, unless the clocks and frame reset are out
    // of sync. for the fine clock 0 may appear but should be mapped to 32.
    // we should check if we ever actually read 31.
    if (clockCoarse == 0) {
      EUDAQ_WARN("Coarse clock == 0. This should not happen.");
    }
    if (clockFine == 31) {
      EUDAQ_WARN("Fine clock == 31. Interesting!");
    }
    if (clockFine == 0) {
      clockFine = 32;
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

    // Get the effective number of bits for the fine TDC time stamp.
    // This is 32 bit nominally.
    double nBitEff = m_fine_ts_effective_bits[quad];

    // timestamp
    // frame start + partial dead time shift
    // coarse clock runs with 408 MHz in ps +
    // fine clock runs with nominally 13056 MHz (408 MHz * nBitEff) in ps +
    // shift by dead time.
    uint64_t timestamp =
        static_cast<uint64_t>((bunchCount - 1) * 1e6 / 3. + 3. * 1e6 / 408. +
                              (clockCoarse - 1) * 1e6 / 408. +
                              (clockFine - 1) * 1e6 / (408. * nBitEff));

    // check frame start if we want valid check
    if (m_checkValid && frameStart > 0 && frameStart != thisPixFrameStart) {
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

    EUDAQ_DEBUG(" \t" + to_string(m_trigger) + " \t" + to_string(m_frame) +
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
  d2->SetTriggerN(trigger_id_fpga);

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
