#ifndef CALICEDATACOLLECTOR_HH
#define CALICEDATACOLLECTOR_HH

#include <iostream>
#include <thread>
#include <vector>

#include "eudaq/DataCollector.hh"
#include "eudaq/DetectorEvent.hh"

class CaliceDataCollector : public eudaq::DataCollector {
  public:
  CaliceDataCollector(const std::string & name, const std::string & runcontrol,
		    const std::string & listenaddress, const std::string & runnumberfile)
    : eudaq::DataCollector(name, runcontrol, listenaddress, runnumberfile),
      done(false)
  {}
    void OnConnect(const eudaq::ConnectionInfo & id) {
      DataCollector::OnConnect(id);
      std::cout << "Connect:    " << id << std::endl;
    }
    virtual void OnConfigure(const eudaq::Configuration & param);
    
    virtual void OnStartRun(unsigned param) {
      DataCollector::OnStartRun(param);
      SetStatus(eudaq::Status::LVL_OK);
    }
    //   virtual bool OnStartRun(unsigned param) {
    //     std::cout << "Start Run: " << param << std::endl;
    //     return false;
    //   }
    //   virtual bool OnStopRun() {
    //     std::cout << "Stop Run" << std::endl;
    //     return false;
    //   }
    virtual void OnTerminate() {
      std::cout << "Terminating" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
    }
    //   virtual void OnStatus() {
    //     DataCollector::OnStatus();
    //     //std::cout << "Status - " << m_status << std::endl;
    //   }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_WARN, "Just testing");
    }
    
    void SendEvent(const eudaq::DetectorEvent &ev);
    
    bool done;
private:
  void AcceptThread();
  static void AcceptThreadEntry(CaliceDataCollector *th){th->AcceptThread();}
  std::thread *_thread;
  int _listenPort;
  
  int _serverfd;
  std::vector<int> _connectfd;
};

extern CaliceDataCollector * g_dc;

#endif
