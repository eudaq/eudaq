// -*- mode: c -*-

#ifndef AHCALCOLLECTION_HH_
#define AHCALCOLLECTION_HH_
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
#include "AhcalHistos.hh"
#include "BaseCollection.hh"

class AhcalCollection : public BaseCollection {
  RQ_OBJECT("AhcalCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, AhcalHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  AhcalCollection() : BaseCollection() {
    std::cout << " Initialising Ahcal Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = AHCAL_COLLECTION_TYPE;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  AhcalHistos *getAhcalHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class AhcalCollection - ;
#endif

#endif /* AHCALCOLLECTION_HH_ */
