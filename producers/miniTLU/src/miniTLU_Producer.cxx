#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/TLU2Packet.hh"
#include "tlu/miniTLUController.hh"
#include <iostream>
#include <ostream>
#include <vector>


typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class miniTLUProducer: public eudaq::Producer {
public:
  miniTLUProducer(const std::string & runcontrol)
    :eudaq::Producer("miniTLU", runcontrol), m_tlu(nullptr){
    m_fsmstate = STATE_UNCONF;

  }

  void MainLoop() {
    while (m_fsmstate != STATE_GOTOTERM){
      // std::cout<< "mainloop"<<std::endl;
      if (m_fsmstate == STATE_UNCONF) {
	eudaq::mSleep(100);
	continue;
      }
      if (m_fsmstate == STATE_RUNNING) {
	// std::cout<< "mainloop running"<<std::endl;
	m_tlu->ReceiveEvents();
	while (!m_tlu->IsBufferEmpty()){
	  minitludata * data = m_tlu->PopFrontEvent();	  
	  m_ev = data->eventnumber;
          uint64_t t = data->timestamp;
          TLUEvent ev(m_run, m_ev, t);
	  SendEvent(ev);
	  m_ev++;
	  delete data;
	}
	continue;
      }
      
      if (m_fsmstate == STATE_GOTOSTOP) {
	SendEvent(TLUEvent::EORE(m_run, ++m_ev));
	m_fsmstate = STATE_CONFED;
	continue;
      }
      eudaq::mSleep(100);
      
    };
  }


  virtual void OnConfigure(const eudaq::Configuration & param) {
    try {
      SetStatus(eudaq::Status::LVL_OK, "Configuring");
      if (m_tlu)
	m_tlu = nullptr;
     
      m_tlu = std::unique_ptr<miniTLUController>(new miniTLUController(param.Get("ConnectionFile","file:///dummy_connections.xml"),
								       param.Get("DeviceName","dummy.udp")));
      

      m_tlu->ResetCounters();
      m_tlu->ResetFIFO();
      m_tlu->ResetEventsBuffer();      
      m_tlu->SetTriggerVeto(1);
      
      m_tlu->InitializeI2C(param.Get("I2C_DAC_Addr",0x1f), param.Get("I2C_ID_Addr",0x50));
      m_tlu->SetThresholdValue(0, param.Get("DACThreshold0",1.3));
      m_tlu->SetThresholdValue(1, param.Get("DACThreshold1",1.3));
      m_tlu->SetThresholdValue(2, param.Get("DACThreshold2",1.3));
      m_tlu->SetThresholdValue(3, param.Get("DACThreshold3",1.3));
      
      m_tlu->SetDUTMask(param.Get("DUTMask",1));
      m_tlu->SetDUTMaskMode(param.Get("DUTMaskMode",62));
      m_tlu->SetDUTMaskModeModifier(param.Get("DUTMaskModeModifier",0));
      m_tlu->SetDUTIgnoreBusy(param.Get("DUTIgnoreBusy",7));
      m_tlu->SetDUTIgnoreShutterVeto(param.Get("DUTIgnoreShutterVeto",1));//ILC stuff related
      m_tlu->SetEnableRecordData(param.Get("EnableRecordData", 1));
      m_tlu->SetInternalTriggerInterval(param.Get("InternalTriggerInterval",0x4000000)); // 160M/interval


      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
      
      m_fsmstate=STATE_CONFED;
      
    } catch (const std::exception & e) {
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error: " + to_string(e.what()));
    } catch (...) {
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error: unknown");
    }
  }

  virtual void OnStartRun(unsigned param) {    
    try {
      m_fsmstate = STATE_GOTORUN;
      SetStatus(eudaq::Status::LVL_OK, "Starting");

      
      m_tlu->ResetCounters();
      m_tlu->ResetEventsBuffer();
      m_tlu->ResetFIFO();
      m_tlu->SetTriggerVeto(0);;


      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << param << std::endl;
      TLUEvent ev(TLUEvent::BORE(m_run));
      ev.SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
      ev.SetTag("BoardID", to_string(m_tlu->GetBoardID()));
      SendEvent(ev);
      
      SetStatus(eudaq::Status::LVL_OK, "Started");
      m_fsmstate = STATE_RUNNING;
      
    } catch (const std::exception & e) {
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error: "+ to_string(e.what()));
    } catch (...) {
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error: unknown");
    }
    std::cout << "Done starting" << std::endl;
  }

  virtual void OnStopRun() {
    try {
      SetStatus(eudaq::Status::LVL_OK, "Stopping");
      m_tlu->SetTriggerVeto(1);
      m_fsmstate = STATE_GOTOSTOP;
      while (m_fsmstate == STATE_GOTOSTOP) {
	eudaq::mSleep(100); //waiting for EORE being send
      }
      SetStatus(eudaq::Status::LVL_OK, "Stopped");

    } catch (const std::exception & e) {
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error: " + to_string(e.what()));
    } catch (...) {
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }

  virtual void OnTerminate() {
    m_fsmstate = STATE_GOTOTERM;
    eudaq::mSleep(1000);
  }

  virtual void OnReset() {
    try {
      m_tlu->SetTriggerVeto(1);
      //TODO:: stop_tlu
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error: " + to_string(e.what()));
    } catch (...) {
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error: unknown");
    }
  }

  virtual void OnStatus() {
    m_status.SetTag("TRIG", to_string(m_ev));
  }


private:
  unsigned m_run, m_ev;
  std::unique_ptr<miniTLUController> m_tlu;

  enum FSMState {
    STATE_ERROR,
    STATE_UNCONF,
    STATE_GOTOCONF,
    STATE_CONFED,
    STATE_GOTORUN,
    STATE_RUNNING,
    STATE_GOTOSTOP,
    STATE_GOTOTERM
  } m_fsmstate;
  
};


int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ miniTLU Producer", "1.0", "The Producer task for the mini Trigger Logic Unit");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    miniTLUProducer producer(rctrl.Value());
    producer.MainLoop();
    std::cout << "Quitting" << std::endl;
    eudaq::mSleep(300);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}


