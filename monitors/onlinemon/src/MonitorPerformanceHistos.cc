/*
 * MonitorPerformanceHistos.cc
 *
 *  Created on: Jul 5, 2011
 *      Author: stanitz
 */

#include "MonitorPerformanceHistos.hh"


#include <iostream>

MonitorPerformanceHistos::MonitorPerformanceHistos()
{
  _AnalysisTimeHisto= new TH1I("Data Analysis Time", "Data Analysis Time",400,0,0.01);
  _FillTimeHisto=new TH1I("Histo Fill Time", "Histo Fill Time",400,0,0.02);
  _ClusteringTimeHisto= new TH1I("Clustering Time", "Clustering Time",400,0,0.01);
  _CorrelationTimeHisto=new TH1I("Correlation Time", "Correlation Time",400,0,0.02);
  if ((_FillTimeHisto==NULL) || (_AnalysisTimeHisto==NULL) || (_ClusteringTimeHisto==NULL) || (_CorrelationTimeHisto==NULL))
  {
    std::cout<< "MonitorPerformanceHistos:: Error allocating Histograms" <<std::endl;
    exit(-1); // we bail out, if can't allocate memory
  }
}

MonitorPerformanceHistos::~MonitorPerformanceHistos()
{
  // TODO Auto-generated destructor stub
}

void MonitorPerformanceHistos::Write()
{
  _AnalysisTimeHisto->Write();
  _FillTimeHisto->Write();
  _ClusteringTimeHisto->Write();
  _CorrelationTimeHisto->Write();
}



void MonitorPerformanceHistos::Fill(SimpleStandardEvent ev)
{
  _AnalysisTimeHisto->Fill(ev.getMonitor_eventanalysistime());
  _FillTimeHisto->Fill(ev.getMonitor_eventfilltime());
  _ClusteringTimeHisto->Fill(ev.getMonitor_clusteringtime());
  _CorrelationTimeHisto->Fill(ev.getMonitor_correlationtime());
}



void MonitorPerformanceHistos::Reset()
{
  _AnalysisTimeHisto->Reset();
  _FillTimeHisto->Reset();
  _ClusteringTimeHisto->Reset();
  _CorrelationTimeHisto->Reset();
}

