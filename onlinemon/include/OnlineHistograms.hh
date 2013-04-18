#ifndef ONLINE_HISTOGRAMS_H
#define ONLINE_HISTOGRAMS_H

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

#include <TH2I.h>
#include <TFile.h>

#include "HitmapHistos.hh"
#include "CorrelationHistos.hh"
#include "BaseCollection.hh"
#include "HitmapCollection.hh"
#include "CorrelationCollection.hh"
#include "MonitorPerformanceCollection.hh"


#include "SimpleStandardEvent.hh"


#include <RQ_OBJECT.h>


#ifndef __CINT__
//#include "OnlineMon.hh"
#endif


using namespace std;

class RootMonitor;




struct Plane {
  std::string sensor;
  int id;

};

inline bool operator==(Plane const &a, Plane const &b) {
  if (a.sensor==b.sensor && a.id == b.id) {
    return true;
  } else {
    return false;
  }
}

inline bool operator<(Plane const &a, Plane const &b) { // Needed to use struct in a map
  return a.sensor<b.sensor || ( a.sensor==b.sensor && a.id < b.id); 
}

class OnlineHistograms {
  protected:
    std::vector<std::string> _sensorVec;


  public:
    OnlineHistograms() {}
    void addSensor(const std::string & name, const int id) {}
    void FillHistos() {}



};

#ifdef __CINT__
#pragma link C++ class OnlineHistograms-;
#pragma link C++ class CorrelationCollection-;
#endif

#endif
