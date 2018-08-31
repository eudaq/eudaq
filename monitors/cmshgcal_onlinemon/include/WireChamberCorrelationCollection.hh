//Thorben Quast, thorben.quast@cern.ch
//June 2018

// -*- mode: c -*-

#ifndef WIRECHAMBERCORRELATIONCOLLECTION_HH_
#define WIRECHAMBERCORRELATIONCOLLECTION_HH_
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
#include "WireChamberCorrelationHistos.hh"
#include "BaseCollection.hh"

class WireChamberCorrelationCollection : public BaseCollection {
  RQ_OBJECT("WireChamberCorrelationCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, WireChamberCorrelationHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2);
  int NumberOfDWCPlanes;

public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  WireChamberCorrelationCollection() : BaseCollection() {
    std::cout << " Initialising WireChamberCorrelation Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = WIRECHAMBER_CORRELATION_COLLECTION_TYPE;
    NumberOfDWCPlanes=-1;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  WireChamberCorrelationHistos *getWireChamberCorrelationHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class WireChamberCorrelationCollection - ;
#endif

#endif /* WIRECHAMBERCORRELATIONCOLLECTION_HH_ */
