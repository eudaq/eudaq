#ifndef ONLINE_MON_H
#define ONLINE_MON_H

// ROOT includes
#include <TSystem.h>
#include <TInterpreter.h>
#include <TQObject.h>
#include <RQ_OBJECT.h>
#include <TPRegexp.h>
#include <TObjString.h>
#include <TStopwatch.h>

// EUDAQ includes
#ifndef __CINT__
#include "eudaq/Configuration.hh"
#include "eudaq/Monitor.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/TLUEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#endif


#include "HitmapCollection.hh"
#include "CorrelationCollection.hh"
#include "MonitorPerformanceCollection.hh"
#include "EUDAQMonitorCollection.hh"
#include "ParaMonitorCollection.hh"

#include "OnlineMonWindow.hh"
//#include "OnlineHistograms.hh"
#include "SimpleStandardEvent.hh"
#include "EventSanityChecker.hh"
#include "OnlineMonConfiguration.hh"

#include "CheckEOF.hh"

// STL includes
#include <string>
#include <memory>

#ifdef WIN32
#define EUDAQ_SLEEP(x) Sleep(x * 1000)
#else
#define EUDAQ_SLEEP(x) sleep(x)
#endif

using namespace std;

class OnlineMonWindow;
class BaseCollection;
class CheckEOF;

class RootMonitor : private eudaq::Holder<int>,
                    // public TApplication,
                    // public TGMainFrame,
                    public eudaq::Monitor {
  RQ_OBJECT("RootMonitor")
protected:
  bool histos_booked;

  std::vector<BaseCollection *> _colls;
  OnlineMonWindow *onlinemon;
  std::string rootfilename;
  std::string configfilename;
  int runnumber;
  bool _writeRoot;
  int _offline;
  CheckEOF _checkEOF;

  bool _planesInitialized;
  // bool _autoReset;

public:
  RootMonitor(const std::string &runcontrol, const std::string &datafile, int x,
              int y, int w, int h, int argc, int offline, const unsigned lim,
              const unsigned skip_, const unsigned int skip_with_counter,
              const std::string &conffile = "");
  ~RootMonitor() { gApplication->Terminate(); }
  void registerSensorInGUI(std::string name, int id);
  HitmapCollection *hmCollection;
  CorrelationCollection *corrCollection;
  EUDAQMonitorCollection *eudaqCollection;
  ParaMonitorCollection *paraCollection;

  virtual void StartIdleing() {}
  OnlineMonWindow *getOnlineMon() { return onlinemon; }

  virtual void OnConfigure(const eudaq::Configuration &param) {
    std::cout << "Configure: " << param.Name() << std::endl;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
  }
  virtual void OnTerminate() {
    std::cout << "Terminating" << std::endl;
    EUDAQ_SLEEP(1);
    gApplication->Terminate();
  }
  virtual void OnReset() {
    std::cout << "Reset" << std::endl;
    //SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
  }
  virtual void OnStartRun(unsigned param);
  virtual void OnEvent(const eudaq::StandardEvent &ev);

  virtual void OnBadEvent(std::shared_ptr<eudaq::Event> ev) {
    EUDAQ_ERROR("Bad event type found in data file");
    std::cout << "Bad Event: " << *ev << std::endl;
  }

  virtual void OnStopRun();
  void setWriteRoot(const bool write) { _writeRoot = write; }
  void autoReset(const bool reset);
  void setReduce(const unsigned int red);
  void setUpdate(const unsigned int up);
  void setCorr_width(const unsigned c_w) {
    corrCollection->setWindowWidthForCorrelation(c_w);
  }
  void setCorr_planes(const unsigned c_p) {
    corrCollection->setPlanesNumberForCorrelation(c_p);
  }
  void setUseTrack_corr(const bool t_c) { useTrackCorrelator = t_c; }
  bool getUseTrack_corr() const { return useTrackCorrelator; }
  void setTracksPerEvent(const unsigned int tracks) { tracksPerEvent = tracks; }
  unsigned int getTracksPerEvent() const { return tracksPerEvent; }

  void SetSnapShotDir(string s);
  string GetSnapShotDir();
  OnlineMonConfiguration mon_configdata; // FIXME
private:
  string snapshotdir;
  EventSanityChecker myevent; // FIXME
  bool useTrackCorrelator;
  TStopwatch my_event_processing_time;
  TStopwatch my_event_inner_operations_time;
  double previous_event_analysis_time;
  double previous_event_fill_time;
  double previous_event_clustering_time;
  double previous_event_correlation_time;
  unsigned int tracksPerEvent;
};

#ifdef __CINT__
#pragma link C++ class RootMonitor - ;
#endif

#endif
