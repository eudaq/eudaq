// -*- mode: c -*-

#include <TROOT.h>
#include "HexagonCorrelationHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

HexagonCorrelationHistos::HexagonCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
  : _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false){

    
  char out[1024], out2[1024], out3[1024], out4[1024];

  _mon = mon;

  // std::cout << "HexagonCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  if (_maxX != -1 && _maxY != -1) {
    for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {

      if (_id >= _ID) continue;

      sprintf(out, "%i vs. %i HG ", _id, _ID);
      sprintf(out2, "h_SignalHGSum_%i_%i", _id, _ID);
      _correlationSignalHGSum[_ID] = new TH2I(out2, out, 100, 0., 100., 100, 0., 100.);

      sprintf(out3, "HG TS3 Sum_{%i} (ADC)", _id);
      sprintf(out4, "HG TS3 SUM_{%i} (ADC)", _ID);
      SetHistoAxisLabels(_correlationSignalHGSum[_ID], out3, out4);  

      sprintf(out, "%i vs. %i TOA", _id, _ID);
      sprintf(out2, "h_corrTOA_%i_%i", _id, _ID);
      _correlationTOA[_ID] = new TH2I(out2, out, 100, 0., 2000., 100, 0., 2000.);
      sprintf(out3, "AVG TOA Module %i (ADC)", _id);
      sprintf(out4, "AVG TOA Module %i (ADC)", _ID);
      SetHistoAxisLabels(_correlationTOA[_ID], out3, out4);  

    }
    

    // make a plane array for calculating e..g hotpixels and occupancy
    plane_map_array = new int *[_maxX];

    if (plane_map_array != NULL) {
      for (int j = 0; j < _maxX; j++) {
        plane_map_array[j] = new int[_maxY];
        if (plane_map_array[j] == NULL) {
          cout << "HexagonCorrelationHistos :Error in memory allocation" << endl;
          exit(-1);
        }
      }
      zero_plane_array();
    }

  } else {
    std::cout << "No max sensorsize known!" << std::endl;
  }


}

int HexagonCorrelationHistos::zero_plane_array() {
  for (int i = 0; i < _maxX; i++) {
    for (int j = 0; j < _maxY; j++) {
      plane_map_array[i][j] = 0;
    }
  }
  return 0;
}


void HexagonCorrelationHistos::Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2) {
  // std::cout<< "FILL with a plane." << std::endl;
  int _ID = plane2.ID();


  //example: sum of HG in TS3 - TS0
  unsigned int sumHGTS3_1 = 0.;
  unsigned int sumHGTS3_2 = 0.;

  for (unsigned int pix = 0; pix < plane1.HitPixels(); pix++) {
    sumHGTS3_1 += (plane1.GetPixel(pix, 3+13) - plane1.GetPixel(pix, 0+13))/50.;
  }
  for (unsigned int pix = 0; pix < plane2.HitPixels(); pix++) {
    sumHGTS3_2 += (plane2.GetPixel(pix, 3+13) - plane2.GetPixel(pix, 0+13))/50.;
  }  
  _correlationSignalHGSum[_ID]->Fill(sumHGTS3_1, sumHGTS3_2);

  if (plane1.HitPixels() > 0 && plane2.HitPixels() > 0){
    //const unsigned int avgTOA_1 = 500 + std::rand()%20;
    //const unsigned int avgTOA_2 = 600 + std::rand()%10;

    const unsigned int avgTOA_1 = plane1.GetPixel(0, 30);
    const unsigned int avgTOA_2 = plane2.GetPixel(0, 30);

    _correlationTOA[_ID]->Fill(avgTOA_1, avgTOA_2);
  }

  //histograms to be filled here

}

void HexagonCorrelationHistos::Reset() {

  for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {
    if (_correlationSignalHGSum.find(_ID)==_correlationSignalHGSum.end()) continue;
    _correlationSignalHGSum[_ID]->Reset();
    _correlationTOA[_ID]->Reset();
  }
    
  // we have to reset the aux array as well
  zero_plane_array();
}

void HexagonCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = false;
}

void HexagonCorrelationHistos::Write() {
  
  for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {
    if (_correlationSignalHGSum.find(_ID)==_correlationSignalHGSum.end()) continue;
    _correlationSignalHGSum[_ID]->Write();
    _correlationTOA[_ID]->Write();

  }
    

}

int HexagonCorrelationHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HexagonCorrelationHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HexagonCorrelationHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
