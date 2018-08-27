#include "WireChamberCorrelationCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool WireChamberCorrelationCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, WireChamberCorrelationHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void WireChamberCorrelationCollection::fillHistograms(const eudaq::StandardPlane &pl1, const eudaq::StandardPlane &pl2) {
  //std::cout<<"In WireChamberCorrelationCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl1.Sensor()!="WireChamber")
    return;
  if (pl2.Sensor()!="WireChamber")
    return;

  if (!isPlaneRegistered(pl1)) {
    registerPlane(pl1);
    isOnePlaneRegistered = true;
  }

  WireChamberCorrelationHistos *wcmap = _map[pl1];
  wcmap->Fill(pl1, pl2);

  ++counting;
}

void WireChamberCorrelationCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In WireChamberCorrelationCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor()=="WireChambers")
	     registerPlane(Plane);
    }
  }
}

void WireChamberCorrelationCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "WireChamberCorrelationCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("WireChamberCorrelation");
    gDirectory->cd("WireChamberCorrelation");

    std::map<eudaq::StandardPlane, WireChamberCorrelationHistos *>::iterator it;
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

void WireChamberCorrelationCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, WireChamberCorrelationHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void WireChamberCorrelationCollection::Reset() {
  std::map<eudaq::StandardPlane, WireChamberCorrelationHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void WireChamberCorrelationCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In WireChamberCorrelationCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane1 = 0; plane1 < ev.NumPlanes(); plane1++) {
    const eudaq::StandardPlane &Plane1 = ev.GetPlane(plane1);
    if (Plane1.Sensor()!="WireChamber") continue;
    for (int plane2 = 0; plane2 < ev.NumPlanes(); plane2++) {
      const eudaq::StandardPlane &Plane2 = ev.GetPlane(plane2);
      if (Plane2.Sensor()!="WireChamber") continue;
      if (Plane1.ID() >= Plane2.ID()) continue;
      fillHistograms(Plane1, Plane2);
    }
  }
}


WireChamberCorrelationHistos *WireChamberCorrelationCollection::getWireChamberCorrelationHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "WireChamber", sensor);
  return _map[p];
}

void WireChamberCorrelationCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In WireChamberCorrelationCollection::registerPlane(StandardPlane)"<<std::endl;

  WireChamberCorrelationHistos *tmphisto = new WireChamberCorrelationHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "WireChamberCorrelationCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());
    
    for (int _ID=0; _ID<4; _ID++) {

      if (p.ID() >= _ID ) continue;
      sprintf(tree, "%s/Chamber %i/CorrelationXX_vs_Chamber%i", p.Sensor().c_str(), p.ID(), _ID);      
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getWireChamberCorrelationHistos(p.Sensor(), p.ID())->getCorrelationXX(_ID), "COLZ2", 0);

      sprintf(tree, "%s/Chamber %i/CorrelationXY_vs_Chamber%i", p.Sensor().c_str(), p.ID(), _ID);      
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getWireChamberCorrelationHistos(p.Sensor(), p.ID())->getCorrelationXY(_ID), "COLZ2", 0);

      sprintf(tree, "%s/Chamber %i/CorrelationYY_vs_Chamber%i", p.Sensor().c_str(), p.ID(), _ID);      
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getWireChamberCorrelationHistos(p.Sensor(), p.ID())->getCorrelationYY(_ID), "COLZ2", 0); 

      sprintf(tree, "%s/Chamber %i/CorrelationYX_vs_Chamber%i", p.Sensor().c_str(), p.ID(), _ID);      
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getWireChamberCorrelationHistos(p.Sensor(), p.ID())->getCorrelationYX(_ID), "COLZ2", 0);      
    }
    



       
    ///
    
    sprintf(tree, "%s/Chamber %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    
  }
}
