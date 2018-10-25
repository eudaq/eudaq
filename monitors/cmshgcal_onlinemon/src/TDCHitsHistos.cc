// -*- mode: c -*-

#include <TROOT.h>
#include "TDCHitsHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>
#include <cmath>

TDCHitsHistos::TDCHitsHistos(eudaq::StandardPlane p, RootMonitor *mon, int _NDWCs)
  : _sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), _wait(false),
    _hitOccupancy(NULL), _hitProbability(NULL), _occupancy(NULL), _hitCount(NULL), _hitSumCount(NULL) {

  lastEventForRefresh = -1;

  char out[1024], out2[1024];

  _mon = mon;

  // std::cout << "TDCHitsHistos::Sensorname: " << _sensor << " "<< _id<<
  // std::endl;

  NChannelsPerTDC = 16;
  NTDCs = ceil(_NDWCs * 4. / NChannelsPerTDC);

  std::cout << "Number of TDCS: " << NTDCs << std::endl;
  sprintf(out, "%s %i hit occupancy", _sensor.c_str(), _id);
  sprintf(out2, "hitOccupancy_%s_%i", _sensor.c_str(), _id);
  _hitOccupancy = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitOccupancy, "channel", "TDC");
  _hitSumCount = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitSumCount, "channel", "TDC");
  //_hitOccupancy = _hitSumCount/_occupancy

  sprintf(out, "%s %i at least one hit probability", _sensor.c_str(), _id);
  sprintf(out2, "hitProbability_%s_%i", _sensor.c_str(), _id);
  _hitProbability = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitProbability, "channel", "TDC");
  _hitCount = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_hitCount, "channel", "TDC");
  //_hitProbability = _hitCount/_occupancy


  sprintf(out, "%s %i _occupancy", _sensor.c_str(), _id);
  sprintf(out2, "_occupancy_%s_%i", _sensor.c_str(), _id);
  _occupancy = new TH2F(out2, out, NChannelsPerTDC, 0, NChannelsPerTDC, NTDCs, 0, NTDCs);
  SetHistoAxisLabels(_occupancy, "channel", "TDC");

  //helper objects
}

void TDCHitsHistos::Fill(const eudaq::StandardPlane &plane, int eventNumber) {
  // std::cout<< "FILL with a plane." << std::endl;

  for (unsigned tdc_index = 0; tdc_index < NTDCs; tdc_index++)for (int channel = 0; channel < NChannelsPerTDC; channel++) {
      _hitSumCount->Fill(1.*channel, 1.*tdc_index, 1.*plane.GetPixel(tdc_index * 16 + channel, 0));
      _hitCount->Fill(1.*channel, 1.*tdc_index, int(plane.GetPixel(tdc_index * 16 + channel, 0) > 0));
      _occupancy->Fill(1.*channel, 1.*tdc_index);
    }

  if (!((lastEventForRefresh == -1) || (eventNumber - lastEventForRefresh > 100))) return;

  //otherwise, update the plots:

  _hitSumCount->Copy(*_hitOccupancy);
  _hitOccupancy->Divide(_occupancy);

  _hitCount->Copy(*_hitProbability);
  _hitProbability->Divide(_occupancy);

  lastEventForRefresh = eventNumber;
}

void TDCHitsHistos::Reset() {
  if (_hitOccupancy == NULL) return;
  _hitOccupancy->Reset();
  if (_hitProbability == NULL) return;
  _hitProbability->Reset();
 if (_occupancy == NULL) return;
  _occupancy->Reset();
 if (_hitSumCount == NULL) return;
  _hitSumCount->Reset();
 if (_hitCount == NULL) return;
  _hitCount->Reset();
  lastEventForRefresh = -1;
}

void TDCHitsHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void TDCHitsHistos::Write() {

  _hitOccupancy->Write();
  _hitProbability->Write();;
  _occupancy->Write();
  _hitSumCount->Write();
  _hitCount->Write();

}

int TDCHitsHistos::SetHistoAxisLabelx(TH2 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int TDCHitsHistos::SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int TDCHitsHistos::SetHistoAxisLabely(TH2 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
