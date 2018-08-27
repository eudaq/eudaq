// -*- mode: c -*-

#ifndef AHCALHISTOS_HH_
#define AHCALHISTOS_HH_

#include <TH2I.h>
#include <TH2Poly.h>
#include <TFile.h>

#include <map>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;

class AhcalHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  TH2I *_hitmap;
  TH1I *_hitXmap;
  TH1I *_hitYmap;

  TH2D *_HotPixelMap;
  //TH1F *_hitOcc;
  TH1I *_nHits;
  TH1I *_nbadHits;
  TH1I *_nHotPixels;

  
public:
  AhcalHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void Fill(const eudaq::StandardPlane &plane);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH2I *getHitmapHisto() { return _hitmap; }
  TH1I *getHitXmapHisto() { return _hitXmap; }
  TH1I *getHitYmapHisto() { return _hitYmap; }
  TH2D *getHotPixelMapHisto() { return _HotPixelMap; }

  //TH1F *getHitOccHisto() {
  //if (_wait)
  //  return NULL;
  //else
  //  return _hitOcc;
  //}
  TH1I *getNHitsHisto() { return _nHits; }
  TH1I *getNbadHitsHisto() { return _nbadHits; }

  
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

  bool is_AHCAL;
};


#ifdef __CINT__
#pragma link C++ class AhcalHistos - ;
#endif

#endif /* AHCALHISTOS_HH_ */
