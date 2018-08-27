// -*- mode: c -*-

#include <TROOT.h>
#include "WireChamberCorrelationHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

WireChamberCorrelationHistos::WireChamberCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
  : _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false){

    
  char out[1024], out2[1024], out3[1024], out4[1024];

  _mon = mon;

  // std::cout << "WireChamberCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {
    for (int _ID=0; _ID<4; _ID++) {

      if (_id >= _ID) continue;

      sprintf(out, "%i vs. %i XX ", _id, _ID);
      sprintf(out2, "h_XXmap_%i_%i", _id, _ID);
      _correlationXX[_ID] = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
      
      sprintf(out3, "X_{%i} (mm)", _id);
      sprintf(out4, "X_{%i} (mm)", _ID);
      SetHistoAxisLabels(_correlationXX[_ID], out3, out4);



      sprintf(out, "%i vs. %i XY ", _id, _ID);
      sprintf(out2, "h_XYmap_%i_%i", _id, _ID);
      _correlationXY[_ID] = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
      
      sprintf(out3, "X_{%i} (mm)", _id);
      sprintf(out4, "Y_{%i} (mm)", _ID);
      SetHistoAxisLabels(_correlationXY[_ID], out3, out4);      



      sprintf(out, "%i vs. %i YY ", _id, _ID);
      sprintf(out2, "h_YYmap_%i_%i", _id, _ID);
      _correlationYY[_ID] = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
      
      sprintf(out3, "Y_{%i} (mm)", _id);
      sprintf(out4, "Y_{%i} (mm)", _ID);
      SetHistoAxisLabels(_correlationYY[_ID], out3, out4);       



      sprintf(out, "%i vs. %i YX ", _id, _ID);
      sprintf(out2, "h_YXmap_%i_%i", _id, _ID);
      _correlationYX[_ID] = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
      
      sprintf(out3, "Y_{%i} (mm)", _id);
      sprintf(out4, "X_{%i} (mm)", _ID);
      SetHistoAxisLabels(_correlationYX[_ID], out3, out4);     
    }
    

    // make a plane array for calculating e..g hotpixels and occupancy
    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "WireChamberCorrelationHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }


}

int WireChamberCorrelationHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void WireChamberCorrelationHistos::Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2) {
  // std::cout<< "FILL with a plane." << std::endl;

  float xl1 = plane1.GetPixel(0);
  float xr1 = plane1.GetPixel(1);
  float yu1 = plane1.GetPixel(2);
  float yd1 = plane1.GetPixel(3);
  
  if ((xl1<=0) || (xr1<=0) || (yu1<=0) || (yd1<=0)) return;

  float x1 = (xr1-xl1)/40*0.2; //one time unit of the tdc corresponds to 25ps, 1. conversion into nm, 
  float y1 = (yd1-yu1)/40*0.2; //2. conversion from nm to mm via the default calibration factor from the DWC manual
  
  
  int _ID = plane2.ID();
  float xl2 = plane2.GetPixel(0);
  float xr2 = plane2.GetPixel(1);
  float yu2 = plane2.GetPixel(2);
  float yd2 = plane2.GetPixel(3);

  if ((xl2<=0) || (xr2<=0) || (yu2<=0) || (yd2<=0)) return;
  
  float x2 = (xr2-xl2)/40*0.2; //one time unit of the tdc corresponds to 25ps, 1. conversion into nm, 
  float y2 = (yd2-yu2)/40*0.2; //2. conversion from nm to mm via the default calibration factor from the DWC manual


  _correlationXX[_ID]->Fill(x1, x2);
  _correlationXY[_ID]->Fill(x1, y2);
  _correlationYY[_ID]->Fill(y1, y2);
  _correlationYX[_ID]->Fill(y1, x2);

}

void WireChamberCorrelationHistos::Reset() {

  for (int _ID=0; _ID<4; _ID++) {
    if (_correlationXX.find(_ID)==_correlationXX.end()) continue;
    _correlationXX[_ID]->Reset();
    _correlationXY[_ID]->Reset();
    _correlationYY[_ID]->Reset();
    _correlationYX[_ID]->Reset();
  }
    
  // we have to reset the aux array as well
  zero_plane_array();
}

void WireChamberCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = false;
}

void WireChamberCorrelationHistos::Write() {
  
  for (int _ID=0; _ID<4; _ID++) {
    if (_correlationXX.find(_ID)==_correlationXX.end()) continue;
    _correlationXX[_ID]->Write();
    _correlationXY[_ID]->Write();
    _correlationYY[_ID]->Write();
    _correlationYX[_ID]->Write();
  }
    
  

  /*
  gSystem->Sleep(100);

  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();
  tmpcan->Close();
  gROOT->SetBatch(kFALSE);


  */

  //std::cout<<"Doing WireChamberCorrelationHistos::Write() after canvas drawing"<<std::endl;

}

int WireChamberCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int WireChamberCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int WireChamberCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
