#include "CaribouEvent2StdEventConverter.hh"
#include "eudaq/Exception.hh"

#include "H2MFrameDecoder.hpp"
#include "h2m_pixels.hpp"
#include "utils/log.hpp"

#include <string>
#include <algorithm>

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      H2MEvent2StdEventConverter>(H2MEvent2StdEventConverter::m_id_factory);
}


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

  // Check if this is the first encounter of the plane
//  if (m_configuration.size() < plane_id + 1) {
//    EUDAQ_DEBUG("Resizing static members for new plane");
//    m_configuration.push_back({});
//    m_trigger.push_back(0);
//    m_frame.push_back(0);
//  }

//  // Shorthand for the configuration of this plane
//  auto* plane_conf = &m_configuration[plane_id];

//  if (!plane_conf->configured && conf != NULL) {
//    plane_conf->zeroSupp = conf->Get("zero_suppression", true);
//    plane_conf->discardDuringReset = conf->Get("discard_during_reset", true);
//    plane_conf->checkValid = conf->Get("check_valid", false);
//    plane_conf->fine_tdc_bin_widths = {
//      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q0", "")),
//      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q1", "")),
//      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q2", "")),
//      getFineTDCWidths(conf->Get("fine_tdc_bin_widths_q3", ""))};
//    plane_conf->frame_start = conf->Get("frame_start", 0);
//    plane_conf->frame_stop = conf->Get("frame_stop", INT8_MAX);

//    EUDAQ_INFO("Using configuration for plane ID " + to_string(plane_id) + ":");
//    EUDAQ_INFO("  zero_suppression = " + to_string(plane_conf->zeroSupp));
//    EUDAQ_INFO("  discard_during_reset = " + to_string(plane_conf->discardDuringReset));
//    EUDAQ_INFO("  check_valid = " + to_string(plane_conf->checkValid));
//    EUDAQ_INFO("  frame_start = " + to_string(plane_conf->frame_start));
//    EUDAQ_INFO("  frame_stop = " + to_string(plane_conf->frame_stop));
//    EUDAQ_INFO("  fine_tdc_bin_widths");
//    EUDAQ_INFO("    _q0 " + to_string(plane_conf->fine_tdc_bin_widths[0]));
//    EUDAQ_INFO("    _q1 " + to_string(plane_conf->fine_tdc_bin_widths[1]));
//    EUDAQ_INFO("    _q2 " + to_string(plane_conf->fine_tdc_bin_widths[2]));
//    EUDAQ_INFO("    _q3 " + to_string(plane_conf->fine_tdc_bin_widths[3]));

//    plane_conf->configured = true;
//  }

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
  else {
    EUDAQ_WARN("Ignoring bad event " + to_string(ev->GetEventNumber()));
    return false;
  } // bad event

  EUDAQ_DEBUG("Length of rawdata (should be 262): " +to_string(rawdata.size()));
  //  first decode the header
  //  auto [ts_trig, ts_sh_open, ts_sh_close, frameID, t0] = decoder.decodeHeader<uint32_t>(rawdata);
  // remove the 6 elements from the header
  rawdata.erase(rawdata.begin(),rawdata.begin()+6);
// Decode the event raw data - no zero supression
  auto frame = decoder.decodeFrame<uint32_t>(rawdata,0x0);


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
    // pearydata is a map of a pair (col,row) and a pointer to a dsipm_pixel
    auto col = pixel.first.first;
    auto row = pixel.first.second;
    // cast into right type of pixel and retrieve stored data
    auto pixHit = dynamic_cast<caribou::h2m_pixel_readout *>(pixel.second.get());

    // Get the pixel timestamp if in ToA, else frame center
    // ToT if existing, else 1
    auto mode = pixHit->GetMode();
    uint64_t timestamp =0x0;
    uint64_t tot = 0x0;
    if(mode == caribou::ACQ_MODE_TOA){
      timestamp = frameEnd-(pixHit->GetToA()*_100MHz_to_ps);
      tot = 1;
    } else if(mode==caribou::ACQ_MODE_TOT){
      timestamp = (frameStart+frameEnd)/2;
      tot = pixHit->GetToT()*_100MHz_to_ps;
    }
    // assemble pixel and add to plane
    plane.PushPixel(col, row, tot, timestamp);
    if(col == 1 && row ==1)
        EUDAQ_DEBUG("Col/row: " + to_string(col) + "/" + to_string(row) + " in mode "+ to_string(int(mode)) + " with ts: " + to_string(timestamp) + " tot: " + to_string(tot));
  } // pixels in frame

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(frameStart);
  d2->SetTimeEnd(frameEnd);
  d2->SetTriggerN(frameID);

  // Identify the detetor type
  d2->SetDetectorType("H2M");

  // Copy tags - guess we have non for now, but just keep them here
  for (const auto &tag : d1->GetTags()) {
    d2->SetTag(tag.first, tag.second);
  }

  // Indicate that data was successfully converted
  return true;
}

