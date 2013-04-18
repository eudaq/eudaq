/*
 * EUDAQMonitorCollection.h
 *
 *  Created on: Sep 27, 2011
 *      Author: stanitz
 */

#ifndef EUDAQMONITORCOLLECTION_HH_
#define EUDAQMONITORCOLLECTION_HH_
//ROOT Includes
#include <RQ_OBJECT.h>
#include <TH2I.h>
#include <TFile.h>

//STL includes
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

//project includes
#include "SimpleStandardEvent.hh"
#include "EUDAQMonitorHistos.hh"
#include "BaseCollection.hh"

using namespace std;




class EUDAQMonitorCollection: public BaseCollection
{
  RQ_OBJECT("EUDAQMonitorCollection")
  protected:
    void fillHistograms(const SimpleStandardEvent &ev);
    bool histos_init;
  public:
    EUDAQMonitorCollection();
    virtual ~EUDAQMonitorCollection();
    void Reset();
    virtual void Write(TFile *file);
    void Calculate(const unsigned int currentEventNumber);
    void bookHistograms(const SimpleStandardEvent &simpev);
    void setRootMonitor(RootMonitor *mon)  {_mon = mon; }
    void Fill(const SimpleStandardEvent &simpev);
    EUDAQMonitorHistos * getEUDAQMonitorHistos();
  private:
    EUDAQMonitorHistos * mymonhistos;

};

#ifdef __CINT__
#pragma link C++ class EUDAQMonitorCollection-;
#endif

#endif /* EUDAQMONITORCOLLECTION_HH_ */
