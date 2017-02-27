#ifndef PICONTROLLER_HH
#define PICONTROLLER_HH

// EUDAQ includes:
#include "eudaq/Controller.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// PI Wrapper / Library includes:
#include "PIWrapper.h"

// PI Stage Controller
class PIController : public eudaq::Controller {

public:
  PIController(const std::string &name, const std::string &runcontrol);
  virtual void OnInitialise(const eudaq::Configuration & init);
  virtual void OnConfigure(const eudaq::Configuration &config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void Loop();
  
private:
  bool m_terminated;
  std::string m_name;
  char *m_hostname;
  int m_portnumber;
  //int m_pinnr;
  //unsigned m_waiting_time;
};
#endif /*PICONTROLLER_HH*/
