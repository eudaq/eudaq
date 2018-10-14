#ifdef WIN32
#include <Windows4Root.h>
#endif

// ROOT includes
#include "TROOT.h"
#include "TNamed.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TGMenu.h"
#include "TGTab.h"
#include "TGButton.h"
#include "TGComboBox.h"
#include "TGLabel.h"
#include "TGTextEntry.h"
#include "TGNumberEntry.h"
#include "TGComboBox.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TRootEmbeddedCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TPaletteAxis.h"
#include "TThread.h"
#include "TFile.h"
#include "TColor.h"
#include "TString.h"
#include "TF1.h"
//#include "TSystem.h" // for TProcessEventTimer
// C++ INCLUDES
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <memory>

//ONLINE MONITOR Includes
#include "OnlineMon.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include "eudaq/FileReader.hh"
using namespace std;

RootMonitor::RootMonitor(const std::string & runcontrol,
			 int /*x*/, int /*y*/, int /*w*/, int /*h*/,
			 const std::string & conffile, const std::string & monname)
  :eudaq::Monitor(monname, runcontrol), _planesInitialized(false), onlinemon(NULL){
  onlinemon = new OnlineMonWindow(gClient->GetRoot(),800,600);

  if (onlinemon==NULL){
    cerr<< "Error Allocationg OnlineMonWindow"<<endl;
    exit(-1);
  }

  m_plane_c = 0;
  m_ev_rec_n = 0;

  //MonitorPerformanceCollection *monCollection =new MonitorPerformanceCollection();

  hexaCollection = new HexagonCollection();
  wcCollection = new WireChamberCollection();
  tdchitsCollection = new TDCHitsCollection();
  //wccorrCollection = new WireChamberCorrelationCollection();
  //digitizerCollection = new DigitizerCollection();
  cmshgcalLayerSumCollection = new CMSHGCalLayerSumCollection();

  // put collections into the vector
  //_colls.push_back(monCollection);
  _colls.push_back(hexaCollection);
  _colls.push_back(wcCollection);
  _colls.push_back(tdchitsCollection);
  //_colls.push_back(wccorrCollection);
  //_colls.push_back(digitizerCollection);
  _colls.push_back(cmshgcalLayerSumCollection);
  
  //monCollection->setRootMonitor(this);
  
  hexaCollection->setRootMonitor(this);
  wcCollection->setRootMonitor(this);
  tdchitsCollection->setRootMonitor(this);
  //wccorrCollection->setRootMonitor(this);
  //digitizerCollection->setRootMonitor(this);
  cmshgcalLayerSumCollection->setRootMonitor(this);
  
  onlinemon->setCollections(_colls);

  //initialize with default configuration
  mon_configdata.SetDefaults();
  configfilename.assign(conffile);

  if (configfilename.length()>1){
    mon_configdata.setConfigurationFileName(configfilename);
    if (mon_configdata.ReadConfigurationFile()!=0){
      // reset defaults, as Config file is bad
      cerr <<" As Config file can't be found, re-applying hardcoded defaults"<<endl;
      mon_configdata.SetDefaults();
    }
  }
  // print the configuration
  mon_configdata.PrintConfiguration();

  if(gStyle!=NULL){
    gStyle->SetPalette(mon_configdata.getDqmColorMap());
    gStyle->SetNumberContours(50);
    gStyle->SetOptStat(0);
    //gStyle->SetStatH(static_cast<Float_t>(0.15));
  }
  else{
    cout<<"Global gStyle Object not found" <<endl;
    exit(-1);
  }
  
  hexaCollection->SetRunMode(mon_configdata.getRunMode());
  //set a few defaults
  snapshotdir=mon_configdata.getSnapShotDir();
  previous_event_analysis_time=0;
  previous_event_fill_time=0;
  previous_event_clustering_time=0;
  previous_event_correlation_time=0;

  onlinemon->SetOnlineMon(this);

  cout << "End of Constructor" << endl;
}

RootMonitor::~RootMonitor(){
  gApplication->Terminate();
}

OnlineMonWindow* RootMonitor::getOnlineMon() const {
  return onlinemon;
}

void RootMonitor::setWriteRoot(const bool write) {
  _writeRoot = write;
}

void RootMonitor::setReduce(const unsigned int red) {
  onlinemon->setReduce(red);
  for (unsigned int i = 0 ; i < _colls.size(); ++i)
  {
    _colls.at(i)->setReduce(red);
  }
}

void RootMonitor::setUseTrack_corr(const bool t_c) {
  useTrackCorrelator = t_c;
}

bool RootMonitor::getUseTrack_corr() const {
  return useTrackCorrelator;
}

void RootMonitor::setTracksPerEvent(const unsigned int tracks) {
  tracksPerEvent = tracks;
}

unsigned int RootMonitor::getTracksPerEvent() const {
  return tracksPerEvent;
}

void RootMonitor::DoConfigure(){
  auto &param = *GetConfiguration();
  std::cout << "Configure: " << param.Name() << std::endl;
}

void RootMonitor::DoTerminate(){
  gApplication->Terminate();
}

void RootMonitor::DoReceive(eudaq::EventSP evsp) {
  if(evsp->GetEventN() > 10 && evsp->GetEventN() % onlinemon->getReduce() != 0){
    return;
  }

  auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(evsp);

  if(!stdev){
    stdev = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(evsp, stdev, nullptr); //no conf
  }

  
  uint32_t ev_plane_c = stdev->NumPlanes();
  cout<<"EventN (evsp) = "<< evsp->GetEventN()<<"   Event (stdev) ="<<stdev->GetEventNumber()<<"  Event ID ="<<std::dec<<stdev->GetEventID()
      <<"  Number planes: "<<std::dec<<ev_plane_c<<endl;

  /*
  if(m_ev_rec_n < 10){
    m_ev_rec_n ++;
    if(ev_plane_c > m_plane_c){
      m_plane_c = ev_plane_c;
    }
    return;
  }
  */
  
  //if(ev_plane_c != m_plane_c){
  //std::cout<< "do nothing for this event at "<< evsp->GetEventN()<< ", ev_plane_c "<<ev_plane_c<<std::endl;
  //return;
  //}

  while(onlinemon==NULL){
    cout<<"We are sleeping here 0"<<endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  my_event_processing_time.Start(true);

  //unsigned int num = (unsigned int) stdev->NumPlanes();
  // Initialize the geometry with the first event received:
  //if(!_planesInitialized) {
  //myevent.setNPlanes(num);
  //std::cout << "Initialized geometry: " << num << " planes." << std::endl;
  //}
  //else {
  //if (myevent.getNPlanes()!=num) {
  //  cout << "Plane Mismatch on " <<stdev->GetEventNumber()<<endl;
  //    cout << "Current/Previous " <<num<<"/"<<myevent.getNPlanes()<<endl;
  //  ostringstream eudaq_warn_message;
  //  eudaq_warn_message << "Plane Mismatch in Event "<<stdev->GetEventNumber() <<" "<<num<<"/"<<myevent.getNPlanes();
  //  EUDAQ_LOG(WARN,(eudaq_warn_message.str()).c_str());
  //  _planesInitialized = false;
  //  num = (unsigned int) stdev->NumPlanes();
  //  eudaq_warn_message << "Continuing and readjusting the number of planes to  " << num;
  //  myevent.setNPlanes(num);
  //}
  //else {

  //  myevent.setNPlanes(num);
  //}
  //}

  
  if(!_planesInitialized){
    cout<<"We are sleeping here 1"<<endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    _planesInitialized = true;
  }


  //stop the Stop watch
  my_event_processing_time.Stop();
  previous_event_analysis_time=my_event_processing_time.RealTime();
  //Filling
  my_event_processing_time.Start(true); //start the stopwatch again
  for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      //cout<<"We are in the loop in Receive  "<<i<<endl;

      _colls.at(i)->Fill(*(stdev.get()), stdev->GetEventNumber());

    }

  onlinemon->setEventNumber(stdev->GetEventNumber());
  onlinemon->increaseAnalysedEventsCounter();

  my_event_processing_time.Stop();
  previous_event_fill_time=my_event_processing_time.RealTime();

  }

void RootMonitor::autoReset(const bool reset) {
  onlinemon->setAutoReset(reset);
}

void RootMonitor::DoStopRun()
{
  m_plane_c = 0;
  m_ev_rec_n = 0;
  while(onlinemon==NULL){
    cout<<"We are sleeping here 2"<<endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if (_writeRoot)
  {
    TFile *f = new TFile(rootfilename.c_str(),"RECREATE");
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      _colls.at(i)->Write(f);
    }
    f->Close();
  }
  onlinemon->UpdateStatus("Run stopped");
}

void RootMonitor::DoStartRun() {
  m_plane_c = 0;
  m_ev_rec_n = 0;
  uint32_t runnumber = GetRunNumber();
  while(onlinemon==NULL){
    cout<<"We are sleeping here 3"<<endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if (onlinemon->getAutoReset())
  {
    onlinemon->UpdateStatus("Resetting..");
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      if (_colls.at(i) != NULL)
        _colls.at(i)->Reset();
    }
  }

  std::cout << "Called on start run " << runnumber <<std::endl;
  onlinemon->UpdateStatus("Starting run..");

  char out[255];
  sprintf(out, "data_root/dqm_run%04d.root", runnumber);
  rootfilename = std::string(out);
  onlinemon->setRunNumber(runnumber);
  onlinemon->setRootFileName(rootfilename);
  

  // Reset the planes initializer on new run start:
  _planesInitialized = false;
}

void RootMonitor::setUpdate(const unsigned int up) {
  onlinemon->setUpdate(up);
}

//sets the location for the snapshots
void RootMonitor::SetSnapShotDir(string s)
{
  snapshotdir=s;
}


//gets the location for the snapshots
string RootMonitor::GetSnapShotDir()const{
  return snapshotdir;
}

uint64_t OfflineReading(eudaq::Monitor *mon, eudaq::FileReaderSP reader, uint32_t max_evc){
  // DoConfigure(); //TODO setup the configure and init file.
  mon->DoStartRun();

  uint32_t ev_c = 0;
  while(1){
    auto ev = std::const_pointer_cast<eudaq::Event>(reader->GetNextEvent());
    if(!ev){
      std::cout<<"End of data file with "<<std::dec<< ev_c << " events" <<std::endl;
      break;
    }
    mon->DoReceive(ev);
    ev_c ++;
    if(ev_c > max_evc && max_evc!=0){
      std::cout<<"Reach to event count "<< std::dec<<ev_c<<std::endl;
      std::cout<<"Stopping proccessing events in the Monitor"<<std::endl;
      break;
    }
  }
  mon->DoStopRun();
  return ev_c;
}

int main(int argc, const char ** argv) {

  eudaq::OptionParser op("EUDAQ Root Monitor", "1.0", "A Monitor using root for gui and graphics");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<int>             x(op, "x", "left",    100, "pos");
  eudaq::Option<int>             y(op, "y", "top",       0, "pos");
  eudaq::Option<int>             w(op, "w", "width",  1400, "pos");
  eudaq::Option<int>             h(op, "g", "height",  700, "pos", "The initial position of the window");
  eudaq::Option<int>             reduce(op, "rd", "reduce",  1, "Reduce the number of events");
  eudaq::Option<unsigned>        corr_width(op, "cw", "corr_width",500, "Width of the track correlation window");
  eudaq::Option<unsigned>        corr_planes(op, "cp", "corr_planes",  5, "Minimum amount of planes for track reconstruction in the correlation");
  eudaq::Option<bool>            track_corr(op, "tc", "track_correlation", false, "Using (EXPERIMENTAL) track correlation(true) or cluster correlation(false)");
  eudaq::Option<int>             update(op, "u", "update",  1000, "update every ms");
  eudaq::Option<int>             offline(op, "o", "offline",  0, "running is offlinemode - analyse until event <num>");
  eudaq::Option<std::string>     datafile(op, "f", "datafile",  "None", "offline mode data file");
  eudaq::Option<std::string>     configfile(op, "c", "config_file"," ", "filename","Config file to use for onlinemon");
  eudaq::Option<std::string>     monitorname(op, "t", "monitor_name"," ", "StdEventMonitor","Name for onlinemon");
  eudaq::OptionFlag do_rootatend (op, "rf","root","Write out root-file after each run");
  eudaq::OptionFlag do_resetatend (op, "rs","reset","Reset Histograms when run stops");
  
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
  } catch (...) {
    return op.HandleMainException();
  }

  if(datafile.Value().find("None")==std::string::npos){
    // If datafile is provided - run offline mode
    rctrl.SetValue("null://");
  }
  if(!rctrl.IsSet())
    rctrl.SetValue("tcp://localhost:44000");

  TApplication theApp("App", &argc, const_cast<char**>(argv),0,0);
  RootMonitor mon(rctrl.Value(),
                  x.Value(), y.Value(), w.Value(), h.Value(),
                  configfile.Value(), monitorname.Value());
  mon.setWriteRoot(do_rootatend.IsSet());
  mon.autoReset(do_resetatend.IsSet());
  mon.setReduce(reduce.Value());
  mon.setUpdate(update.Value());
  //mon.setCorr_width(corr_width.Value());
  //mon.setCorr_planes(corr_planes.Value());
  mon.setUseTrack_corr(track_corr.Value());
  eudaq::Monitor *m = dynamic_cast<eudaq::Monitor*>(&mon);
  std::future<uint64_t> fut_async_rd;
  if(datafile.Value().find("None")!=std::string::npos){
    m->Connect();
  }
  else{
    // If datafile is provided - run offline mode
    std::string infile_path = datafile.Value();
    eudaq::FileReaderSP reader = eudaq::FileReader::Make("native", infile_path);
    if(!reader){
      std::cerr<<"OnlineMon:: ERROR, unable to access data file "<< infile_path<<std::endl;
      throw;
    }
    fut_async_rd = std::async(std::launch::async, &OfflineReading, &mon, reader, offline.Value());
  }
  theApp.Run(); //execute
  if(fut_async_rd.valid())
    fut_async_rd.get();
  return 0;
}
