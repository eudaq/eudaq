#include "CaribouEvent2StdEventConverter.hh"
#include "eudaq/Exception.hh"

#include <devices/H2M/H2MFrameDecoder.hpp>
#include <devices/H2M/h2m_pixels.hpp>
#include <peary/utils/log.hpp>

#include <string>
#include <algorithm>
#include <cmath>

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      H2MEvent2StdEventConverter>(H2MEvent2StdEventConverter::m_id_factory);
}

static bool m_first_time = true;
static std::vector<std::vector<float>> vtot; // for ToT calibration

size_t H2MEvent2StdEventConverter::last_frame_id_ = 0;
bool H2MEvent2StdEventConverter::frame_id_jumped_ = false;
bool H2MEvent2StdEventConverter::Converting(
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

  // Read acquisition mode from configuration, defaulting to ToT.
  uint8_t acq_mode = conf->Get("acq_mode", 0x1);
  uint64_t delay_to_frame_end = conf->Get("delay_to_frame_end", -999);
  // Load ToT calibration data from configuration
  if (m_first_time) {
    if (conf && conf->Has("calibration_path_tot")) {
      std::string calibrationPathToT = conf->Get("calibration_path_tot", "");

      EUDAQ_INFO("Applying ToT calibration from " + calibrationPathToT);

      loadCalibration(calibrationPathToT, ' ', vtot);
    } else {
      EUDAQ_INFO("No calibration file path for ToT; data will be uncalibrated.");
    }
    m_first_time = false;
  }

  // get an instance of the frame decoder
  static caribou::H2MFrameDecoder decoder;

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
  else if(ev->NumBlocks() == 0) {
    EUDAQ_DEBUG("Ignoring empty event " + to_string(ev->GetEventNumber()) +  (ev->IsBORE() ? " (BORE)" : (ev->IsEORE() ? " (EORE)" : "")));
    return false;
  }
  else {
    EUDAQ_WARN("Ignoring bad event " + to_string(ev->GetEventNumber()));
    return false;
  } // bad event

  EUDAQ_DEBUG("Length of rawdata (should be 262): " +to_string(rawdata.size()));
  //  first decode the header
  auto [ts_trig, ts_sh_open, ts_sh_close, frameID, length, t0] = decoder.decodeHeader<uint32_t>(rawdata);

  if(t0 == false || ts_sh_close < ts_sh_open) {
      EUDAQ_DEBUG("No T0 signal seen yet, skipping event");
      return false;
  }

  // Optionally check for frame ID jump:
  if(conf->Get("wait_for_frame_jump", 0)) {
    if(frameID < last_frame_id_) {
      frame_id_jumped_ = true;
    }

    if(!frame_id_jumped_) {
      EUDAQ_DEBUG("No frame ID jump seen yet, skipping event");
      last_frame_id_ = frameID;
      return false;
    }
  }

  // remove the 6 elements from the header
  rawdata.erase(rawdata.begin(),rawdata.begin()+6);

  // Decode the event raw data - no zero suppression
  caribou::pearydata frame;
  try{
    frame = decoder.decodeFrame<uint32_t>(rawdata, acq_mode);
  }
  catch(caribou::DataCorrupt&){
    EUDAQ_ERROR("H2M decoder failed!");
    EUDAQ_ERROR("Length of rawdata (should be 262): " +to_string(rawdata.size()));
    return false;
  }

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(plane_id, "Caribou", "H2M");
  // go through pixels and add info to plane
  plane.SetSizeZS(64, 16, 0);
  // start and end are in 100MHz units -> to ps
  int _100MHz_to_ps = 10000;
  uint64_t frameStart = ts_sh_open*_100MHz_to_ps;
  uint64_t frameEnd = ts_sh_close*_100MHz_to_ps;
  EUDAQ_DEBUG("FrameID: "+ to_string(frameID) +"\t Shutter open: "+to_string(frameStart)+"\t shutter close "+to_string(frameEnd) +"\t t_0: "+to_string(t0));
  for (const auto &pixel : frame) {

    // get pixel information
    // pearydata is a map of a pair (col,row) and a pointer to a h2m_pixel
    auto [col, row] = pixel.first;

    // cast into right type of pixel and retrieve stored data
    auto pixHit = dynamic_cast<caribou::h2m_pixel_readout *>(pixel.second.get());

    // Pixel value of whatever equals zero means: no hit
    if(pixHit->GetData() == 0) {
      continue;
    }

    // Fetch timestamp from pixel if on ToA mode,
    // otherwise set to frame center (CERN mode) or configure a position with respect to the frame end (in ps, DESY mode)
    uint64_t not_toa_time = delay_to_frame_end > 0 ? frameEnd - delay_to_frame_end : ((frameStart + frameEnd) / 2);
    uint64_t timestamp = pixHit->GetMode() == caribou::ACQ_MODE_TOA ? (frameEnd - (pixHit->GetToA() * _100MHz_to_ps)) : not_toa_time;
    uint64_t tot = pixHit->GetToT();

    // best guess for charge is ToT if no calibration is available
    EUDAQ_DEBUG("tot is: " + to_string(tot));

    double charge = static_cast<float>(tot);
    if(!vtot.empty()) {
        EUDAQ_DEBUG("Applying calibration to DUT");
        size_t scol = static_cast<size_t>(col);
        size_t srow = static_cast<size_t>(row);
        float a = vtot.at(64 * srow + scol).at(0);
        float b = vtot.at(64 * srow + scol).at(1);
        float c = vtot.at(64 * srow + scol).at(2);
        float t = vtot.at(64 * srow + scol).at(3);

        // Calculating calibrated tot
        charge = (sqrt(a * a * t * t + 2 * a * b * t + 4 * a * c - 2 * a * t * static_cast<float>(tot) +
                             b * b - 2 * b * static_cast<float>(tot) + static_cast<float>(tot * tot)) +
                        a * t - b + static_cast<float>(tot)) /
                       (2 * a);
        EUDAQ_DEBUG("charge is: " + to_string(charge));


     } else{
       EUDAQ_DEBUG("ToT calibration is not applied");
     }  // end applyCalibration


    // assemble pixel and add to plane
    plane.PushPixel(col, row, charge, timestamp);
  } // pixels in frame

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(frameStart);
  d2->SetTimeEnd(frameEnd);
  // The frame ID is not necessarily related to a trigger but rather to an event ID:
  d2->SetEventN(frameID);

  // Identify the detetor type
  d2->SetDetectorType("H2M");

  // Copy tags - guess we have non for now, but just keep them here
  for (const auto &tag : d1->GetTags()) {
    d2->SetTag(tag.first, tag.second);
  }

  // Indicate that data was successfully converted
  return true;
}

void H2MEvent2StdEventConverter::loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat) const {
    std::ifstream f;
    f.open(path);
    dat.clear();

    // check if file is open
    if(!f.is_open()) {
        throw DataInvalid("Cannot open calibration file:\n\t" + path);
        return;
    }

    // read file line by line
    int i = 0;
    std::string line;
    while(!f.eof()) {
        std::getline(f, line);

        // check if line is empty or a comment
        // if not write to output vector

        if(line.size() > 0 && isdigit(line.at(0))) {
            std::stringstream ss(line);
            std::string word;
            std::vector<float> row;
            for (size_t i = 0; i < 6; ++i) {
                std::getline(ss, word, delim);
                if (i >= 2) { // Keep only calibration data
                    row.push_back(std::stof(word));
                }
            }
            if (row.size() != 4) { // Expected A, B, C, D parameters
                throw DataInvalid("Unexpected number of parameters in calibration file. Expected 4 parameters (A, B, C, D), but found " + std::to_string(row.size()) + "\n\t");
            }

            dat.push_back(row);
        }
    }

    // warn if too few entries
    if(dat.size() != 64 * 16) {
        throw DataInvalid("Something went wrong. Found only " + to_string(i) + " entries. Not enough for H2M.\n\t");
    }

    f.close();
}
