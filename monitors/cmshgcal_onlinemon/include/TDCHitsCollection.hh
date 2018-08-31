//Thorben Quast, thorben.quast@cern.ch
//June 2017

// -*- mode: c -*-

#ifndef TDCHITSCOLLECTION_HH_
#define TDCHITSCOLLECTION_HH_
// ROOT Includes
#include <RQ_OBJECT.h>
#include <TH2I.h>
#include <TFile.h>

// STL includes
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

#include "eudaq/StandardEvent.hh"

// Project Includes
#include "SimpleStandardEvent.hh"
#include "TDCHitsHistos.hh"
#include "BaseCollection.hh"

class TDCHitsCollection : public BaseCollection {
  RQ_OBJECT("TDCHitsCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, TDCHitsHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &plane, int evNumber);
  int NumberOfDWCPlanes;  

public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  TDCHitsCollection() : BaseCollection() {
    std::cout << " Initialising TDC Hits Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = TDCHITS_COLLECTION_TYPE;
    NumberOfDWCPlanes = -1;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  TDCHitsHistos *getTDCHitsHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class TDCHitsCollection - ;
#endif

#endif /* TDCHITSCOLLECTION_HH_ */
