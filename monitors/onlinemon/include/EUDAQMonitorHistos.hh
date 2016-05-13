/*
 * EUDAQMonitorHistos.hh
 *
 *  Created on: Sep 27, 2011
 *      Author: stanitz
 */

#ifndef EUDAQMONITORHISTOS_HH_
#define EUDAQMONITORHISTOS_HH_

#include "TH1F.h"
#include "TH2I.h"
#include "TProfile.h"
#include "TFile.h"
#include "TGraph.h"

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "SimpleStandardEvent.hh"

using namespace std;

class RootMonitor;

class EUDAQMonitorHistos {
protected:
  TProfile *Hits_vs_EventsTotal;
  TProfile **Hits_vs_Events;
  TProfile *Hits_vs_PlaneHisto;
  TH1F *Planes_perEventHisto;
  TProfile **TLUdelta_perEventHisto;
  //    TH2I * TracksPerEvent;
  TProfile *TracksPerEvent;
  TGraph *m_EventN_vs_TimeStamp;

  std::mutex mu;
public:
  EUDAQMonitorHistos(const SimpleStandardEvent &ev);
  virtual ~EUDAQMonitorHistos();
  void Fill(const SimpleStandardEvent &ev);
  void Fill(const unsigned int evt_nr,
            const unsigned int tracks); // only for tracks per event histogram
  void Write();
  void Reset();
  TProfile *getHits_vs_Events(unsigned int i) const;
  TProfile *getHits_vs_EventsTotal() const;
  TProfile *getHits_vs_PlaneHisto() const;
  TH1F *getPlanes_perEventHisto() const;
  TProfile *getTLUdelta_perEventHisto(unsigned int i) const;
  //    TH2I *getTracksPerEventHisto() const;
  TProfile *getTracksPerEventHisto() const;

  void setPlanes_perEventHisto(TH1F *Planes_perEventHisto);
  unsigned int getNplanes() const;
  TNamed *getEventN_vs_TimeStamp() const;
  std::mutex* getMutexEventN_vs_TimeStamp(){return &mu;};

private:
  unsigned int nplanes;
};

#ifdef __CINT__
#pragma link C++ class EUDAQMonitorHistos - ;
#endif

#endif /* EUDAQMONITORHISTOS_HH_ */
