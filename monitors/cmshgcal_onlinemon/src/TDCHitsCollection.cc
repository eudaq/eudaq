#include "TDCHitsCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool TDCHitsCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, TDCHitsHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void TDCHitsCollection::fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &pl, int eventNumber) {
  //std::cout<<"In TDCHitsCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (NumberOfDWCPlanes == -1) {
    NumberOfDWCPlanes = 0;
    for (int plane = 0; plane < ev.NumPlanes(); plane++) {
      const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
      if (Plane.Sensor() == "DWC") NumberOfDWCPlanes++;
    }
  }


  if (pl.Sensor() != "DWC_fullTDC")
    return;

  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  TDCHitsHistos *wcmap = _map[pl];
  wcmap->Fill(pl, eventNumber);

  ++counting;
}

void TDCHitsCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In TDCHitsCollection::bookHistograms(StandardEvent)"<<std::endl;
  if (NumberOfDWCPlanes == -1) {
    NumberOfDWCPlanes = 0;
    for (int plane = 0; plane < ev.NumPlanes(); plane++) {
      const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
      if (Plane.Sensor() == "DWC") NumberOfDWCPlanes++;
    }
  }


  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor() == "DWC_fullTDC") registerPlane(Plane);
    }
  }
}

void TDCHitsCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "TDCHitsCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("DelayWireChamber");
    gDirectory->cd("DelayWireChamber");

    std::map<eudaq::StandardPlane, TDCHitsHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {

      char sensorfolder[255] = "";
      sprintf(sensorfolder, "%s_%d", it->first.Sensor().c_str(), it->first.ID());

      // cout << "Making new subfolder " << sensorfolder << endl;
      gDirectory->mkdir(sensorfolder);
      gDirectory->cd(sensorfolder);
      it->second->Write();

      // gDirectory->ls();
      gDirectory->cd("..");
    }
    gDirectory->cd("..");
  }
}

void TDCHitsCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, TDCHitsHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void TDCHitsCollection::Reset() {
  std::map<eudaq::StandardPlane, TDCHitsHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void TDCHitsCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In TDCHitsCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor() == "DWC_fullTDC")
      fillHistograms(ev, Plane, evNumber);
  }
}


TDCHitsHistos *TDCHitsCollection::getTDCHitsHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "DWC_fullTDC", sensor);
  return _map[p];
}

void TDCHitsCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In TDCHitsCollection::registerPlane(StandardPlane)"<<std::endl;

  TDCHitsHistos *tmphisto = new TDCHitsHistos(p, _mon, NumberOfDWCPlanes);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());

  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "TDCHitsCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];


    sprintf(tree, "%s/HitOccupancy", "DWC");      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getTDCHitsHistos(p.Sensor(), p.ID())->getHitOccupancy(), "COLZ2", 0);

    sprintf(tree, "%s/HitProbability", "DWC");      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getTDCHitsHistos(p.Sensor(), p.ID())->getHitProbability(), "COLZ2", 0);
    sprintf(folder, "%s", "DWC");
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
  }
}
