#include "AhcalCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool AhcalCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, AhcalHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void AhcalCollection::fillHistograms(const eudaq::StandardPlane &pl) {
  //std::cout<<"In AhcalCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor().find("Calice")==std::string::npos)
    return;
  
  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  AhcalHistos *ahcalmap = _map[pl];
  ahcalmap->Fill(pl);

  ++counting;
}

void AhcalCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In AhcalCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor().find("Calice")!=std::string::npos)
	registerPlane(Plane);
    }
  }
}

void AhcalCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "AhcalCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("Ahcal");
    gDirectory->cd("Ahcal");

    std::map<eudaq::StandardPlane, AhcalHistos *>::iterator it;
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

void AhcalCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, AhcalHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void AhcalCollection::Reset() {
  std::map<eudaq::StandardPlane, AhcalHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void AhcalCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In AhcalCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);

    //std::cout<<"Trying to Fill a plane: "<<plane<<" sensor="<<Plane.Sensor()<<"  ID="<<Plane.ID()<<std::endl;
    if (Plane.Sensor().find("Calice")!=std::string::npos)
      fillHistograms(Plane);
  }
}

//void AhcalCollection::Fill(const SimpleStandardEvent &ev) {
//  std::cout<<"In AhcalCollection::Fill(SimpleStandardEvent)"<<std::endl;
  // It's needed here because the mother class - BasicCollection - expects it
//}


AhcalHistos *AhcalCollection::getAhcalHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "Calice", sensor);

  //std::cout<<"In getAhcalHistos..  sensor="<<sensor<<"  id="<<id<<std::endl;
  return _map[p];
}

void AhcalCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In AhcalCollection::registerPlane(StandardPlane)"<<std::endl;

  AhcalHistos *tmphisto = new AhcalHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "AhcalCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());
    
    sprintf(tree, "%s/Module %i/RawHitmap", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getAhcalHistos(p.Sensor(), p.ID())->getHitmapHisto(), "COLZ2", 0);
    _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
    
    sprintf(tree, "%s/Module %i/RawHitmap X Projection", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getAhcalHistos(p.Sensor(), p.ID())->getHitXmapHisto());

    sprintf(tree, "%s/Module %i/RawHitmap Y Projection", p.Sensor().c_str(),p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getAhcalHistos(p.Sensor(), p.ID())->getHitYmapHisto());
    
    sprintf(tree, "%s/Module %i/NumHits", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getAhcalHistos(p.Sensor(), p.ID())->getNHitsHisto());


    
    
    // PER EVENT Histograms:

    
    ///
    
    sprintf(tree, "%s/Module %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    
  }
}
