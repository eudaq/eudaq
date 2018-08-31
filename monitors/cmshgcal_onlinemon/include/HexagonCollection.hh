// -*- mode: c -*-

#ifndef HEXAGONCOLLECTION_HH_
#define HEXAGONCOLLECTION_HH_
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
#include "HexagonHistos.hh"
#include "BaseCollection.hh"

class HexagonCollection : public BaseCollection {
  RQ_OBJECT("HexagonCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, HexagonHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane, int evNumber=-1);
  int _runMode;
  
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  HexagonCollection() : BaseCollection() {
    std::cout << " Initialising Hexagon Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = HEXAGON_COLLECTION_TYPE;
  }
  void SetRunMode(int mo) { _runMode = mo; }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  HexagonHistos *getHexagonHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class HexagonCollection - ;
#endif

#endif /* HEXAGONCOLLECTION_HH_ */
