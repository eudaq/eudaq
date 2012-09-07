/*
 * MonitorPerformanceHistos.hh
 *
 *  Created on: Jul 5, 2011
 *      Author: stanitz
 */

#ifndef MONITORPERFORMANCEHISTOS_HH_
#define MONITORPERFORMANCEHISTOS_HH_


#include <TH1I.h>
#include <TFile.h>

#include <map>
#include "SimpleStandardEvent.hh"


using namespace std;
class RootMonitor;

class MonitorPerformanceHistos
{
protected:
    TH1I * _AnalysisTimeHisto;
    TH1I * _FillTimeHisto;
    TH1I * _ClusteringTimeHisto;
    TH1I * _CorrelationTimeHisto;

public:
	MonitorPerformanceHistos();
	virtual ~MonitorPerformanceHistos();
	void Fill( SimpleStandardEvent ev);
	void Write();
	void Reset();
	TH1I*  getAnalysisTimeHisto() {return _AnalysisTimeHisto;}
	TH1I*  getFillTimeHisto() {return _FillTimeHisto;}
    TH1I*  getClusteringTimeHisto() {return _ClusteringTimeHisto;}
    TH1I*  getCorrelationTimeHisto() {return _CorrelationTimeHisto;}


};

#ifdef __CINT__
#pragma link C++ class MonitorPerformanceHistos-;
#endif

#endif /* MONITORPERFORMANCEHISTOS_HH_ */
