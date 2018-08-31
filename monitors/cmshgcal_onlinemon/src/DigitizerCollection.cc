#include "DigitizerCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool DigitizerCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, DigitizerHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void DigitizerCollection::fillHistograms(const eudaq::StandardEvent &ev, const eudaq::StandardPlane &pl, int eventNumber) {
  //std::cout<<"In DigitizerCollection::fillHistograms(StandardPlane)"<<std::endl;

  if (pl.Sensor() != "DIGITIZER")
    return;

  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  DigitizerHistos *histomap = _map[pl];
  histomap->Fill(pl, eventNumber);

  ++counting;
}

void DigitizerCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  //std::cout<<"In DigitizerCollection::bookHistograms(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor() == "DIGITIZER") registerPlane(Plane);
    }
  }
}

void DigitizerCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "DigitizerCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("Digitizer");
    gDirectory->cd("Digitizer");

    std::map<eudaq::StandardPlane, DigitizerHistos *>::iterator it;
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

void DigitizerCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, DigitizerHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void DigitizerCollection::Reset() {
  std::map<eudaq::StandardPlane, DigitizerHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void DigitizerCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //std::cout<<"In DigitizerCollection::Fill(StandardEvent)"<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    if (Plane.Sensor() == "DIGITIZER")
      fillHistograms(ev, Plane, evNumber);
  }
}


DigitizerHistos *DigitizerCollection::getDigitizerHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "DIGITIZER", sensor);
  return _map[p];
}

void DigitizerCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In DigitizerCollection::registerPlane(StandardPlane)"<<std::endl;

  DigitizerHistos *tmphisto = new DigitizerHistos(p, _mon);
  _map[p] = tmphisto;


  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "DigitizerCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];


    for (int gr = 0; gr < 4; gr++) for (int ch = 0; ch < 9; ch++) {
        int key = gr * 100 + ch;
        sprintf(tree, "%s/group%i/IntegratedWaveform_ch%i", "DIGITIZER", gr, ch);
        _mon->getOnlineMon()->registerTreeItem(tree);
        _mon->getOnlineMon()->registerHisto(tree, getDigitizerHistos(p.Sensor(), p.ID())->getIntegratedWaveform(key), "COLZ", 0);

        sprintf(tree, "%s/group%i/LastWaveform_ch%i", "DIGITIZER", gr, ch);
        _mon->getOnlineMon()->registerTreeItem(tree);
        _mon->getOnlineMon()->registerHisto(tree, getDigitizerHistos(p.Sensor(), p.ID())->getLastWaveform(key), "", 0);
        sprintf(folder, "%s/group%i", "DIGITIZER", gr);
        _mon->getOnlineMon()->addTreeItemSummary(folder, tree);


        //some are added as summary objects
        if ((gr == 0) && (ch == 0)) {
          sprintf(tree, "%s/group%i/IntegratedWaveform_ch%i", "DIGITIZER", gr, ch);
          _mon->getOnlineMon()->addTreeItemSummary("DIGITIZER", tree);
          sprintf(tree, "%s/group%i/LastWaveform_ch%i", "DIGITIZER", gr, ch);
          _mon->getOnlineMon()->addTreeItemSummary("DIGITIZER", tree);
        }
        if ((gr == 0) && (ch == 1)) {
          sprintf(tree, "%s/group%i/IntegratedWaveform_ch%i", "DIGITIZER", gr, ch);
          _mon->getOnlineMon()->addTreeItemSummary("DIGITIZER", tree);
          sprintf(tree, "%s/group%i/LastWaveform_ch%i", "DIGITIZER", gr, ch);
          _mon->getOnlineMon()->addTreeItemSummary("DIGITIZER", tree);
        }
      }
  }
}
