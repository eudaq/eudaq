
#ifndef PARAMONITORCOLLECTION_HH_
#define PARAMONITORCOLLECTION_HH_
#include <TH2I.h>
#include <TFile.h>
#include <TGraph.h>

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

// project includes
#include "SimpleStandardEvent.hh"
#include "BaseCollection.hh"

#include <mutex>

using namespace std;

class ParaMonitorCollection : public BaseCollection {
protected:
  void fillHistograms(const SimpleStandardEvent &ev);
  bool histos_init;

  std::map<std::string, TGraph*> m_graphMap; 
  std::mutex m_mu;
public:
  ParaMonitorCollection();
  virtual ~ParaMonitorCollection();
  void Reset();
  virtual void Write(TFile *file);
  void Calculate(const unsigned int currentEventNumber);
  void bookHistograms(const SimpleStandardEvent &simpev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  void Fill(const SimpleStandardEvent &simpev);
  void InitPlots(const SimpleStandardEvent &simpev);
private:

};

#endif 
