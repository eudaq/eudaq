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
#include <memory>


// PI Wrapper / Library includes:
#include "PIWrapper.h"

using namespace piwrapper;

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
  ~PIController();

private:
  std::unique_ptr<PIWrapper> wrapper;
  //PIWrapper wrapper(); 
  bool m_terminated;
  std::string m_name;
  char *m_hostname;
  int m_portnumber;
  char *m_axis1 = "1";
  char *m_axis2 = "2";
  char *m_axis3 = "3";
  char *m_axis4 = "4";
  double m_axis1max = 0.0;
  double m_axis2max = 0.0;
  double m_axis3max = 0.0;
  double m_axis4max = 0.0;
  double m_position1 = 0.0;
  double m_position2 = 0.0;
  double m_position3 = 0.0;
  double m_position4 = 0.0;
  double m_velocity1 = 0.0;
  double m_velocity2 = 0.0;
  double m_velocity3 = 0.0;
  double m_velocity4 = 0.0;
  double m_velocitymax = 10.0;

  //int m_pinnr;
  //unsigned m_waiting_time;
};
#endif /*PICONTROLLER_HH*/
