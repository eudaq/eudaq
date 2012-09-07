/*
 * MonitorPerformanceCollection.cc
 *
 *  Created on: Jul 6, 2011
 *      Author: stanitz
 */

#include "MonitorPerformanceCollection.hh"
#include "OnlineMon.hh"

MonitorPerformanceCollection::MonitorPerformanceCollection(): BaseCollection()
{
	mymonhistos=new MonitorPerformanceHistos();
	histos_init=false;
	cout << " Initialising MonitorPerformance Collection" <<endl;
	CollectionType=MONITORPERFORMANCE_COLLECTION_TYPE;
}

MonitorPerformanceCollection::~MonitorPerformanceCollection()
{
	// TODO Auto-generated destructor stub
}

void MonitorPerformanceCollection::Write(TFile *file)
{
	if (file==NULL)
	{
		cout << "MonitorPerformanceCollection::Write File pointer is NULL"<<endl;
		exit(-1);
	}
	if (gDirectory!=NULL) //check if this pointer exists
	{
		gDirectory->mkdir("MonitorPerformance");
		gDirectory->cd("MonitorPerformance");
		mymonhistos->Write();
		gDirectory->cd("..");
	}
}

void MonitorPerformanceCollection::Calculate(const unsigned int currentEventNumber)
{

}

void MonitorPerformanceCollection::Reset()
{
	mymonhistos->Reset();
}

void MonitorPerformanceCollection::Fill(const SimpleStandardEvent &simpev)
{
	if (histos_init==false)
	{
		bookHistograms(simpev);
		histos_init=true;
	}
	mymonhistos->Fill(simpev);
}


void MonitorPerformanceCollection::bookHistograms(const SimpleStandardEvent & simpev)
{
	if (_mon != NULL)
	{
		cout << "MonitorPerformanceCollection:: Monitor running in online-mode" << endl;
		string performance_folder_name="Monitor Performance";
		_mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Data Analysis Time"));
		_mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Data Analysis Time"),mymonhistos->getAnalysisTimeHisto());
		_mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Histo Fill Time"));
		_mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Histo Fill Time"),mymonhistos->getFillTimeHisto());
        _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Clustering Time"));
        _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Clustering Time"),mymonhistos->getClusteringTimeHisto());
        _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Correlation Time"));
        _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Correlation Time"),mymonhistos->getCorrelationTimeHisto());
		_mon->getOnlineMon()->makeTreeItemSummary(performance_folder_name.c_str()); //make summary page
	}
}

MonitorPerformanceHistos * MonitorPerformanceCollection::getMonitorPerformanceHistos()
{
	return mymonhistos;
}
