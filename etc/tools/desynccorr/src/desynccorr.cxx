#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <deque>
#include <algorithm>

#include "TFile.h"
#include "TH2I.h"

struct pixel_hit {
  int x;
  int y;
  pixel_hit(int x, int y): 
    x(x), y(y) {}
  pixel_hit() = delete;
};

struct cluster {
  double x;
  double y;
  std::vector<pixel_hit> pixel_hits;
  cluster(double x, double y) : x(x), y(y) {}
  cluster(double x, double y, std::vector<pixel_hit> &&pixel_hits) : x(x), y(y), pixel_hits(std::move(pixel_hits)) {}
  cluster() = delete;
};

//poor man's (woman's, person's?) clustering
std::vector<cluster> clusterHits(std::vector<pixel_hit> const & hits, int spatCutSqrd){
  //single pixel_hit events are easy, we just got one single hit cluster
  if(hits.size() == 1){
    std::vector<cluster> result;
    auto const & pix = hits[0];
    //For single pixel clusters the cluster position is in the centre of the pixel
    //Since we start counting at pixel 1 (not at 0), this is shifter by -0.5 px //check if this holds in EUDAQ, this was for something else
    result.emplace_back(pix.x-0.5, pix.y-0.5);
    result.back().pixel_hits = hits;
    return result;
  //multi pixel_hit events are more complicated
  } else {
    std::vector<pixel_hit> hitPixelVec = hits;
    std::vector<pixel_hit> newlyAdded;    
    std::vector<cluster> clusters;

    while( !hitPixelVec.empty() ) {
      //this is just a placeholder cluster so far, we will add hits and do the CoG computation later
      clusters.emplace_back(-1.,-1);      
      newlyAdded.push_back( hitPixelVec.front() );
      clusters.back().pixel_hits.push_back( hitPixelVec.front() );
      hitPixelVec.erase( hitPixelVec.begin() );
      
      while( !newlyAdded.empty() ) {
        bool newlyDone = true;
        int  x1, x2, y1, y2, dX, dY;

        for( auto candidate = hitPixelVec.begin(); candidate != hitPixelVec.end(); ++candidate ){
          //get the relevant infos from the newly added pixel
          x1 = newlyAdded.front().x;
          y1 = newlyAdded.front().y;

          //and the pixel we test against
          x2 = candidate->x;
          y2 = candidate->y;
          dX = x1-x2;
          dY = y1-y2;
          int spatDistSqrd = dX*dX+dY*dY;
  
          if( spatDistSqrd <= spatCutSqrd ) {
            newlyAdded.push_back( *candidate );  
            clusters.back().pixel_hits.push_back( *candidate );
            hitPixelVec.erase( candidate );
            newlyDone = false;
            break;
          }
        }
        if(newlyDone) {
          newlyAdded.erase(newlyAdded.begin());
        }
      }
    }
    //do the CoG cluster computation
    for(auto& c: clusters) {
      float x_sum = 0;
      float y_sum = 0;
      for(auto const & h: c.pixel_hits) {
        x_sum += h.x;
        y_sum += h.y;
            }
      c.x = x_sum/c.pixel_hits.size()-0.5;
      c.y = y_sum/c.pixel_hits.size()-0.5;
    }

  return clusters;
  }      
}

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

int main( int argc, char ** argv ){
  eudaq::OptionParser op("EUDAQ desynccorrelator", "1.0", "", 0, 10);
  eudaq::Option<std::string> pFile(op, "f", "filename", "", "string", "Input RAW file path");
  eudaq::Option<bool> pAnticorrelate(op, "a", "anti", false, "bool", "Anticorrelate planes");
  eudaq::Option<int> pRefPlane(op, "r", "refplane", 1, "int", "PlaneID acting as reference");
  eudaq::Option<int> pDUTPlane(op, "d", "dutplane", 24, "int", "PlaneID acting as DUT");
  eudaq::Option<bool> pFlipX(op, "x", "flipx", false, "bool", "Flip x-axis");
  eudaq::Option<bool> pFlipY(op, "y", "flipy", false, "bool", "Flip y-axis");
  eudaq::Option<size_t> pNEvts(op, "n", "nevts", 20000, "size_t", "Number of events to process");
  eudaq::Option<size_t> pMaxOffset(op, "t", "maxoffset", 50, "size_t", "Max offset to look at in +/- events");
  eudaq::Option<size_t> pEvtsPerBin(op, "b", "evtsperbin", 200, "size_t", "Events per time axis bin");
  eudaq::Option<std::string> pOFile(op, "o", "outfilename", "correlator", "string", "Output filename (w/o extension)");

  bool anticorrelate = true;
  double flipx = 1.;
  double flipy = 1.;
  std::string filename;
  int id_ref = 1;
  int id_sample = 24;
  size_t nev = 20000;
  int max_shift = 25;
  size_t evtsperbin = 200;
  std::string ofile;
  try{
    op.Parse(argv);
    
    if(!pFile.IsSet() || !pDUTPlane.IsSet() ) {
      usage();
      return -1;
    }
    anticorrelate = pAnticorrelate.Value();
    filename = pFile.Value();
    id_ref = pRefPlane.Value();
    id_sample = pDUTPlane.Value();
    if(pFlipX.Value()) flipx = -1;
    if(pFlipY.Value()) flipy = -1;
    nev = pNEvts.Value();
    max_shift = pMaxOffset.Value();
    evtsperbin = pEvtsPerBin.Value();
    ofile = pOFile.Value()+".root";
  }catch(...){
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

  double ref_pitch_x = 0;
  double ref_pitch_y = 0;
  double sample_pitch_x = 0;
  double sample_pitch_y = 0;

  int ref_px_x = 0;
  int ref_px_y = 0;
  int sample_px_x = 0;
  int sample_px_y = 0;

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
      //nd store it. This allows us to be stable against the cases where the first event(s) are corrupted and
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
          if(id == id_ref) {
		  ref_px_x = detPxSize[id].first; 
		  ref_px_y = detPxSize[id].second;
		  ref_pitch_x = detPxPitch[id].first;
		  ref_pitch_y = detPxPitch[id].second;
	  } else if (id == id_sample) {
		  sample_px_x = detPxSize[id].first; 
		  sample_px_y = detPxSize[id].second;
		  sample_pitch_x = detPxPitch[id].first;
		  sample_pitch_y = detPxPitch[id].second;
	  }
          noiseHitMap[id] = std::vector<long int>(detPxSize[id].first*detPxSize[id].second, 0);
	}
      }

      if(id == id_ref || id == id_sample) {
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

  std::vector<int> noise_pix_ref;
  std::vector<int> noise_pix_dut;

  for(auto const & [plane, hitmap]: noiseHitMap) {
    if(!( plane == id_ref || plane == id_sample)) continue;
    std::cout << "Noisy pixels on plane: " << plane << ":\n";
    for(size_t px = 0; px < hitmap.size(); px++) {
      if(hitmap[px]*1./noiseevts > 0.005) {
	      std::cout << "Pixel: " << px%detPxSize[plane].first << '/' << px/detPxSize[plane].first << '\n';
	      if(plane == id_ref) noise_pix_ref.emplace_back(px);
	      else if(plane == id_sample) noise_pix_dut.emplace_back(px);
      }
    }
  }

  std::cout << "Done..\n";
  std::vector<TH2I> xhist_v;
  std::vector<TH2I> yhist_v;

  for(int i = -max_shift; i <= max_shift; i++) {
    auto suffix = "_shift"+std::to_string(i);
    if(anticorrelate) xhist_v.emplace_back(std::string("p1x-p2y_temp_corr_"+suffix).c_str(), std::string("p1x-p2y_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
    else xhist_v.emplace_back(std::string("p1x-p2x_temp_corr_"+suffix).c_str(), std::string("p1x-p2x_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
    if(anticorrelate) yhist_v.emplace_back(std::string("p1y-p2x_temp_corr_"+suffix).c_str(), std::string("p1y-p2x_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
    else yhist_v.emplace_back(std::string("p1y-p2y_temp_corr_"+suffix).c_str(), std::string("p1y-p2y_temp_corr_"+suffix).c_str(), nev/evtsperbin, 0, nev/evtsperbin-1, 300, -20000, 20000);
  }

  std::deque<std::vector<cluster>> plane1_stream;
  std::deque<std::vector<cluster>> plane2_stream;

  for(size_t ix = 0; ix < nev; ix++) {  
    if(ix%100 == 0) std::cout << "Event " << ix << " (bin " << ix/evtsperbin << ")\n";
    bool hasEvt = reader.NextEvent(0);
    if(!hasEvt) {
      std::cout << "EOF reached!\n";	    
      break;
    }
    auto & evt = reader.GetDetectorEvent();
    auto stdEvt = eudaq::PluginManager::ConvertToStandard(evt);

    std::vector<pixel_hit> plane1_hits;
    std::vector<pixel_hit> plane2_hits;

    for(size_t plix = 0; plix < stdEvt.NumPlanes(); plix++) {
      auto & plane = stdEvt.GetPlane(plix);
      auto id = plane.ID();
      if(id == id_ref || id == id_sample) {
        for(size_t piix = 0; piix <  plane.HitPixels(); piix++) {
	  auto x = plane.GetX(piix);
	  auto y = plane.GetY(piix);
          if(id == id_ref) {
            auto i = y*ref_px_x + x;
	    if(std::find(noise_pix_ref.begin(), noise_pix_ref.end(), i) == noise_pix_ref.end()) plane1_hits.emplace_back(x, y);
	  }
          else {
            auto i = y*sample_px_x + x;
	    if(std::find(noise_pix_dut.begin(), noise_pix_dut.end(), i) == noise_pix_dut.end()) plane2_hits.emplace_back(x, y);
	  }
        }
      }
    }

    plane1_stream.push_front(clusterHits(plane1_hits, 4));
    plane2_stream.push_front(clusterHits(plane2_hits, 4));

    //if we didn't process yet enough events to loop back -max_shift and in front +max_shift,
    //we can't process the data
    if(ix < 2*max_shift) continue;

    auto correlate = [&](size_t shift){
      for(auto &p1h: plane1_stream[max_shift]) {
        for(auto &p2h: plane2_stream[max_shift-shift]) {
          double xdist, ydist;

          if(anticorrelate) xdist = (p1h.x-ref_px_x/2)*ref_pitch_x-(p2h.y-sample_px_y/2)*sample_pitch_y*flipy;
	  else xdist =              (p1h.x-ref_px_x/2)*ref_pitch_x-(p2h.x-sample_px_x/2)*sample_pitch_x*flipx;
          if(anticorrelate) ydist = (p1h.y-ref_px_y/2)*ref_pitch_y-(p2h.x-sample_px_x/2)*sample_pitch_x*flipx;
	  else ydist =              (p1h.y-ref_px_y/2)*ref_pitch_y-(p2h.y-sample_px_y/2)*sample_pitch_y*flipy;
          
	  xhist_v[shift+max_shift].Fill((ix)/evtsperbin, xdist);
          yhist_v[shift+max_shift].Fill((ix)/evtsperbin, ydist);
        }
      }
    };

    for(int i = -max_shift; i <= max_shift; i++) {
      correlate(i);
    }

    plane1_stream.pop_back();
    plane2_stream.pop_back();

  }
    
  f.cd();
  auto savehistostack = [&](std::vector<TH2I> hstack){
    for(auto& h: hstack) h.Write();
  };

  auto xdir = f.mkdir(anticorrelate ? "xycorr" : "xxcorr");
  auto ydir = f.mkdir(anticorrelate ? "yxcorr" : "yycorr");
  xdir->cd();
  savehistostack(xhist_v);
  ydir->cd();
  savehistostack(yhist_v);
  f.cd();

  auto evo_histos = [&](std::string name, std::vector<TH2I>& hstack) {
    int bins = nev/evtsperbin;
    auto evolution = std::make_unique<TH2D>(name.c_str(), name.c_str(), bins, 0, bins-1, 2*max_shift+1, -max_shift, max_shift );
    evolution->GetXaxis()->SetTitle(std::string("event block ["+std::to_string(evtsperbin)+" evts/bin]").c_str());
    evolution->GetYaxis()->SetTitle("event offset");
    for(int jx = 0; jx < hstack.size(); jx++) {
     auto& h = hstack[jx];
     for(int ix = 0; ix < bins; ix++) {
       auto proj = h.ProjectionY("", ix+1, ix+1);
       //double integral = proj->Integral();
       //if(integral < 1.) integral = 1;
       //proj->Scale(1./integral);
       auto max_bin = proj->GetMaximumBin();
       evolution->Fill(ix, jx-max_shift, proj->GetBinContent(max_bin));
       delete proj;
     }
    }
    evolution->Write();
    return evolution;
  };

  std::unique_ptr<TH2D> h1 = nullptr;
  std::unique_ptr<TH2D> h2 = nullptr;

  if(anticorrelate) { 
   h1 = evo_histos("evo_xy", xhist_v);
   h2 = evo_histos("evo_yx", yhist_v);
  } else {
   h1 = evo_histos("evo_xx", xhist_v);
   h2 = evo_histos("evo_yy", yhist_v);
  }
   auto h3 = std::unique_ptr<TH1I>(static_cast<TH1I*>(h1->Clone()));
   h3->Add(h2.get());
   h3->SetNameTitle("evo_both","evo_both");
   h3->Write();
   h3->SetNameTitle("evo_both_cut","evo_both_cut");
   auto maxbin = h3->GetMaximumBin();
   auto maxval = h3->GetBinContent(maxbin);
   auto maxval_minus = h3->GetBinContent(maxbin-1);
   auto maxval_plus = h3->GetBinContent(maxbin+1);
   h3->SetAxisRange(0.7*maxval, maxval+maxval_minus+maxval_plus, "Z");
   h3->Write();
  return 0;
}
