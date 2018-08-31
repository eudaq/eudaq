//Thorben Quast, thorben.quast@cern.ch
//August 2018

// -*- mode: c -*-

#ifndef DIGITIZERHISTOS_HH_
#define DIGITIZERHISTOS_HH_

#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class DigitizerHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  std::map<int, TH1F*> lastWaveform;
  int lastShownEvent;
  std::map<int, TH2F*> IntegratedWaveform;

    
public:
  DigitizerHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void Fill(const eudaq::StandardPlane &plane, int eventNumber);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH1F *getLastWaveform(int key) { if(lastWaveform.find(key)!=lastWaveform.end()) return lastWaveform.at(key); else {std::cout<<"Last Waveform with key "<<key<<" is not in the graphics map-->debug"<<std::endl; return NULL;}}
  TH2F *getIntegratedWaveform(int key) { if(IntegratedWaveform.find(key)!=IntegratedWaveform.end()) return IntegratedWaveform.at(key); else {std::cout<<"Integrated Waveform with key "<<key<<" is not in the graphics map-->debug"<<std::endl; return NULL;}}

  
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int SetHistoAxisLabelx(TH2 *histo, string xlabel);
  int SetHistoAxisLabely(TH2 *histo, string ylabel);
  int SetHistoAxisLabels(TH2 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
                       // single event
  
  RootMonitor *_mon;

};


#ifdef __CINT__
#pragma link C++ class DigitizerHistos - ;
#endif

#endif /* DIGITIZERHISTOS_HH_ */
