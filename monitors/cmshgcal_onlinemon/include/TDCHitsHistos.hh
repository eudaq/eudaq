//Thorben Quast, thorben.quast@cern.ch
//July 2018

// -*- mode: c -*-

#ifndef TDCHITSHISTOS_HH_
#define TDCHITSHISTOS_HH_

#include <TH2F.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class TDCHitsHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  int NTDCs;
  int NChannelsPerTDC;

  TH2F *_hitOccupancy;
  TH2F *_hitProbability;
  
  TH2F *_occupancy;
  TH2F *_hitSumCount;
  TH2F *_hitCount;

  int lastEventForRefresh;

    
public:
  TDCHitsHistos(eudaq::StandardPlane p, RootMonitor *mon, int _NDWCs);

  void Fill(const eudaq::StandardPlane &plane, int eventNumber);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH2F *getHitOccupancy() { return _hitOccupancy; }
  TH2F *getHitProbability() { return _hitProbability; }

  
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
#pragma link C++ class TDCHitsHistos - ;
#endif

#endif /* TDCHITSHISTOS_HH_ */
