// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#ifndef DWCTOHGCALCORRELATIONHISTOS_H
#define DWCTOHGCALCORRELATIONHISTOS_H

#include <TH2F.h>
#include <TFile.h>
#include "HexagonHistos.hh"   //for the sc map

#include <map>
#include <cstdlib>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;



class DWCToHGCALCorrelationHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  map < pair < int,int >, int > _ski_to_ch_map; //channel mapping, just a placeholder, not implemented
  
  std::map<int, TH2F *> _DWC_map_ForChannel;
  TH2I* Occupancy_ForChannel;
  
public:
  DWCToHGCALCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon);


  void Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plDWC0);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH2F *getDWC_map_ForChannel(int channel) { return _DWC_map_ForChannel[channel]; }
  TH2I *getOccupancy_ForChannel() { return Occupancy_ForChannel; }

  
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
#pragma link C++ class DWCToHGCALCorrelationHistos - ;
#endif

#endif /* CROSSCORRELATIONHISTOS_HH_ */
