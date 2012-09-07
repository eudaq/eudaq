/*
 * MonitorPerformanceCollection.hh
 *
 *  Created on: Jul 6, 2011
 *      Author: stanitz
 */

#ifndef MONITORPERFORMANCECOLLECTION_HH_
#define MONITORPERFORMANCECOLLECTION_HH_
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
#include "MonitorPerformanceHistos.hh"
#include "BaseCollection.hh"

using namespace std;

class MonitorPerformanceCollection: public BaseCollection
{
	RQ_OBJECT("MonitorPerformanceCollection")
protected:

	void fillHistograms(const SimpleStandardEvent &ev);
	bool histos_init;
public:
	MonitorPerformanceCollection();
	virtual ~MonitorPerformanceCollection();
	void Reset();
	virtual void Write(TFile *file);
	void Calculate(const unsigned int currentEventNumber);
	void bookHistograms(const SimpleStandardEvent &simpev);
	void setRootMonitor(RootMonitor *mon)  {_mon = mon; }
	void Fill(const SimpleStandardEvent &simpev);
	MonitorPerformanceHistos * getMonitorPerformanceHistos();
private:
	MonitorPerformanceHistos * mymonhistos;
};

#ifdef __CINT__
#pragma link C++ class MonitorPerformanceCollection-;
#endif

#endif /* MONITORPERFORMANCECOLLECTION_HH_ */
