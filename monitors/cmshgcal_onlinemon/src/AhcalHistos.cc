// -*- mode: c -*-

#include <TROOT.h>
#include "AhcalHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include "TGraph.h"
#include <cstdlib>
#include <sstream>
#include "overFlowBins.hh"

AhcalHistos::AhcalHistos(eudaq::StandardPlane p, RootMonitor *mon)
  :_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
  _hitmap(NULL), _hitXmap(NULL),  _hitYmap(NULL),
  _nHits(NULL), _nbadHits(NULL), _nHotPixels(NULL){
    

  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "AhcalHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {

    sprintf(out, "%s %i Raw Hitmap", _sensor.c_str(), _id);
    sprintf(out2, "h_hitmap_%s_%02d", _sensor.c_str(), _id);
    _hitmap = new TH2I(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_hitmap, "X", "Y");
    // std::cout << "Created Histogram " << out2 << std::endl;

    sprintf(out, "%s %i Raw Hitmap X-Projection", _sensor.c_str(), _id);
    sprintf(out2, "h_hitXmap_%s_%02d", _sensor.c_str(), _id);
    _hitXmap = new TH1I(out2, out, _maxX + 1, 0, _maxX);
    SetHistoAxisLabelx(_hitXmap, "X");

    sprintf(out, "%s %i Raw Hitmap Y-Projection", _sensor.c_str(), _id);
    sprintf(out2, "h_hitYmap_%s_%02d", _sensor.c_str(), _id);
    _hitYmap = new TH1I(out2, out, _maxY + 1, 0, _maxY);
    SetHistoAxisLabelx(_hitYmap, "Y");


    //sprintf(out, "%s %i hot Pixel Map", _sensor.c_str(), _id);
    //sprintf(out2, "h_hotpixelmap_%s_%02d", _sensor.c_str(), _id);
    //_HotPixelMap = new TH2D(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    //SetHistoAxisLabels(_HotPixelMap, "X", "Y");


    sprintf(out, "%s %i Number of Hits", _sensor.c_str(), _id);
    sprintf(out2, "h_raw_nHits_%s_%02d", _sensor.c_str(), _id);
    _nHits = new TH1I(out2, out, 60, 0, 60);
    SetHistoAxisLabelx(_nHits, "Number of Hits above ZS");
    //_nHits->SetStats(1);

    /* Not used
    sprintf(out, "%s %i Number of Invalid Hits", _sensor.c_str(), _id);
    sprintf(out2, "h_nbadHits_%s_%i", _sensor.c_str(), _id);
    _nbadHits = new TH1I(out2, out, 50, 0, 50);
    SetHistoAxisLabelx(_nbadHits, "n_{BadHits}");

    sprintf(out, "%s %i Number of Hot Pixels", _sensor.c_str(), _id);
    sprintf(out2, "h_nhotpixels_%s_%i", _sensor.c_str(), _id);
    _nHotPixels = new TH1I(out2, out, 50, 0, 50);
    SetHistoAxisLabelx(_nHotPixels, "n_{HotPixels}");
    */

    // make a plane array for calculating e..g hotpixels and occupancy

    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "AhcalHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }


}

int AhcalHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void AhcalHistos::Fill(const eudaq::StandardPlane &plane) {
  // std::cout<< "FILL with a plane." << std::endl;

  if (_nHits != NULL) {
    _nHits->Fill(plane.HitPixels());   
    handleOverflowBins(_nHits);
  }


  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++)
    {
      //std::cout<<" We are getting a pixel with pix="<<pix<<std::endl;

      // Zero suppress empty picels
      if (plane.GetPixel(pix) <= 0)
	continue;
      

      const int pixel_x = plane.GetX(pix);
      const int pixel_y = plane.GetY(pix);

      if (_hitmap != NULL)
	_hitmap->Fill(pixel_x, pixel_y);
      if (_hitXmap != NULL)
	_hitXmap->Fill(pixel_x);
      if (_hitYmap != NULL)
	_hitYmap->Fill(pixel_y);

    }
}

void AhcalHistos::Reset() {

  _hitmap->Reset();
  _hitXmap->Reset();
  _hitYmap->Reset();

  _nHits->Reset();
  //_nbadHits->Reset();
  //_nHotPixels->Reset();
  //_HotPixelMap->Reset();


  // we have to reset the aux array as well
  zero_plane_array();
}

void AhcalHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void AhcalHistos::Write() {

  _hitmap->Write();
  _hitXmap->Write();
  _hitYmap->Write();

  _nHits->Write();

  //std::cout<<"Doing AhcalHistos::Write() before canvas drawing"<<std::endl;

  /*
  gSystem->Sleep(100);

  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();
  tmpcan->Close();
  gROOT->SetBatch(kFALSE);


  */
  
  //std::cout<<"Doing AhcalHistos::Write() after canvas drawing"<<std::endl;

}

int AhcalHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int AhcalHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int AhcalHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
