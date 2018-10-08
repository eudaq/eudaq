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
using namespace std;

RootMonitor::RootMonitor(const std::string & runcontrol,
       int /*x*/, int /*y*/, int /*w*/, int /*h*/,
       int argc, int offline, const std::string & conffile, const std::string & monname)
  :eudaq::Monitor(monname, runcontrol), _offline(offline), _planesInitialized(false), onlinemon(NULL){
  if (_offline <= 0)
  {
    onlinemon = new OnlineMonWindow(gClient->GetRoot(),800,600);
    if (onlinemon==NULL)
    {
      cerr<< "Error Allocationg OnlineMonWindow"<<endl;
      exit(-1);
    }
  }

  // hmCollection = new HitmapCollection();
  // corrCollection = new CorrelationCollection();
  // MonitorPerformanceCollection *monCollection =new MonitorPerformanceCollection();
  // eudaqCollection = new EUDAQMonitorCollection();
  // paraCollection = new ParaMonitorCollection();

  hexaCollection = new HexagonCollection();
  // hexaCorrelationCollection = new HexagonCorrelationCollection();
  wcCollection = new WireChamberCollection();
  tdchitsCollection = new TDCHitsCollection();
  wccorrCollection = new WireChamberCorrelationCollection();
  // beamTelescopeHitCollection = new HitmapCollection();
  // beamTelescopeCorrCollection = new CorrelationCollection();
  // //dwcToHGCALCorrelationCollection = new DWCToHGCALCorrelationCollection();
  digitizerCollection = new DigitizerCollection();


  cmshgcalLayerSumCollection = new CMSHGCalLayerSumCollection();
  
  cout << "--- Done ---"<<endl<<endl;

  // put collections into the vector
  // _colls.push_back(hmCollection);
  // _colls.push_back(corrCollection);
  // _colls.push_back(monCollection);
  // _colls.push_back(eudaqCollection);
  // _colls.push_back(paraCollection);
  _colls.push_back(hexaCollection);
  // _colls.push_back(hexaCorrelationCollection);
  _colls.push_back(wcCollection);
  //_colls.push_back(cmshgcalLayerSumCollection);
  _colls.push_back(tdchitsCollection);
  _colls.push_back(wccorrCollection);
  //_colls.push_back(beamTelescopeHitCollection);
  //_colls.push_back(beamTelescopeCorrCollection);
  //_colls.push_back(dwcToHGCALCorrelationCollection);
   _colls.push_back(digitizerCollection);
 
  // set the root Monitor
  if (_offline <= 0) {
    hexaCollection->setRootMonitor(this);
    // hexaCorrelationCollection->setRootMonitor(this);
    wcCollection->setRootMonitor(this);
    // cmshgcalLayerSumCollection->setRootMonitor(this);
    tdchitsCollection->setRootMonitor(this);
    wccorrCollection->setRootMonitor(this);
    // beamTelescopeHitCollection->setRootMonitor(this);
    // beamTelescopeCorrCollection->setRootMonitor(this);
    // //dwcToHGCALCorrelationCollection->setRootMonitor(this);
    digitizerCollection->setRootMonitor(this);
    // hmCollection->setRootMonitor(this);
    // corrCollection->setRootMonitor(this);
    // monCollection->setRootMonitor(this);
    // eudaqCollection->setRootMonitor(this);
    // paraCollection->setRootMonitor(this);

    onlinemon->setCollections(_colls);
  }

  //initialize with default configuration
  mon_configdata.SetDefaults();
  configfilename.assign(conffile);

  std::cout << "Config file for DQM : " << configfilename << std::endl;

  if (configfilename.length()>1)
  {
    mon_configdata.setConfigurationFileName(configfilename);
    if (mon_configdata.ReadConfigurationFile()!=0)
    {
      // reset defaults, as Config file is bad
      cerr <<" As Config file can't be found, re-applying hardcoded defaults"<<endl;
      mon_configdata.SetDefaults();

    }
  }


  // print the configuration
  mon_configdata.PrintConfiguration();

  if(gStyle!=NULL)
    {
      gStyle->SetPalette(mon_configdata.getDqmColorMap());
      gStyle->SetNumberContours(50);
      gStyle->SetOptStat(0);
      //gStyle->SetStatH(static_cast<Float_t>(0.15));
    }
  else
    {
      cout<<"Global gStyle Object not found" <<endl;
      exit(-1);
    }
  
  hexaCollection->SetRunMode(mon_configdata.getRunMode());

  cout << "End of Constructor" << endl;

  //set a few defaults
  snapshotdir=mon_configdata.getSnapShotDir();
  previous_event_analysis_time=0;
  previous_event_fill_time=0;
  previous_event_clustering_time=0;
  previous_event_correlation_time=0;

  onlinemon->SetOnlineMon(this);    

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

// void RootMonitor::setCorr_width(const unsigned c_w) {
//   corrCollection->setWindowWidthForCorrelation(c_w);
// }
// void RootMonitor::setCorr_planes(const unsigned c_p) {
//   corrCollection->setPlanesNumberForCorrelation(c_p);
// }

void RootMonitor::setReduce(const unsigned int red) {
  if (_offline <= 0) onlinemon->setReduce(red);
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
  auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(evsp);
  if(!stdev){
    stdev = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(evsp, stdev, nullptr); //no conf
  }
  
  auto &ev = *(stdev.get());
  while(_offline <= 0 && onlinemon==NULL){
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
    
#ifdef DEBUG
  cout << "Called onEvent " << ev.GetEventNumber()<< endl;
  cout << "Number of Planes " << ev.NumPlanes()<< endl;
#endif
  _checkEOF.EventReceived();

  //    cout << "Called onEvent " << ev.GetEventNumber()<< endl;
  //start timing to measure processing time
  my_event_processing_time.Start(true);

  bool reduce=false; //do we use Event reduction
  bool skip_dodgy_event=false; // do we skip this event because we consider it dodgy

  if (_offline > 0) //are we in offline mode , activated with -o
  {
    if (_offline <(int)  ev.GetEventNumber())
    {
      TFile *f = new TFile(rootfilename.c_str(),"RECREATE");
      if (f!=NULL)
      {
        for (unsigned int i = 0 ; i < _colls.size(); ++i)
        {
          _colls.at(i)->Write(f);
        }
        f->Close();
      }
      else
      {
        cout<< "Can't open "<<rootfilename<<endl;
      }
      exit(0); // Kill the program
    }
    reduce = true;

  }
  else
  {
    reduce = (ev.GetEventNumber() % onlinemon->getReduce() == 0);
  }


  if (reduce)
  {
    unsigned int num = (unsigned int) ev.NumPlanes();
    // Initialize the geometry with the first event received:
    if(!_planesInitialized) {
      myevent.setNPlanes(num);
      std::cout << "Initialized geometry: " << std::dec << num << " planes." << std::endl;
    }
    else {
      if (myevent.getNPlanes()!=num) {

        cout << "Plane Mismatch on " <<ev.GetEventNumber()<<endl;
        cout << "Current/Previous " <<num<<"/"<<myevent.getNPlanes()<<endl;
        skip_dodgy_event=false; //we may want to skip this FIXME
        ostringstream eudaq_warn_message;
        eudaq_warn_message << "Plane Mismatch in Event "<<ev.GetEventNumber() <<" "<<num<<"/"<<myevent.getNPlanes();
        EUDAQ_LOG(WARN,(eudaq_warn_message.str()).c_str());
  _planesInitialized = false;
  num = (unsigned int) ev.NumPlanes();
  eudaq_warn_message << "Continuing and readjusting the number of planes to  " << num;
  myevent.setNPlanes(num);
      }
      else {
        myevent.setNPlanes(num);
      }
    }

    SimpleStandardEvent simpEv;
    if ((ev.GetEventNumber() == 1) && (_offline <0)) //only update Display, when GUI is active
    {
      onlinemon->UpdateStatus("Getting data..");
    }
    // store the processing time of the previous EVENT, as we can't track this during the  processing
    simpEv.setMonitor_eventanalysistime(previous_event_analysis_time);
    simpEv.setMonitor_eventfilltime(previous_event_fill_time);
    simpEv.setMonitor_eventclusteringtime(previous_event_clustering_time);
    simpEv.setMonitor_eventcorrelationtime(previous_event_correlation_time);
    // add some info into the simple event header
    simpEv.setEvent_number(ev.GetEventNumber());
    simpEv.setEvent_timestamp(ev.GetTimestampBegin());
    
    std::string tagname;
    tagname = "Temperature";
    if(ev.HasTag(tagname)){
      double val;
      val = ev.GetTag(tagname, val);
      simpEv.setSlow_para(tagname,val);
    }
    tagname = "Voltage";
    if(ev.HasTag(tagname)){
      double val;
      val = ev.GetTag(tagname, val);
      simpEv.setSlow_para(tagname,val);
    }

    // std::vector<std::string> paralist = ev.GetTagList("PLOT_");
    // for(auto &e: paralist){
    //   double val ;
    //   val=ev.GetTag(e, val);
    //   simpEv.setSlow_para(e,val);
    // }
    
    if (skip_dodgy_event)
      return; //don't process any further

    my_event_inner_operations_time.Start(true);
    // Don't do clustering for hexaboard. This would not make any sense
    my_event_inner_operations_time.Stop();
    previous_event_clustering_time = my_event_inner_operations_time.RealTime();
    
    if(!_planesInitialized)
      {
#ifdef DEBUG
  cout << "Waiting for booking of Histograms..." << endl;
#endif
  std::this_thread::sleep_for(std::chrono::seconds(1));
#ifdef DEBUG
  cout << "...long enough"<< endl;
#endif
  _planesInitialized = true;
      }

    //stop the Stop watch
    my_event_processing_time.Stop();
#ifdef DEBUG
    cout << "\t RootMonitor::OnEvent(). Analysing time: "<< my_event_processing_time.RealTime()<<endl;
#endif
    previous_event_analysis_time=my_event_processing_time.RealTime();

    //Filling
    my_event_processing_time.Start(true); //start the stopwatch again
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
      {

  _colls.at(i)->Fill(ev, ev.GetEventNumber());

  // CollType is used to check which kind of Collection we are having
  if (_colls.at(i)->getCollectionType()==HITMAP_COLLECTION_TYPE) // Calculate is only implemented for HitMapCollections
    {
      _colls.at(i)->Calculate(ev.GetEventNumber());
    }

      }
    if (_offline <= 0)
      {
  onlinemon->setEventNumber(ev.GetEventNumber());
  onlinemon->increaseAnalysedEventsCounter();
      }
  } // end of reduce if
  my_event_processing_time.Stop();
#ifdef DEBUG
  cout << "\t RootMonitor::OnEvent(). Filling time: "<< my_event_processing_time.RealTime()<<endl;
  cout << "----------------------------------------"  <<endl<<endl;
#endif
  previous_event_fill_time=my_event_processing_time.RealTime();

  if (ev.IsBORE())
    {
      std::cout << "This is a BORE" << std::endl;
    }

}

void RootMonitor::autoReset(const bool reset) {
  //_autoReset = reset;
  if (_offline <= 0) onlinemon->setAutoReset(reset);

}

void RootMonitor::DoStopRun()
{
  while(_offline <= 0 && onlinemon==NULL){
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
  uint32_t param = GetRunNumber();
  while(_offline <= 0 && onlinemon==NULL){
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

  std::cout << "Called on start run" << param <<std::endl;
  onlinemon->UpdateStatus("Starting run..");
  char out[255];
  sprintf(out, "data_root/dqm_run%d.root",param);
  rootfilename = std::string(out);
  runnumber = param;

  if (_offline <= 0) {
    onlinemon->setRunNumber(runnumber);
    onlinemon->setRootFileName(rootfilename);
  }

  // Reset the planes initializer on new run start:
  _planesInitialized = false;
}

void RootMonitor::setUpdate(const unsigned int up) {
  if (_offline <= 0) onlinemon->setUpdate(up);
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
  eudaq::Option<std::string>     configfile(op, "c", "config_file"," ", "filename","Config file to use for onlinemon");
  eudaq::Option<std::string>     monitorname(op, "t", "monitor_name"," ", "StdEventMonitor","Name for onlinemon");  
  eudaq::OptionFlag do_rootatend (op, "rf","root","Write out root-file after each run");
  eudaq::OptionFlag do_resetatend (op, "rs","reset","Reset Histograms when run stops");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

    if (!rctrl.IsSet()) rctrl.SetValue("null://");
    TApplication theApp("App", &argc, const_cast<char**>(argv),0,0);
    RootMonitor mon(rctrl.Value(),
        x.Value(), y.Value(), w.Value(), h.Value(),
        argc, offline.Value(), configfile.Value(),monitorname.Value());
    mon.setWriteRoot(do_rootatend.IsSet());
    mon.autoReset(do_resetatend.IsSet());
    mon.setReduce(reduce.Value());
    mon.setUpdate(update.Value());
    // mon.setCorr_width(corr_width.Value());
    // mon.setCorr_planes(corr_planes.Value());
    mon.setUseTrack_corr(track_corr.Value());

    cout <<"Monitor Settings:" <<endl;
    cout <<"Update Interval :" <<update.Value() <<" ms" <<endl;
    cout <<"Reduce Events   :" <<reduce.Value() <<endl;
    //TODO: run cmd data thread
    eudaq::Monitor *m = dynamic_cast<eudaq::Monitor*>(&mon);
    m->Connect();
    theApp.Run(); //execute
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
