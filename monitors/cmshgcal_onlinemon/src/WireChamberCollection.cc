#include "WireChamberCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool WireChamberCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, WireChamberHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void WireChamberCollection::fillHistograms(const eudaq::StandardPlane &pl) {
  //std::cout<<"In WireChamberCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor() != "DWC")
    return;

  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  WireChamberHistos *wcmap = _map[pl];
  wcmap->Fill(pl);

  ++counting;
}

void WireChamberCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In WireChamberCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor() == "DWC")
        registerPlane(Plane);
    }
  }
}

void WireChamberCollection::Write(TFile *file) {
  cout << "WireChamberCorrelationCollection::Write"<<endl;

  if (file == NULL) {
    cout << "WireChamberCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }

  if (gDirectory != NULL) // check if this pointer exists
  {
    if (gDirectory->GetDirectory("DelayWireChamber")==NULL)
      gDirectory->mkdir("DelayWireChamber");
    gDirectory->cd("DelayWireChamber");

    std::map<eudaq::StandardPlane, WireChamberHistos *>::iterator it;
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

void WireChamberCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, WireChamberHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void WireChamberCollection::Reset() {
  std::map<eudaq::StandardPlane, WireChamberHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void WireChamberCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In WireChamberCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor() == "DWC")
      fillHistograms(Plane);
  }
}


WireChamberHistos *WireChamberCollection::getWireChamberHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "DWC", sensor);
  return _map[p];
}

void WireChamberCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In WireChamberCollection::registerPlane(StandardPlane)"<<std::endl;

  WireChamberHistos *tmphisto = new WireChamberHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());

  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "WireChamberCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());


    sprintf(tree, "%s/Chamber %i/Valid Measurement", p.Sensor().c_str(), p.ID());      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getWireChamberHistos(p.Sensor(), p.ID())->getGoodAllHisto(), "", 0);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/Chamber %i/reco X", p.Sensor().c_str(), p.ID());      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getWireChamberHistos(p.Sensor(), p.ID())->getRecoXHisto(), "", 0);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/Chamber %i/reco Y", p.Sensor().c_str(), p.ID());      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getWireChamberHistos(p.Sensor(), p.ID())->getRecoYHisto(), "", 0);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    sprintf(tree, "%s/Chamber %i/reco XY", p.Sensor().c_str(), p.ID());      //Todo: Register here when more is added
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getWireChamberHistos(p.Sensor(), p.ID())->getXYmapHisto(), "COLZ2", 0);
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


    ///

    sprintf(tree, "%s/Chamber %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);


  }
}
