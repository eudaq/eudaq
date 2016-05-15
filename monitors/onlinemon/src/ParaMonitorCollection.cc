
#include "ParaMonitorCollection.hh"
#include "OnlineMon.hh"

ParaMonitorCollection::ParaMonitorCollection()
  : BaseCollection(){
  histos_init = false;
  cout << " Initialising ParaMonitor Collection" << endl;
  CollectionType = PARAMONITOR_COLLECTION_TYPE;

  std::string name;
  TGraph *tg;
  name="Temperature";
  tg = new TGraph();
  tg->SetTitle(name.c_str());
  m_graphMap[name] = tg;    
  
  name="Voltage";
  tg = new TGraph();
  tg->SetTitle(name.c_str());
  m_graphMap[name] = tg;

}

ParaMonitorCollection::~ParaMonitorCollection() {
  for(auto &e: m_graphMap){
      delete e.second;
  }
  m_graphMap.clear();
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
    for(auto &e: m_graphMap){
      e.second->Write();
    }
    gDirectory->cd("..");
  }
}

void ParaMonitorCollection::Calculate(
    const unsigned int /*currentEventNumber*/) {}

void ParaMonitorCollection::Reset() {
    for(auto &e: m_graphMap){
      e.second->Set(0);
    }
}

void ParaMonitorCollection::Fill(const SimpleStandardEvent &simpev) {
  if (histos_init == false) {
    bookHistograms(simpev);
    histos_init = true;
  }
  for(auto &e: m_graphMap){
    double value;
    std::string str = e.first;
    if(simpev.getSlow_para(str, value)){
      TGraph *tg=e.second;
      std::lock_guard<std::mutex> lck(m_mu);
      tg->SetPoint(tg->GetN(), simpev.getEvent_timestamp(), value);
    }
  }
  
}

void ParaMonitorCollection::bookHistograms(
    const SimpleStandardEvent & /*simpev*/) {
  if (_mon != NULL) {
    string folder_name = "Paramater Monitor";
    for(auto &e: m_graphMap){
      std::string name = folder_name+"/"+e.first;
      _mon->getOnlineMon()->registerTreeItem(name);
      _mon->getOnlineMon()->registerHisto(name,e.second);
      _mon->getOnlineMon()->registerMutex(name,&m_mu);
    }
    _mon->getOnlineMon()->makeTreeItemSummary(
        folder_name.c_str()); // make summary page
  }
}
