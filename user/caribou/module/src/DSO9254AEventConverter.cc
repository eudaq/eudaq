#include "CaribouEvent2StdEventConverter.hh"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "time.h"
#include "utils/log.hpp"
#include <sstream>

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      DSO9254AEvent2StdEventConverter>(
      DSO9254AEvent2StdEventConverter::m_id_factory);
}

bool DSO9254AEvent2StdEventConverter::m_configured(0);
int DSO9254AEvent2StdEventConverter::m_channels(0);
int DSO9254AEvent2StdEventConverter::m_digital(1);
bool DSO9254AEvent2StdEventConverter::m_generateRoot(1);
uint64_t DSO9254AEvent2StdEventConverter::m_trigger(0);
std::map<int, std::vector<unsigned int>>
    DSO9254AEvent2StdEventConverter::m_chanToPix;

bool DSO9254AEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StandardEventSP d2,
    eudaq::ConfigurationSPC conf) const {

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if (!ev) {
    return false;
  }

  // load parameters from config file
  if (!m_configured) {
    // define if plots are being stored - always from config, defaults to zero
    // (TODO CHANGEME)
    m_generateRoot = conf->Get("generateRoot", 1);

    // generate rootfile to write waveforms as TH1D  - RECREATE it here and
    // append later
    TFile *histoFile = nullptr;
    if (m_generateRoot) {
      histoFile =
          new TFile(Form("waveforms_run%i.root", ev->GetRunN()), "RECREATE");
      if (!histoFile) {
        EUDAQ_ERROR(to_string(histoFile->GetName()) + " can not be opened");
        return false;
      }
      histoFile->Close();
    }
    // check for tags that are specific for a oszi
    auto tags = d1->GetTags();
    m_digital = 0;

    for (uint i = 0; i < 15; ++i) {
      // check if analog channel is existing
      if (tags.count((":WAVeform:SOURce CHANnel" + std::to_string(i)))) {
        m_channels++;
      }
      // if one digital channel is on read all out
      if (tags.count((":DIGital" + std::to_string(i) + ":DISPlay 1"))) {
        m_digital = 1;
      }
    }

    // this is the fallback for older data recorded
    if (m_channels == 0 && m_digital == 0) {
      EUDAQ_DEBUG(
          "No channel tags in first event - fallback to manual configuration");
      m_channels = conf->Get("channels", 4);
      m_digital = conf->Get("digital", 0);
    }
    EUDAQ_INFO("Analog channels active: " + std::to_string(m_channels));
    EUDAQ_INFO("Digital channels active: " + std::to_string(m_digital));

    // get scope channel to pixel mapping.
    //   syntax: "ch_1_col ch_1_row ch_2_col ch_2_row
    //            ch_3_col ch_3_row ch_4_col ch_4_row"
    std::string channel_mapping =
        conf->Get("channel_mapping", "1 0 0 0 1 1 0 1");
    parse_channel_mapping(channel_mapping);

    // make sure to only do this once
    m_configured = true;
    // the first part is always only the configuration, no real data in there
    return true;
  } // configure

  // Data container:
  caribou::pearyRawData rawdata;

  // internal containers
  uint64_t timestamp;

  // Bad event
  if (ev->NumBlocks() != 1) {
    EUDAQ_WARN("Ignoring bad packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // all four scope channels and digital channels in one data block
  auto datablock = ev->GetBlock(0);
  // Calulate positions and length of data blocks:
  // FIXME FIXME FIXME by Simon: this is prone to break since you are selecting
  // bits from a 64bit
  //                             word - but there is no guarantee in the
  //                             endianness here and you might end up having the
  //                             wrong one.
  // Finn: I am not sure if I actually solved this issue! Lets discuss.
  rawdata.resize(sizeof(datablock[0]) * datablock.size() / sizeof(uintptr_t));
  std::memcpy(&rawdata[0], &datablock[0],
              sizeof(datablock[0]) * datablock.size());
  uint64_t block_position = 0;

  // first we read the analog data:
  auto waveforms_analog =
      read_data(rawdata, d1->GetEventNumber(), block_position, m_channels);

  if (waveforms_analog.front().empty()) {
    EUDAQ_WARN("No scope data in block " + to_string(ev->GetEventN()));
    return false;
  }
  EUDAQ_DEBUG("Analog channel reading complete: " +
              std::to_string(block_position));
  // second the trigger IDs are extracted from the digital data
  std::vector<uint64_t> triggers;
  std::vector<waveform> waveforms_digital;
  if (m_digital) {
    waveforms_digital =
        read_data(rawdata, d1->GetEventNumber(), block_position, 1).front();
    EUDAQ_DEBUG("Digital channel reading complete: " +
                std::to_string(block_position));
    triggers = calc_triggers(waveforms_digital);
  }
  // close rootfile
  if (m_generateRoot) {
    savePlots(waveforms_analog, waveforms_digital, d1->GetEventNumber(),
              d1->GetRunN());
    EUDAQ_DEBUG("Histograms written");
  }

  if (waveforms_analog.front().size() != triggers.size()) {
    EUDAQ_ERROR("tiggers and data size do not match");
  }
  for (int seg = 0; seg < waveforms_analog.front().size(); ++seg) {
    // create sub-event for segments
    auto sub_event = eudaq::StandardEvent::MakeShared();

    // Create a StandardPlane representing one sensor plane
    eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");

    // index number, which is the number of pixels hit per event
    int index = 0;

    // fill plane
    plane.SetSizeZS(2, 2, 0);
    // Set waveforms to each hit pixel.
    for (int ch = 0; ch < waveforms_analog.size(); ch++) {
      auto wave = waveforms_analog.at(ch).at(seg);
      auto data = wave.data;
      std::vector<double> w;
      for (auto &wa : data) {
        w.push_back(static_cast<double>(wa) * wave.dy);
      }
      plane.PushPixel(m_chanToPix[ch].front(), m_chanToPix[ch].back(), 1);
      plane.SetWaveform(index, w, wave.x0, wave.dx, 0);

      // Increase index number
      index++;
    }

    // derive trigger number from block number - if defined by digital channels
    // use it, otherwise not :)
    int triggerN =
        m_digital
            ? triggers.at(seg)
            : (ev->GetEventN() - 1) * waveforms_analog.front().size() + seg + 1;

    EUDAQ_DEBUG("Block number " + to_string(ev->GetEventN()) + " " +
                " segments, segment number " + to_string(seg) +
                " trigger number " + to_string(triggerN));

    sub_event->AddPlane(plane);
    sub_event->SetDetectorType("DSO9254A");
    sub_event->SetTimeBegin(0); // forcing corry to fall back on trigger IDs
    sub_event->SetTimeEnd(0);
    sub_event->SetTriggerN(triggerN);

    // add sub event to StandardEventSP for output to corry
    d2->AddSubEvent(sub_event);

    d2->SetDetectorType("DSO9254A");
  }

  // Indicate that data were successfully converted
  return true;
}
// return the vectors of waveform objects (size given by number of segments) for
// each active channesl
std::vector<std::vector<waveform>>
DSO9254AEvent2StdEventConverter::read_data(caribou::pearyRawData &rawdata,
                                           int evt, uint64_t &block_position,
                                           int n_channels) {

  std::vector<std::vector<waveform>> waves;
  waves.resize(n_channels);
  // needed per event
  uint64_t block_words;
  uint64_t pream_words;
  uint64_t chann_words;

  for (int nch = 0; nch < (n_channels); ++nch) {

    EUDAQ_DEBUG("Reading channel " + to_string(nch) + " of " +
                to_string(m_channels));

    // Obtaining Data Stucture
    block_words = rawdata[block_position + 0];
    pream_words = rawdata[block_position + 1];
    chann_words = rawdata[block_position + 2 + pream_words];
    EUDAQ_DEBUG("  " + to_string(rawdata.size()) + " 8 bit data blocks");
    EUDAQ_DEBUG("  " + to_string(block_position) +
                " current position in block");
    EUDAQ_DEBUG("  " + to_string(block_words) + " words per data block");
    EUDAQ_DEBUG("  " + to_string(pream_words) + " words per preamble");
    EUDAQ_DEBUG("  " + to_string(chann_words) + " words per channel data");
    EUDAQ_DEBUG("  " + to_string(rawdata.size()) + " size of rawdata");

    // Data sanity check: Total block size should be:
    // pream_words+channel_data_words+2
    if (block_words - pream_words - 2 - chann_words) {
      EUDAQ_WARN("Total data block size does not match the sum of "
                 "preamble+channel data + 2 (header)");
    }

    // read preamble:
    std::string preamble = "";
    for (int i = block_position + 2; i < block_position + 2 + pream_words;
         i++) {
      preamble += (char)rawdata[i];
    }
    EUDAQ_DEBUG("Preamble " + preamble);
    // parse preamble to vector of strings
    std::string val;
    std::vector<std::string> vals;
    std::istringstream stream(preamble);
    while (std::getline(stream, val, ',')) {
      vals.push_back(val);
    }

    // Pick needed preamble elements - this is constant for all elements of
    // channel and segment
    waveform wave;
    wave.points = stoi(vals[2]);
    wave.dx = stod(vals[4]) * 1e9;
    wave.x0 = stod(vals[5]) * 1e9;
    wave.dy = stod(vals[7]);
    wave.y0 = stod(vals[8]);
    int segments = 0;
    EUDAQ_DEBUG("Acq mode (2=SEGM): " + to_string(vals[18]));
    if (vals.size() == 25) { // this is segmented mode, possibly more than one
                             // waveform in block
      EUDAQ_DEBUG("Segments: " + to_string(vals[24]));
      segments = stoi(vals[24]);
    }

    int points_per_words = wave.points / (chann_words / segments);

    for (int s = 0; s < segments; s++) { // loop semgents
      EUDAQ_DEBUG("segment: " + std::to_string(s) + " out of " +
                  std::to_string((segments)));
      auto current_wave = wave;
      // read channel data
      std::vector<int16_t> words;
      words.resize(points_per_words); // rawdata contains 8 bit words, scope
                                      // sends 2 8bit words
      int16_t wfi = 0;
      // Read from segment start until the next segment begins:
      for (int i = block_position + 3 + pream_words +
                   (s + 0) * chann_words / segments;
           i <
           block_position + 3 + pream_words + (s + 1) * chann_words / segments;
           i++) {
        // copy channel data from entire segment data block
        memcpy(&words.front(), &rawdata[i], points_per_words * sizeof(int16_t));
        for (auto word : words) {
          current_wave.data.push_back((word));
          wfi++;

        } // words

      } // waveform
      waves.at(nch).push_back(current_wave);

    } // segments

    // update position for next iteration
    block_position += block_words + 1;

  } // for channel

  return waves;
}

std::vector<uint64_t>
DSO9254AEvent2StdEventConverter::calc_triggers(std::vector<waveform> &waves) {

  std::vector<uint64_t> triggers;
  int m_trigOffset = 1;
  int m_trigIDOffset = 5;
  int m_clkOffset = 14;

  for (auto &wave : waves) {
    auto data = wave.data;
    double binsIn25ns = 25. / wave.dx;
    uint64_t triggerID = 0x0;
    uint pos = 0;
    for (pos = 0; pos < data.size(); pos++) {

      if (((uint64_t(data.at(pos)) >> m_trigOffset) & 0x1) == 1) {
        EUDAQ_DEBUG("Found trigger at: " + std::to_string(pos));
        pos += 1.5 * binsIn25ns;
        break;
      }
    }
    // now fetch the data
    for (int i = 0; i < 15; ++i) {
      triggerID += uint64_t(((uint64_t(data.at(pos)) >> m_trigIDOffset) & 0x1))
                   << i;
      pos += binsIn25ns;
    }
    EUDAQ_DEBUG("TriggerID: " + std::to_string(triggerID));

    // check if the trigger counter has reached its maximum
    if ((m_trigger & 0x7FFF) > triggerID) {
      EUDAQ_INFO("Trigger counter overflow, adding 0x8000");
      m_trigger += 0x8000;
    }
    triggerID += (m_trigger & 0xFFFFFFFFFFFF8000);
    m_trigger = triggerID;
    triggers.push_back(triggerID);
  }

  return triggers;
}

void DSO9254AEvent2StdEventConverter::savePlots(
    std::vector<std::vector<waveform>> &analog, std::vector<waveform> &digital,
    int evt, int run) {
  TFile *histoFile = nullptr;
  if (m_generateRoot) {
    histoFile = new TFile(Form("waveforms_run%i.root", run), "UPDATE");
    if (!histoFile) {
      EUDAQ_THROW(to_string(histoFile->GetName()) + " can not be opened");
    }
  }

  // analog waveforms:
  int segment = 0;
  for (int i = 0; i < analog.front().size(); ++i) {
    for (int channel = 0; channel < analog.size(); channel++) {
      auto wave = analog.at(channel).at(i);
      double x0 = wave.x0;
      double y0 = wave.y0;
      double dx = wave.dx;
      double dy = wave.dy;
      uint points = wave.points;
      auto plot = new TGraph(points);
      for (uint point = 0; point < wave.data.size(); ++point) {
        plot->SetPoint(point, x0 + point * dx,
                       (static_cast<double>(wave.data.at(point)) * dy + y0));
      }
      plot->SetTitle(("waveform channel" + std::to_string(channel) +
                      "; time / ns; amplitue / V")
                         .c_str());
      plot->Write(("ch_" + std ::to_string(channel) + "_evt_" +
                   std::to_string(evt) + "_segment_" + std::to_string(segment))
                      .c_str());
    }
    segment++;
  }

  // digital waveforms:
  segment = 0;
  for (auto wave : digital) {
    double x0 = wave.x0;
    double y0 = wave.y0;
    double dx = wave.dx;
    double dy = wave.dy;
    uint points = wave.points;
    auto trigger = new TGraph(points);
    auto triggerID = new TGraph(points);
    auto clk = new TGraph(points);
    for (uint point = 0; point < wave.data.size(); ++point) {
      trigger->SetPoint(point, x0 + point * dx,
                        ((wave.data.at(point) >> 1) & 0x1));
      triggerID->SetPoint(point, x0 + point * dx,
                          (((wave.data.at(point) >> 5) & 0x1)));
      clk->SetPoint(point, x0 + point * dx,
                    (((wave.data.at(point) >> 14) & 0x1)));
    }
    trigger->SetTitle("digital trigger pulse; time / ns; ");
    triggerID->SetTitle("digital trigger ID pulse; time / ns; ");
    clk->SetTitle("digital clock pulse; time / ns; ");
    trigger->Write(("trigger_evt_" + std::to_string(evt) + "_segment_" +
                    std::to_string(segment))
                       .c_str());
    triggerID->Write(("triggerID_evt_" + std::to_string(evt) + "_segment_" +
                      std::to_string(segment))
                         .c_str());
    clk->Write(("clk_evt_" + std::to_string(evt) + "_segment_" +
                std::to_string(segment))
                   .c_str());
    segment++;
  }

  histoFile->Close();
}

void DSO9254AEvent2StdEventConverter::parse_channel_mapping(
    std::string in_map) {

  EUDAQ_INFO("Decoding scope channel mapping " + in_map);

  std::stringstream elements_string(in_map);
  std::vector<unsigned int> elements{0, 0, 0, 0, 0, 0, 0, 0};

  for (auto &element : elements) {

    // check if there are enough values
    if (elements_string.eof()) {
      EUDAQ_WARN("channel_mapping " + in_map +
                 " does not provide enough elements. Need 8!");
    }

    elements_string >> element;
    std::cout << element << std::endl;

    // check if value makes sense
    if (element > 1) {
      EUDAQ_WARN("channel_mapping contains elements larger than 1. Expecting "
                 "2x2 matrix!");
    }
  }

  // check if we have to many values
  if (!elements_string.eof()) {
    EUDAQ_WARN("channel_mapping " + in_map +
               " provide too many elements. Check mapping!");
  }

  // store lookup table as member variable and print for user to double check
  for (unsigned int ch = 0; ch < 4; ch++) {
    m_chanToPix[ch] = {elements.at(2 * ch), elements.at(2 * ch + 1)};
    EUDAQ_INFO("Mapping scope channel " + to_string(ch + 1) + " to pixel " +
               to_string(elements.at(2 * ch)) + " " +
               to_string(elements.at(2 * ch + 1)));
  }

  return;
}