
#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include <iostream>
#include <ostream>
#include <cctype>
#include <memory>

typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;
#ifdef WIN32
ZESTSC1_ERROR_FUNC ZestSC1_ErrorHandler=NULL;  // Windows needs some parameters for this. i dont know where it will be called so we need to check it in future
char *ZestSC1_ErrorStrings[]={"bla bla","blub"};
#endif

class TLUProducer: public eudaq::Producer {
public:
	TLUProducer(const std::string & runcontrol) :
	  eudaq::Producer("TLU", runcontrol), m_run(0), m_ev(0), trigger_interval(0), dut_mask(0), veto_mask(0), and_mask(255),
	  or_mask(0), pmtvcntlmod(0), strobe_period(0), strobe_width(0), enable_dut_veto(0), trig_rollover(0), readout_delay(100),
	  timestamps(true), done(false), timestamp_per_run(false), TLUStarted(false), TLUJustStopped(false), lasttime(0), m_tlu(0) {
	  for(int i = 0; i < TLU_PMTS; i++)
	  {
	      pmtvcntl[i] = PMT_VCNTL_DEFAULT;
	      pmt_id[i] = "<unknown>";
	      pmt_gain_error[i] = 1.0;
	      pmt_offset_error[i] = 0.0;
	  }
	}
	void MainLoop() {
		do {
			if (!m_tlu) {
				eudaq::mSleep(50);
				continue;
			}
			bool JustStopped = TLUJustStopped;
			if (JustStopped) {
				m_tlu->Stop();
				eudaq::mSleep(100);
			}
			if (TLUStarted || JustStopped) {
				eudaq::mSleep(readout_delay);
				m_tlu->Update(timestamps); // get new events
				if (trig_rollover > 0 && m_tlu->GetTriggerNum() > trig_rollover) {
					bool inhibit = m_tlu->InhibitTriggers();
					m_tlu->ResetTriggerCounter();
					m_tlu->InhibitTriggers(inhibit);
				}
				//std::cout << "--------" << std::endl;
				for (size_t i = 0; i < m_tlu->NumEntries(); ++i) {
					m_ev = m_tlu->GetEntry(i).Eventnum();
					uint64_t t = m_tlu->GetEntry(i).Timestamp();
					int64_t d = t - lasttime;
					//float freq= 1./(d*20./1000000000);
					float freq = 1. / Timestamp2Seconds(d);
					if (m_ev < 10 || m_ev % 1000 == 0) {
						std::cout << "  " << m_tlu->GetEntry(i) << ", diff=" << d << (d <= 0 ? "  ***" : "") << ", freq=" << freq << std::endl;
					}
					lasttime = t;
					TLUEvent ev(m_run, m_ev, t);
					ev.SetTag("trigger",m_tlu->GetEntry(i).trigger2String());
					if (i == m_tlu->NumEntries() - 1) {
						ev.SetTag("PARTICLES", to_string(m_tlu->GetParticles()));
						for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
							ev.SetTag("SCALER" + to_string(i), to_string(m_tlu->GetScaler(i)));
						}
					}
					SendEvent(ev);
				}
//         if (m_tlu->NumEntries()) {
//           std::cout << "========" << std::endl;
//         } else {
//           std::cout << "." << std::flush;
//         }
			}
			if (JustStopped) {
				m_tlu->Update(timestamps);
				SendEvent(TLUEvent::EORE(m_run, ++m_ev));
				TLUJustStopped = false;
			}
		} while (!done);
	}
	virtual void OnConfigure(const eudaq::Configuration & param) {
		SetStatus(eudaq::Status::LVL_OK, "Wait");
		try {
			std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
			if (m_tlu)
				m_tlu = 0;
			int errorhandler = param.Get("ErrorHandler", 2);
			m_tlu = std::make_shared<TLUController>(errorhandler);

			trigger_interval = param.Get("TriggerInterval", 0);
			dut_mask = param.Get("DutMask", 2);
			and_mask = param.Get("AndMask", 0xff);
			or_mask = param.Get("OrMask", 0);
			strobe_period = param.Get("StrobePeriod", 0);
			strobe_width = param.Get("StrobeWidth", 0);
			enable_dut_veto = param.Get("EnableDUTVeto", 0);
			handshake_mode = param.Get("HandShakeMode" , 63);
			veto_mask = param.Get("VetoMask", 0);
			trig_rollover = param.Get("TrigRollover", 0);
			timestamps = param.Get("Timestamps", 1);
			for(int i = 0; i < TLU_PMTS; i++)  // Override with any individually set values
			{
			  pmtvcntl[i] = (unsigned)param.Get("PMTVcntl" + to_string(i + 1), "PMTVcntl", PMT_VCNTL_DEFAULT);
			  pmt_id[i] = param.Get("PMTID" + to_string(i + 1), "<unknown>");
			  pmt_gain_error[i] = param.Get("PMTGainError" + to_string(i + 1), 1.0);
			  pmt_offset_error[i] = param.Get("PMTOffsetError" + to_string(i + 1), 0.0);
			}
			pmtvcntlmod = param.Get("PMTVcntlMod", 0);  // If 0, it's a standard TLU; if 1, the DAC output voltage is doubled
			readout_delay = param.Get("ReadoutDelay", 1000);
			timestamp_per_run = param.Get("TimestampPerRun", false);
			// ***
			m_tlu->SetDebugLevel(param.Get("DebugLevel", 0));
			m_tlu->SetFirmware(param.Get("BitFile", ""));
			m_tlu->SetVersion(param.Get("Version", 0));
			m_tlu->Configure();
			for (int i = 0; i < tlu::TLU_LEMO_DUTS; ++i) {
				m_tlu->SelectDUT(param.Get("DUTInput", "DUTInput" + to_string(i), "RJ45"), 1 << i, false);
			}
			m_tlu->SetTriggerInterval(trigger_interval);
			m_tlu->SetPMTVcntlMod(pmtvcntlmod);
			m_tlu->SetPMTVcntl(pmtvcntl, pmt_gain_error, pmt_offset_error);
			m_tlu->SetDUTMask(dut_mask);
			m_tlu->SetVetoMask(veto_mask);
			m_tlu->SetAndMask(and_mask);
			m_tlu->SetOrMask(or_mask);
			m_tlu->SetStrobe(strobe_period, strobe_width);
			m_tlu->SetEnableDUTVeto(enable_dut_veto);
			m_tlu->SetHandShakeMode(handshake_mode);
			m_tlu->SetTriggerInformation(USE_TRIGGER_INPUT_INFORMATION);
			m_tlu->ResetTimestamp();

			// by dhaas
			eudaq::mSleep(1000);

			m_tlu->Update(timestamps);
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
		try {
			m_run = param;
			m_ev = 0;
			std::cout << "Start Run: " << param << std::endl;
			TLUEvent ev(TLUEvent::BORE(m_run));
			ev.SetTag("FirmwareID", to_string(m_tlu->GetFirmwareID()));
			ev.SetTag("TriggerInterval", to_string(trigger_interval));
			ev.SetTag("DutMask", "0x" + to_hex(dut_mask));
			ev.SetTag("AndMask", "0x" + to_hex(and_mask));
			ev.SetTag("OrMask", "0x" + to_hex(or_mask));
			ev.SetTag("VetoMask", "0x" + to_hex(veto_mask));
			for(int i = 0; i < TLU_PMTS; i++)  // Separate loops so they are sequential in file
			{
			    ev.SetTag("PMTID" + to_string(i + 1), pmt_id[i]);
			}
			ev.SetTag("PMTVcntlMod", to_string(pmtvcntlmod));
			for(int i = 0; i < TLU_PMTS; i++)
			{
			    ev.SetTag("PMTVcntl" + to_string(i + 1), to_string(pmtvcntl[i]));
			}
			for(int i = 0; i < TLU_PMTS; i++)  // Separate loops so they are sequential in file
			{
			    ev.SetTag("PMTGainError" + to_string(i + 1), pmt_gain_error[i]);
			}
			for(int i = 0; i < TLU_PMTS; i++)  // Separate loops so they are sequential in file
			{
			    ev.SetTag("PMTOffsetError" + to_string(i + 1), pmt_offset_error[i]);
			}
			ev.SetTag("ReadoutDelay", to_string(readout_delay));
			//      SendEvent(TLUEvent::BORE(m_run).SetTag("Interval",trigger_interval).SetTag("DUT",dut_mask));
			ev.SetTag("TimestampZero", to_string(m_tlu->TimestampZero()));
			eudaq::mSleep(5000); // temporarily, to fix startup with EUDRB
			SendEvent(ev);
			if (timestamp_per_run)
				m_tlu->ResetTimestamp();
			eudaq::mSleep(5000);
			m_tlu->ResetTriggerCounter();
			if (timestamp_per_run)
				m_tlu->ResetTimestamp();
			m_tlu->ResetScalers();
			m_tlu->Update(timestamps);
			m_tlu->Start();
			TLUStarted = true;
			SetStatus(eudaq::Status::LVL_OK, "Started");
		} catch (const std::exception & e) {
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
			m_tlu->Stop(); // stop
			m_tlu->Update(false); // empty events
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
		if (m_tlu) {
			m_status.SetTag("TIMESTAMP", to_string(Timestamp2Seconds(m_tlu->GetTimestamp())));
			m_status.SetTag("LASTTIME", to_string(Timestamp2Seconds(lasttime)));
			m_status.SetTag("PARTICLES", to_string(m_tlu->GetParticles()));
			m_status.SetTag("STATUS", m_tlu->GetStatusString());
			for (int i = 0; i < 4; ++i) {
				m_status.SetTag("SCALER" + to_string(i), to_string(m_tlu->GetScaler(i)));
			}
		}
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
    unsigned trigger_interval, dut_mask, veto_mask, and_mask, or_mask, pmtvcntl[TLU_PMTS], pmtvcntlmod;
	uint32_t strobe_period, strobe_width;
    unsigned enable_dut_veto , handshake_mode;
	unsigned trig_rollover, readout_delay;
	bool timestamps, done, timestamp_per_run;
	bool TLUStarted;
	bool TLUJustStopped;
	uint64_t lasttime;
	std::shared_ptr<TLUController> m_tlu;
	std::string pmt_id[TLU_PMTS];
	double pmt_gain_error[TLU_PMTS], pmt_offset_error[TLU_PMTS];
};

int main(int /*argc*/, const char ** argv) {
	eudaq::OptionParser op("EUDAQ TLU Producer", "1.0", "The Producer task for the Trigger Logic Unit");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
	eudaq::Option<std::string> op_trace(op, "t", "tracefile", "", "filename", "Log file for tracing USB access");
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());
		if (op_trace.Value() != "") {
			setusbtracefile(op_trace.Value());
		}
		TLUProducer producer(rctrl.Value());
		producer.MainLoop();
		std::cout << "Quitting" << std::endl;
		eudaq::mSleep(300);
	} catch (...) {
		return op.HandleMainException();
	}
	return 0;
}

