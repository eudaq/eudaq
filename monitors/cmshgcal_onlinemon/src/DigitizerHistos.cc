// -*- mode: c -*-

#include <TROOT.h>
#include "DigitizerHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include <cstdlib>
#include <sstream>
#include <cmath>

DigitizerHistos::DigitizerHistos(eudaq::StandardPlane plane, RootMonitor *mon)
  : _sensor(plane.Sensor()), _id(plane.ID()), _maxX(plane.XSize()),  _maxY(plane.YSize()), _wait(false) {

  char out[1024], out2[1024];

  _mon = mon;


  std::vector<double> pxl = plane.GetPixels<double>();
  int Npixels = pxl.size();

  for (size_t ipix = 0; ipix < Npixels; ipix++) {
    unsigned digitizer_group = plane.GetX(ipix);
    unsigned digitizer_channel = plane.GetY(ipix);  //the y-coordinate of the pixels corresponds to the tdc channels 0-15
    unsigned n_samples = plane.GetPixel(ipix, 0);  //frames correspond to the number of hits in a readout cycle. They are limited to max. 1+20 with the first one being the integer value of number of hits.

    int key = digitizer_group * 100 + digitizer_channel;

    sprintf(out, "%s %i integrated waveform, group %i, channel %i", _sensor.c_str(), _id, digitizer_group, digitizer_channel);
    sprintf(out2, "%s_%i_integrated_waveform_group_%i_channel_%i", _sensor.c_str(), _id, digitizer_group, digitizer_channel);
    IntegratedWaveform[key] = new TH2F(out2, out, n_samples, 0.5, n_samples + 0.5, 100., 0., 1.);
    IntegratedWaveform[key]->SetStats(false);
    SetHistoAxisLabels(IntegratedWaveform[key], "sample index", "normalised ADC");

    sprintf(out, "%s %i last waveform, group %i, channel %i", _sensor.c_str(), _id, digitizer_group, digitizer_channel);
    sprintf(out2, "%s_%i_last_waveform_group_%i_channel_%i", _sensor.c_str(), _id, digitizer_group, digitizer_channel);
    lastWaveform[key] = new TH1F(out2, out, n_samples, 0.5, n_samples + 0.5);
    lastWaveform[key]->SetStats(false);
    lastWaveform[key]->Sumw2();
    lastWaveform[key]->GetYaxis()->SetTitle("uncorrected ADC");
  }
  lastShownEvent = -1;



  //helper objects
}

void DigitizerHistos::Fill(const eudaq::StandardPlane &plane, int eventNumber) {

  std::vector<double> pxl = plane.GetPixels<double>();
  int Npixels = pxl.size();
  float min, max, sampleValue;
  int N_samples = -1;
  int key;
  unsigned digitizer_group, digitizer_channel, n_samples;

  for (size_t ipix = 0; ipix < Npixels; ipix++) {
    digitizer_group = plane.GetX(ipix);
    digitizer_channel = plane.GetY(ipix);  //the y-coordinate of the pixels corresponds to the tdc channels 0-15
    n_samples = plane.GetPixel(ipix, 0);  //frames correspond to the number of hits in a readout cycle. They are limited to max. 1+20 with the first one being the integer value of number of hits.

    //book keeping for the individual graphs
    if (n_samples > N_samples) N_samples = n_samples;

    key = digitizer_group * 100 + digitizer_channel;

    min = max = plane.GetPixel(ipix, 1);
    for (int sample = 2; sample <= n_samples; sample++) {
      sampleValue = plane.GetPixel(ipix, sample);
      if (sampleValue < min) min = sampleValue;
      if (sampleValue > max) max = sampleValue;
    }

    //actual filling
    for (int sample = 1; sample <= n_samples; sample++) {
      sampleValue = plane.GetPixel(ipix, sample);
      IntegratedWaveform[key]->Fill(sample, (1. / (max - min)) * (sampleValue - min));
    }
  }


  if ((lastShownEvent == -1) || (eventNumber - lastShownEvent > 100))  { //refresh the explicit graphs every hunderd events
    for (std::map<int, TH1F*>::iterator it2 = lastWaveform.begin(); it2 != lastWaveform.end(); it2++ )     {
      it2->second->Reset();
      char out[1024];
      sprintf(out, "Event: %i", eventNumber);

      it2->second->GetXaxis()->SetTitle(("sample index, " + std::string(out)).c_str());
    }


    for (size_t ipix = 0; ipix < Npixels; ipix++) {
      digitizer_group = plane.GetX(ipix);
      digitizer_channel = plane.GetY(ipix);  //the y-coordinate of the pixels corresponds to the tdc channels 0-15

      key = digitizer_group * 100 + digitizer_channel;


      //actual filling
      for (int sample = 1; sample <= n_samples; sample++) {
        sampleValue = plane.GetPixel(ipix, sample);
        lastWaveform[key]->Fill(sample, sampleValue);
      }
    }

    lastShownEvent = eventNumber;
  }

}

void DigitizerHistos::Reset() {
  std::map<int, TH2F*>::iterator it1;
  for ( it1 = IntegratedWaveform.begin(); it1 != IntegratedWaveform.end(); it1++ ) it1->second->Reset();
  std::map<int, TH1F*>::iterator it2;
  for ( it2 = lastWaveform.begin(); it2 != lastWaveform.end(); it2++ ) it2->second->Reset();
  lastShownEvent = -1;
}

void DigitizerHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void DigitizerHistos::Write() {
  std::map<int, TH2F*>::iterator it1;
  for ( it1 = IntegratedWaveform.begin(); it1 != IntegratedWaveform.end(); it1++ ) it1->second->Write();
  std::map<int, TH1F*>::iterator it2;
  for ( it2 = lastWaveform.begin(); it2 != lastWaveform.end(); it2++ ) it2->second->Write();
}

int DigitizerHistos::SetHistoAxisLabelx(TH2 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int DigitizerHistos::SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int DigitizerHistos::SetHistoAxisLabely(TH2 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}
