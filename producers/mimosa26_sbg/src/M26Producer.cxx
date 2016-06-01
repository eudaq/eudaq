#include "M26Controller.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "irc_exceptions.hh"

#include <stdio.h>
#include <stdlib.h>
#include <memory>

using eudaq::RawDataEvent;

class NiProducer : public eudaq::Producer {
  public:
    NiProducer(const std::string &runcontrol)
      : eudaq::Producer("Mimosa26_SBG", runcontrol), done(false), running(false),
      stopping(false) {

	configure = false;

	std::cout << "M26_SBG_Producer was started successfully " << std::endl;
      }
    void MainLoop() {
      do {
	if (!running) {
	  eudaq::mSleep(50);
	  continue;
	}
	if (running) {

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
	}

      } while (!done);
    }

    virtual void OnConfigure(const eudaq::Configuration &param) {

      std::cout << "Configuring ...(" << param.Name() << ")" << std::endl;
      if (!configure) {
	try{
	  m26_control = std::make_shared<M26Controller>();
	  m26_control->Connect(param);
	  m26_control->Init(param);
	  m26_control->LoadFW(param);
	  m26_control->JTAG_Reset();
	  eudaq::mSleep(200);
	  m26_control->JTAG_Load(param);
	  eudaq::mSleep(200);
	  m26_control->JTAG_Start();
	  m26_control->Configure_Run(param);

	  // ---
	  configure = true;
	} catch  (const std::exception &e) { 
	  printf("while connecting: Caught exeption: %s\n", e.what());
	  configure = false;
	} catch (...) { 
	  printf("while configuring: Caught unknown exeption:\n");
	  configure = false;
	}

      }

      if (!configure) {
	EUDAQ_ERROR("Error during configuring.");
	SetStatus(eudaq::Status::LVL_ERROR, "Stop error");
      }
      else {
	std::cout << "... was configured " << param.Name() << " " << std::endl;
	EUDAQ_INFO("Configured (" + param.Name() + ")");
	SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
      }

    }

    virtual void OnStartRun(unsigned param) {
      std::cout << "Start Run: " << param << std::endl;
      try {
	m_run = param;
	m_ev = 0;

	eudaq::RawDataEvent ev(RawDataEvent::BORE("NI", m_run));

	ev.SetTag("DET", "MIMOSA26");
	//ev.SetTag("MODE", "ZS2");
	ev.SetTag("BOARDS", NumBoards);
	for (unsigned char i = 0; i < 6; i++)
	  ev.SetTag("ID" + to_string(i), to_string(MimosaID[i]));
	for (unsigned char i = 0; i < 6; i++)
	  ev.SetTag("MIMOSA_EN" + to_string(i), to_string(MimosaEn[i]));
	SendEvent(ev);
	eudaq::mSleep(500);

	m26_control->Start();

	running = true;

	SetStatus(eudaq::Status::LVL_OK, "Started");
      } catch (const std::exception &e) {
	printf("Caught exception: %s\n", e.what());
	SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
      } catch (...) {
	printf("Unknown exception\n");
	SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
      }
    }
    virtual void OnStopRun() {
      try {
	std::cout << "Stop Run" << std::endl;

	m26_control->Stop();
	eudaq::mSleep(5000);
	running = false;
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
    virtual void OnTerminate() {
      std::cout << "Terminate (press enter)" << std::endl;
      done = true;
      //m26_control->DatatransportClientSocket_Close();
      //m26_control->ConfigClientSocket_Close();
      eudaq::mSleep(1000);
    }

  private:
    unsigned m_run, m_ev;
    bool done, running, stopping, configure;
    struct timeval tv;
    std::shared_ptr<M26Controller> m26_control;

    char *Buffer1;
    unsigned int datalength1;
    char *Buffer2;
    unsigned int datalength2;
    unsigned int ConfDataLength;
    std::vector<unsigned char> ConfDataError;
    int r;
    int t;

    unsigned int Header0;
    unsigned int Header1;
    unsigned int MIMOSA_Counter0;
    unsigned int MIMOSA_Counter1;
    unsigned int MIMOSA_Datalen0;
    unsigned int MIMOSA_Datalen1;
    unsigned int MIMOSA_Datalen;
    unsigned int MIMOSA_Trailer0;
    unsigned int MIMOSA_Trailer1;
    unsigned int StatusLine;
    unsigned int OVF;
    unsigned int NumOfState;
    unsigned int AddLine;
    unsigned int State;
    unsigned int AddColum;
    unsigned int HitPix;

    unsigned int data[7000];
    unsigned char NumOfDetector;
    unsigned int NumOfData;
    unsigned int NumOfLengt;
    unsigned int MIMOSA_DatalenTmp;
    int datalengthAll;

    unsigned TriggerType;
    unsigned Det;
    unsigned Mode;
    unsigned NiVersion;
    unsigned NumBoards;
    unsigned FPGADownload;
    unsigned MimosaID[6];
    unsigned MimosaEn[6];
    bool OneFrame;
    bool NiConfig;

    unsigned char conf_parameters[10];
};
// ----------------------------------------------------------------------
int main(int /*argc*/, const char **argv) {
  std::cout << "Start Producer \n" << std::endl;

  eudaq::OptionParser op("EUDAQ NI Producer", "0.1",
      "The Producer task for the NI crate");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "NI", "string",
      "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    NiProducer producer(rctrl.Value());
    producer.MainLoop();
    eudaq::mSleep(500);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
