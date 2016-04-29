/*
 * ParaMonitorHistos.hh
 *
 *  Created on: Jul 5, 2011
 *      Author: stanitz
 */

#ifndef PARAMONITORHISTOS_HH_
#define PARAMONITORHISTOS_HH_

#include <TH1I.h>
#include <TFile.h>

#include <map>
#include "SimpleStandardEvent.hh"

using namespace std;
class RootMonitor;

class ParaMonitorHistos {
protected:
  TH1I *m_TempHisto;

public:
  ParaMonitorHistos();
  virtual ~ParaMonitorHistos();
  void Fill(SimpleStandardEvent ev);
  void Write();
  void Reset();
  TH1I *getTempHisto() { return m_TempHisto; }

};

#endif
