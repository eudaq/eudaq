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

/*int EUDAQ_SEND(SOCKET s, unsigned char *buf, int len, int flags) {

#ifdef WIN32
  // on windows this function takes a signed char ...
  // in prinziple I always distrust reinterpreter_cast but in this case it
  // should be save
  return send(s, reinterpret_cast<const char *>(buf), len, flags);

#else
  return send(s, buf, len, flags);
#endif
}
*/

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

    m_detectorType = param.Get("Det", "");
    if(m_detectorType.compare("MIMOSA26") == 0) {
      SensorType = 1; // compare() returns 0 if equal
      std::cout << "  -- Found matching sensor. " << std::endl;
    }
    else throw irc::InvalidConfig("Invalid Sensor type!");
    
    m_JTAG_file = param.Get("JTAG_file", "");
    std::ifstream infile(m_JTAG_file.c_str());
    if(!infile.good()) throw irc::InvalidConfig("Invalid path to JTAG file!");


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

