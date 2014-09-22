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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
//#include <cctype>
//#include <memory>


typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class miniTLUProducer: public eudaq::Producer {
public:
  miniTLUProducer(const std::string & runcontrol) :
    eudaq::Producer("miniTLU", runcontrol), m_tlu(nullptr), readout_delay(100), dump_events(0), m_evtqueue(128), TLUJustStopped(false), m_senderthreads(0) {
  }

  void PacketSender(int id, RawDataQueue &queue) {
    while(true) {
	std::vector<uint64_t > mydump = queue.fetch();
	if (TLUStarted) {
        	eudaq::TLU2Packet packet(0);
        	std::cout << "[" << id << "] Received data in the queue: " << std::hex << mydump.size() << " (" << queue.size() << " )" << std::endl;
		int len = mydump.size();
		for (int i = 0; i < len;) {
			if (i + 1 == len) {
				std::cout << "Error! Strange packet: " << mydump.at(i) << std::endl;
				i++;
				continue;
			}
			uint32_t evtType = (mydump.at(i) >> 60)&0xf;
			uint32_t inputTrig = (mydump.at(i) >> 48)&0xfff;
			uint64_t timeStamp = (mydump.at(i))&0xffffffffffff;
			i++;
			uint32_t evtNumber = (mydump.at(i))&0xffffffff;
			i++;
			packet.GetMetaData().add(true, 0x1, evtType);
			packet.GetMetaData().add(true, 0x2, inputTrig);
			packet.GetMetaData().add(true, 0x3, timeStamp);
			packet.GetMetaData().add(true, 0x4, evtNumber);
		} 
        	packet.SetData(&mydump);
		{
	 		std::unique_lock<std::mutex> locker(m_locksend); 
         		SendPacket( packet );
		}
	}
    }
  }

  void MainLoop() {
    std::cout << "Main loop!" << std::endl;
    m_ev = 0;

    do {
      if (!m_tlu) {
	eudaq::mSleep(50);
	continue;
      }
      bool JustStopped = TLUJustStopped;
      if (JustStopped) {
	//m_tlu->Stop();
	eudaq::mSleep(100);
      }
      if (TLUStarted || JustStopped) {
       	eudaq::mSleep(readout_delay);
        m_tlu->CheckEventFIFO();
        m_tlu->ReadEventFIFO(std::ref(m_evtqueue));
        if (dump_events) m_tlu->DumpEvents();
        m_tlu->ClearEventFIFO();
      }
      if (JustStopped) {
	// 	m_tlu->Update(timestamps);
	SendEvent(TLUEvent::EORE(m_run, ++m_ev));
	TLUJustStopped = false;
      }
    } while (!done);
    std::cout << "Exiting main loop" << std::endl;
  }


  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      if (m_tlu) {
      } else {
        m_tlu = std::unique_ptr<miniTLUController>(new miniTLUController(param.Get("ConnectionFile","file:///dummy_connections.xml"),param.Get("DeviceName","dummy.udp")));
      }

      std::cout << "Firmware version " << std::hex << m_tlu->GetFirmwareVersion() << std::endl;
	
      m_tlu->SetCheckConfig(param.Get("CheckConfig",1));

      readout_delay = param.Get("ReadoutDelay",100);
      dump_events = param.Get("DumpEvents",0);
      m_tlu->SetMaxReadSize(param.Get("MaxRead",0x2000));
      if (dump_events) std::cout << "Dump of events enabled. Expect bandwidth issues." << std::endl;
      m_tlu->AllTriggerVeto();
      m_tlu->InitializeI2C(param.Get("I2C_DAC_Addr",0x1f),
			   param.Get("I2C_ID_Addr",0x50));
      if (param.Get("UseIntDACValues",1)) {
	m_tlu->SetDACValue(0, param.Get("DACIntThreshold0",0x4100));
	m_tlu->SetDACValue(1, param.Get("DACIntThreshold1",0x4100));
	m_tlu->SetDACValue(2, param.Get("DACIntThreshold2",0x4100));
	m_tlu->SetDACValue(3, param.Get("DACIntThreshold3",0x4100));
      } else {
	m_tlu->SetThresholdValue(0, param.Get("DACThreshold0",1.3));
	m_tlu->SetThresholdValue(1, param.Get("DACThreshold1",1.3));
	m_tlu->SetThresholdValue(2, param.Get("DACThreshold2",1.3));
	m_tlu->SetThresholdValue(3, param.Get("DACThreshold3",1.3));
      }
      if(param.Get("resetClocks",0)) {
	m_tlu->ResetBoard();
      }
      if(param.Get("resetSerdes",0)) {
        m_tlu->ResetSerdes();
      }
      m_tlu->ConfigureInternalTriggerInterval(param.Get("InternalTriggerInterval",42));
      m_tlu->SetTriggerMask(param.Get("TriggerMask",0x0));
      m_tlu->SetDUTMask(param.Get("DUTMask",0x0));
      m_tlu->SetEnableRecordData(param.Get("EnableRecordData",0x0));
      // write DUT mask (not implemented)
      // write DUT style (not implemented)

      // by dhaas
      eudaq::mSleep(1000);

      for (; m_senderthreads < param.Get("SenderThreads",0xf); m_senderthreads++) {
	std::thread packet_sender0(&miniTLUProducer::PacketSender, this, m_senderthreads, std::ref(m_evtqueue));
	packet_sender0.detach();
      }


// 	m_tlu->Update(timestamps);
      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }

  virtual void OnStartRun(unsigned param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    if(!m_tlu) {
	SetStatus(eudaq::Status::LVL_ERROR, "miniTLU connection not present!");
	return;
    }
    try {
      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << param << std::endl;
      TLUEvent ev(TLUEvent::BORE(m_run));
 	ev.SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
	ev.SetTag("BoardID", to_string(m_tlu->GetBoardID()));
	eudaq::mSleep(5000); // temporarily, to fix startup with EUDRB
      SendEvent(ev);
      eudaq::mSleep(5000);
      
      m_tlu->ResetCounters();
      std::cout << "Words in FIFO before start " << m_tlu->GetEventFifoFillLevel() << std::endl;
      m_tlu->CheckEventFIFO();
      m_tlu->ReadEventFIFO();
      m_tlu->ClearEventFIFO(); // software side
      m_tlu->ResetEventFIFO(); // hardware side
      m_tlu->NoneTriggerVeto();
      
      SetStatus(eudaq::Status::LVL_OK, "Started");
      TLUStarted = true;
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
    std::cout << "Done starting" << std::endl;
  }

  virtual void OnStopRun() {
    try {
      std::cout << "Stop Run" << std::endl;
      m_tlu->AllTriggerVeto();
      TLUStarted = false;
      TLUJustStopped = true;
      while (TLUJustStopped) {
	eudaq::mSleep(100);
      }
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
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
    eudaq::mSleep(1000);
  }

  virtual void OnReset() {
    try {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
      // 	m_tlu->Stop(); // stop
// 	m_tlu->Update(false); // empty events
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  }

  virtual void OnStatus() {
    m_status.SetTag("TRIG", to_string(m_ev));
//       if (m_tlu) {
// 	m_status.SetTag("TIMESTAMP", to_string(Timestamp2Seconds(m_tlu->GetTimestamp())));
// 	m_status.SetTag("LASTTIME", to_string(Timestamp2Seconds(lasttime)));
// 	m_status.SetTag("PARTICLES", to_string(m_tlu->GetParticles()));
// 	m_status.SetTag("STATUS", m_tlu->GetStatusString());
// 	for (int i = 0; i < 4; ++i) {
// 	  m_status.SetTag("SCALER" + to_string(i), to_string(m_tlu->GetScaler(i)));
// 	}
//       }
      //std::cout << "Status " << m_status << std::endl;
  }

  virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0)
      std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Unrecognised command");
  }

private:
  unsigned m_run, m_ev;
  unsigned trigger_interval, dut_mask, veto_mask, and_mask, or_mask;
  uint32_t strobe_period, strobe_width;
  unsigned enable_dut_veto;
  unsigned trig_rollover, readout_delay, dump_events;
  bool timestamps, done, timestamp_per_run;
  bool TLUStarted;
  bool TLUJustStopped;
  std::unique_ptr<miniTLUController> m_tlu;
  uint64_t lasttime;
  std::mutex              m_locksend;
  RawDataQueue m_evtqueue;
  unsigned m_senderthreads;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ TLU Producer", "1.0", "The Producer task for the Trigger Logic Unit");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> op_trace(op, "t", "tracefile", "", "filename", "Log file for tracing USB access");
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


