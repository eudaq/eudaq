#include "M26Controller.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/types.h>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define EUDAQ_BCOPY(source, dest, NSize) bcopy(source, dest, NSize)
#define EUDAQ_Sleep(x) sleep(x)
#define EUDAQ_CLOSE_SOCKET(x) close(x)
#else

#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")

#include "irc_exceptions.hh"

#define EUDAQ_Sleep(x) Sleep(x)
#define EUDAQ_BCOPY(source, dest, NSize) memmove(dest, source, NSize)

#define EUDAQ_CLOSE_SOCKET(x) closesocket(x)

#endif

int EUDAQ_SEND(SOCKET s, unsigned char *buf, int len, int flags) {

#ifdef WIN32
  // on windows this function takes a signed char ...
  // in prinziple I always distrust reinterpreter_cast but in this case it
  // should be save
  return send(s, reinterpret_cast<const char *>(buf), len, flags);

#else
  return send(s, buf, len, flags);
#endif
}

//unsigned char start[5] = "star"; // deprecated
//unsigned char stop[5] = "stop";

// -- Unfortunately, the interface is not a standard api that can be used to steer the hardware a la IRC::IRCsteering_object->DoFunction()
// Instead, you can call functions that create variables in some shared memory which "internally" steer the hardware

M26Controller::M26Controller() {
}

M26Controller::~M26Controller() {
}

// --- SBG integration
void M26Controller::Connect(const eudaq::Configuration &param) {

  std::cout << " Connecting to shared memory. " << std::endl;
  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    IRC__FBegin ( APP_VGErrUserLogLvl, APP_ERR_LOG_FILE, APP_VGMsgUserLogLvl, APP_MSG_LOG_FILE );
    IRC_RCBT2628__FRcBegin ();

  } catch (const std::exception &e) { 
    printf("while connecting: Caught exeption: %s\n", e.what());
    //SetStatus(eudaq::Status::LVL_ERROR, "Stop error"); // only the producer can SetStatus
  } catch (...) { 
    printf("while connecting: Caught unknown exeption:\n");
    //SetStatus(eudaq::Status::LVL_ERROR, "Stop error");
  }

}

void M26Controller::Init(const eudaq::Configuration &param) {

  std::cout << " Initialising. " << std::endl;
  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 SensorType = -1; // 0 = ASIC__NONE, 1 = ASIC__MI26, 2 = ASIC__ULT1 // FIXME get from config file
    std::string m_sensorType;
    m_sensorType = param.Get("Det", "");
    if(m_sensorType.compare("MIMOSA26") == 0) {
      SensorType = 1; // compare() returns 0 if equal
      std::cout << "  -- Found matching sensor. " << std::endl;
    }
    else throw irc::InvalidConfig("Invalid Sensor type!");

    ret = IRC_RCBT2628__FRcSendCmdInit ( SensorType, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );

    /* debug */ // std::cout << " SendCCmdInit = " << ret << std::endl;

    if(ret) EUDAQ_ERROR("Send CmdInit failed"); // FIXME make/add better error handling

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 5000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdInit failed"); // FIXME make/add better error handling

  } catch (irc::InvalidConfig &e) { 
    printf("while initialising: Caught exeption: %s\n", e.what());
    // m26 CONTROLLER IS NO PRODUCER -> DOESNT KNOW ABOUT EUdaq_ERROR // EUDAQ_ERROR(string("Invalid configuration settings: " + string(e.what())));
    throw; // propagating exception to caller
  } catch (const std::exception &e) { 
    printf("while initialising: Caught exeption: %s\n", e.what());
    throw;
  } catch (...) { 
    printf("while initialising: Caught unknown exeption:\n");
    throw;
  }


}

void M26Controller::LoadFW(const eudaq::Configuration &param) {

  std::cout << " LoadWF. " << std::endl;
  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = 777;

    ret = IRC_RCBT2628__FRcSendCmdFwLoad ( CmdNumber/*maybe*/, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) EUDAQ_ERROR("Send CmdFwLoad failed"); // FIXME make/add better error handling

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ ); // orig 10000

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdLoadFW failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while loading FW: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while loading FW: Caught unknown exeption:\n");
  }


}

void M26Controller::UnLoadFW() {
  // unload FW
  std::cout << " Unload FW " << std::endl;

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = -1; // not used

    ret = IRC_RCBT2628__FRcSendCmdFwUnload ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) EUDAQ_ERROR("Send CmdFwUnloadFW failed"); // FIXME make/add better error handling

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ ); // orig 10000

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdFwUnloadFW failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while unloading FW: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while unloading FW: Caught unknown exeption:\n");
  }


}

void M26Controller::JTAG_Reset() {

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    std::cout << " Send JTAG RESET to sensors " << std::endl;

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = 111; // not used

    ret = IRC_RCBT2628__FRcSendCmdJtagReset ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdJtagReset failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdJtagReset failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while JTAG reset: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while JTAG reset: Caught unknown exeption:\n");
  }


}


void M26Controller::JTAG_Load(const eudaq::Configuration & param) {
  // JTAG sensors via mcf file
  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    std::cout << " JTAG sensors " << std::endl;
    std::string m_jtag;
    m_jtag = param.Get("JTAG_file", "");

    /*debug*/ std::cout << "  -- JTAG file " << m_jtag << std::endl;

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = 666;

    ret = IRC_RCBT2628__FRcSendCmdJtagLoad ( CmdNumber, (char*)(m_jtag.c_str()), &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdJtagLoad failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdJtagLoad failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while JTAG load: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while JTAG load: Caught unknown exeption:\n");
  }


}

void M26Controller::JTAG_Start() {

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    std::cout << " Send JTAG START to sensors " << std::endl;

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = -1; // not used

    ret = IRC_RCBT2628__FRcSendCmdJtagStart ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdJtagStart failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdJtagStart failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while JTAG start: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while JTAG start: Caught unknown exeption:\n");
  }


}


void M26Controller::Configure_Run(const eudaq::Configuration & param) {
  // Configure Run

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    std::cout << " Configure Run " << std::endl;

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;
    IRC_RCBT2628__TCmdRunConf RunConf;
    RunConf.MapsName         = 1; // ASIC__MI26;
    RunConf.MapsNb           = 1;
    RunConf.RunNo            = 2; // FIXME If at all, must come from eudaq !! Finally, the controller does not need to know!!
    RunConf.TotEvNb          = 1000; // ? does the controller need to know?
    RunConf.EvNbPerFile      = 1000; // should be controlled by eudaq
    RunConf.FrameNbPerAcq    = 400;
    RunConf.DataTransferMode = 2; // 0 = No, 1 = IPHC, 2 = EUDET2, 3 = EUDET3 // deprecated! send all data to producer!
    RunConf.TrigMode         = 0; // ??
    RunConf.SaveToDisk       = 0; // 0 = No, 1 = Multithreading, 2 = Normal
    RunConf.SendOnEth        = 0; // deprecated, not need with eudaq
    RunConf.SendOnEthPCent   = 0; // deprecated, not need with eudaq

    sprintf ( RunConf.DestDir, "c:\\Data\\test\\1" );
    sprintf ( RunConf.FileNamePrefix, "run_" );
    std::string m_jtag;
    m_jtag = param.Get("JTAG_file", "");
    char temp[256];
    strncpy(temp,m_jtag.c_str(),256);
    //RunConf.JtagFileName =  temp;
    sprintf ( RunConf.JtagFileName, temp );

    SInt32 CmdNumber = 777;

    ret = IRC_RCBT2628__FRcSendCmdRunConf ( CmdNumber, &RunConf, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdRunConf failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdRunConf failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while Conf run: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while Conf run: Caught unknown exeption:\n");
  }


}

void M26Controller::Start_Run() { 

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = 1; // Start the run

    ret = IRC_RCBT2628__FRcSendCmdRunStartStop ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdRunStartStop('Start') failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 20000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdRunStartStop failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while start run: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while start run: Caught unknown exeption:\n");
  }

}

void M26Controller::Stop_Run() { 

  try{
    std::lock_guard<std::mutex> lck(m_mutex);

    SInt32 ret = -1;
    DaqAnswer_CmdReceived = -1;
    DaqAnswer_CmdExecuted = -1;
    SInt32 VLastCmdError = -1;

    SInt32 CmdNumber = 0; // Stop the run

    ret = IRC_RCBT2628__FRcSendCmdRunStartStop ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

    if(ret) {
      EUDAQ_ERROR("Send CmdRunStartStop('Stop') failed"); // FIXME make/add better error handling
      return;
    }

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 20000 /* TimeOutMs */ );

    // the error logic is inverted here *argh
    if(!ret) EUDAQ_ERROR("Execution of CmdRunStartStop failed"); // FIXME make/add better error handling

  } catch (const std::exception &e) { 
    printf("while stop run: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while stop run: Caught unknown exeption:\n");
  }

}



// --- Artem
//void NiController::Configure(const eudaq::Configuration & /*param*/) {
// NiIPaddr = param.Get("NiIPaddr", "");
//}

//void NiController::TagsSetting() {
// ev.SetTag("DET", 12);
//}
//void NiController::GetProduserHostInfo() {
/*** get Producer information, NAME and INET ADDRESS ***/
//  gethostname(ThisHost, MAXHOSTNAME);
//  printf("----TCP/Producer running at host NAME: %s\n", ThisHost);
//  hclient = gethostbyname(ThisHost);
//  if (hclient != 0) {
//    EUDAQ_BCOPY(hclient->h_addr, &(client.sin_addr), hclient->h_length);
//    printf("----TCP/Producer INET ADDRESS is: %s \n",
//           inet_ntoa(client.sin_addr));
//  } else {
//    printf("----TCP/Producer -- Warning! -- failed at executing "
//           "gethostbyname() for: %s. Check your /etc/hosts list \n",
//           ThisHost);
//  }
//}
//void NiController::Start() { ConfigClientSocket_Send(start, sizeof(start)); }
//void NiController::Stop() { ConfigClientSocket_Send(stop, sizeof(stop)); }

//void NiController::ConfigClientSocket_Open(const eudaq::Configuration &param) {
/*** Network configuration for NI, NAME and INET ADDRESS ***/

//  std::string m_server;
//  m_server = param.Get("NiIPaddr", "");

//  std::string m_config_socket_port;
//  m_config_socket_port = param.Get("NiConfigSocketPort", "49248");

//  // convert string in config into IPv4 address
//  hostent *host = gethostbyname(m_server.c_str());
//  if (!host) {
//    EUDAQ_ERROR("ConfSocket: Bad NiIPaddr value in config file: must be legal "
//                "IPv4 address!");
//    perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
//           "address: ");
//  }
//  memcpy((char *)&config.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

//  if ((sock_config = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//    EUDAQ_ERROR("ConfSocket: Error creating the TCP socket  ");
//    perror("ConfSocket Error: socket()");
//    exit(1);
//  } else
//    printf("----TCP/NI crate: SOCKET is OK...\n");

//  printf("----TCP/NI crate INET ADDRESS is: %s \n", inet_ntoa(config.sin_addr));
//  printf("----TCP/NI crate INET PORT is: %s \n", m_config_socket_port.c_str());

//  config.sin_family = AF_INET;
//  int i_auto = std::stoi(m_config_socket_port, nullptr, 10);
//  config.sin_port = htons(i_auto);
//  memset(&(config.sin_zero), '\0', 8);
//  if (connect(sock_config, (struct sockaddr *)&config,
//              sizeof(struct sockaddr)) == -1) {
//    EUDAQ_ERROR("ConfSocket: National Instruments crate doesn't appear to be "
//                "running  ");
//    perror("ConfSocket Error: connect()");
//    EUDAQ_Sleep(60);
//    exit(1);
//  } else
//    printf("----TCP/NI crate The CONNECT is OK...\n");
//}
/*void NiController::ConfigClientSocket_Send(unsigned char *text, size_t len) {
  bool dbg = false;
  if (dbg)
  printf("size=%zu", len);

  if (EUDAQ_SEND(sock_config, text, len, 0) == -1)
  perror("Server-send() error lol!");
  }*/
/*void NiController::ConfigClientSocket_Close() {

  EUDAQ_CLOSE_SOCKET(sock_config);
  }*/
//unsigned int
//NiController::ConfigClientSocket_ReadLength(const char * /*string[4]*/) {
/*  unsigned int datalengthTmp;
    unsigned int datalength;
    int i;
    bool dbg = false;
    if ((numbytes = recv(sock_config, Buffer_length, 2, 0)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: Read length error ");
    perror("recv()");
    exit(1);
    } else {
    if (dbg)
    printf("|==ConfigClientSocket_ReadLength ==|    numbytes=%u \n",
    static_cast<uint32_t>(numbytes));
    i = 0;
    if (dbg) {
    while (i < numbytes) {
    printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i + 1]);
    i = i + 2;
    }
    }
    datalengthTmp = 0;
    datalengthTmp = 0xFF & Buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & Buffer_length[1];
    datalength = datalengthTmp;

    if (dbg)
    printf(" data= %d", datalength);
    if (dbg)
    printf("\n");
    }
    return datalength;
    }*/
/*std::vector<unsigned char>
  NiController::ConfigClientSocket_ReadData(int datalength) {
  std::vector<unsigned char> ConfigData(datalength);
  unsigned int stored_bytes;
  unsigned int read_bytes_left;
  unsigned int i;
  bool dbg = false;

  stored_bytes = 0;
  read_bytes_left = datalength;
  while (read_bytes_left > 0) {
  if ((numbytes = recv(sock_config, Buffer_data, read_bytes_left, 0)) == -1) {
  EUDAQ_ERROR("|==ConfigClientSocket_ReadLength==| Read data error ");
  perror("recv()");
  exit(1);
  } else {
  if (dbg)
  printf("|==ConfigClientSocket_ReadLength==|    numbytes=%u \n",
  static_cast<uint32_t>(numbytes));
  read_bytes_left = read_bytes_left - numbytes;
  for (int k = 0; k < numbytes; k++) {
  ConfigData[stored_bytes] = Buffer_data[k];
  stored_bytes++;
  }
  i = 0;
  if (dbg) {
  while ((int)i < numbytes) {
  printf(" 0x%x \n", 0xFF & Buffer_data[i]);
  i++;
  }
  }
  }
  }
  if (dbg)
  printf("\n");
  return ConfigData;
  }*/

/*void NiController::DatatransportClientSocket_Open(
  const eudaq::Configuration &param) {
// Creation for the data transmit socket, NAME and INET ADDRESS 
std::string m_server;
m_server = param.Get("NiIPaddr", "");

std::string m_data_transport_socket_port;
m_data_transport_socket_port =
param.Get("NiDataTransportSocketPort", "49250");

// convert string in config into IPv4 address
hostent *host = gethostbyname(m_server.c_str());
if (!host) {
EUDAQ_ERROR("ConfSocket: Bad NiIPaddr value in config file: must be legal "
"IPv4 address!");
perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
"address: ");
}
memcpy((char *)&datatransport.sin_addr.s_addr, host->h_addr_list[0],
host->h_length);

if ((sock_datatransport = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
EUDAQ_ERROR("DataTransportSocket: Error creating socket ");
perror("DataTransportSocket Error: socket()");
exit(1);
} else
printf("----TCP/NI crate DATA TRANSPORT: The SOCKET is OK...\n");

printf("----TCP/NI crate DATA TRANSPORT INET ADDRESS is: %s \n",
inet_ntoa(datatransport.sin_addr));
printf("----TCP/NI crate DATA TRANSPORT INET PORT is: %s \n",
m_data_transport_socket_port.c_str());

datatransport.sin_family = AF_INET;
int i_auto = std::stoi(m_data_transport_socket_port, nullptr, 10);
datatransport.sin_port = htons(i_auto);
memset(&(datatransport.sin_zero), '\0', 8);
if (connect(sock_datatransport, (struct sockaddr *)&datatransport,
sizeof(struct sockaddr)) == -1) {
EUDAQ_ERROR("DataTransportSocket: National Instruments crate doesn't "
"appear to be running  ");
perror("DataTransportSocket: connect()");
EUDAQ_Sleep(60);
exit(1);
} else
printf("----TCP/NI crate DATA TRANSPORT: The CONNECT executed OK...\n");
}*/
//unsigned int
//NiController::DataTransportClientSocket_ReadLength(const char * /*string[4]*/) {
/*  unsigned int datalengthTmp;
    unsigned int datalength;
    int i;
    bool dbg = false;
    if ((numbytes = recv(sock_datatransport, Buffer_length, 2, 0)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: Read length error ");
    perror("recv()");
    exit(1);
    } else {
    if (dbg)
    printf("|==DataTransportClientSocket_ReadLength ==|    numbytes=%u \n",
    static_cast<uint32_t>(numbytes));
    i = 0;
    if (dbg) {
    while (i < numbytes) {
    printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i + 1]);
    i = i + 2;
    }
    }
    datalengthTmp = 0;
    datalengthTmp = 0xFF & Buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & Buffer_length[1];
    datalength = datalengthTmp;

    if (dbg)
    printf(" data= %d", datalength);
    if (dbg)
    printf("\n");
    }
    return datalength;
    }*/
/*std::vector<unsigned char>
  NiController::DataTransportClientSocket_ReadData(int datalength) {

  std::vector<unsigned char> mimosa_data(datalength);
  unsigned int stored_bytes;
  unsigned int read_bytes_left;
  unsigned int i;
  bool dbg = false;

  stored_bytes = 0;
  read_bytes_left = datalength;
  while (read_bytes_left > 0) {
  if ((numbytes =
  recv(sock_datatransport, Buffer_data, read_bytes_left, 0)) == -1) {
  EUDAQ_ERROR("DataTransportSocket: Read data error ");
  perror("recv()");
  exit(1);
  } else {
  if (dbg)
  printf("|==DataTransportClientSocket_ReadData==|    numbytes=%u \n",
  static_cast<uint32_t>(numbytes));
  read_bytes_left = read_bytes_left - numbytes;
  for (int k = 0; k < numbytes; k++) {
  mimosa_data[stored_bytes] = Buffer_data[k];
  stored_bytes++;
  }
  i = 0;
  if (dbg) {
  while ((int)i < numbytes) {
  printf(" 0x%x%x", 0xFF & Buffer_data[i], 0xFF & Buffer_data[i + 1]);
  i = i + 2;
  }
  }
  }
  }
  if (dbg)
  printf("\n");
  return mimosa_data;
  }*/
//void NiController::DatatransportClientSocket_Close() {
//  EUDAQ_CLOSE_SOCKET(sock_datatransport);
//}
//NiController::~NiController() {
//
//}
