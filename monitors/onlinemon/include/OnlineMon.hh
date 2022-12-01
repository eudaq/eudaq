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
#include "eudaq/Monitor.hh"
#include "eudaq/Event.hh"
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
#include "SimpleStandardEvent.hh"
#include "OnlineMonConfiguration.hh"

// STL includes
#include <string>
#include <memory>

using namespace std;

class OnlineMonWindow;
class BaseCollection;

class RootMonitor : public eudaq::Monitor{
  RQ_OBJECT("RootMonitor")
public:
  RootMonitor(const std::string &runcontrol, 
	      int x, int y, int w, int h,
              const std::string &conffile = "", const std::string &monname = "");
  ~RootMonitor() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReceive(eudaq::EventSP) override;
  
  void autoReset(const bool reset);
  
  void setWriteRoot(const bool write);
  void setReduce(const unsigned int red);
  void setUpdate(const unsigned int up);
  void setCorr_width(const unsigned c_w);
  void setCorr_planes(const unsigned c_p);
  void setUseTrack_corr(const bool t_c);
  void setTracksPerEvent(const unsigned int tracks);
  void SetSnapShotDir(string s);

  bool getUseTrack_corr() const;
  unsigned int getTracksPerEvent() const;
  string GetSnapShotDir() const;
  OnlineMonWindow *getOnlineMon() const;
  OnlineMonConfiguration mon_configdata; // FIXME
  std::shared_ptr<eudaq::Configuration> eu_cfgPtr;
private:
  std::vector<BaseCollection *> _colls;
  OnlineMonWindow *onlinemon;
  std::string rootfilename;
  std::string configfilename;
  bool _writeRoot;
  bool _planesInitialized;
  HitmapCollection *hmCollection;
  CorrelationCollection *corrCollection;
  EUDAQMonitorCollection *eudaqCollection;
  ParaMonitorCollection *paraCollection;
  string snapshotdir;
  bool useTrackCorrelator;
  TStopwatch my_event_processing_time;
  TStopwatch my_event_inner_operations_time;
  double previous_event_analysis_time;
  double previous_event_fill_time;
  double previous_event_clustering_time;
  double previous_event_correlation_time;
  unsigned int tracksPerEvent;
  uint32_t m_plane_c;
  uint32_t m_ev_rec_n = 0;
};

#ifdef __CINT__
#pragma link C++ class RootMonitor - ;
#endif

#endif
