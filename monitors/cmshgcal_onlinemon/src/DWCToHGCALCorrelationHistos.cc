// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//04 June 2018

#include <TROOT.h>
#include "DWCToHGCALCorrelationHistos.hh"
#include "HexagonHistos.hh"   //for the channel mapping
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>


DWCToHGCALCorrelationHistos::DWCToHGCALCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
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
        _DWC_map_ForChannel[key] = new TH2F(out2, out, 100, -50., 50., 100, -50, 50.);   //1mmx1mm resolution
        _DWC_map_ForChannel[key]->SetOption("COLZ");
        _DWC_map_ForChannel[key]->SetStats(false);
        SetHistoAxisLabels(_DWC_map_ForChannel[key], "DWC_{0}, x [mm]", "DWC_{0}, y [mm]");      
      }
    }

    
    // make a plane array for calculating e..g hotpixels and occupancy
    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "DWCToHGCALCorrelationHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }

}

int DWCToHGCALCorrelationHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void DWCToHGCALCorrelationHistos::Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plDWC0) {
  
  float xl = plDWC0.GetPixel(0);
  float xr = plDWC0.GetPixel(1);
  float yu = plDWC0.GetPixel(2);
  float yd = plDWC0.GetPixel(3);

  if ((xl<=0) || (xr<=0) || (yu<=0) || (yd<=0)) return;     //need reconstructed DWC point to fill correlation
  
  
  float dwc_x = (xr-xl)/40*0.2; //one time unit of the tdc corresponds to 25ps, 1. conversion into nm, 
  float dwc_y = (yd-yu)/40*0.2; //conversion from nm to mm via the default calibration factor from the DWC manual


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
    _DWC_map_ForChannel[1000*skiroc+channel]->Fill(dwc_x, dwc_y);   //yes/no entries
  
  }
  

}

void DWCToHGCALCorrelationHistos::Reset() {
  Occupancy_ForChannel->Reset();
  for (int ski=0; ski<4; ski++) for (int ch=0; ch<=64; ch+=2)
    _DWC_map_ForChannel[1000*ski+ch]->Reset();
  
  // we have to reset the aux array as well
  zero_plane_array();
}

void DWCToHGCALCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void DWCToHGCALCorrelationHistos::Write() {
  Occupancy_ForChannel->Write();
  for (int ski=0; ski<4; ski++) for (int ch=0; ch<=64; ch+=2) 
    _DWC_map_ForChannel[1000*ski+ch]->Write();

}

int DWCToHGCALCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int DWCToHGCALCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int DWCToHGCALCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
