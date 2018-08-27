//Thorben Quast, thorben.quast@cern.ch
//June 2017

// -*- mode: c -*-

#ifndef WIRECHAMBERCOLLECTION_HH_
#define WIRECHAMBERCOLLECTION_HH_
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
#include "WireChamberHistos.hh"
#include "BaseCollection.hh"

class WireChamberCollection : public BaseCollection {
  RQ_OBJECT("WireChamberCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, WireChamberHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  WireChamberCollection() : BaseCollection() {
    std::cout << " Initialising WireChamber Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = WIRECHAMBER_COLLECTION_TYPE;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  WireChamberHistos *getWireChamberHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class WireChamberCollection - ;
#endif

#endif /* WIRECHAMBERCOLLECTION_HH_ */
