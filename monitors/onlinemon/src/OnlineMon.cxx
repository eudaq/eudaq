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
    std::cerr<< "Error Allocationg OnlineMonWindow"<<endl;
    exit(-1);
  }

  m_plane_c = 0;
  m_ev_rec_n = 0;
  
  hmCollection = new HitmapCollection();
  corrCollection = new CorrelationCollection();
  MonitorPerformanceCollection *monCollection =new MonitorPerformanceCollection();
  eudaqCollection = new EUDAQMonitorCollection();
  paraCollection = new ParaMonitorCollection();

  // put collections into the vector
  _colls.push_back(hmCollection);
  _colls.push_back(corrCollection);
  _colls.push_back(monCollection);
  _colls.push_back(eudaqCollection);
  _colls.push_back(paraCollection);

  // set the root Monitor
  hmCollection->setRootMonitor(this);
  corrCollection->setRootMonitor(this);
  monCollection->setRootMonitor(this);
  eudaqCollection->setRootMonitor(this);
  paraCollection->setRootMonitor(this);

  onlinemon->setCollections(_colls);

  // Config for converters
  eu_cfgPtr = eudaq::Configuration::MakeUniqueReadFile(conffile);

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

void RootMonitor::setCorr_width(const unsigned c_w) {
  corrCollection->setWindowWidthForCorrelation(c_w);
}
void RootMonitor::setCorr_planes(const unsigned c_p) {
  corrCollection->setPlanesNumberForCorrelation(c_p);
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
    eudaq::StdEventConverter::Convert(evsp, stdev, eu_cfgPtr);
  }
  
  uint32_t ev_plane_c = stdev->NumPlanes();
  if(m_ev_rec_n < 10){
    m_ev_rec_n ++;
    if(ev_plane_c > m_plane_c){
      m_plane_c = ev_plane_c;
    }
    return;
  }

  if(ev_plane_c != m_plane_c){
    std::cout<< "Event #"<< evsp->GetEventN()<< " has "<<ev_plane_c<<" plane(s), while we expect "<< m_plane_c <<" plane(s).  (Event is skipped)" <<std::endl;
    return;
  }
    
  my_event_processing_time.Start(true);

  uint32_t num = stdev->NumPlanes();

  SimpleStandardEvent simpEv;
  // store the processing time of the previous EVENT, as we can't track this during the  processing
  simpEv.setMonitor_eventanalysistime(previous_event_analysis_time);
  simpEv.setMonitor_eventfilltime(previous_event_fill_time);
  simpEv.setMonitor_eventclusteringtime(previous_event_clustering_time);
  simpEv.setMonitor_eventcorrelationtime(previous_event_correlation_time);
  // add some info into the simple event header
  simpEv.setEvent_number(stdev->GetEventNumber());
  simpEv.setEvent_timestamp(stdev->GetTimestampBegin());
    
  for (unsigned int i = 0; i < num;i++){
    const eudaq::StandardPlane & plane = stdev->GetPlane(i);
    
    string sensorname;
    if ((plane.Type() == std::string("DEPFET")) &&(plane.Sensor().length()==0)){ // FIXME ugly hack for the DEPFET
      sensorname=plane.Type();
    }
    else{
      sensorname=plane.Sensor();
    }
    // DEAL with Fortis ...
    if (strcmp(plane.Sensor().c_str(), "FORTIS") == 0 ){
      continue;
    }
    SimpleStandardPlane simpPlane(sensorname,plane.ID(),plane.XSize(),plane.YSize(),&mon_configdata);
    for (unsigned int lvl1 = 0; lvl1 < plane.NumFrames(); lvl1++){
      for (unsigned int index = 0; index < plane.HitPixels(lvl1);index++){
        SimpleStandardHit hit((int)plane.GetX(index,lvl1),(int)plane.GetY(index,lvl1));
        hit.setTOT((int)plane.GetPixel(index,lvl1)); //this stores the analog information if existent, else it stores 1
        hit.setLVL1(lvl1);
          
        if (simpPlane.getAnalogPixelType()){ //this is analog pixel, apply threshold
          //this should be moved into converter
          if (simpPlane.is_DEPFET){
            if ((hit.getTOT()< -20) || (hit.getTOT()>120)){
              continue;
            }
          }
          if (simpPlane.is_EXPLORER){
            if (lvl1!=0) continue;
            hit.setTOT((int)plane.GetPixel(index));
            if (hit.getTOT() < 20){
              continue;
            }
          }
          if (simpPlane.is_APTS){
            if ((hit.getTOT()<80)){ // TODO: make generic and configurable.
              continue;
            }
          }
          if (simpPlane.is_OPAMP){
            if ((hit.getTOT()<80)){ // TODO: make generic and configurable.
              continue;
            }
          }
          simpPlane.addHit(hit);
        }
        else{ //purely digital pixel
          simpPlane.addHit(hit);
        }          
      }
    }
    simpEv.addPlane(simpPlane);
  }
  my_event_inner_operations_time.Start(true);
  simpEv.doClustering();
  my_event_inner_operations_time.Stop();
  previous_event_clustering_time = my_event_inner_operations_time.RealTime();

  if(!_planesInitialized){
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
      if (_colls.at(i) == corrCollection)
        {
          my_event_inner_operations_time.Start(true);
          if (getUseTrack_corr() == true)
            {
              tracksPerEvent = corrCollection->FillWithTracks(simpEv);
              if (eudaqCollection->getEUDAQMonitorHistos() != NULL) //workaround because Correlation Collection is before EUDAQ Mon collection
                eudaqCollection->getEUDAQMonitorHistos()->Fill(simpEv.getEvent_number(), tracksPerEvent);
            }
          else
            _colls.at(i)->Fill(simpEv);
          my_event_inner_operations_time.Stop();
          previous_event_correlation_time = my_event_inner_operations_time.RealTime();
        }
      else
        _colls.at(i)->Fill(simpEv);

      // CollType is used to check which kind of Collection we are having
      if (_colls.at(i)->getCollectionType()==HITMAP_COLLECTION_TYPE) // Calculate is only implemented for HitMapCollections
        {
          _colls.at(i)->Calculate(stdev->GetEventNumber());
        }
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

  if (onlinemon->getAutoReset())
  {
    onlinemon->UpdateStatus("Resetting..");
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      if (_colls.at(i) != NULL)
        _colls.at(i)->Reset();
    }
  }

  onlinemon->UpdateStatus("Starting run..");
  char out[255];
  sprintf(out, "run%d.root", runnumber);
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

uint64_t OfflineReading(eudaq::Monitor *mon, eudaq::FileReaderSP reader, uint32_t ev_n_l, uint32_t ev_n_h, uint32_t ev_c_max){
  // DoConfigure(); //TODO setup the configure and init file.
  mon->DoStartRun();
  uint32_t ev_c = 0;
  while(1){
    auto ev = std::const_pointer_cast<eudaq::Event>(reader->GetNextEvent());
    if(!ev){
      std::cout<<"end of data file with "<< ev_c << " events" <<std::endl;
      break;
    }
    uint32_t ev_n = ev->GetEventN();
    if(ev_n>=ev_n_l & ev_n<=ev_n_h){
      mon->DoReceive(ev);
      ev_c ++;
      if(ev_c > ev_c_max){
	std::cout<<"reach to event count "<< ev_c<<std::endl;
	break;
      }
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
  eudaq::Option<int>             reduce(op, "rd", "reduce",  1, "Reduce the number of events");
  eudaq::Option<unsigned>        corr_width(op, "cw", "corr_width",500, "Width of the track correlation window");
  eudaq::Option<unsigned>        corr_planes(op, "cp", "corr_planes",  5, "Minimum amount of planes for track reconstruction in the correlation");
  eudaq::Option<bool>            track_corr(op, "tc", "track_correlation", false, "Using (EXPERIMENTAL) track correlation(true) or cluster correlation(false)");
  eudaq::Option<int>             update(op, "u", "update",  1000, "update every ms");
  eudaq::Option<uint32_t>        event_id_low(op, "e", "event_id_low",  0, "running is offlinemode - analyse begin event id <num>");
  eudaq::Option<uint32_t>        event_id_high(op, "E", "event_id_high", 0xffffffff, "running is offlinemode - analyse until event id <num>");
  eudaq::Option<uint32_t>        event_amount_max(op, "ea", "event_amount_max", 0xffffffff, "running is offlinemode - analyse until reach events amount");
  
  eudaq::Option<std::string>     datafile(op, "d", "datafile",  " ", "offline mode data file");
  eudaq::Option<std::string>     configfile(op, "c", "config_file"," ", "filename","Config file to use for onlinemon");
  eudaq::Option<std::string>     monitorname(op, "t", "monitor_name","StdEventMonitor", "StdEventMonitor","Name for onlinemon");	
  eudaq::OptionFlag do_rootatend (op, "rf","root","Write out root-file after each run");
  eudaq::OptionFlag do_resetatend (op, "rs","reset","Reset Histograms when run stops");
  
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
  } catch (...) {
    return op.HandleMainException();
  }

  bool offline = datafile.Value()!="" && datafile.Value()!=" "; 

  if(offline){
    rctrl.SetValue("null://");
  }
  if(!rctrl.IsSet())
    rctrl.SetValue("tcp://localhost:44000");
    
  TApplication theApp("App", &argc, const_cast<char**>(argv),0,0);
  RootMonitor mon(rctrl.Value(),
		  100, 0, 1400, 700,
                  configfile.Value(), monitorname.Value());
  mon.setWriteRoot(do_rootatend.IsSet());
  mon.autoReset(do_resetatend.IsSet());
  mon.setReduce(reduce.Value());
  mon.setUpdate(update.Value());
  mon.setCorr_width(corr_width.Value());
  mon.setCorr_planes(corr_planes.Value());
  mon.setUseTrack_corr(track_corr.Value());
  eudaq::Monitor *m = dynamic_cast<eudaq::Monitor*>(&mon);
  std::future<uint64_t> fut_async_rd;

  if(offline){
    std::string infile_path = datafile.Value(); 
    eudaq::FileReaderSP reader = eudaq::FileReader::Make("native", infile_path);
    if(!reader){
      std::cerr<<"OnlineMon:: ERROR, unable to access data file "<< infile_path<<std::endl;
      throw;
    }
    fut_async_rd = std::async(std::launch::async, &OfflineReading, &mon, reader, event_id_low.Value(), event_id_high.Value(), event_amount_max.Value() );
  }
  else{
    m->Connect();
  }

  theApp.Run(); //execute
  if(fut_async_rd.valid())
    fut_async_rd.get();
  return 0;
}
