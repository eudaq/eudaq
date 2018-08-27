//Thorben Quast, thorben.quast@cern.ch
//June 2018

// -*- mode: c -*-

#ifndef HEXAGONCORRELATIONCOLLECTION_HH_
#define HEXAGONCORRELATIONCOLLECTION_HH_
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
#include "HexagonCorrelationHistos.hh"
#include "BaseCollection.hh"


#define NHEXAGONS_PER_SENSOR 8

class HexagonCorrelationCollection : public BaseCollection {
  RQ_OBJECT("HexagonCorrelationCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, HexagonCorrelationHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane1, const eudaq::StandardPlane &plane2);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  HexagonCorrelationCollection() : BaseCollection() {
    std::cout << " Initialising HexagonCorrelation Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = HEXAGON_CORRELATION_COLLECTION_TYPE;
  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  HexagonCorrelationHistos *getHexagonCorrelationHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
};

#ifdef __CINT__
#pragma link C++ class HexagonCorrelationCollection - ;
#endif

#endif /* HEXAGONCORRELATIONCOLLECTION_HH_ */
