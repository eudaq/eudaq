//#include "M26Controller.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "M26Producer.hh"

// IRC exceptions
#include "irc_exceptions.hh"

//#include <stdio.h>
//#include <stdlib.h>
//#include <memory>
#include <pthread.h>
#include <sched.h>

using eudaq::RawDataEvent;


M26Producer::M26Producer(const std::string &name,
    const std::string &runcontrol,
    const std::string &verbosity)
  : eudaq::Producer(name, runcontrol), m_producerName(name), m_terminated(false), m_configured(false), 
  m_running(false), m_triggering(false), m_stopping(false) {
    //m_configured = false;
    //std::cout << "M26_SBG_Producer was started successfully " << std::endl;
  }

//void reset_com(SInt32 &int1, SInt32 &int2, SInt32 &int3, SInt32 &int4, SInt32 &int5, std::string &cmd) {
void M26Producer::reset_com() {
  ret = -1;
  DaqAnswer_CmdReceived= -1;
  DaqAnswer_CmdExecuted = -1;
  VLastCmdError = -1;
  CmdNumber = -1;
  cmd = "";
}

void sendError(std::string cmd){
  EUDAQ_ERROR(std::string("Send/Receive " + cmd + " failed")); 
  throw irc::SendReceiveException(std::string("Failed to send/receive "+cmd));
}

void executeError(std::string cmd){
  EUDAQ_ERROR(std::string("Execution of " + cmd + " failed")); 
  throw irc::ExecuteException(std::string("Failed to execute "+cmd));
}

void M26Producer::ReadoutLoop() {
  // loop until terminated
  do {
    // if not running, move this thread to end of scheduler queue
    if (!m_running) {
      sched_yield();
      continue;
    } else {

      //try to read shrd mem
      try{
	std::lock_guard<std::mutex> lck(m_mutex);
	// check an read
	std::cout << " reading" << std::endl;

	// structure of this part depends on how data is send from SBG LV DAQ
	// - likely to be package with 4 frames per trigger -> 4*n frames = 1 acquisition
	// ->Is "4" configurable?
	// - What of more than 1 trigger occurred during "4" frames
	// -> Data dublication? Does the FW propagate trigger into into RAM such that we know which frames to dublicate
	// -> Does the MAxTrigNumber influence the FW: longer BUSY? if not, the TLU does not know about this and will continue to issue triggers. 
	//    Hence LVDAQ might not send data on a certain trigger, but all the other systems are! I think we either need a long busy ("4" frames) or we can not limit the number of triggers so we (need to) know how to send all matching frames to the producer and the producer can dublicate them in such a way that it sends the correct package of "4" frames to the Data Collector

	//datalength1 = ni_control->DataTransportClientSocket_ReadLength("priv");
	std::vector<unsigned char> mimosa_data_0(1);
	//mimosa_data_0 = ni_control->DataTransportClientSocket_ReadData(datalength1);
	unsigned char test = 125; 
	mimosa_data_0.push_back(test);

	//datalength2 = ni_control->DataTransportClientSocket_ReadLength("priv");
	std::vector<unsigned char> mimosa_data_1(1);
	//mimosa_data_1 = ni_control->DataTransportClientSocket_ReadData(datalength2);
	test = 127; 
	mimosa_data_1.push_back(test);


	eudaq::RawDataEvent ev("NI", m_run, m_ev++);
	ev.AddBlock(0, mimosa_data_0);
	ev.AddBlock(1, mimosa_data_1);
	SendEvent(ev);
	eudaq::mSleep(283);
	continue;

      } catch (irc::DataNoEvent &) {
	// no event in shrd mem
	sched_yield();
      }

    }

  } while (!m_terminated);
}

void M26Producer::OnConfigure(const eudaq::Configuration &param) {

  m_config = param;

  std::cout << "Configuring ...(" << m_config.Name() << ")" << std::endl;
  try{
    if (!m_configured) {

      //m26_control->Connect(m_config);
      std::cout << " Connecting to shared memory. " << std::endl;
      try{

        reset_com();

        //one could also do: if(!IRC__FBegin()) but is less readible imho
	ret = IRC__FBegin ( APP_VGErrUserLogLvl, APP_ERR_LOG_FILE, APP_VGMsgUserLogLvl, APP_MSG_LOG_FILE );
	if(ret) throw irc::GlobalConnectError("Failed to perform global IRC init. Is LV running?");

	ret = IRC_RCBT2628__FRcBegin ();
	if(ret) throw irc::GlobalSharedMemError("Failed to find shared memory. Is LV running?");

      } catch (const std::exception &e) { 
	printf("while connecting: Caught exeption: %s\n", e.what());
	//SetStatus(eudaq::Status::LVL_ERROR, "Stop error"); 
	throw;
      } catch (...) { 
	printf("while connecting: Caught unknown exeption:\n");
	//SetStatus(eudaq::Status::LVL_ERROR, "Stop error");
	throw;
      }

      //m26_control->Init(m_config);
      std::cout << " Initialising. " << std::endl;
      try {

        reset_com();
	cmd = "CmdInit";

	SInt32 SensorType = -1; // 0 = ASIC__NONE, 1 = ASIC__MI26, 2 = ASIC__ULT1

	m_detectorType = m_config.Get("Det", "");
	std::cout << " this is the det type = " << m_detectorType << std::endl;
	if(m_detectorType.compare("MIMOSA26") == 0) {
	  SensorType = 1; // compare() returns 0 if equal
	  std::cout << "  -- Found matching sensor. " << std::endl;
	}
	else throw irc::InvalidConfig("Invalid Sensor type >>> " + m_detectorType  + " <<< !");

	m_numDetectors = m_config.Get("NumDetectors", -1);
	if(m_numDetectors < 1 || m_numDetectors > 16) throw irc::InvalidConfig("Invalid number of detectors!");

	for(unsigned i = 0; i < m_numDetectors; i++) m_MimosaID.push_back(m_config.Get("MimosaID_"+to_string(i),-1));
	/*FIXME if very verbose*/ for(unsigned i = 0; i < m_numDetectors; i++) std::cout << m_MimosaID.at(i) << std::endl;

	for(unsigned i = 0; i < m_numDetectors; i++) m_MimosaEn.push_back(m_config.Get("MimosaEn_"+to_string(i),-1));
	/*FIXME if very verbose*/ for(unsigned i = 0; i < m_numDetectors; i++) std::cout << m_MimosaEn.at(i) << std::endl;

	ret = IRC_RCBT2628__FRcSendCmdInit ( SensorType, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );
	if(ret) sendError(cmd);

	ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 5000 /* TimeOutMs */ );
	// the error logic is inverted here *argh
	if(!ret) executeError(cmd);

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

      //m26_control->LoadFW(m_config);
      std::cout << " LoadWF. " << std::endl;
      try{

        reset_com();
	cmd = "CmdFwLoad";

	ret = IRC_RCBT2628__FRcSendCmdFwLoad ( CmdNumber/*not used*/, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );
	if(ret) sendError(cmd);

	ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ ); // orig 10000
	if(!ret) executeError(cmd);


      } catch (const std::exception &e) { 
	printf("while loading FW: Caught exeption: %s\n", e.what());
	throw;
      } catch (...) { 
	printf("while loading FW: Caught unknown exeption:\n");
	throw;
      }

      //m26_control->JTAG_Reset();
      std::cout << " Send JTAG RESET to sensors " << std::endl;
      try{

        reset_com();
        cmd = "JTAGReset";

	ret = IRC_RCBT2628__FRcSendCmdJtagReset ( CmdNumber/*not used*/, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );
	if(ret) sendError(cmd);

	ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );
	if(!ret) executeError(cmd);

      } catch (const std::exception &e) { 
	printf("while JTAG reset: Caught exeption: %s\n", e.what());
	throw;
      } catch (...) { 
	printf("while JTAG reset: Caught unknown exeption:\n");
	throw;
      }

      eudaq::mSleep(200);

      //m26_control->JTAG_Load(m_config);
      std::cout << " JTAG sensors " << std::endl;
      // JTAG sensors via mcf file
      try{

	m_JTAG_file = m_config.Get("JTAG_file", "");
	std::ifstream infile(m_JTAG_file.c_str());
	if(!infile.good()) throw irc::InvalidConfig("Invalid path to JTAG file!");

	/*debug*/ std::cout << "  -- JTAG file " << m_JTAG_file << std::endl;

        reset_com();
	cmd = "CmdJtagLoad";

	ret = IRC_RCBT2628__FRcSendCmdJtagLoad ( CmdNumber/*not used*/, (char*)(m_JTAG_file.c_str()), &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );
	if(ret) sendError(cmd);

	ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );
	if(!ret) executeError(cmd);

      } catch (const std::exception &e) { 
	printf("while JTAG load: Caught exeption: %s\n", e.what());
	throw;
      } catch (...) { 
	printf("while JTAG load: Caught unknown exeption:\n");
	throw;
      }

      eudaq::mSleep(200);
      //m26_control->JTAG_Start();
      std::cout << " Send JTAG START to sensors " << std::endl;
      try{


        reset_com();
	cmd = "CmdJtagStart";

	ret = IRC_RCBT2628__FRcSendCmdJtagStart ( CmdNumber/*not used*/, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );

	if(ret) sendError(cmd);

	ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );
	if(!ret) executeError(cmd);

      } catch (const std::exception &e) { 
	printf("while JTAG start: Caught exeption: %s\n", e.what());
	throw;
      } catch (...) { 
	printf("while JTAG start: Caught unknown exeption:\n");
	throw;
      }


    } /* end if first time configured */
    //m26_control->Configure_Run(m_config);
    std::cout << " Configure Run " << std::endl;
    try{

      reset_com();
      cmd = "CmdRunConf";

      IRC_RCBT2628__TCmdRunConf RunConf;

      // FIXME get from m_config
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
      char temp[256];
      strncpy(temp,m_JTAG_file.c_str(),256);
      sprintf ( RunConf.JtagFileName, temp );

      ret = IRC_RCBT2628__FRcSendCmdRunConf ( CmdNumber/*not used*/, &RunConf, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 100 /* TimeOutMs */ );
      if(ret) sendError(cmd);

      ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ );
      if(!ret) executeError(cmd);

    } catch (const std::exception &e) { 
      printf("while Conf run: Caught exeption: %s\n", e.what());
      throw;
    } catch (...) { 
      printf("while Conf run: Caught unknown exeption:\n");
      throw;
    }

    /*try {

      } catch (const std::exception &e) { 
      printf("while ...: Caught exeption: %s\n", e.what());
    //SetStatus(eudaq::Status::LVL_ERROR, "Stop error"); 
    } catch (...) { 
    printf("while ...: Caught unknown exeption:\n");
    //SetStatus(eudaq::Status::LVL_ERROR, "Stop error");
    }*/


    // ---
    m_configured = true;

  } catch  (const std::exception &e) { 
    printf("while configuring: Caught exeption: %s\n", e.what());
    m_configured = false;
  } catch (...) { 
    printf("while configuring: Caught unknown exeption:\n");
    m_configured = false;
  }


  if (!m_configured) {
    EUDAQ_ERROR("Error during configuring.");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop error");
  } else {
    std::cout << "... was configured " << m_config.Name() << " " << std::endl;
    EUDAQ_INFO("Configured (" + m_config.Name() + ")");
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + m_config.Name() + ")");
  }

}

void M26Producer::OnStartRun(unsigned param) {
  std::cout << "Start Run: " << param << std::endl;
  try {
    m_run = param;
    m_ev = 0;

    eudaq::RawDataEvent ev(RawDataEvent::BORE("NI", m_run));

    ev.SetTag("DET", m_detectorType);
    //ev.SetTag("MODE", "ZS2");
    ev.SetTag("BOARDS", m_numDetectors);
    for (unsigned char i = 0; i < m_numDetectors; i++)
      ev.SetTag("ID" + to_string(i), to_string(m_MimosaID.at(i)));
    for (unsigned char i = 0; i < m_numDetectors; i++)
      ev.SetTag("MIMOSA_EN" + to_string(i), to_string(m_MimosaEn.at(i)));
    SendEvent(ev);
    eudaq::mSleep(500);

    //m26_control->Start_Run();
    try{
      std::lock_guard<std::mutex> lck(m_mutex);

      reset_com();
      CmdNumber = 1; // Start the run
      cmd = "CmdRunStart";

      ret = IRC_RCBT2628__FRcSendCmdRunStartStop ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );
      if(ret) sendError(cmd);

      ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 20000 /* TimeOutMs */ );
      if(!ret) executeError(cmd);

    } catch (const std::exception &e) { 
      printf("while start run: Caught exeption: %s\n", e.what());
      throw;
    } catch (...) { 
      printf("while start run: Caught unknown exeption:\n");
      throw;
    }


    m_running = true;

    SetStatus(eudaq::Status::LVL_OK, "Started");
  } catch (const std::exception &e) {
    printf("Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
  } catch (...) {
    printf("Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
  }
}

void M26Producer::OnStopRun() {
  try {
    std::cout << "Stop Run" << std::endl;

    //m26_control->Stop_Run();
    try{
      std::lock_guard<std::mutex> lck(m_mutex);

      reset_com();
      CmdNumber = 0; // Stop the run
      cmd = "CmdRunStop";

      ret = IRC_RCBT2628__FRcSendCmdRunStartStop ( CmdNumber, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );
      if(ret) sendError(cmd);

      ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 20000 /* TimeOutMs */ );
      if(!ret) executeError(cmd);

    } catch (const std::exception &e) { 
      printf("while stop run: Caught exeption: %s\n", e.what());
      throw;
    } catch (...) { 
      printf("while stop run: Caught unknown exeption:\n");
      throw;
    }



    eudaq::mSleep(5000);
    m_running = false;
    eudaq::mSleep(100);
    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
    SendEvent(eudaq::RawDataEvent::EORE("NI", m_run, m_ev));

  } catch (const std::exception &e) {
    printf("Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void M26Producer::OnTerminate() {
  std::cout << "Terminate (press enter)" << std::endl;
  m_terminated = true;

  std::cout << " Unload FW " << std::endl;

  try{

    reset_com();
    cmd = "CmdFwUnload";

    ret = IRC_RCBT2628__FRcSendCmdFwUnload ( CmdNumber/*not used*/, &DaqAnswer_CmdReceived, &DaqAnswer_CmdExecuted, 500 /* TimeOutMs */ );
    if(ret) sendError(cmd);

    ret = IRC_RCBT2628__FRcGetLastCmdError ( &VLastCmdError, 10000 /* TimeOutMs */ ); // orig 10000
    if(!ret) executeError(cmd);

  } catch (const std::exception &e) { 
    printf("while unloading FW: Caught exeption: %s\n", e.what());
  } catch (...) { 
    printf("while unloading FW: Caught unknown exeption:\n");
  }

  //m26_control->DatatransportClientSocket_Close();
  //m26_control->ConfigClientSocket_Close();
  eudaq::mSleep(1000);
}


// ----------------------------------------------------------------------
int main(int /*argc*/, const char **argv) {
  std::cout << "Start Producer \n" << std::endl;

  eudaq::OptionParser op("EUDAQ M26 Producer", 
      "0.1", "The Producer task for the NI crate");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "Mimosa26_SBG", "string",
      "The name of this Producer");
  eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    M26Producer producer(name.Value(), rctrl.Value(), verbosity.Value());
    producer.ReadoutLoop();
    //eudaq::mSleep(500);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
