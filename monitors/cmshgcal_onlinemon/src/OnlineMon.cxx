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

//ONLINE MONITOR Includes
#include "OnlineMon.hh"
#include "eudaq/Configuration.hh"

using namespace std;
// Enable this for debug options:
#ifndef DEBUG
#define DEBUG
#endif

RootMonitor::RootMonitor(const std::string & runcontrol, const std::string & datafile, int /*x*/, int /*y*/, int /*w*/,
			 int /*h*/, int argc, int offline, const unsigned lim, const unsigned skip_, const unsigned int skip_with_counter, const std::string & conffile)
  : eudaq::Holder<int>(argc), eudaq::Monitor("OnlineMon", runcontrol, lim, skip_, skip_with_counter, datafile), _offline(offline), _planesInitialized(false) {

  if (_offline <= 0)
  {
    onlinemon = new OnlineMonWindow(gClient->GetRoot(),800,600);
    if (onlinemon==NULL)
    {
      cerr<< "Error Allocationg OnlineMonWindow"<<endl;
      exit(-1);
    }
  }

  hexaCollection = new HexagonCollection();
  hexaCorrelationCollection = new HexagonCorrelationCollection();
  ahcalCollection = new AhcalCollection();
  wcCollection = new WireChamberCollection();
  wccorrCollection = new WireChamberCorrelationCollection();
  beamTelescopeHitCollection = new HitmapCollection();
  beamTelescopeCorrCollection = new CorrelationCollection();
  daturaToHGCALCorrelationCollection = new DATURAToHGCALCorrelationCollection();
  dwcToHGCALCorrelationCollection = new DWCToHGCALCorrelationCollection();

  cout << "--- Done ---"<<endl<<endl;

  // put collections into the vector
  _colls.push_back(hexaCollection);
  _colls.push_back(hexaCorrelationCollection);
  _colls.push_back(ahcalCollection);
  _colls.push_back(wcCollection);
  _colls.push_back(wccorrCollection);
  _colls.push_back(beamTelescopeHitCollection);
  _colls.push_back(beamTelescopeCorrCollection);
  _colls.push_back(daturaToHGCALCorrelationCollection);
  _colls.push_back(dwcToHGCALCorrelationCollection);
  
  // set the root Monitor
  if (_offline <= 0) {
    hexaCollection->setRootMonitor(this);
    hexaCorrelationCollection->setRootMonitor(this);
    ahcalCollection->setRootMonitor(this);
    wcCollection->setRootMonitor(this);
    wccorrCollection->setRootMonitor(this);
    beamTelescopeHitCollection->setRootMonitor(this);
    beamTelescopeCorrCollection->setRootMonitor(this);
    daturaToHGCALCorrelationCollection->setRootMonitor(this);
    dwcToHGCALCorrelationCollection->setRootMonitor(this);

    onlinemon->setCollections(_colls);
  }

  //initialize with default configuration
  mon_configdata.SetDefaults();
  configfilename.assign(conffile);

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


  if (gStyle!=NULL)
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
  
  // print the configuration
  //mon_configdata.PrintConfiguration();


  cout << "Datafile: " << datafile << endl;
  if (datafile != "") {
    cout << "Calling program from file" << endl;
    size_t first = datafile.find_last_of("/")+1;
    //extract filename
    string filename=datafile.substr(first,datafile.length());
    char out[1024] = "";
    char num[1024] = "";

    if (string::npos!=filename.find_last_of(".raw"))
    {
      filename.copy(num,filename.length()-filename.find_last_of(".raw"),0);
    }
    else
    {
      filename.copy(num,filename.length(),0);
    }
    filename=filename+".root";
    filename.copy(out,filename.length(),0);
    int n=atoi(num);


    if (_offline <= 0)
    {
      onlinemon->setRunNumber(n);
    }

    cout << " \n \t ** ROOT output filename is: " << out << endl;
    if (_offline <= 0)
    {
      onlinemon->setRootFileName(out);
    }
    rootfilename = out;
    if (_offline > 0) {
      _checkEOF.setCollections(_colls);
      _checkEOF.setRootFileName(out);
      _checkEOF.startChecking(10000);
    }
  }

  m_tot_time = 0;

  //set a few defaults
  snapshotdir=mon_configdata.getSnapShotDir();
  previous_event_analysis_time=0;
  previous_event_fill_time=0;
  previous_event_clustering_time=0;
  previous_event_correlation_time=0;

  onlinemon->SetOnlineMon(this);

  cout << "End of Constructor" << endl;
}



void RootMonitor::setReduce(const unsigned int red) {
  if (_offline <= 0) onlinemon->setReduce(red);
  for (unsigned int i = 0 ; i < _colls.size(); ++i)
  {
    _colls.at(i)->setReduce(red);
  }
}

void RootMonitor::OnEvent(const eudaq::StandardEvent & ev) {
  cout << "\t Called RootMonitor::onEvent " << ev.GetEventNumber()<< endl;
  //cout << "Number of Planes " << ev.NumPlanes()<< endl;

  //start timing to measure processing time
  my_event_processing_time.Start(true);

  _checkEOF.EventReceived();

  bool reduce=false; //do we use Event reduction
  bool skip_dodgy_event=false; // do we skip this event because we consider it dodgy

  if (_offline > 0) //are we in offline mode , activated with -o
  {
    if (_offline < (int)ev.GetEventNumber())
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
      std::cout << "Initialized geometry: " << num << " planes." << std::endl;
    }
    else {
      if (myevent.getNPlanes()!=num) {

        cout << "Plane Mismatch on " <<ev.GetEventNumber()<<endl;
        cout << "Current/Previous " <<num<<"/"<<myevent.getNPlanes()<<endl;
        skip_dodgy_event=true; //we may want to skip this FIXME
        ostringstream eudaq_warn_message;
        eudaq_warn_message << "Plane Mismatch in Event "<<ev.GetEventNumber() <<" "<<num<<"/"<<myevent.getNPlanes()
			   <<"  If this happens at the end of RUN - it's normal.";
        EUDAQ_LOG(WARN,(eudaq_warn_message.str()).c_str());

      }
      else {
        myevent.setNPlanes(num);
      }
    }


    
    if ((ev.GetEventNumber() == 1) && (_offline <0)) //only update Display, when GUI is active
      onlinemon->UpdateStatus("Getting data..");
    
    
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
      EUDAQ_SLEEP(1);
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

  m_tmp_time1 += my_event_processing_time.RealTime();

  
  myStopWatch.Stop();
  double tmp_ev_time = myStopWatch.RealTime();
  m_tmp_time2 += tmp_ev_time;
  std::cout<<"\t another per-event time: "<<tmp_ev_time<<std::endl;

  m_tot_time += m_tmp_time2;

  std::cout<<"\t Total time: "<<m_tot_time<<std::endl;

  myStopWatch.Start(true);

  const int nEvForTiming = 20;
  if (ev.GetEventNumber()%nEvForTiming==0){
    std::cout<<"\t Time per "<<nEvForTiming<<"  is: "<<m_tmp_time1 <<"  another t="<<m_tmp_time2<<std::endl;    
    m_tmp_time1=0;
    m_tmp_time2=0;
  }

}

void RootMonitor::autoReset(const bool reset) {
  //_autoReset = reset;
  if (_offline <= 0) onlinemon->setAutoReset(reset);

}

void RootMonitor::OnStopRun()
{
  Monitor::OnStopRun();

  if (_writeRoot)
  {
    TFile *f = new TFile(rootfilename.c_str(),"RECREATE");
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      _colls.at(i)->Write(f);

      //_colls.at()
    }
    f->Close();
  }
  onlinemon->UpdateStatus("Run stopped");
}


/* TODO: implement onconfigure in here:
virtual void RootMonitor::OnConfigure(const eudaq::Configuration &param) {
  if (mon_configdata.ReadConfigurationFile()!=0) {
    // reset defaults, as Config file is bad
    cerr <<" As Config file can't be found, re-applying hardcoded defaults"<<endl;
    mon_configdata.SetDefaults();
  }

  // print the configuration
  mon_configdata.PrintConfiguration();
}
*/

void RootMonitor::OnStartRun(unsigned param) {

  std::cout<<"Doing RootMonitor::OnStartRun() "<<std::endl;
  
  if (onlinemon->getAutoReset())
  {
    onlinemon->UpdateStatus("Resetting..");
    for (unsigned int i = 0 ; i < _colls.size(); ++i)
    {
      if (_colls.at(i) != NULL)
        _colls.at(i)->Reset();
    }
  }

  Monitor::OnStartRun(param); // This is eudaq-monitor thing
  
  std::cout << "\n In RootMonitor::OnStartRun(): Called on start run  RUN=" << param <<std::endl;
  onlinemon->UpdateStatus("Starting run..");
  char out[255];
  sprintf(out, "../data_root/run%d.root",param);
  rootfilename = std::string(out);
  runnumber = param;

  if (_offline <= 0) {
    onlinemon->setRunNumber(runnumber);
    onlinemon->setRootFileName(rootfilename);
  }

  // Reset the planes initializer on new run start:
  _planesInitialized = false;

  onlinemon->UpdateStatus("Running");

  myStopWatch.Start(true);
  
  SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
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
string RootMonitor::GetSnapShotDir()
{
  return snapshotdir;
}

int main(int argc, const char ** argv) {

  eudaq::OptionParser op("EUDAQ Root Monitor", "1.0", "A Monitor using root for gui and graphics");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> file (op, "f", "data-file", "", "filename",
      "A data file to load - setting this changes the default"
      " run control address to 'null://'");
  eudaq::Option<int>             x(op, "x", "left",    500, "pos");
  eudaq::Option<int>             y(op, "y", "top",       0, "pos");
  eudaq::Option<int>             w(op, "w", "width",  1400, "pos");
  eudaq::Option<int>             h(op, "g", "height",  700, "pos", "The initial position of the window");
  eudaq::Option<int>             reduce(op, "rd", "reduce",  1, "Reduce the number of events");
  eudaq::Option<unsigned>        limit(op, "n", "limit", 0, "Event number limit for analysis");
  eudaq::Option<unsigned>        skip_counter(op, "sc", "skip_count", 10, "Number of events to skip per every taken event");
  eudaq::Option<unsigned>        skipping(op, "s", "skip", 0, "Percentage of events to skip");
  eudaq::Option<unsigned>        corr_width(op, "cw", "corr_width",500, "Width of the track correlation window");
  eudaq::Option<unsigned>        corr_planes(op, "cp", "corr_planes",  5, "Minimum amount of planes for track reconstruction in the correlation");
  eudaq::Option<bool>            track_corr(op, "tc", "track_correlation", false, "Using (EXPERIMENTAL) track correlation(true) or cluster correlation(false)");
  eudaq::Option<int>             update(op, "u", "update",  500, "update every ms");
  eudaq::Option<int>             offline(op, "o", "offline",  0, "running is offlinemode - analyse until event <num>");
  eudaq::Option<std::string>     configfile(op, "c", "config_file"," ", "filename","Config file to use for onlinemon");
  eudaq::OptionFlag do_rootatend (op, "rf","root","Write out root-file after each run");
  eudaq::OptionFlag do_resetatend (op, "rs","reset","Reset Histograms when run stops");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    if (file.IsSet() && !rctrl.IsSet()) rctrl.SetValue("null://");
    if (gROOT!=NULL)
    {
    //  gROOT->Reset();
     // gROOT->SetStyle("Plain"); //$$ change
    }
    else
    {
      cout<<"Global gROOT Object not found" <<endl;
      exit(-1);
    }

    // start the GUI
    //    cout<< "DEBUG: LIMIT VALUE " << (unsigned)limit.Value();
    TApplication theApp("App", &argc, const_cast<char**>(argv),0,0);
    RootMonitor mon(rctrl.Value(), file.Value(), x.Value(), y.Value(),
		    w.Value(), h.Value(), argc, offline.Value(), limit.Value(),
		    skipping.Value(), skip_counter.Value(), configfile.Value());
    mon.setWriteRoot(do_rootatend.IsSet());
    mon.autoReset(do_resetatend.IsSet());
    mon.setReduce(reduce.Value());
    mon.setUpdate(update.Value());
    //mon.setCorr_width(corr_width.Value());
    //mon.setCorr_planes(corr_planes.Value());
    mon.setUseTrack_corr(track_corr.Value());

    cout <<"Monitor Settings:" <<endl;
    cout <<"Update Interval :" <<update.Value() <<" ms" <<endl;
    cout <<"Reduce Events   :" <<reduce.Value() <<endl;
    if (offline.Value() >0)
    {
      //cout <<"Offline Mode   :" <<"active" <<endl;
      //cout <<"Events         :" <<offline.Value()<<endl;
      cout <<"Offline Mode not supported"<<endl;
      exit(-1);
    }

    theApp.Run(); //execute
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}








