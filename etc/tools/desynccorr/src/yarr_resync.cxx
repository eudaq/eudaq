#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"

#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <deque>
#include <algorithm>
#include <numeric>

#include "jsoncons/json.hpp"

#include "syncobject.h"

void usage() {
  std::cout << "EUDAQ resync options:\n"
  	  << "(nb: bool must be provided as 1 or 0)\n"
	    << "-f (--filename) string\t\tInput RAW file path [REQUIRED]\n"
	    << "-s (--syncfilepath) string\tPath to search for sync file [def: ./]\n"
	    << "-a (--all) bool\t\t\tResync all events, even if they are not considered to be good [def: 0]\n";
}

int main( int argc, char ** argv ){
  eudaq::OptionParser op("EUDAQ desynccorrelator", "1.0", "", 0, 10);
  eudaq::Option<std::string> pFile(op, "f", "filename", "", "string", "Input RAW file path");
  eudaq::Option<std::string> pSyncPath(op, "s", "syncfilepath", "./", "string", "Output filename (w/o extension)");
  eudaq::Option<bool> pAll(op, "a", "all", false, "bool", "Resync all events");

  std::string filename;
  std::string syncfilepath = "./";
  bool all = false;

  try{
    op.Parse(argv);
    
    if(!pFile.IsSet()) {
      usage();
      return -1;
    }
    filename = pFile.Value();
    all = pAll.Value();
    syncfilepath = pSyncPath.Value();
  } catch(...) {
    usage();
    return -1;
  }

  eudaq::FileReader resync_reader = ("native", filename);
  auto run_number = resync_reader.RunNumber();

  std::string syncfile = syncfilepath+"run"+std::to_string(run_number)+"_sync.json";
  std::cout << "Attempting to open syncfile: " << syncfile << '\n';

  plane_sync s(syncfile);
  size_t nev = s.good_events();
  int max_shift = s.max_shift();

  if(s.run_number() != run_number) {
    std::cout << "Run number from resync file: " << s.run_number() << " does not match run number from data file: " << run_number << ". Terminating.\n";
    throw std::runtime_error("Run number mismatch!");
  }

  auto writer = std::unique_ptr<eudaq::FileWriter>(eudaq::FileWriterFactory::Create(""));
  writer->SetFilePattern(filename+"_new");
  writer->StartRun(1);

  std::map<int, std::deque<eudaq::RawDataEvent::data_t>> data_cache;
  eudaq::DetectorEvent borevent = resync_reader.GetDetectorEvent();

  //data structure to hold for each PRODID (first index)
  //all the modules (second id) and the value is a list of blocks for this
  std::map<int, std::vector<int>> data_block_to_module;

  for(std::size_t x = 0; x < borevent.NumEvents(); x++) {
    auto rev = dynamic_cast<eudaq::RawDataEvent*>(borevent.GetEvent(x));
    if(rev && rev->GetSubType() == "Yarr") {
      auto prodID = std::stoi(rev->GetTag("PRODID"));
      auto moduleChipInfoJson = jsoncons::json::parse(rev->GetTag("MODULECHIPINFO"));
      for(const auto& uid : moduleChipInfoJson["uid"].array_range()) {
        data_block_to_module[prodID].push_back(110 + prodID*20 + uid["internalModuleIndex"].as<unsigned int>());
      }
    }
  }
  writer->WriteEvent(borevent);

  auto full_event_buffer = overflowbuffer<eudaq::DetectorEvent>(max_shift, eudaq::DetectorEvent(0,0,0));
  for(size_t ix = 0; ix < nev; ix++) {  
    if(ix%100 == 0) std::cout << "Event " << ix << "\n";
    bool hasEvt = resync_reader.NextEvent(0);
    if(!hasEvt) {
      std::cout << "EOF reached!\n";	    
      break;
    }
    auto dev = resync_reader.GetDetectorEvent();
    full_event_buffer.push(dev);

    if(ix < 2*max_shift) continue;
    if(!all && !s.is_good_evt(ix)) continue;

    int current_evt = ix-max_shift;
    int run_number = 0;
    eudaq::DetectorEvent outEvt(run_number, current_evt, 0);

    auto orig_evt = full_event_buffer.get(0);
    outEvt.setTimeStamp(orig_evt.GetTimestamp());

    for(size_t x = 0; x < orig_evt.NumEvents(); x++){
      auto evt = orig_evt.GetEventPtr(x);
      auto rev = dynamic_cast<eudaq::RawDataEvent*>(evt.get());
      if(rev && rev->GetSubType() != "Yarr") {
          outEvt.AddEvent(evt);
      } else {
        auto tev = dynamic_cast<eudaq::TLUEvent*>(evt.get());
        if(tev) {
          outEvt.AddEvent(evt);
        }
      }
    }

    //Here we add the YARR events
      for(auto & [id, vec]: data_block_to_module) {
          auto new_yarr_evt = new eudaq::RawDataEvent("Yarr", run_number, current_evt);
          auto new_yarr_evt_ptr = std::shared_ptr<eudaq::Event>(new_yarr_evt);
          new_yarr_evt_ptr->SetTag("PRODID", id);
          int sensor_id = -1;
          eudaq::DetectorEvent* shifted_evt = nullptr;
          for(std::size_t iy = 0; iy < vec.size(); iy++){
            if(vec[iy] != sensor_id) {
              sensor_id = vec[iy];
              auto shift_needed = s.get_resync_value(sensor_id, current_evt);
              //std::cout << "Shift needed on plane " << sensor_id << " is " << shift_needed << '\n';
              shifted_evt = &full_event_buffer.get(shift_needed);
            }
            auto & yarr_subevt = shifted_evt->GetRawSubEvent("Yarr", id);
            auto block_data = yarr_subevt.GetBlock(iy);
            auto block_id = yarr_subevt.GetID(iy);
            new_yarr_evt->AddBlock(block_id, block_data);
          }
          outEvt.AddEvent(new_yarr_evt_ptr);
      }
    writer->WriteEvent(outEvt);
  }
  return 0;
}