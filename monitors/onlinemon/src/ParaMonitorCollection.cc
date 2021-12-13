
#include "ParaMonitorCollection.hh"
#include "OnlineMon.hh"

ParaMonitorCollection::ParaMonitorCollection()
  : BaseCollection(){
  histos_init = false;
  cout << " Initialising ParaMonitor Collection" << endl;
  CollectionType = PARAMONITOR_COLLECTION_TYPE;


}

ParaMonitorCollection::~ParaMonitorCollection() {
  for(auto &e: m_graphMap){
      delete e.second;
  }
  m_graphMap.clear();
}

void ParaMonitorCollection::InitPlots(const SimpleStandardEvent &simpev){
  std::string name;
  TGraph *tg;
  std::vector<std::string> list = simpev.getSlowList();
  for(auto &e: list){
    name = e;
    tg = new TGraph();
    // tg->SetMarkerStyle(5);
    // tg->SetMarkerSize(4);
    tg->SetTitle(name.c_str());
    m_graphMap[name] = tg;    
  }
  
  if(m_graphMap.size())
    histos_init = true;
}


void ParaMonitorCollection::Write(TFile *file) {
  if (file == NULL) {
    cout << "ParaMonitorCollection::Write File pointer is NULL" << endl;
    exit(-1);
  }
  if (gDirectory) // check if this pointer exists
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
  // This means we can't change the plots as we go, maybe OK?
  if (histos_init == false) {
    InitPlots(simpev);
    bookHistograms(simpev);
  }
  unsigned int clkpersec = 48000000*8;
  for(auto &e: m_graphMap){
    double value;
    std::string str = e.first;
    if(simpev.getSlow_para(str, value)){
      TGraph *tg=e.second;
      std::lock_guard<std::mutex> lck(m_mu);
      tg->SetPoint(tg->GetN(), simpev.getEvent_timestamp()/clkpersec, value);
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
