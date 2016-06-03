#ifndef M26PRODUCER_HH
#define M26PRODUCER_HH

// EUDAQ includes
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"
//#include "eudaq/Utils.hh"
//#include "eudaq/Logger.hh"

// IRC DLL header
#include "iphc_run_ctrl_exp.h"

//system includes
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <mutex>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
//#include <netdb.h>
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//typedef int SOCKET;
#endif

using eudaq::to_string;

// M26Producer
class M26Producer : public eudaq::Producer {

public:
  M26Producer(const std::string &name, const std::string &runcontrol, const std::string &verbosity);
  //virtual ~M26Producer();
  virtual void OnConfigure(const eudaq::Configuration &config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();
 

private:
  void ReadDataFromShrdMemory();

  void reset_com();

  unsigned m_ev, m_run;
  std::string m_verbosity, m_producerName;
  bool m_terminated, m_configured, m_running, m_triggering, m_stopping;
  eudaq::Configuration m_config;

  // Add one mutex to be able to protect calls
  //std::shared_ptr<unsigned> dummy;
  std::mutex m_mutex;

  eudaq::Timer *m_reset_timer;

  // info from the config file
  std::string m_detectorType;
  unsigned m_detectorTypeCode;
  unsigned m_numDetectors;
  unsigned m_sumEnDetectors;
  std::string m_JTAG_file;
  std::vector<unsigned> m_MimosaID;
  std::vector<unsigned> m_MimosaEn;

  //SInt8 APP_VGErrFileLogLvl = 1;
  SInt8 APP_VGErrUserLogLvl = 2;
  //SInt8 APP_VGMsgFileLogLvl = 127;
  SInt8 APP_VGMsgUserLogLvl = 127;

  char * APP_ERR_LOG_FILE = "C:/opt/eudaq/logs/IRC_err.txt";
  char * APP_MSG_LOG_FILE = "C:/opt/eudaq/logs/IRC_msgs.txt";

  // For IRC DLL communication
  SInt32 ret;
  SInt32 DaqAnswer_CmdReceived;
  SInt32 DaqAnswer_CmdExecuted;
  SInt32 VLastCmdError;
  SInt32 CmdNumber;
  std::string cmd;

};

#endif /* M26PRODUCER_HH */
