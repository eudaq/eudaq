// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#ifndef DATURATOHGCALCORRELATIONHISTOS_H
#define DATURATOHGCALCORRELATIONHISTOS_H

#include <TH2F.h>
#include <TFile.h>
#include "HexagonHistos.hh"   //for the sc map

#include <map>
#include <cstdlib>

#include "eudaq/StandardEvent.hh"

using namespace std;

class RootMonitor;



class DATURAToHGCALCorrelationHistos {
protected:
  string _sensor;
  int _id;
  int _maxX;
  int _maxY;
  bool _wait;

  map < pair < int,int >, int > _ski_to_ch_map; //channel mapping
  
  std::map<int, TH2F *> _MIMOSA_map_ForChannel;
  TH2I* Occupancy_ForChannel;
  
public:
  DATURAToHGCALCorrelationHistos(eudaq::StandardPlane p, RootMonitor *mon);

  void findClusters(std::vector<std::pair<int, int> >& entities, std::vector<std::pair<float, float> >& clusters );

  void Fill(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA3, const eudaq::StandardPlane &plMIMOSA4);
  void Reset();

  void Calculate(const int currentEventNum);
  void Write();

  TH2F *getMIMOSA_map_ForChannel(int channel) { return _MIMOSA_map_ForChannel[channel]; }
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


  //implements the electronic map of layer one from July 2017 (H2)
  // SKIROC CHANNEL  IX IV
  std::vector<std::pair<int, int> > clusterEntitites;
  std::vector<std::pair<float, float> > clusters2;  
  std::vector<bool > good_cluster2;  
  std::vector<std::pair<float, float> > clusters3;  
};


#ifdef __CINT__
#pragma link C++ class DATURAToHGCALCorrelationHistos - ;
#endif

#endif /* CROSSCORRELATIONHISTOS_HH_ */
