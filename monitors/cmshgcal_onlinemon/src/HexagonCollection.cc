#include "HexagonCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool HexagonCollection::isPlaneRegistered(eudaq::StandardPlane p) {
  std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
  it = _map.find(p);
  return (it != _map.end());
}

void HexagonCollection::fillHistograms(const eudaq::StandardPlane &pl, int evNumber) {
  //std::cout<<"In HexagonCollection::fillHistograms(StandardPlane)"<<std::endl;

  // if (pl.Sensor().find("HexaBoard")==std::string::npos)
  //   return;
  
  if (!isPlaneRegistered(pl)) {
    registerPlane(pl);
    isOnePlaneRegistered = true;
  }

  HexagonHistos *hexmap = _map[pl];
  hexmap->Fill(pl, evNumber);

  ++counting;
}

void HexagonCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  std::cout<<"In HexagonCollection::bookHistograms(StandardEvent)"<<std::endl;
  
  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane Plane = ev.GetPlane(plane);
    if (!isPlaneRegistered(Plane)) {
      if (Plane.Sensor().find("HexaBoard")!=std::string::npos){
	registerPlane(Plane);
      }
    }
  }
}

void HexagonCollection::Write(TFile *file) {
  if (file == NULL) {
    // cout << "HexagonCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }  

  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("Hexagon");
    gDirectory->cd("Hexagon");

    std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
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

void HexagonCollection::Calculate(const unsigned int currentEventNumber) {
  if ((currentEventNumber > 10 && currentEventNumber % 1000 * _reduce == 0)) {
    std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it) {
      // std::cout << "Calculating ..." << std::endl;
      it->second->Calculate(currentEventNumber / _reduce);
    }
  }
}

void HexagonCollection::Reset() {
  std::map<eudaq::StandardPlane, HexagonHistos *>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it) {
    (*it).second->Reset();
  }
}


void HexagonCollection::Fill(const eudaq::StandardEvent &ev, int evNumber) {
  //  std::cout<<"In HexagonCollection::Fill(StandardEvent) with "<<ev.NumPlanes()<<" planes."<<std::endl;

  for (int plane = 0; plane < ev.NumPlanes(); plane++) {
    const eudaq::StandardPlane &Plane = ev.GetPlane(plane);
    //std::cout<<"Trying to Fill a plane: "<<plane<<" sensor="<<Plane.Sensor()<<"  ID="<<Plane.ID()<< "with npixels: "<<Plane.TotalPixels()<<std::endl;

    if (Plane.Sensor().find("HexaBoard")!=std::string::npos){
      //std::cout << "I found hexaboard planes with " << Plane.HitPixels() << std::endl;
      fillHistograms(Plane, evNumber);
    }
  }
}

//void HexagonCollection::Fill(const SimpleStandardEvent &ev) {
//  std::cout<<"In HexagonCollection::Fill(SimpleStandardEvent)"<<std::endl;
  // It's needed here because the mother class - BasicCollection - expects it
//}


HexagonHistos *HexagonCollection::getHexagonHistos(std::string sensor, int id) {
  const eudaq::StandardPlane p(id, "HexaBoard", sensor);
  return _map[p];
}

void HexagonCollection::registerPlane(const eudaq::StandardPlane &p) {
  //std::cout<<"In HexagonCollection::registerPlane(StandardPlane)"<<std::endl;

  HexagonHistos *tmphisto = new HexagonHistos(p, _mon);
  _map[p] = tmphisto;
  
  if (_mon != NULL) {
    if (_mon->getOnlineMon() == NULL) {
      return; // don't register items
    }
    // cout << "HexagonCollection:: Monitor running in online-mode" << endl;
    char tree[1024], folder[1024];
    sprintf(folder, "%s", p.Sensor().c_str());

    //std::cout<<"Hexa collection runMode="<<_runMode<<std::endl;
    
    if (_runMode!=0){
      sprintf(tree, "%s/Module %i/Occ_Full_Selection", p.Sensor().c_str(), p.ID());
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccSelectHisto(), "COLZ2 TEXT");
      _mon->getOnlineMon()->addTreeItemSummary(folder, tree);
      
      //sprintf(tree, "%s/Module %i/Occ_ADC_HG", p.Sensor().c_str(), p.ID());
      //_mon->getOnlineMon()->registerTreeItem(tree);
      //_mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccAdcHisto(), "COLZ2 TEXT");
      //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);
      
      sprintf(tree, "%s/Module %i/Occ_TOT", p.Sensor().c_str(), p.ID());
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccTotHisto(), "COLZ2 TEXT");
      //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);
      
      sprintf(tree, "%s/Module %i/Occ_TOA", p.Sensor().c_str(), p.ID());
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsOccToaHisto(), "COLZ2 TEXT");
      //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);
      
      sprintf(tree, "%s/Module %i/RawHitmap", p.Sensor().c_str(), p.ID());
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHitmapHisto(), "COLZ2", 0);
    
      //sprintf(tree, "%s/Module %i/Hit_1D_Occupancy", p.Sensor().c_str(), p.ID());
      //_mon->getOnlineMon()->registerTreeItem(tree);
      //_mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHit1DoccHisto());
      
      sprintf(tree, "%s/Module %i/NumHits", p.Sensor().c_str(), p.ID());
      _mon->getOnlineMon()->registerTreeItem(tree);
      _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getNHitsHisto());
    }
    
    sprintf(tree, "%s/Module %i/SignalADC_LG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getSigAdcLGHisto());

    sprintf(tree, "%s/Module %i/SignalADC_HG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getSigAdcHGHisto());

    sprintf(tree, "%s/Module %i/PedestalLG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getPedLGHisto());

    sprintf(tree, "%s/Module %i/PedestalHG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getPedHGHisto());
    
    if (_runMode==0)
      _mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    sprintf(tree, "%s/Module %i/LGvsTOTfast", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getLGvsTOTfastHisto(),"COL2");

    sprintf(tree, "%s/Module %i/LGvsTOTslow", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getLGvsTOTslowHisto(),"COL2");

    sprintf(tree, "%s/Module %i/HGvsLG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHGvsLGHisto(), "COL2");

    sprintf(tree, "%s/Module %i/TOAvsChId", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getTOAvsChIdHisto(), "COL2");

    // ----------
    // Waveforms
    
    sprintf(tree, "%s/Module %i/WaveForm_Histo_LG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getWaveformLGHisto(), "COL2");
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);

    sprintf(tree, "%s/Module %i/WaveForm_Histo_HG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getWaveformHGHisto(), "COL2");
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    sprintf(tree, "%s/Module %i/TS_of_Max_ADC_LG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getPosOfMaxADCinLGHisto());
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);

    sprintf(tree, "%s/Module %i/TS_of_Max_ADC_HG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getPosOfMaxADCinHGHisto());
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);
    
    sprintf(tree, "%s/Module %i/WaveForm_Prof_LG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, (TH1D*)getHexagonHistos(p.Sensor(), p.ID())->getWaveformLGProfile());
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);

    sprintf(tree, "%s/Module %i/WaveForm_Prof_HG", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, (TH1D*)getHexagonHistos(p.Sensor(), p.ID())->getWaveformHGProfile());
    _mon->getOnlineMon()->addTreeItemSummary("HexaBoard/Waveforms", tree);
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    // End of Waveforms
    //----------------

    
    // --------------------
    // PER EVENT Histograms:
    sprintf(tree, "%s/Module %i/HG_ADC_in_last_event", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree, getHexagonHistos(p.Sensor(), p.ID())->getHexagonsChargeHisto(), "COLZ2 TEXT");
    //_mon->getOnlineMon()->addTreeItemSummary(folder, tree);

    
    sprintf(tree, "%s/Module %i", p.Sensor().c_str(), p.ID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);

    //sprintf(tree, "%s/Waveforms", p.Sensor().c_str());
    //_mon->getOnlineMon()->makeTreeItemSummary(tree);
    
    //sprintf(tree, "%s", p.Sensor().c_str());
    //_mon->getOnlineMon()->makeTreeItemSummary(tree);
    
  }

  //std::cout << "Registered Plane: " << p.Sensor() << " " << p.ID() << std::endl;

}
