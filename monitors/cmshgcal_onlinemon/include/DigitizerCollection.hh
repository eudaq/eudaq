//Thorben Quast, thorben.quast@cern.ch
//August 2017

// -*- mode: c -*-

#ifndef DIGITIZERCOLLECTION_HH_
#define DIGITIZERCOLLECTION_HH_
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
#include "DigitizerHistos.hh"
#include "BaseCollection.hh"

class DigitizerCollection : public BaseCollection {
  RQ_OBJECT("DigitizerCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, DigitizerHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &plane, int eventNumber);

public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  DigitizerCollection() : BaseCollection() {
    std::cout << " Initialising Digitizer Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = DIGITIZER_COLLECTION_TYPE;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  DigitizerHistos *getDigitizerHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class DigitizerCollection - ;
#endif

#endif /* DIGITIZERCOLLECTION_HH_ */
