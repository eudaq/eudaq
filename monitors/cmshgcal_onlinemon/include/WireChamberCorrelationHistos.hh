//Thorben Quast, thorben.quast@cern.ch
//June 2018

// -*- mode: c -*-

#ifndef WIRECHAMBERCORRELATIONHISTOS_HH_
#define WIRECHAMBERCORRELATIONHISTOS_HH_

#include <TH2F.h>
#include <TH2I.h>
#include <TH1F.h>
#include <TH2Poly.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class WireChamberCorrelationHistos {
protected:
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;
  
  std::map<int, TH2F *> _correlationXX;
  std::map<int, TH2F *> _correlationXY;
  std::map<int, TH2F *> _correlationYY;
  std::map<int, TH2F *> _correlationYX;
  
public:
  WireChamberCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void Fill(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();
  
  TH2F *getCorrelationXX(int ref_planeIndex) { return _correlationXX[ref_planeIndex]; }
  TH2F *getCorrelationXY(int ref_planeIndex) { return _correlationXY[ref_planeIndex]; }
  TH2F *getCorrelationYY(int ref_planeIndex) { return _correlationYY[ref_planeIndex]; }
  TH2F *getCorrelationYX(int ref_planeIndex) { return _correlationYX[ref_planeIndex]; }

  void setRootMonitor(RootMonitor *mon) { _mon = mon; }

private:
  int **plane_map_array; // store an array representing the map
  int zero_plane_array(); // fill array with zeros;
  int SetHistoAxisLabelx(TH1 *histo, string xlabel);
  int SetHistoAxisLabely(TH1 *histo, string ylabel);
  int SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel);
  int filling_counter; // we don't need occupancy to be refreshed for every
                       // single event
  
  RootMonitor *_mon;
};


#ifdef __CINT__
#pragma link C++ class WireChamberCorrelationHistos - ;
#endif

#endif /* WIRECHAMBERCORRELATIONHISTOS_HH_ */
