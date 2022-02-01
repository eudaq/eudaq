/*
 * EUDAQMonitorCollection.cpp
 *
 *  Created on: Sep 27, 2011
 *      Author: stanitz
 */

#include "../include/EUDAQMonitorCollection.hh"
#include "OnlineMon.hh"

EUDAQMonitorCollection::EUDAQMonitorCollection() : BaseCollection() {
  mymonhistos = NULL;
  histos_init = false;
  CollectionType = EUDAQMONITOR_COLLECTION_TYPE;
}

EUDAQMonitorCollection::~EUDAQMonitorCollection() {
  // TODO Auto-generated destructor stub
}

void EUDAQMonitorCollection::fillHistograms(const SimpleStandardEvent &ev) {
  Fill(ev);
}

void EUDAQMonitorCollection::Reset() {
  if (mymonhistos != NULL)
    mymonhistos->Reset();
}

void EUDAQMonitorCollection::Write(TFile *file) {
  if (file == NULL) {
    std::cerr << "EUDAQMonitorCollection::Write File pointer is NULL" << endl;
    exit(-1);
  }
  if (gDirectory) // check if this pointer exists
  {
    gDirectory->mkdir("EUDAQMonitor");
    gDirectory->cd("EUDAQMonitor");
    mymonhistos->Write();
    gDirectory->cd("..");
  }
}

void EUDAQMonitorCollection::bookHistograms(
    const SimpleStandardEvent & /*simpev*/) {
  if (_mon != NULL) {
    string performance_folder_name = "EUDAQ Monitor";
    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Number of Planes"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Number of Planes"),
        mymonhistos->getPlanes_perEventHisto());
    _mon->getOnlineMon()->registerMutex(
        (performance_folder_name + "/Number of Planes"),
        mymonhistos->getMutexPlanes_perEvent());

    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Hits vs. Plane"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Hits vs. Plane"),
        mymonhistos->getHits_vs_PlaneHisto());
    _mon->getOnlineMon()->registerMutex(
        (performance_folder_name + "/Hits vs. Plane"),
        mymonhistos->getMutexHits_vs_Plane());

    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Hits vs. Event"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Hits vs. Event"),
        mymonhistos->getHits_vs_EventsTotal());
    _mon->getOnlineMon()->registerMutex(
        (performance_folder_name + "/Hits vs. Event"),
        mymonhistos->getMutexHits_vs_EventsTotal());

    
    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/EventN vs TimeStamp"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/EventN vs TimeStamp"),
        mymonhistos->getEventN_vs_TimeStamp(), "AP");
    _mon->getOnlineMon()->registerMutex(
        (performance_folder_name + "/EventN vs TimeStamp"),
        mymonhistos->getMutexEventN_vs_TimeStamp());
    
    
    if (_mon->getUseTrack_corr()) {
      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Tracks per Event"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Tracks per Event"),
          mymonhistos->getTracksPerEventHisto());
      _mon->getOnlineMon()->registerMutex(
          (performance_folder_name + "/Tracks_per Event"),
          mymonhistos->getMutexTracksPerEvent());
  
    }

    _mon->getOnlineMon()->makeTreeItemSummary(
        performance_folder_name.c_str()); // make summary page

    stringstream namestring;
    string name_root = performance_folder_name + "/Planes";
    for (unsigned int i = 0; i < mymonhistos->getNplanes(); i++) {
      stringstream namestring_hits;
      namestring_hits << name_root << "/Hits Sensor Plane " << i;
      _mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
      _mon->getOnlineMon()->registerHisto(namestring_hits.str(),
                                          mymonhistos->getHits_vs_Events(i));
      _mon->getOnlineMon()->registerMutex(namestring_hits.str(),
                                          mymonhistos->getMutexHits_vs_Events(i));
    }
    _mon->getOnlineMon()->makeTreeItemSummary(
        name_root.c_str()); // make summary page
  }
}

EUDAQMonitorHistos *EUDAQMonitorCollection::getEUDAQMonitorHistos() {
  return mymonhistos;
}

void EUDAQMonitorCollection::Calculate(
    const unsigned int /*currentEventNumber*/) {}

void EUDAQMonitorCollection::Fill(const SimpleStandardEvent &simpev) {
  if (histos_init == false) {
    mymonhistos = new EUDAQMonitorHistos(simpev);
    // mymonhistos2 = new ParaMonitorHistos();
    if (mymonhistos == NULL) {
      std::cerr << "EUDAQMonitorCollection:: Can't book histograms " << endl;
      exit(-1);
    }
    bookHistograms(simpev);
    histos_init = true;
  }
  mymonhistos->Fill(simpev);
  // mymonhistos2->Fill(simpev);
}
