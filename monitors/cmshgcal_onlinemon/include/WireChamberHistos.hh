//Thorben Quast, thorben.quast@cern.ch
//June 2017

// -*- mode: c -*-

#ifndef WIRECHAMBERHISTOS_HH_
#define WIRECHAMBERHISTOS_HH_

#include <TH2F.h>
#include <TH2I.h>
#include <TH1F.h>
#include <TH2Poly.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class WireChamberHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  TH1F *_recoX;
  TH1F *_recoY;
  TH1I *_goodAll;
  TH2F *_XYmap;
    
public:
  WireChamberHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void Fill(const eudaq::StandardPlane &plane);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH1I *getGoodAllHisto() { return _goodAll; }
  TH1F *getRecoXHisto() { return _recoX; }
  TH1F *getRecoYHisto() { return _recoY; }
  TH2F *getXYmapHisto() { return _XYmap; }

  
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int SetHistoAxisLabelx(TH1 *histo, string xlabel);
  int SetHistoAxisLabely(TH1 *histo, string ylabel);
  int SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
                       // single event
  
  RootMonitor *_mon;

  bool is_WIRECHAMBER;
};


#ifdef __CINT__
#pragma link C++ class WireChamberHistos - ;
#endif

#endif /* WIRECHAMBERHISTOS_HH_ */
