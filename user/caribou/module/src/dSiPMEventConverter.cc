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
    LOG(INFO) << "\tcol \trow \thit \tvalid \tbc \tcclck t\fclck \tts";
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

    // assemble time info from bunchCounter and clocks. we have a certain amount
    // of dead time, due to the readout and frame reset. For now I am shifting
    // the begin by that dead time, which is prpbably wrong. FIXME we do get the
    // bunch counter id for every pixel, from which we can derive the start and
    // the end of each frame. re-calculate for checks

    // frame start
    // bunch counter (starts with 1) runs with 3 MHz clock in ps +
    // a fixed offset for dead time, 4 cycles of the 204 MHz clock in ps
    uint64_t thisPixFrameStart =
        static_cast<uint64_t>((bunchCount - 1) * 1e6 / 3. + 4. * 1e6 / 204.);

    // frame end
    // frame start + frame length (3 MHz clock - dead time)
    uint64_t thisPixFrameEnd =
        static_cast<uint64_t>((bunchCount - 1) * 1e6 / 3. + 1e6 / 3);

    // timestamp
    // frame start +
    // coarse clock runs with 408 MHz in ps +
    // fine clock runs with (408 * 32 = 13056) MHz in ps +
    // shift by dead time.
    uint64_t timestamp = static_cast<uint64_t>(
        (bunchCount - 1) * 1e6 / 3. + clockCoarse * 1e6 / 408. +
        clockFine * 1e6 / 13056. + 4. * 1e6 / 204.);

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
                << clockFine << "\t" << timestamp;
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
