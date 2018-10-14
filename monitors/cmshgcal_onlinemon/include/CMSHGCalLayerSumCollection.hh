// -*- mode: c -*-

#ifndef CMSHGCAL_LAYERSUM_COLLECTION_HH_
#define CMSHGCAL_LAYERSUM_COLLECTION_HH_
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
#include "CMSHGCalLayerSumHistos.hh"
//#include "NoisyChannel.hh"
#include "BaseCollection.hh"

using namespace std;

class CMSHGCalLayerSumCollection : public BaseCollection {
  RQ_OBJECT("CMSHGCalLayerSumCollection")

public:
  CMSHGCalLayerSumCollection() : BaseCollection() {
    mymonhistos = NULL;
    histos_init = false; 
    std::cout << " Initialising CMSHGCalLayerSum Collection" << std::endl;
    CollectionType = CMSHGCAL_LAYERSUM_COLLECTION_TYPE;
  }

  //virtual ~CMSHGCalLayerSumCollection();
  
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  void Fill(const SimpleStandardEvent &simpev){};
  void Fill(const eudaq::StandardEvent &ev, int evNumber=-1);
  //void Fill(const SimpleStandardEvent &simpev, int evNumber=-1);
  void Reset();
  virtual void Write(TFile *file);
  virtual void Calculate(const unsigned int currentEventNumber);
  //void bookHistograms(const SimpleStandardEvent &simpev);
  void bookHistograms(const eudaq::StandardEvent &ev);
  CMSHGCalLayerSumHistos *getCMSHGCalLayerSumHistos();
  //void setReduce(const unsigned int red);
  unsigned int getCollectionType();
  
protected:
  void fillHistograms(const SimpleStandardEvent &ev);
  bool histos_init;
  CMSHGCalLayerSumHistos *mymonhistos;
};

#ifdef __CINT__
#pragma link C++ class CMSHGCalLayerSumCollection - ;
#endif

#endif /* CMSHGCAL_LAYERSUM_COLLECTION_HH_ */
