//author: Thorben Quast, thorben.quast@cern.ch
//04 June 2018

#include "DWCToHGCALCorrelationCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool DWCToHGCALCorrelationCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, DWCToHGCALCorrelationHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void DWCToHGCALCorrelationCollection::fillHistograms(const eudaq::StandardPlane &pl, const eudaq::StandardPlane &plDWC) {
  //std::cout<<"In DWCToHGCALCorrelationCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor().find("HexaBoard")==std::string::npos)
    return;
  if (plDWC.Sensor().find("DWC")==std::string::npos)
    return;
  
  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }



  DWCToHGCALCorrelationHistos *wcmap = _map[pl];
  wcmap->Fill(pl, plDWC);

  ++counting;
}

void DWCToHGCALCorrelationCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In DWCToHGCALCorrelationCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor().find("HexaBoard")!=std::string::npos)
	     registerPlane(Plane);
    }
  }
}

void DWCToHGCALCorrelationCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "DWCToHGCALCorrelationCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("DWCToHGCALCorrelation");
    gDirectory->cd("DWCToHGCALCorrelation");

    std::map<eudaq::StandardPlane, DWCToHGCALCorrelationHistos *>::iterator it;
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

void DWCToHGCALCorrelationCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, DWCToHGCALCorrelationHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void DWCToHGCALCorrelationCollection::Reset() {
  std::map<eudaq::StandardPlane, DWCToHGCALCorrelationHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void DWCToHGCALCorrelationCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor().find("HexaBoard")!=std::string::npos) {
      if(Plane.ID()>0) continue;    //only ID=0 for hexaboards for now (04 June 2018)

      int plane_dwc0_idx=-1;
      for (int plane_idx = 0; plane_idx < ev.NumPlanes(); plane_idx++) {
        const eudaq::StandardPlane &Plane2 = ev.GetPlane(plane_idx);
        if (Plane2.Sensor().find("DWC")==std::string::npos) continue;
        fillHistograms(Plane, Plane2);
      }
    }
  }
}


DWCToHGCALCorrelationHistos *DWCToHGCALCorrelationCollection::getDWCToHGCALCorrelationHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HexaBoard", sensor);
  return _map[p];
}

void DWCToHGCALCorrelationCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In DWCToHGCALCorrelationCollection::registerPlane(StandardPlane)"<<std::endl;

  DWCToHGCALCorrelationHistos *tmphisto = new DWCToHGCALCorrelationHistos(p, _mon);
  _map[p] = tmphisto;

  // std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() <<
  // std::endl;
  // PlaneRegistered(p.Sensor(),p.ID());
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "DWCToHGCALCorrelationCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());

    sprintf(tree, "DWCToHGCal/%s/Module %i/ChannelOccupancy", p.Sensor().c_str(), p.ID());      
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getDWCToHGCALCorrelationHistos(p.Sensor(), p.ID())->getOccupancy_ForChannel(), "", 0);    

    for (int ski=0; ski<4; ski++) {
      for (int ch=0; ch<=64; ch++) {
        if (ch%2==1) continue;
        int key = ski*1000+ch;
        sprintf(tree, "DWCToHGCal/%s/Module %i/Skiroc_%i/Ch_%iCorrelationToMIMOSA3", p.Sensor().c_str(), p.ID(), ski, ch);      
        _mon->getOnlineMon()->registerTreeItem(tree);
        _mon->getOnlineMon()->registerHisto(tree, getDWCToHGCALCorrelationHistos(p.Sensor(), p.ID())->getDWC_map_ForChannel(key), "", 0);
        
      }
    }    
  }
}


