/*
 * ParaMonitorCollection.hh
 *
 *  Created on: Jul 6, 2011
 *      Author: stanitz
 */

#ifndef PARAMONITORCOLLECTION_HH_
#define PARAMONITORCOLLECTION_HH_
#include <TH2I.h>
#include <TFile.h>

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

// project includes
#include "SimpleStandardEvent.hh"
#include "ParaMonitorHistos.hh"
#include "BaseCollection.hh"

using namespace std;

class ParaMonitorCollection : public BaseCollection {
protected:
  void fillHistograms(const SimpleStandardEvent &ev);
  bool histos_init;

public:
  ParaMonitorCollection();
  virtual ~ParaMonitorCollection();
  void Reset();
  virtual void Write(TFile *file);
  void Calculate(const unsigned int currentEventNumber);
  void bookHistograms(const SimpleStandardEvent &simpev);
  void setRootMonitor(RootMonitor *mon) { _mon = mon; }
  void Fill(const SimpleStandardEvent &simpev);
  ParaMonitorHistos* getParaMonitorHistos();

private:
  ParaMonitorHistos *histos;
};

#endif 
