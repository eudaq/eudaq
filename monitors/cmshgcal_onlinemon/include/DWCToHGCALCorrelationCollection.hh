// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#ifndef DWCTOHGCAL_COLLECTION_
#define DWCTOHGCAL_COLLECTION_
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
#include "BaseCollection.hh"
#include "DWCToHGCALCorrelationHistos.hh"

class DWCToHGCALCorrelationCollection : public BaseCollection {
  RQ_OBJECT("DWCToHGCALCorrelationCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, DWCToHGCALCorrelationHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plDWC0);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  DWCToHGCALCorrelationCollection() : BaseCollection() {
    std::cout << " Initialising DWCToHGCALCorrelationCollection Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = DWCTOHGCAL_COLLECTION_TYPE;

  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  DWCToHGCALCorrelationHistos *getDWCToHGCALCorrelationHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);

};




#ifdef __CINT__
#pragma link C++ class DWCToHGCALCorrelationCollection - ;
#endif

#endif /* DWCTOHGCAL_COLLECTION_ */
