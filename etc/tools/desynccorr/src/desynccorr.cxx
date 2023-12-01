#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <deque>
#include <algorithm>
#include <numeric>

#include "syncobject.h"
#include "clustering.h"

#include "TFile.h"
#include "TH2I.h"
#include "TGraph.h"

void usage() {
  std::cout << "EUDAQ desynccorrelator options:\n"
	    << "(nb: bool must be provided as 1 or 0)\n"
      << "(nb: lists are csv, order is determined by DUT list)\n"
	    << "-f (--filename) string\t\tInput RAW file path [REQUIRED]\n"
	    << "-o (--outfilename) string\tOutput ROOT file name (w/o extension) [def: histo_run<runnumber>]\n"
	    << "-a (--anti) list<bool>\t\tDo anticorrelations (xy,yx) instead of correlations (xx,yy) [def: 0]\n"
	    << "-r (--refplane) int\t\tPlaneID acting as reference [def: 1]\n"
	    << "-d (--dutplane) list<int>\tPlaneIDs  acting as DUT [REQUIRED]\n"
	    << "-x (--flipx) list<bool>\t\tFlip x-axis (of DUT) [def: 0]\n"
	    << "-y (--flipy) list<bool>\t\tFlip y-axis (of DUT) [def: 0]\n"
	    << "-n (--nevts) size_t\t\tNumber of events to process [def: 20000]\n"
	    << "-t (--maxoffset) size_t\t\t+/-offset to look for correlations [def: 50]\n"
	    << "-b (--evtsperbin) size_t\tEvents per time axis (x-axis) bin [def: 200]\n"
      << "-w (--writesyncfile) bool\tWrite the output sync file [def: 1]\n";
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
  int px_max = 0;
  std::vector<TH2I> xhist_v;
  std::vector<TH2I> yhist_v;
  std::deque<std::vector<cluster>> stream;
  std::vector<pixel_hit> hits;
  std::vector<std::vector<int>> evt_size_history;
};

int main(int argc, char ** argv){
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
  eudaq::Option<std::string> pOFile(op, "o", "outfilename", "", "string", "Output filename (w/o extension)");
  eudaq::Option<bool> pSyncFileWriting(op, "w", "writesyncfile", true, "bool", "Write the output sync file");

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
  bool write_sync_file = true;

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

    write_sync_file = pSyncFileWriting.Value();
    nev = pNEvts.Value();
    max_shift = pMaxOffset.Value();
    evtsperbin = pEvtsPerBin.Value();
    if(pOFile.IsSet()) {
      ofile = pOFile.Value()+".root";
    }
  } catch(...) {
    usage();
    return -1;
  }

  auto TLU_timestamps = std::vector<std::vector<uint64_t>>{};
  TLU_timestamps.resize(nev/fine_evt_per_bin+1);

  eudaq::FileReader reader = ("native", filename);
  eudaq::FileReader noise_reader = ("native", filename);

  auto run_number = reader.RunNumber();
  std::cout << "Run number: " << run_number << '\n';

  //if the output root file name was not passed via the command line, set the default here
  if(ofile.empty()){
    ofile = "histo_run"+std::to_string(run_number)+".root";
  }
  eudaq::PluginManager::Initialize(reader.GetDetectorEvent());

  TFile f(ofile.c_str(), "RECREATE");
 
  std::map<int, std::pair<int,int>> detPxSize;
  std::map<int, std::pair<double, double>> detPxPitch;
  std::map<int, std::vector<long int>> noiseHitMap;

  size_t noiseevts = 10000;
  //first we have to do a noise run
  for(size_t ix = 0; ix < noiseevts; ix++) {  
    if(ix%500 == 0) std::cout << "Noise event " << ix << '\n';
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
		        //Some converter plugins mess up the X/Y sizes ... in order to have enough space for the noise maps
		        //we just take the larger axis and assume a squared pixel matrix of that size
		        plane_ops[id].px_max = std::max(plane_ops[id].px_x, plane_ops[id].px_y);
		        plane_ops[id].pitch_x = detPxPitch[id].first;
		        plane_ops[id].pitch_y= detPxPitch[id].second;
		        noiseHitMap[id] = std::vector<long int>(plane_ops[id].px_max*plane_ops[id].px_max, 0);
          }
	      }
      }

      if(is_ref_or_dut(id)) {
        for(size_t piix = 0; piix <  plane.HitPixels(); piix++) {
	        int i = plane.GetY(piix)*plane_ops[id].px_max+plane.GetX(piix);
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
    for(size_t px = 0; px < hitmap.size(); px++) {
      if(hitmap[px]*1./noiseevts > 0.005) {
        plane_ops[plane].noise_vec.emplace_back(px);
      }
    }
    std::cout << "Masked " << plane_ops[plane].noise_vec.size() << " pixels on plane " << plane << '\n';
  }

  std::cout << "Done with noise masking!\n";

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
    if(ix%500 == 0) std::cout << "Event " << ix << " with event number " << evt_nr << " (bin " << evt_nr/evtsperbin << ")\n";
    if(evt_nr > nev) {
      std::cout << "Reached event with event number: " << evt_nr << ". Stopping.\n";
      break;
    }

    auto fine_bin = evt_nr/fine_evt_per_bin;
    auto fine_bin_remainder = evt_nr%fine_evt_per_bin;

    //Timestamp handling
    auto timestamp = evt.GetTimestamp();
    TLU_timestamps[fine_bin].emplace_back(timestamp);
    //Since we average over a few bins, e.g. [evt1, evt2, evt3], [evt4, evt5, evt6], [evt7, evt8, evt9]
    //We need to add the timestamp of e.g. evt3 to both subvectors since the [d]eltas we need in the blocks
    //are [d12, d23], [d34, d45, d56], [d67, d78, d89] - the deltas are computed later
    if((fine_evt_per_bin-fine_bin_remainder == 1) && (fine_bin+1 < TLU_timestamps.size())) {
      TLU_timestamps[fine_bin+1].emplace_back(timestamp);
    }

    auto stdEvt = eudaq::PluginManager::ConvertToStandard(evt);
    for(size_t plix = 0; plix < stdEvt.NumPlanes(); plix++) {
      auto & plane = stdEvt.GetPlane(plix);
      auto id = plane.ID();
      if(is_ref_or_dut(id)) {
        auto & plane_data = plane_ops[id];
        plane_data.evt_size_history[fine_bin].emplace_back(plane.HitPixels());
        for(size_t piix = 0; piix < plane.HitPixels(); piix++) {
	        auto x = plane.GetX(piix);
	        auto y = plane.GetY(piix);
          auto i = y*plane_data.px_max + x;
	        if(std::find(plane_data.noise_vec.begin(), plane_data.noise_vec.end(), i) == plane_data.noise_vec.end()) plane_data.hits.emplace_back(x, y);
        }
      }
    }

    //This adds the event to the stream
    for(auto& [id, ops]: plane_ops) {
      ops.stream.push_front(clusterHits(ops.hits, 4));
      ops.hits.clear();
    }

    //if we didn't process yet enough events to loop back -max_shift and in front +max_shift,
    //we can't process the data
    if(ix < 2*max_shift) continue;

    auto correlate = [&](plane_options & ref, plane_options & dut, size_t shift){
      for(auto &p1h: ref.stream[max_shift]) {
        for(auto &p2h: dut.stream[max_shift-shift]) {
          double xdist, ydist;
          if(dut.anti) xdist = (p1h.x-ref.px_x/2)*ref.pitch_x-(p2h.y-dut.px_y/2)*dut.pitch_y*dut.flipy;
	        else xdist =         (p1h.x-ref.px_x/2)*ref.pitch_x-(p2h.x-dut.px_x/2)*dut.pitch_x*dut.flipx;
          if(dut.anti) ydist = (p1h.y-ref.px_y/2)*ref.pitch_y-(p2h.x-dut.px_x/2)*dut.pitch_x*dut.flipx;
	        else ydist =         (p1h.y-ref.px_y/2)*ref.pitch_y-(p2h.y-dut.px_y/2)*dut.pitch_y*dut.flipy;
	        dut.xhist_v[shift+max_shift].Fill((evt_nr-max_shift)/evtsperbin, xdist);
          dut.yhist_v[shift+max_shift].Fill((evt_nr-max_shift)/evtsperbin, ydist);
        }
      }
    };
    for(auto dut_id: id_samples) {
      for(int i = -max_shift; i <= max_shift; i++) {
        correlate(plane_ops[id_ref], plane_ops[dut_id], i);
      }
    }

    //Removing an event from the stream to stay in sync - probably this could be done earlier
    for(auto& [id, ops]: plane_ops) {
      ops.stream.pop_back();
    }
  } //main event loop

  f.cd();

  auto savehistostack = [](std::vector<TH2I> hstack){
    for(auto& h: hstack) h.Write();
  };

  auto max_trigrate_plots = [&](std::vector<std::vector<uint64_t>> const & v){
    auto x = std::vector<double>(v.size(), 0);
    auto y_max = std::vector<double>(v.size(), 0);
    auto y_min = std::vector<double>(v.size(), 0);
    auto y_avg = std::vector<double>(v.size(), 0);
    for(std::size_t i = 0; i < v.size(); i++) {
      x.at(i) = i*fine_evt_per_bin;
      std::vector<double> result;
      if(!v[i].empty()) result.resize(v[i].size()-1);
      for(std::size_t j = 1; j < v[i].size(); j++) {
        //The TLU clock has a period of 8 x 3.125 ns, the timestamp is in 1/8th of that
        result[j-1] = 1./(3.125E-9*(v[i][j]-v[i][j-1]));
      }

      //in theory, it would be better to remove entries from blocks where we don't have any data
      //and not leave them at default 0, but that would add some noise to the code ...
      const auto [min_it, max_it] = std::minmax_element(result.begin(), result.end());
      if(max_it != result.end()) {
        y_max[i] = *max_it;
      }
      if(min_it != result.end()) {
        y_min[i] = *min_it;
      }
      auto sum = std::accumulate(result.begin(), result.end(), 0);
      y_avg[i] = 1.*sum/fine_evt_per_bin;
    }

    auto g1 = TGraph(x.size(), x.data(), y_max.data());
    g1.SetTitle(std::string("max. trigger rate in "+std::to_string(fine_evt_per_bin)+" evt blocks;event number;max. trigger rate per "+std::to_string(fine_evt_per_bin)+" evts [Hz]").c_str());
    auto g2 = TGraph(x.size(), x.data(), y_min.data());
    g2.SetTitle(std::string("min. trigger rate in "+std::to_string(fine_evt_per_bin)+" evt blocks;event number;min. trigger rate per "+std::to_string(fine_evt_per_bin)+" evts [Hz]").c_str());
    auto g3 = TGraph(x.size(), x.data(), y_avg.data());
    g3.SetTitle(std::string("avg. trigger rate in "+std::to_string(fine_evt_per_bin)+" evt blocks;event number;avg. trigger rate per "+std::to_string(fine_evt_per_bin)+" evts [Hz]").c_str());

    //Top range of the trigger rate, set to 10kHz
    auto const max_trig_rate_plots = 10000;
    //We use a (roughly, integer division truncation) 100 Hz/bin binning for the 1D histograms
    auto h1 = TH1D("trig_max_1D", std::string("max. trigger rate per "+std::to_string(fine_evt_per_bin)+" event block; trigger rate [Hz]; events [a.u.]").c_str(), max_trig_rate_plots/100, 0, max_trig_rate_plots);
    auto h2 = TH1D("trig_min_1D", std::string("min. trigger rate per "+std::to_string(fine_evt_per_bin)+" event block; trigger rate [Hz]; events [a.u.]").c_str(), max_trig_rate_plots/100, 0, max_trig_rate_plots);
    auto h3 = TH1D("trig_avg_1D", std::string("avg. trigger rate per "+std::to_string(fine_evt_per_bin)+" event block; trigger rate [Hz]; events [a.u.]").c_str(), max_trig_rate_plots/100, 0, max_trig_rate_plots);

    h1.FillN(y_max.size(), y_max.data(), nullptr);
    h2.FillN(y_min.size(), y_min.data(), nullptr);
    h3.FillN(y_avg.size(), y_avg.data(), nullptr);

    g1.Write(std::string("max_trig").c_str());
    g2.Write(std::string("min_trig").c_str());
    g3.Write(std::string("avg_trig").c_str());

    h1.Write();
    h2.Write();
    h3.Write();
  };

  max_trigrate_plots(TLU_timestamps);

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

  plane_sync s(run_number, evtsperbin, max_shift);

  for(auto & [id, ops]: plane_ops) {
    if(id == id_ref) continue;

    auto sync_stream = std::vector<int>{};
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

  if(write_sync_file) {
    s.write_json("run"+std::to_string(run_number)+"_sync.json");
  } else {
    std::cout << "Skipped writing sync file!\n";
  }
  return 0;
}