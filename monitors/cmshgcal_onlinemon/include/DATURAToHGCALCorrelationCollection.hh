// -*- mode: c -*-

//author: Thorben Quast, thorben.quast@cern.ch
//26 March 2018

#ifndef DATURATOHGCAL_COLLECTION_
#define DATURATOHGCAL_COLLECTION_
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
#include "DATURAToHGCALCorrelationHistos.hh"

class DATURAToHGCALCorrelationCollection : public BaseCollection {
  RQ_OBJECT("DATURAToHGCALCorrelationCollection")
protected:
  bool isOnePlaneRegistered;
  std::map<eudaq::StandardPlane, DATURAToHGCALCorrelationHistos *> _map;
  bool isPlaneRegistered(eudaq::StandardPlane p);
  void fillHistograms(const eudaq::StandardPlane &plane, const eudaq::StandardPlane &plMIMOSA3, const eudaq::StandardPlane &plMIMOSA4);
    
public:
  void registerPlane(const eudaq::StandardPlane &p);
  void bookHistograms(const eudaq::StandardEvent &ev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  DATURAToHGCALCorrelationCollection() : BaseCollection() {
    std::cout << " Initialising DATURAToHGCALCorrelationCollection Collection" << std::endl;
    isOnePlaneRegistered = false;
    CollectionType = DATURATOHGCAL_COLLECTION_TYPE;

  }
  void Fill(const SimpleStandardEvent &simpev) { ; };
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  DATURAToHGCALCorrelationHistos *getDATURAToHGCALCorrelationHistos(std::string sensor, int id);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);

};




#ifdef __CINT__
#pragma link C++ class DATURAToHGCALCorrelationCollection - ;
#endif

#endif /* DATURATOHGCAL_COLLECTION_ */
