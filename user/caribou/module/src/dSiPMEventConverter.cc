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

bool dSiPMEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StandardEventSP d2,
    eudaq::ConfigurationSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  if (!m_configured) {
    m_zeroSupp = conf->Get("zero_suppression", true);

    LOG(DEBUG) << "Using configuration:";
    LOG(DEBUG) << "  zero_suppression = " << m_zeroSupp;

    m_configured = true;
  }

  // get an instance of the frame decoder
  static caribou::dSiPMFrameDecoder decoder;

  // No event
  if (!ev) {
    return false;
  }

  // Data container:
  caribou::pearyRawData rawdata;

  // Retrieve data from event
  if (ev->NumBlocks() == 1) {

    // contains all data, split it into timestamps and pixel data, returned as
    // std::vector<uint8_t>
    auto datablock = ev->GetBlock(0);

    // get number of words in datablock
    auto data_length = datablock.size();

    // translate data block into pearyRawData format
    rawdata.resize(data_length / sizeof(uintptr_t));
    memcpy(&rawdata[0], &datablock[0], data_length);

    // print some info
    LOG(DEBUG) << "Data block contains " << data_length << " words.";

  } // data from good event
  else {
    LOG(WARNING) << "Ignoring bad event " << ev->GetEventNumber();
    return false;
  } // bad event

  // call frame decoder from dSiPM device in peary, to translate pearyRawData to
  // pearydata
  auto frame = decoder.decodeFrame<bool>(rawdata, m_zeroSupp);

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "dSiPM");

  // prepare for info
  if (ev->GetEventNumber() == 1 ||
      ev->GetEventNumber() < 100 && ev->GetEventNumber() % 10 == 0 ||
      ev->GetEventNumber() < 1000 && ev->GetEventNumber() % 100 == 0 ||
      ev->GetEventNumber() < 10000 && ev->GetEventNumber() % 1000 == 0) {
    LOG(INFO)
        << "\tcol \trow \thit \tvalid \tbc \tcclck t\fclck \tts \tfs \tfe";
  }

  // go through pixels and add info to plane
  plane.SetSizeZS(32, 32, 0);
  uint64_t frameStart = 0;
  uint64_t frameEnd = 0;
  for (const auto &pixel : frame) {

    // get pixel information
    // pearydata is a map of a pair (col,row) and a pointer to a dsipm_pixel
    auto col = pixel.first.first;
    auto row = pixel.first.second;
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

    // assemble time info from bunchCounter and clocks. we have a dead time of
    // 4 * 1 / 204 MHz due between read going low and frame reset going high.
    // 3 * 1 / 408 MHz of that dead time are at the begin of a bunch, the rest
    // is at the end.
    // all three clocks should start with 1, which i subtract. For the coarse
    // clock 0 should never appear, unless the clocks and frame reset are out
    // of sync. for the fine clock 0 may appear but should be mapped to 32.
    // we should check if we ever actually read 31.
    if (clockCoarse == 0) {
      LOG(ERROR) << "Coarse clock == 0. This should not happen";
    }
    if (clockFine == 31) {
      LOG(DEBUG) << "Fine clock == 31. Interesting!";
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

    // timestamp
    // frame start + partial dead time shift
    // coarse clock runs with 408 MHz in ps +
    // fine clock runs with (408 * 32 = 13056) MHz in ps +
    // shift by dead time.
    uint64_t timestamp = static_cast<uint64_t>(
        (bunchCount - 1) * 1e6 / 3. + 3. * 1e6 / 408. +
        (clockCoarse - 1) * 1e6 / 408. + (clockFine - 1) * 1e6 / 13056.);

    // check frame start
    if (frameStart > 0 && frameStart != thisPixFrameStart) {
      LOG(WARNING) << "This frame start does not match prev. pixels frame "
                      "start from (same event)";
      LOG(WARNING) << " bunch counter ID " << bunchCount;
      LOG(WARNING) << " this frame start " << thisPixFrameStart;
      LOG(WARNING) << " previous frame start " << frameStart;
      return false;
    }
    // update frame start and end
    frameStart = thisPixFrameStart;
    frameEnd = thisPixFrameEnd;

    // check valid bits
    if (hitBit == true && validBit == false) {
      LOG(WARNING)
          << "This pixel is hit, but the valid bit for the quadrant is not set";
      LOG(WARNING) << " col and row " << col << " " << row;
      return false;
    }

    // print some info for some events:
    if (ev->GetEventNumber() == 1 ||
        ev->GetEventNumber() < 100 && ev->GetEventNumber() % 10 == 0 ||
        ev->GetEventNumber() < 1000 && ev->GetEventNumber() % 100 == 0 ||
        ev->GetEventNumber() < 10000 && ev->GetEventNumber() % 1000 == 0) {
      LOG(INFO) << "\t" << col << "\t" << row << "\t" << hitBit << "\t"
                << validBit << "\t" << bunchCount << "\t" << clockCoarse << "\t"
                << clockFine << "\t" << timestamp << "\t" << frameStart << "\t"
                << frameEnd;
    }

    // assemble pixel and add to plane
    plane.PushPixel(col, row, hitBit, timestamp);

  } // pixels in frame

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(frameStart);
  d2->SetTimeEnd(frameEnd);

  // Identify the detetor type
  d2->SetDetectorType("dSiPM");

  // Indicate that data was successfully converted
  return true;
}
