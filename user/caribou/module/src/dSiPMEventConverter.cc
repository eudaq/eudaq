#include "CaribouEvent2StdEventConverter.hh"

#include "dSiPMFrameDecoder.hpp"
#include "dSiPMPixels.hpp"
#include "utils/log.hpp"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      dSiPMEvent2StdEventConverter>(dSiPMEvent2StdEventConverter::m_id_factory);
}

std::vector<bool> dSiPMEvent2StdEventConverter::m_configured({});
std::vector<bool> dSiPMEvent2StdEventConverter::m_zeroSupp({});
std::vector<bool> dSiPMEvent2StdEventConverter::m_checkValid({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::frame_start({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::frame_stop({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::m_trigger({});
std::vector<uint64_t> dSiPMEvent2StdEventConverter::m_frame({});
TTree* dSiPMEvent2StdEventConverter::m_ttree_ugly(nullptr);
std::mutex dSiPMEvent2StdEventConverter::m_ttree_mutex({});

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

  if (m_configured.size() < plane_id + 1) {
    EUDAQ_DEBUG("Resizing static members for new plane");

    m_configured.push_back(false);
    m_zeroSupp.push_back(true);
    m_checkValid.push_back(false);
    frame_start.push_back(0);
    frame_stop.push_back(2);
    m_trigger.push_back(0);
    m_frame.push_back(0);
  }

  if (!m_configured[plane_id] && conf != NULL) {
    m_zeroSupp[plane_id] = conf->Get("zero_suppression", true);
    m_checkValid[plane_id] = conf->Get("check_valid", false);
    frame_start[plane_id] = conf->Get("frame_start", 0);
    frame_stop[plane_id] = conf->Get("frame_stop", 2);

    EUDAQ_INFO("Using configuration for plane ID " + to_string(plane_id) + ":");
    EUDAQ_INFO("  zero_suppression = " + to_string(m_zeroSupp[plane_id]));
    EUDAQ_INFO("  check_valid = " + to_string(m_checkValid[plane_id]));
    EUDAQ_INFO("  frame_start = " + to_string(frame_start[plane_id]));
    EUDAQ_INFO("  frame_end = " + to_string(frame_stop[plane_id]));

    m_configured[plane_id] = true;
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
  auto frame = decoder.decodeFrame(rawdata, m_zeroSupp[plane_id]);
  EUDAQ_DEBUG("Found " + to_string(rawdata.size()) + " words in frame.");
  EUDAQ_DEBUG("Decoded into " + to_string(frame.size()) + " pixels in frame.");

  // decode trailer with time info from FPGA
  auto [ts_control_fpga, ts_fine_fpga, ts_readout_fpga, trigger_id_fpga] =
    decoder.decodeTrailer(rawdata);
  // derive frame counter (inside trigger number)
  m_frame[plane_id] = (trigger_id_fpga == m_trigger[plane_id] ? m_frame[plane_id]+1 : 0);
  // store for next frame
  m_trigger[plane_id] = trigger_id_fpga;

  EUDAQ_DEBUG("Decoded trigger "  + to_string(m_trigger[plane_id]) + " frame " + to_string(m_frame[plane_id]));
  if (m_frame[plane_id] < frame_start[plane_id] || m_frame[plane_id] > frame_stop[plane_id]) {
    EUDAQ_DEBUG("Skipping frame");
    return false;
  }

    // TTree yeah
  uint8_t tt_quadrant;
  uint16_t tt_col, tt_row;
  uint8_t tt_hitBit;
  bool tt_validBit;
  uint64_t tt_bunchCount;
  uint8_t tt_clockCoarse, tt_clockFine;
  uint64_t tt_timestamp;
  {
    auto lock = std::unique_lock(m_ttree_mutex);
    if (m_ttree_ugly == nullptr) {
      m_ttree_ugly = new TTree("ugly", "ugly");
      m_ttree_ugly->Branch("plane_id", &plane_id);
      m_ttree_ugly->Branch("trigger", &m_trigger[plane_id]);
      m_ttree_ugly->Branch("frame", &m_frame[plane_id]);
      m_ttree_ugly->Branch("quadrant", &tt_quadrant);
      m_ttree_ugly->Branch("col", &tt_col);
      m_ttree_ugly->Branch("row", &tt_row);
      m_ttree_ugly->Branch("hitBit", &tt_hitBit);
      m_ttree_ugly->Branch("validBit", &tt_validBit);
      m_ttree_ugly->Branch("bunchCount", &tt_bunchCount);
      m_ttree_ugly->Branch("clockCoarse", &tt_clockCoarse);
      m_ttree_ugly->Branch("clockFine", &tt_clockFine);
      m_ttree_ugly->Branch("timestamp", &tt_timestamp);
    }
    lock.unlock();
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
    if (m_checkValid[plane_id] == true && hitBit == true && validBit == false) {
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

    // Effective number of bits 26 execpt if clockCoarse mod 4 == 0, then 25
    double nBitEff = clockCoarse % 4 == 0 ? 25 : 26;

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
    if (m_checkValid[plane_id] && frameStart > 0 && frameStart != thisPixFrameStart) {
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

    // Fill TTree
    tt_quadrant = getQuadrant(col, row);
    tt_col = col;
    tt_row = row;
    tt_hitBit = hitBit;
    tt_validBit = validBit;
    tt_bunchCount = bunchCount;
    tt_clockCoarse = clockCoarse;
    tt_clockFine = clockFine;
    tt_timestamp = timestamp;
    auto lock = std::unique_lock(m_ttree_mutex);
    m_ttree_ugly->Fill();
    lock.unlock();

  } // pixels in frame

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(frameStart);
  d2->SetTimeEnd(frameEnd);
  d2->SetTriggerN(trigger_id_fpga);

  // Add tag with frame number for additional information
  d2->SetTag("frame", m_frame[plane_id]);

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
