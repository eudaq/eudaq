#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/TLU2Packet.hh"

#include "miniTLUController.hh"
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
    :eudaq::Producer("minitlu", runcontrol), m_tlu(nullptr){
    m_fsmstate = STATE_UNCONF;
    
  }

  void MainLoop() {
    while (m_fsmstate != STATE_GOTOTERM){
      if (m_fsmstate == STATE_UNCONF) {
	eudaq::mSleep(100);
	continue;
      }
      if (m_fsmstate == STATE_RUNNING) {
	m_tlu->ReceiveEvents();
	while (!m_tlu->IsBufferEmpty()){
	  minitludata * data = m_tlu->PopFrontEvent();	  
	  m_ev = data->eventnumber;
          uint64_t t = data->timestamp;
          TLUEvent ev(m_run, m_ev, t);

	  std::stringstream  triggerss;
	  triggerss<< data->input3 << data->input2 << data->input1 << data->input0;
	  ev.SetTag("trigger", triggerss.str());	  
	  if(m_tlu->IsBufferEmpty()){
	    uint32_t sl0,sl1,sl2,sl3, pt;
	    m_tlu->GetScaler(sl0,sl1,sl2,sl3);
	    pt=m_tlu->GetPreVetoTriggers();  
	    ev.SetTag("SCALER0", to_string(sl0));
	    ev.SetTag("SCALER1", to_string(sl1));
	    ev.SetTag("SCALER2", to_string(sl2));
	    ev.SetTag("SCALER3", to_string(sl3));
	    ev.SetTag("PARTICLES", to_string(pt));
	  }
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
      if (m_tlu)
	m_tlu = nullptr;
     
      m_tlu = std::unique_ptr<miniTLUController>(new miniTLUController(param.Get("ConnectionFile","file:///dummy_connections.xml"),
								       param.Get("DeviceName","dummy.udp")));
	    
      m_tlu->ResetSerdes();
      m_tlu->ResetCounters();
      m_tlu->SetTriggerVeto(1);
      m_tlu->ResetFIFO();
      m_tlu->ResetEventsBuffer();      
      
      m_tlu->InitializeI2C(param.Get("I2C_DAC_Addr",0x1f), param.Get("I2C_ID_Addr",0x50));
      m_tlu->SetThresholdValue(0, param.Get("DACThreshold0",1.3));
      m_tlu->SetThresholdValue(1, param.Get("DACThreshold1",1.3));
      m_tlu->SetThresholdValue(2, param.Get("DACThreshold2",1.3));
      m_tlu->SetThresholdValue(3, param.Get("DACThreshold3",1.3));
      

      m_tlu->SetDUTMask(param.Get("DUTMask",1));
      m_tlu->SetDUTMaskMode(param.Get("DUTMaskMode",0xff));
      m_tlu->SetDUTMaskModeModifier(param.Get("DUTMaskModeModifier",0xff));
      m_tlu->SetDUTIgnoreBusy(param.Get("DUTIgnoreBusy",7));
      m_tlu->SetDUTIgnoreShutterVeto(param.Get("DUTIgnoreShutterVeto",1));//ILC stuff related
      m_tlu->SetEnableRecordData(param.Get("EnableRecordData", 1));
      m_tlu->SetInternalTriggerInterval(param.Get("InternalTriggerInterval",0)); // 160M/interval
      m_tlu->SetTriggerMask(param.Get("TriggerMask",0));
      m_tlu->SetPulseStretch(param.Get("PluseStretch ",0));
      m_tlu->SetPulseDelay(param.Get("PluseDelay",0));

      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");

      m_fsmstate=STATE_CONFED;
      
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configure ERROR (" +  to_string(e.what()) + ")");
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configure ERROR (unknown)");
    }
  }

  virtual void OnStartRun(unsigned param) {    
    try {
      m_fsmstate = STATE_GOTORUN;
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

      m_lasttime=m_tlu->GetCurrentTimestamp()/40000000;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Started");
      m_fsmstate = STATE_RUNNING;
      
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error: (" +  to_string(e.what()) + ")");
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error (unknown)");
    }
    std::cout << "Done starting" << std::endl;
  }

  virtual void OnStopRun() {
    try {
      m_tlu->SetTriggerVeto(1);
      m_fsmstate = STATE_GOTOSTOP;
      while (m_fsmstate == STATE_GOTOSTOP) {
	eudaq::mSleep(100); //waiting for EORE being send
      }
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error: (" +  to_string(e.what()) + ")");
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error (unknown)");
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
    } catch (const std::exception & e) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Rest Error: (" +  to_string(e.what()) + ")");
    } catch (...) {
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Reset Error (unknown)");
    }
  }

  virtual void OnStatus() {
    if (m_tlu) {
      uint64_t time = m_tlu->GetCurrentTimestamp();
      time = time/40000000; // in second
      m_status.SetTag("TIMESTAMP", to_string(time));
      m_status.SetTag("LASTTIME", to_string(m_lasttime));
      m_lasttime = time;
      
      uint32_t sl0,sl1,sl2,sl3, pret, post;
      pret=m_tlu->GetPreVetoTriggers();
      post=m_tlu->GetPostVetoTriggers();  
      m_tlu->GetScaler(sl0,sl1,sl2,sl3);
      m_status.SetTag("SCALER0", to_string(sl0));
      m_status.SetTag("SCALER1", to_string(sl1));
      m_status.SetTag("SCALER2", to_string(sl2));
      m_status.SetTag("SCALER3", to_string(sl3));
      m_status.SetTag("PARTICLES", to_string(pret));
      m_status.SetTag("TRIG", to_string(post));
      
    }
  }


private:
  unsigned m_run, m_ev;
  std::unique_ptr<miniTLUController> m_tlu;
  uint64_t m_lasttime;
  
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


