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
#include "clustering.h"

#include "TFile.h"
#include "TH2I.h"
#include "TGraph.h"


void usage() {
  std::cout << "EUDAQ desynccorrelator options:\n"
	    << "(nb: bool must be provided as 1 or 0)\n"
	    << "-f (--filename) string\t\tInput RAW file path [REQUIRED]\n"
	    << "-o (--outfilename) string\tOutput ROOT file name (w/o extension) [def: correlator]\n"
	    << "-a (--anti) bool\t\tDo anticorrelations (xy,yx) instead of correlations (xx,yy) [def: 0]\n"
	    << "-r (--refplane) int\t\tPlaneID acting as reference [def: 1]\n"
	    << "-d (--dutplane) int\t\tPlaneID acting as DUT [REQUIRED]\n"
	    << "-x (--flipx) bool\t\tFlip x-axis (of DUT) [def: 0]\n"
	    << "-y (--flipy) bool\t\tFlip y-axis (of DUT) [def: 0]\n"
	    << "-n (--nevts) size_t\t\tNumber of events to process [def: 20000]\n"
	    << "-t (--maxoffset) size_t\t\t+/-offset to look for correlations [def: 50]\n"
	    << "-b (--evtsperbin) size_t\tEvents per time axis (x-axis) bin [def: 200]\n";
}

struct plane_options {
  bool anti = false;
  double flipx = 1.;
  double flipy = 1.;
  std::vector<int> noise_vec;
  double pitch_x = 0.;
  double pitch_y = 0.;
  int px_x = 0;
  int px_y = 0;
  std::vector<TH2I> xhist_v;
  std::vector<TH2I> yhist_v;
  std::deque<std::vector<cluster>> stream;
  std::vector<pixel_hit> hits;
  std::vector<std::vector<int>> evt_size_history;
};

int main( int argc, char ** argv ){
  eudaq::OptionParser op("EUDAQ desynccorrelator", "1.0", "", 0, 10);
  eudaq::Option<std::string> pFile(op, "f", "filename", "", "string", "Input RAW file path");
  eudaq::Option<std::string> pAnticorrelate(op, "a", "anti", "0", "bool", "Anticorrelate planes");
  eudaq::Option<int> pRefPlane(op, "r", "refplane", 1, "int", "PlaneID acting as reference");
  eudaq::Option<std::string> pDUTPlane(op, "d", "dutplane", "24", "int", "PlaneID acting as DUT");
  eudaq::Option<std::string> pFlipX(op, "x", "flipx", "0", "bool", "Flip x-axis");
  eudaq::Option<std::string> pFlipY(op, "y", "flipy", "0", "bool", "Flip y-axis");
  eudaq::Option<size_t> pNEvts(op, "n", "nevts", 20000, "size_t", "Number of events to process");
  eudaq::Option<size_t> pMaxOffset(op, "t", "maxoffset", 50, "size_t", "Max offset to look at in +/- events");
  eudaq::Option<size_t> pEvtsPerBin(op, "b", "evtsperbin", 200, "size_t", "Events per time axis bin");
  eudaq::Option<std::string> pOFile(op, "o", "outfilename", "correlator", "string", "Output filename (w/o extension)");

  TH1::AddDirectory(kFALSE);

  std::vector<int> id_samples;
  std::map<int, plane_options> plane_ops;
  std::vector<int> noise_pix_ref;
  std::string filename;
  int id_ref = 1;
  size_t nev = 20000;
  int max_shift = 25;
  size_t evtsperbin = 200;
  std::string ofile;
  int fine_evt_per_bin = 10;

  auto is_ref_or_dut = [&](int id)->bool {
    if(id == id_ref) return true;
    else if(std::find(id_samples.begin(), id_samples.end(), id) != id_samples.end()) return true;
    return false;
  };

  try{
    op.Parse(argv);
    
    if(!pFile.IsSet() || !pDUTPlane.IsSet() ) {
      usage();
      return -1;
    }
    filename = pFile.Value();
    id_ref = pRefPlane.Value();

    std::stringstream ss_duts;
    std::stringstream ss_anticorr;
    std::stringstream ss_flipx;
    std::stringstream ss_flipy;

    std::string seg_duts;
    std::string seg_anticorr;
    std::string seg_flipx;
    std::string seg_flipy;

    ss_duts << pDUTPlane.Value();
    ss_anticorr << pAnticorrelate.Value();
    ss_flipx << pFlipX.Value();
    ss_flipy << pFlipY.Value();

    while(std::getline(ss_duts, seg_duts, ',')){
      auto id = std::stoi(seg_duts);
      id_samples.emplace_back(id);
      std::getline(ss_anticorr, seg_anticorr, ',');
      std::getline(ss_flipx, seg_flipx, ',');
      std::getline(ss_flipy, seg_flipy, ',');
      plane_ops[id].anti = static_cast<bool>(std::stoi(seg_anticorr));
      plane_ops[id].flipx = static_cast<bool>(std::stoi(seg_flipx)) ? -1. : 1;
      plane_ops[id].flipy = static_cast<bool>(std::stoi(seg_flipy)) ? -1. : 1;
    }

    plane_ops[id_ref] = plane_options{};

    nev = pNEvts.Value();
    max_shift = pMaxOffset.Value();
    evtsperbin = pEvtsPerBin.Value();
    ofile = pOFile.Value()+".root";
  } catch(...) {
    usage();
    return -1;
  }     

  eudaq::FileReader reader = ("native", filename);
  eudaq::FileReader noise_reader = ("native", filename);

  std::cout << "Run number: " << reader.RunNumber() << '\n';
  
  eudaq::PluginManager::Initialize(reader.GetDetectorEvent());

  TFile f(ofile.c_str(), "RECREATE");
 
  std::map<int, std::pair<int, int>> detPxSize;
  std::map<int, std::pair<double, double>> detPxPitch;
  std::map<int, std::vector<long int>> noiseHitMap;

  size_t noiseevts = 10000;
  //first we have to do a noise run
  for(size_t ix = 0; ix < noiseevts; ix++) {  
    if(ix%100 == 0) std::cout << "Noise event " << ix << '\n';
    bool hasEvt = noise_reader.NextEvent(0);
    if(!hasEvt) {
      std::cout << "EOF reached!\n";	    
      break;
    }

    auto & evt = noise_reader.GetDetectorEvent();
    auto stdEvt = eudaq::PluginManager::ConvertToStandard(evt);
    for(size_t plix = 0; plix < stdEvt.NumPlanes(); plix++) {
      auto & plane = stdEvt.GetPlane(plix);
      auto id = plane.ID();
      //for the first 100 events we expect "new" detectors to pop up, in this case we retrieve their information
      //and store it. This allows us to be stable against the cases where the first event(s) are corrupted and
      //DUT DAQ systems did not push any data into the first few events
      if(ix < 100) {
	      if(detPxSize.find(id) == detPxSize.end()) {
          detPxSize[id] = std::make_pair(plane.XSize(), plane.YSize());
	        auto type = plane.Type();
	        if(type == "NI") detPxPitch[id] = std::make_pair(18.4, 18.4);
	        else if(type == "Yarr") detPxPitch[id] = std::make_pair(50., 50.);
	        else if(type == "USBPIX_GEN3") detPxPitch[id] = std::make_pair(250., 50.);
	        std::cout << "Added " << id << " size: " << detPxSize[id].first  << '(' << detPxPitch[id].first << " um)/" 
		                << detPxSize[id].second <<  '(' << detPxPitch[id].second << " um)\n";
          if(is_ref_or_dut(id)) {
		        plane_ops[id].px_x = detPxSize[id].first; 
		        plane_ops[id].px_y = detPxSize[id].second;
		        plane_ops[id].pitch_x = detPxPitch[id].first;
		        plane_ops[id].pitch_y= detPxPitch[id].second;
          }
          noiseHitMap[id] = std::vector<long int>(detPxSize[id].first*detPxSize[id].second, 0);
	      }
      }

      if(is_ref_or_dut(id)) {
        for(size_t piix = 0; piix <  plane.HitPixels(); piix++) {
	        int i = plane.GetY(piix)*detPxSize[id].first+plane.GetX(piix);
	        if(i < noiseHitMap[id].size()) {
            noiseHitMap[id].at(i)++;
	        } else {
		        std::cout << "Data corruption in event on plane: " << id 
			                << "\n -- Pixel " << plane.GetX(piix) << '/' << plane.GetY(piix) << " with ix: " << i << " does not fit into: " << noiseHitMap[id].size() << '\n';
		        std::cout << "PlaneSize: "<< plane.XSize() << '/' << plane.YSize() << '\n';
	        }
        }
      }
    }
  }

  for(auto const & [plane, hitmap]: noiseHitMap) {
    if(!is_ref_or_dut(plane)) continue;
    std::cout << "Noisy pixels on plane: " << plane << ":\n";
    for(size_t px = 0; px < hitmap.size(); px++) {
      if(hitmap[px]*1./noiseevts > 0.005) {
	      std::cout << "Pixel: " << px%detPxSize[plane].first << '/' << px/detPxSize[plane].first << '\n';
        plane_ops[plane].noise_vec.emplace_back(px);
      }
    }
  }

  std::cout << "Done..\n";

  for(auto& [id, ops]: plane_ops) {
    for(int i = -max_shift; i <= max_shift; i++) {
      auto suffix = "_shift"+std::to_string(i);

      auto p1name = "p"+std::to_string(id_ref);
      auto p2name = "p"+std::to_string(id);

      if(ops.anti) ops.xhist_v.emplace_back(std::string(p1name+"x-"+p2name+"y_temp_corr_"+suffix).c_str(), std::string(p1name+"x-"+p2name+"y_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
      else ops.xhist_v.emplace_back(std::string(p1name+"x-"+p2name+"x_temp_corr_"+suffix).c_str(), std::string(p1name+"x-"+p2name+"x_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
      if(ops.anti) ops.yhist_v.emplace_back(std::string(p1name+"y-"+p2name+"x_temp_corr_"+suffix).c_str(), std::string(p1name+"y-"+p2name+"x_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
      else ops.yhist_v.emplace_back(std::string(p1name+"y-"+p2name+"y_temp_corr_"+suffix).c_str(), std::string(p1name+"y-"+p2name+"y_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
      ops.evt_size_history.resize(nev/fine_evt_per_bin+1);
    }
  }

  int previous_event_number = -1;

  //Main event loop
  for(size_t ix = 0; ix < nev; ix++) {
    bool hasEvt = reader.NextEvent(0);
    if(!hasEvt) {
      std::cout << "EOF reached!\n";	    
      break;
    }
    auto & evt = reader.GetDetectorEvent();

    //All the event number consistency checking and log message printing
    auto evt_nr = evt.GetEventNumber();
    if(previous_event_number >= 0 && previous_event_number != evt_nr-1) {
      std::cout << "Jump in event number detected! Jumped from " << previous_event_number << " to " << evt_nr << '\n' << "This will cause problems in the resynchronisation!\n";
    }
    previous_event_number = evt_nr;
    if(ix%100 == 0) std::cout << "Event " << ix << " with event number " << evt_nr << " (bin " << evt_nr/evtsperbin << ")\n";
    if(evt_nr > nev) {
      std::cout << "Reached event with event number: " << evt_nr << ". Stopping.\n";
      break;
    }

    auto stdEvt = eudaq::PluginManager::ConvertToStandard(evt);
    for(size_t plix = 0; plix < stdEvt.NumPlanes(); plix++) {
      auto & plane = stdEvt.GetPlane(plix);
      auto id = plane.ID();
      if(is_ref_or_dut(id)) {
        auto & plane_data = plane_ops[id];
        plane_data.evt_size_history[evt_nr/fine_evt_per_bin].emplace_back(plane.HitPixels());
        for(size_t piix = 0; piix < plane.HitPixels(); piix++) {
	        auto x = plane.GetX(piix);
	        auto y = plane.GetY(piix);
          auto i = y*plane_data.px_x + x;
	        if(std::find(plane_data.noise_vec.begin(), plane_data.noise_vec.end(), i) == plane_data.noise_vec.end()) plane_data.hits.emplace_back(x, y);
        }
      }
    }

    for(auto& [id, ops]: plane_ops) {
      ops.stream.push_front(clusterHits(ops.hits, 4));
      ops.hits.clear();
    }

    //if we didn't process yet enough events to loop back -max_shift and in front +max_shift,
    //we can't process the data
    if(ix < 2*max_shift) continue;

    auto correlate = [&](plane_options& ref, plane_options& dut, size_t shift){
      for(auto &p1h: ref.stream[max_shift]) {
        for(auto &p2h: dut.stream[max_shift-shift]) {
          double xdist, ydist;

          if(dut.anti) xdist = (p1h.x-ref.px_x/2)*ref.pitch_x-(p2h.y-dut.px_y/2)*dut.pitch_y*dut.flipy;
	        else xdist =         (p1h.x-ref.px_x/2)*ref.pitch_x-(p2h.x-dut.px_x/2)*dut.pitch_x*dut.flipx;
          if(dut.anti) ydist = (p1h.y-ref.px_y/2)*ref.pitch_y-(p2h.x-dut.px_x/2)*dut.pitch_x*dut.flipx;
	        else ydist =         (p1h.y-ref.px_y/2)*ref.pitch_y-(p2h.y-dut.px_y/2)*dut.pitch_y*dut.flipy;
          
	        dut.xhist_v[shift+max_shift].Fill(evt_nr/evtsperbin, xdist);
          dut.yhist_v[shift+max_shift].Fill(evt_nr/evtsperbin, ydist);
        }
      }
    };

    for(auto dut_id: id_samples) {
      for(int i = -max_shift; i <= max_shift; i++) {
        correlate(plane_ops[id_ref], plane_ops[dut_id], i);
      }
    }

    for(auto& [id, ops]: plane_ops) {
      ops.stream.pop_back();
    }
  }

  f.cd();

  auto savehistostack = [&](std::vector<TH2I> hstack){
    for(auto& h: hstack) h.Write();
  };

  auto max_hit_plots = [&](std::vector<std::vector<int>> const & v, int pid){
    auto x = std::vector<double>(v.size(), 0);
    auto y_max = std::vector<double>(v.size(), 0);
    auto y_avg = std::vector<double>(v.size(), 0);
    for(std::size_t i = 0; i < v.size(); i++) {
      x.at(i) = i*fine_evt_per_bin;
      auto max_it = std::max_element(v[i].begin(), v[i].end());
      if(max_it != v[i].end()) {
        y_max[i] = *max_it;
      }
      auto sum = std::accumulate(v[i].begin(), v[i].end(), 0);
      y_avg[i] = sum*1./fine_evt_per_bin;
    }
    auto g1 = TGraph(x.size(), x.data(), y_max.data());
    g1.SetTitle(std::string("unnorm. max. occupancy in "+std::to_string(fine_evt_per_bin)+" evt blocks on plane "+std::to_string(pid)+";event number;max. hits/evt per "+std::to_string(fine_evt_per_bin)+" evts").c_str());
    auto g2 = TGraph(x.size(), x.data(), y_avg.data());
    g2.SetTitle(std::string("unnorm. avg. occupancy in "+std::to_string(fine_evt_per_bin)+" evt blocks on plane "+std::to_string(pid)+";event number;avg. hits/evt per "+std::to_string(fine_evt_per_bin)+" evts").c_str());
    g1.Write(std::string("p"+std::to_string(pid)+"_max_occ").c_str());
    g2.Write(std::string("p"+std::to_string(pid)+"_avg_occ").c_str());
  };

  for(auto& [id, ops]: plane_ops) {
    if(id == id_ref) continue;
    auto pdir = f.mkdir(std::string("plane"+std::to_string(id)).c_str());
    pdir->cd();
    auto xdir = pdir->mkdir(ops.anti ? "xycorr" : "xxcorr");
    auto ydir = pdir->mkdir(ops.anti ? "yxcorr" : "yycorr");
    xdir->cd();
    savehistostack(ops.xhist_v);
    ydir->cd();
    savehistostack(ops.yhist_v);
    f.cd();
  }

  auto evo_histos = [&](std::string name, std::vector<TH2I>& hstack) {
    int bins = nev/evtsperbin;
    auto evolution = std::make_unique<TH2D>(name.c_str(), name.c_str(), bins, 0, bins-1, 2*max_shift+1, -max_shift, max_shift );
    evolution->GetXaxis()->SetTitle(std::string("event block ["+std::to_string(evtsperbin)+" evts/bin]").c_str());
    evolution->GetYaxis()->SetTitle("event offset");
    for(int jx = 0; jx < hstack.size(); jx++) {
     auto& h = hstack[jx];
     for(int ix = 0; ix < bins; ix++) {
       auto proj = h.ProjectionY("", ix+1, ix+1);
       auto max_bin = proj->GetMaximumBin();
       evolution->Fill(ix, jx-max_shift, proj->GetBinContent(max_bin));
       delete proj;
     }
    }
    evolution->Write();
    return evolution;
  };

  plane_sync s(evtsperbin);
  std::map<int, std::vector<int>> sync;

  for(auto & [id, ops]: plane_ops) {
    if(id == id_ref) continue;

    auto& sync_stream = sync[id];

    std::unique_ptr<TH2D> h1 = nullptr;
    std::unique_ptr<TH2D> h2 = nullptr;

    std::string prefix = "p"+std::to_string(id)+"_";
    if(ops.anti) { 
     h1 = evo_histos(prefix+"evo_xy", ops.xhist_v);
     h2 = evo_histos(prefix+"evo_yx", ops.yhist_v);
    } else {
     h1 = evo_histos(prefix+"evo_xx", ops.xhist_v);
     h2 = evo_histos(prefix+"evo_yy", ops.yhist_v);
    }
   auto h3 = std::unique_ptr<TH2D>(static_cast<TH2D*>(h1->Clone()));
   h3->Add(h2.get());
   h3->SetNameTitle(std::string(prefix+"evo_both").c_str(),"evo_both");
   h3->Write();
      
    max_hit_plots(ops.evt_size_history, id);

   int bins = nev/evtsperbin;
   std::cout << "Sync on plane " << id << '\n';
   for(int ix = 0; ix < bins; ix++) {
     auto proj = h3->ProjectionY("", ix+1, ix+1);
     auto max_bin = proj->GetMaximumBin();
     sync_stream.emplace_back(std::round(proj->GetBinCenter(max_bin)));
     std::cout << std::round(proj->GetBinCenter(max_bin)) << " ";
   }
   s.add_plane(id, sync_stream);
   std::cout << '\n';
  }

  s.write_json("");




  eudaq::FileReader resync_reader = ("native", filename);
  auto writer = std::unique_ptr<eudaq::FileWriter>(eudaq::FileWriterFactory::Create(""));
  writer->SetFilePattern(filename+"_new");
  writer->StartRun(1);

  std::deque<eudaq::StandardEvent> stream;
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
    if(ix%100 == 0) std::cout << "Event " << ix << " (bin " << ix/evtsperbin << ")\n";
    bool hasEvt = resync_reader.NextEvent(0);
    if(!hasEvt) {
      std::cout << "EOF reached!\n";	    
      break;
    }
    auto dev = resync_reader.GetDetectorEvent();
    full_event_buffer.push(dev);
/*
    std::cout << "Detector event has " << dev.NumEvents() << " events with subtypes:\n";

    std::vector<std::shared_ptr<eudaq::Event>> non_yarr_evt;

    for(size_t x = 0; x < dev.NumEvents(); x++){
      auto evt = std::shared_ptr<eudaq::Event>(const_cast<eudaq::Event*>(dev.GetEvent(x)));
      auto rev = dynamic_cast<eudaq::RawDataEvent*>(evt.get());
      if(rev) {
        std::cout << "Evt " << x << " subtype: " << rev->GetSubType() << '\n';
        auto & decoder_plugin = eudaq::PluginManager::GetInstance().GetPlugin(*evt);
        auto sev = eudaq::StandardEvent();
        try {
          decoder_plugin.GetStandardSubEvent(sev, *evt);
        } catch (...) { 
          std::cout << "Exception ..\n";
        }
        std::cout << "-- we have: " << sev.NumPlanes() << " planes!\n";
        for(size_t y = 0; y < sev.NumPlanes(); y++) {
          std::cout << "---- plane: " << sev.GetPlane(y).ID() << '\n';
          if(rev->GetSubType() == "Yarr") {
              auto id = rev->GetID(y);
              auto sid = sev.GetPlane(y).ID() - 110;
              std::cout << "Diff is: " << sid - id << '\n';
              std::cout << "Data block for plane: " << 110 + rev->GetID(y) << "\n";
              eudaq::RawDataEvent::data_t block = rev->GetBlock(y);
              std::cout << "Data block size: " << block.size() << "\n";
          } else {
            non_yarr_evt.emplace_back(evt);
          }
        }
      } else {
        auto tev = dynamic_cast<eudaq::TLUEvent*>(evt.get());
        if(tev) {
          std::cout << "Evt " << x << " is a TLU event\n";
          non_yarr_evt.emplace_back(evt);
        } else {
          std::cout << "Corruption!\n";
        }
      }
      non_yarr_evts.push(non_yarr_evt);
    }

    //stream.emplace_back(eudaq::PluginManager::ConvertToStandard(dev));
    */
    if(ix < 2*max_shift) continue;
    if(!s.is_good_evt(ix)) continue;

    int current_evt = ix-max_shift;
    int run_number = 1;

    eudaq::DetectorEvent outEvt(run_number, current_evt, 0);
    auto orig_evt = full_event_buffer.get(0);
    for(size_t x = 0; x < orig_evt.NumEvents(); x++){
      auto evt = orig_evt.GetEventPtr(x);
      auto rev = dynamic_cast<eudaq::RawDataEvent*>(evt.get());
      if(rev) {
        if(rev->GetSubType() != "Yarr") {
          outEvt.AddEvent(evt);
        }
      } else {
        auto tev = dynamic_cast<eudaq::TLUEvent*>(evt.get());
        if(tev) {
          outEvt.AddEvent(evt);
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
              //std::cout << "Updating sensor to: " << vec[ix] << '\n';
              sensor_id = vec[iy];
              //auto shift_needed = planeShift(sensor_id, current_evt); //TODO
              auto shift_needed = s.get_resync_value(sensor_id, ix);
              //sync[sensor_id].at(ix/evtsperbin);
              shifted_evt = &full_event_buffer.get(shift_needed);
            }
            auto & yarr_subevt = shifted_evt->GetRawSubEvent("Yarr", id);
            auto block_data = yarr_subevt.GetBlock(iy);
            auto block_id = yarr_subevt.GetID(iy);
            new_yarr_evt->AddBlock(block_id, block_data);
          }
          outEvt.AddEvent(new_yarr_evt_ptr);
      }
    }

    writer->WriteEvent(outEvt);
    //std::cout << "Wrote event\n";
  }
  return 0;
}