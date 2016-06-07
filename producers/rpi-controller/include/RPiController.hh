#ifndef RPICONTROLLER_HH
#define RPICONTROLLER_HH

// EUDAQ includes:
#include "eudaq/Controller.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// Raspberry Pi Controller
class RPiController : public eudaq::Controller {

public:
  RPiController(const std::string &name, const std::string &runcontrol);
  virtual void OnConfigure(const eudaq::Configuration &config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void Loop();
  
private:
  bool m_terminated;
  int m_pinnr;
  unsigned m_waiting_time;
  std::string m_name;
};
#endif /*RPICONTROLLER_HH*/
