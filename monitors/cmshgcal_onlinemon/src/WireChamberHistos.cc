// -*- mode: c -*-

#include <TROOT.h>
#include "WireChamberHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

WireChamberHistos::WireChamberHistos(eudaq::StandardPlane p, RootMonitor *mon)
  : _sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
    _XYmap(NULL), _goodAll(NULL), _recoX(NULL), _recoY(NULL) {

  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "WireChamberHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;


  sprintf(out, "%s %i good All", _sensor.c_str(), _id);
  sprintf(out2, "h_goodAll_%s_%i", _sensor.c_str(), _id);
  _goodAll = new TH1I(out2, out, 2, 0, 2);
  SetHistoAxisLabels(_goodAll, "measurement ok ?", "N");

  sprintf(out, "%s %i reco X", _sensor.c_str(), _id);
  sprintf(out2, "h_recoX_%s_%i", _sensor.c_str(), _id);
  _recoX = new TH1F(out2, out, 100, -50., 50.);
  SetHistoAxisLabels(_recoX, "X (mm)", "N");

  sprintf(out, "%s %i reco Y", _sensor.c_str(), _id);
  sprintf(out2, "h_recoY_%s_%i", _sensor.c_str(), _id);
  _recoY = new TH1F(out2, out, 100, -50., 50.);
  SetHistoAxisLabels(_recoY, "Y (mm)", "N");

  sprintf(out, "%s %i XY map", _sensor.c_str(), _id);
  sprintf(out2, "h_XYmap_%s_%i", _sensor.c_str(), _id);
  _XYmap = new TH2F(out2, out, 100, -50., 50., 100, -50., 50.);
  SetHistoAxisLabels(_XYmap, "X (mm)", "Y (mm)");

}

void WireChamberHistos::Fill(const eudaq::StandardPlane &plane) {
  // std::cout<< "FILL with a plane." << std::endl;

  float xl = plane.GetPixel(0);
  float xr = plane.GetPixel(1);
  float yu = plane.GetPixel(2);
  float yd = plane.GetPixel(3);

  int good_xl = xl > 0 ? 1 : 0;
  int good_xr = xr > 0 ? 1 : 0;
  int good_yu = yu > 0 ? 1 : 0;
  int good_yd = yd > 0 ? 1 : 0;
  int good_x = (good_xl + good_xr) == 2 ? 1 : 0;
  int good_y = (good_yu + good_yd) == 2 ? 1 : 0;
  int good_all = (good_x + good_y) == 2 ? 1 : 0;

  float x = (xr - xl) / 40 * 0.2; //one time unit of the tdc corresponds to 25ps, 1. conversion into nm,
  float y = (yd - yu) / 40 * 0.2; //2. conversion from nm to mm via the default calibration factor from the DWC manual

  _goodAll->Fill(good_all);

  if (good_x) {
    //std::cout<<"In Filling of 1D-Hists: x="<<x<<std::endl;
    _recoX->Fill(x);
  }

  if (good_y) {
    //std::cout<<"In Filling of 1D-Hists: y="<<y<<std::endl;
    _recoY->Fill(y);
  }

  if (good_all) {
    //std::cout<<"In Filling of 2D-Hists: x="<<x<<" y="<<y<<std::endl;
    _XYmap->Fill(x, y);
  }

}

void WireChamberHistos::Reset() {

  _XYmap->Reset();
  _recoX->Reset();
  _recoY->Reset();
  _goodAll->Reset();

}

void WireChamberHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void WireChamberHistos::Write() {

  _XYmap->Write();

  _recoX->Write();
  _recoY->Write();
  _goodAll->Write();

  //std::cout<<"Doing WireChamberHistos::Write() before canvas drawing"<<std::endl;

  /*
  gSystem->Sleep(100);

  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();
  tmpcan->Close();
  gROOT->SetBatch(kFALSE);


  */

  //std::cout<<"Doing WireChamberHistos::Write() after canvas drawing"<<std::endl;

}

int WireChamberHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int WireChamberHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int WireChamberHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
