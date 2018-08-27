// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#include <TROOT.h>
#include "DATURAToHGCALCorrelationHistos.hh"
#include "HexagonHistos.hh"   //for the channel mapping
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>


double MAXCLUSTERRADIUS2 = 9.0;
float pixelGap_MIMOSA26 = 0.0184; //mm

//some simple clustering
void DATURAToHGCALCorrelationHistos::findClusters(std::vector<std::pair<int, int> >& entities, std::vector<std::pair<float, float> >& clusters ) {
  std::vector<bool> used;
  for (size_t i=0; i<entities.size(); i++) used.push_back(false);

  for (size_t i=0; i<entities.size(); i++) {
    if (used[i]) continue;
    float cluster_X=entities[i].first, cluster_Y=entities[i].second;
    size_t clusterEntries=1;
    used[i]=true;
    for (size_t j=i+1; j<entities.size(); j++) {
      if (used[j]) continue;
      float d = pow(entities[j].first-cluster_X, 2) + pow(entities[j].second-cluster_Y, 2);
      if (d > MAXCLUSTERRADIUS2) continue;
      used[j] = true;
      cluster_X = (clusterEntries*cluster_X + entities[j].first);
      cluster_Y = (clusterEntries*cluster_Y + entities[j].second);
      clusterEntries++;
      cluster_X /= clusterEntries;
      cluster_Y /= clusterEntries;
    }  
  
    clusters.push_back(std::make_pair(cluster_X, cluster_Y));
  }
}



DATURAToHGCALCorrelationHistos::DATURAToHGCALCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false){
    
  char out[1024], out2[1024];

  _mon = mon;

  if (_maxX != -1 && _maxY != -1) {

    sprintf(out, "%s %i, Channel Occupancy", _sensor.c_str(), _id);
    sprintf(out2, "h_hgcal_channel_occupancy_%s_%i", _sensor.c_str(), _id);
    Occupancy_ForChannel = new TH2I(out2, out, 4, -0.5, 3.5, 64, -0.5, 63.5);
    Occupancy_ForChannel->SetOption("COLZ");
    Occupancy_ForChannel->SetStats(false);
    SetHistoAxisLabels(Occupancy_ForChannel, "Skiroc index", "channel index");   
    

    for (int ski=0; ski<4; ski++) {
      for (int ch=0; ch<=64; ch++) {
        if (ch%2==1) continue;
        int key = ski*1000+ch;
        sprintf(out, "%s %i, skiroc %i, channel %i projected on MIMOSA26 plane 3", _sensor.c_str(), _id, ski, ch);
        sprintf(out2, "h_hgcal_ski%i_ch%i_vs_MIMOSA26_3_%s_%i", ski, ch, _sensor.c_str(), _id);
        _MIMOSA_map_ForChannel[key] = new TH2F(out2, out, 40, 0, 1153*pixelGap_MIMOSA26, 20, 0, 557*pixelGap_MIMOSA26);   //1mmx1mm resolution
        _MIMOSA_map_ForChannel[key]->SetOption("COLZ");
        _MIMOSA_map_ForChannel[key]->SetStats(false);
        SetHistoAxisLabels(_MIMOSA_map_ForChannel[key], "MIMOSA26, plane 3, x [mm]", "MIMOSA26, plane 3, y [mm]");      
      }
    }

    
    // make a plane array for calculating e..g hotpixels and occupancy
    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "DATURAToHGCALCorrelationHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }

}

int DATURAToHGCALCorrelationHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void DATURAToHGCALCorrelationHistos::Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA2, const eudaq::StandardPlane &plMIMOSA3) {
  clusterEntitites.clear();
  clusters2.clear();
  good_cluster2.clear();
  clusters3.clear();


  //general strategy
  //1. cluster hits in MIMOSA planes 3 and 4
  //2. selection of clusters in plane 3 that seem correlated to a cluster in plane 4
  //3. signal selection of HGCal cells
  //4. fill MIMOSA plane 3 display for each cell to see hexagonal structures in case of synchronised data streams
    
  //1st: MIMOSA clustering
  std::vector<double> pxl2 = plMIMOSA2.GetPixels<double>();
  for (size_t ipix = 0; ipix < pxl2.size(); ++ipix) clusterEntitites.push_back(std::make_pair(plMIMOSA2.GetX(ipix), plMIMOSA2.GetY(ipix)));     
  //find the clusters2      
  findClusters(clusterEntitites, clusters2);


  clusterEntitites.clear();
  //2nd: MIMOSA clustering
  std::vector<double> pxl3 = plMIMOSA3.GetPixels<double>();
  for (size_t ipix = 0; ipix < pxl3.size(); ++ipix) clusterEntitites.push_back(std::make_pair(plMIMOSA3.GetX(ipix), plMIMOSA3.GetY(ipix)));     
  //find the clusters in plane4      
  findClusters(clusterEntitites, clusters3);
  

  //cluster in plane 3 selection based on correlation to any cluster in plane 4 (radial distance below 200 pixel units)
  for (size_t cl2=0; cl2<clusters2.size(); cl2++) {
    good_cluster2.push_back(false);
    for (size_t cl3=0; cl3<clusters3.size(); cl3++) {
      if (pow(clusters2[cl2].first-clusters3[cl3].first,2) + pow(clusters2[cl2].second-clusters3[cl3].second,2) > 40000) {    //cluster selection based on the presence of a matching hit in MIMOSA plane 4 (distance < 300 pixel units)
        good_cluster2[cl2] = true;
        break;
      }
    }
  }


  //reconstruct energy and perform simple selection signal
  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++) {
    const int skiroc = plane.GetX(pix);    // x pixel corresponds to the skiroc index
    const int channel = plane.GetY(pix);    //y pixel corresponds to the channel id

    //energy selection
    if (plane.GetPixel(pix, 3+13) - plane.GetPixel(pix, 0+13) < 20.) continue;
    if (plane.GetPixel(pix, 5+13) - plane.GetPixel(pix, 3+13) > 0) continue;
    if (plane.GetPixel(pix, 5+13) - plane.GetPixel(pix, 2+13) > 0) continue;
    
    //fill the occupancy in any case
    Occupancy_ForChannel->Fill(skiroc, channel);

    for (size_t cl2=0; cl2<clusters2.size(); cl2++) {
      if (!good_cluster2[cl2]) continue;
      int key = 1000*skiroc+channel;
      _MIMOSA_map_ForChannel[key]->Fill(clusters2[cl2].first*pixelGap_MIMOSA26, clusters2[cl2].second*pixelGap_MIMOSA26);   //yes/no entries
    }
  }

}

void DATURAToHGCALCorrelationHistos::Reset() {
  Occupancy_ForChannel->Reset();
  for (int ski=0; ski<4; ski++) for (int ch=0; ch<=64; ch+=2)
    _MIMOSA_map_ForChannel[1000*ski+ch]->Reset();
  
  // we have to reset the aux array as well
  zero_plane_array();
}

void DATURAToHGCALCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void DATURAToHGCALCorrelationHistos::Write() {
  Occupancy_ForChannel->Write();
  for (int ski=0; ski<4; ski++) for (int ch=0; ch<=64; ch+=2) 
    _MIMOSA_map_ForChannel[1000*ski+ch]->Write();

}

int DATURAToHGCALCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int DATURAToHGCALCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int DATURAToHGCALCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
