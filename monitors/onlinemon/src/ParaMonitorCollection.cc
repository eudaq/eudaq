/*
 * ParaMonitorCollection.cc
 *
 *  Created on: Jul 6, 2011
 *      Author: stanitz
 */

#include "ParaMonitorCollection.hh"
#include "OnlineMon.hh"

ParaMonitorCollection::ParaMonitorCollection()
    : BaseCollection() {
  histos = new ParaMonitorHistos();
  histos_init = false;
  cout << " Initialising ParaMonitor Collection" << endl;
  CollectionType = PARAMONITOR_COLLECTION_TYPE;
}

ParaMonitorCollection::~ParaMonitorCollection() {
  // TODO Auto-generated destructor stub
}

void ParaMonitorCollection::Write(TFile *file) {
  if (file == NULL) {
    cout << "ParaMonitorCollection::Write File pointer is NULL" << endl;
    exit(-1);
  }
  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("ParaMonitor");
    gDirectory->cd("ParaMonitor");
    histos->Write();
    gDirectory->cd("..");
  }
}

void ParaMonitorCollection::Calculate(
    const unsigned int /*currentEventNumber*/) {}

void ParaMonitorCollection::Reset() { histos->Reset(); }

void ParaMonitorCollection::Fill(const SimpleStandardEvent &simpev) {
  if (histos_init == false) {
    bookHistograms(simpev);
    histos_init = true;
  }
  histos->Fill(simpev);
}

void ParaMonitorCollection::bookHistograms(
    const SimpleStandardEvent & /*simpev*/) {
  if (_mon != NULL) {
    string folder_name = "Paramater Monitor";
    _mon->getOnlineMon()->registerTreeItem(
        (folder_name + "/Temperature Time"));
    _mon->getOnlineMon()->makeTreeItemSummary(
        folder_name.c_str()); // make summary page
  }
}

ParaMonitorHistos *
ParaMonitorCollection::getParaMonitorHistos() {
  return histos;
}
