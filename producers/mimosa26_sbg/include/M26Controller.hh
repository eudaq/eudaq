#include "eudaq/Utils.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>

#ifndef WIN32

#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int SOCKET;
#endif

#include <mutex>

#include "iphc_run_ctrl_exp.h"

using eudaq::to_string;

// the port client will be connecting to
//#define PORT_CONFIG 49248
//#define PORT_DATATRANSF 49250
// max number of bytes we can get at once
//#define MAXDATASIZE 300
//#define MAXHOSTNAME 80

//#define START 0x1254
//#define STOP 0x1255

class M26Controller {

public:
  M26Controller();
  virtual ~M26Controller();
  //void Configure(const eudaq::Configuration &conf);
  //void GetProduserHostInfo();
  //void Start();
  //void Stop();
  //void TagsSetting();
  //void DatatransportClientSocket_Open(const eudaq::Configuration &conf);
  //void DatatransportClientSocket_Close();
  //unsigned int DataTransportClientSocket_ReadLength(const char string[4]);
  //std::vector<unsigned char> DataTransportClientSocket_ReadData(int datalength);

  //void ConfigClientSocket_Open(const eudaq::Configuration &conf);
  //void ConfigClientSocket_Close();
  //void ConfigClientSocket_Send(unsigned char *text, size_t len);
  //unsigned int ConfigClientSocket_ReadLength(const char string[4]);
  //std::vector<unsigned char> ConfigClientSocket_ReadData(int datalength);

  // -------------- SBG integration
  void Connect(const eudaq::Configuration &param);
  void Init(const eudaq::Configuration &param);
  void LoadFW(const eudaq::Configuration &param);
  void UnLoadFW();
  void JTAG_Reset();
  void JTAG_Load(const eudaq::Configuration &param);
  void JTAG_Start();
  void Configure_Run(const eudaq::Configuration &param);
  void UnConfigure_Run();
  void Start_Run();
  void Stop_Run();
  void Status_Run(); // FIXME maybe not feasible with current FSM
  void Quit();


private:
  //struct hostent *hclient, *hconfig, *hdatatransport;
  //struct sockaddr_in client;
  //struct sockaddr_in config;
  //struct sockaddr_in datatransport;

  //unsigned char conf_parameters[10];

  //struct timeval tv;
  //unsigned int sec;
  //unsigned int microsec;

  //char ThisHost[80];

  //char Buffer_data[7000];
  //char Buffer_length[7000];

  //uint32_t data_trans_addres; // = INADDR_NONE;
  //SOCKET sock_config;
  //SOCKET sock_datatransport;
  //int numbytes;

  // these are needed from the JTAG files, other go the private section of the producer
  std::string m_detectorType;
  unsigned m_numDetectors;
  std::string m_JTAG_file;

  // NiIPaddr;
  //unsigned TriggerType;
  //unsigned Mode;
  //unsigned NiVersion;
  //unsigned FPGADownload;
  //unsigned MimosaID[6];
  //unsigned MimosaEn[6];
  //bool OneFrame;

  // -------------- SBG integration
  //SInt8 APP_VGErrFileLogLvl = 1;
  SInt8 APP_VGErrUserLogLvl = 2;
  //SInt8 APP_VGMsgFileLogLvl = 127;
  SInt8 APP_VGMsgUserLogLvl = 127;

  char * APP_ERR_LOG_FILE = "C:/opt/eudaq/logs/IRC_err.txt";
  char * APP_MSG_LOG_FILE = "C:/opt/eudaq/logs/IRC_msgs.txt";

  SInt32 DaqAnswer_CmdReceived;
  SInt32 DaqAnswer_CmdExecuted;

  // Add one mutex to be able to protect calls
  std::mutex m_mutex;

};

