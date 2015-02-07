#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include "tlu/USBTracer.hh"
#include <iostream>
#include <ostream>
#include <cctype>
#include <memory>
#include <ctime>

typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;
#ifdef WIN32
ZESTSC1_ERROR_FUNC ZestSC1_ErrorHandler=NULL;  // Windows needs some parameters for this. i dont know where it will be called so we need to check it in future
char *ZestSC1_ErrorStrings[]={"bla bla","blub"};
#endif

class ExampleTLUProducer: public eudaq::Producer {
public:
	ExampleTLUProducer(const std::string & name, const std::string & runcontrol) :
	  eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), trigger_interval(0), 
	  timestamps(true), done(false), timestamp_per_run(false), TLUStarted(false), TLUJustStopped(false), lasttime(0) {
	}
	void MainLoop() {
		do {
			bool JustStopped = TLUJustStopped;
			if (JustStopped) {
				eudaq::mSleep(100);
			}

			if( TLUStarted ) {
                        	m_ev++;
	                        uint64_t t = clock(); 
				TLUEvent ev(m_run, m_ev, t);
				SendEvent(ev);
                        }

			if (JustStopped) {
				SendEvent(TLUEvent::EORE(m_run, ++m_ev));
				TLUJustStopped = false;
			}

		} while (!done);
	}
	virtual void OnConfigure(const eudaq::Configuration & param) {
		SetStatus(eudaq::Status::LVL_OK, "Wait");
		try {
			std::cout << "Configuring (" << param.Name() << ")..." << std::endl;

			trigger_interval = param.Get("TriggerInterval", 0);
			timestamps = param.Get("Timestamps", 1);
			timestamp_per_run = param.Get("TimestampPerRun", false);
			// ***
			eudaq::mSleep(1000);

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
			ev.SetTag("TimestampZero", to_string( clock() ) );
			eudaq::mSleep(5000); // temporarily, to fix startup with EUDRB
			SendEvent(ev);
			eudaq::mSleep(5000);
			TLUStarted = true;
			SetStatus(eudaq::Status::LVL_OK, "Started");
		} catch (const std::exception & e) {
			printf("Caught exception: %s\n", e.what());
			SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
		} catch (...) {
			printf("Unknown exception\n");
			SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
		}

   	        // At the end, set the status that will be displayed in the Run Control.
		SetStatus(eudaq::Status::LVL_OK, "Running");
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
		SetStatus(eudaq::Status::LVL_OK, "Terminating");
		std::cout << "Terminate (press enter)" << std::endl;
		done = true;
	}

	virtual void OnReset() {
		try {
			std::cout << "Reset" << std::endl;
			SetStatus(eudaq::Status::LVL_OK);
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
        unsigned trigger_interval;
	bool timestamps, done, timestamp_per_run;
	bool TLUStarted;
	bool TLUJustStopped;
	uint64_t lasttime;
};

int main(int /*argc*/, const char ** argv) {
	eudaq::OptionParser op("EUDAQ TLU Producer", "1.0", "The Producer task for the Trigger Logic Unit");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
	eudaq::Option<std::string> op_trace(op, "t", "tracefile", "", "filename", "Log file for tracing USB access");
        eudaq::Option<std::string> name (op, "n", "name", "Example", "string", "The name of this Producer");
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());
		if (op_trace.Value() != "") {
			setusbtracefile(op_trace.Value());
		}
		ExampleTLUProducer producer(name.Value(), rctrl.Value());
		producer.MainLoop();
		std::cout << "Quitting" << std::endl;
	} catch (...) {
		return op.HandleMainException();
	}
	return 0;
}

