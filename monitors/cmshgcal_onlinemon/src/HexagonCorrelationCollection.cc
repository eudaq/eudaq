#include "HexagonCorrelationCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool HexagonCorrelationCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, HexagonCorrelationHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void HexagonCorrelationCollection::fillHistograms(const eudaq::StandardPlane &pl1, const eudaq::StandardPlane &pl2) {
  //std::cout<<"In HexagonCorrelationCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl1.Sensor().find("HexaBoard")==std::string::npos)
    return;

  if (pl2.Sensor().find("HexaBoard")==std::string::npos)
    return;


  if (!isPlaneRegistered(pl1)) {
    registerPlane(pl1);
    isOnePlaneRegistered = true;
  }

  HexagonCorrelationHistos *wcmap = _map[pl1];
  wcmap->Fill(pl1, pl2);

  ++counting;
}

void HexagonCorrelationCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In HexagonCorrelationCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor()=="HexaBoard")
	     registerPlane(Plane);
    }
  }
}

void HexagonCorrelationCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "HexagonCorrelationCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("HexagonCorrelation");
    gDirectory->cd("HexagonCorrelation");

    std::map<eudaq::StandardPlane, HexagonCorrelationHistos *>::iterator it;
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

void HexagonCorrelationCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, HexagonCorrelationHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void HexagonCorrelationCollection::Reset() {
  std::map<eudaq::StandardPlane, HexagonCorrelationHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void HexagonCorrelationCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In HexagonCorrelationCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane1 = 0; plane1 < ev.NumPlanes(); plane1++) {
    const eudaq::StandardPlane &Plane1 = ev.GetPlane(plane1);
    
    if (Plane1.Sensor().find("HexaBoard")==std::string::npos) continue;
  
    for (int plane2 = 0; plane2 < ev.NumPlanes(); plane2++) {
      const eudaq::StandardPlane &Plane2 = ev.GetPlane(plane2);
      if (Plane2.Sensor() != Plane1.Sensor()) continue; //only correlate boards from the same readout board for now            
      if (Plane1.ID() >= Plane2.ID()) continue;
      fillHistograms(Plane1, Plane2);
    }
  }
}


HexagonCorrelationHistos *HexagonCorrelationCollection::getHexagonCorrelationHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HexaBoard", sensor);
  return _map[p];
}

void HexagonCorrelationCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In HexagonCorrelationCollection::registerPlane(StandardPlane)"<<std::endl;

  HexagonCorrelationHistos *tmphisto = new HexagonCorrelationHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "HexagonCorrelationCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    
    sprintf(folder, "HexagonCorrelation/%s/Module %i", p.Sensor().c_str(), p.ID());
    
    for (int _ID=0; _ID<NHEXAGONS_PER_SENSOR; _ID++) {   //maximum eight hexagons per readout board
      if (p.ID() >= _ID ) continue;
      sprintf(tree, "HexagonCorrelation/%s/Module %i/SignalADC_HG_vs_Module %i", p.Sensor().c_str(), p.ID(), _ID);
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonCorrelationHistos(p.Sensor(), p.ID())->getCorrelationSignalHGSum(_ID), "COLZ2", 0);
      
      _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
      
      
      sprintf(tree, "HexagonCorrelation/%s/Module %i/TOA_corr_Module %i", p.Sensor().c_str(), p.ID(), _ID);
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonCorrelationHistos(p.Sensor(), p.ID())->getCorrelationTOA(_ID), "COL2");
      

    }
    
       
    ///
    
    //sprintf(tree, "%s/Chamber %i", p.Sensor().c_str(), p.ID());

    
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    
  }
}
