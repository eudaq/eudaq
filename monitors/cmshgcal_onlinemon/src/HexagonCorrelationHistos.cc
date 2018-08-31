// -*- mode: c -*-

#include <TROOT.h>
#include "HexagonCorrelationHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>

HexagonCorrelationHistos::HexagonCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon)
: _sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false){


  char out[1024], out2[1024], out3[1024], out4[1024];

  _mon = mon;

  // std::cout << "HexagonCorrelationHistos::Sensorname: " << _sensor << " "<< _id<<std::endl;

  for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {

    if (_id >= _ID) continue;

    sprintf(out, "TOA Correlation: module %i vs. %i", _id, _ID);
    sprintf(out2, "h_corrTOA_%s_%ivs%i", _sensor.c_str(), _id, _ID);
    _correlationTOA[_ID] = new TH2I(out2, out, 50, 1000., 3000., 50, 1000., 3000.);
    sprintf(out3, "AVG TOA, Module %i (ADC)", _id);
    sprintf(out4, "AVG TOA, Module %i (ADC)", _ID);
    SetHistoAxisLabels(_correlationTOA[_ID], out3, out4);


    sprintf(out, "LG Correlation: module %i vs. %i", _id, _ID);
    sprintf(out2, "h_SignalLGSum_%s_%ivs%i", _sensor.c_str(), _id, _ID);
    _correlationSignalLGSum[_ID] = new TH2I(out2, out, 50, 0., 6000., 50, 0., 6000.);
    //TB 2017: Energy sum in a layer of the EE part for 90 GeV electrons barely reaches 1000 MIPs.
    // Also, 1 MIP ~ 5 LG ADC //Thorben Quast, 07 June 2018

    sprintf(out3, "LG Sum of (TS3 - TS0), Module %i (ADC)", _id);
    sprintf(out4, "LG Sum of (TS3 - TS0), Module %i (ADC)", _ID);
    SetHistoAxisLabels(_correlationSignalLGSum[_ID], out3, out4);

  }
}


void HexagonCorrelationHistos::Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2) {

  // std::cout<<"[HexagonCorrelationHistos::Fill]"<<std::endl;
  
  int _ID = plane2.ID();

  // TOA
  if (plane1.HitPixels() > 0 && plane2.HitPixels() > 0){
    const unsigned int avgTOA_1 = plane1.GetPixel(0, 30);
    const unsigned int avgTOA_2 = plane2.GetPixel(0, 30);

    if (avgTOA_1!=0 && avgTOA_2!=0)
      _correlationTOA[_ID]->Fill(avgTOA_1, avgTOA_2);

    //sum of HG in TS3 - TS0
    const unsigned int sumLG_TS3_1 = plane1.GetPixel(0, 31);
    const unsigned int sumLG_TS3_2 = plane2.GetPixel(0, 31);

    if (sumLG_TS3_1>20 && sumLG_TS3_2>20)
      _correlationSignalLGSum[_ID]->Fill(sumLG_TS3_1, sumLG_TS3_2);
  }

}

void HexagonCorrelationHistos::Reset() {

  for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {
    if (_correlationSignalLGSum.find(_ID)==_correlationSignalLGSum.end()) continue;
    _correlationSignalLGSum[_ID]->Reset();
    _correlationTOA[_ID]->Reset();
  }
}

void HexagonCorrelationHistos::Calculate(const int currentEventNum) {
  _wait = false;
}

void HexagonCorrelationHistos::Write() {

  for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {
    if (_correlationSignalLGSum.find(_ID)==_correlationSignalLGSum.end()) continue;
    _correlationSignalLGSum[_ID]->Write();
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
