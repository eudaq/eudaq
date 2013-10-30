#include "MVDController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/counted_ptr.hh"
#include <iostream>
#include <ostream>
#include <sys/time.h>

using eudaq::RawDataEvent;

class MVDProducer: public eudaq::Producer {
public:
	MVDProducer(const std::string & runcontrol) :
		eudaq::Producer("MVD", runcontrol), m_run(0), m_ev(0), done(false),
				m_running(false), m_stoppedrun(false), m_mvd(0) {
	}
	void MainLoop() {
		do {
			eudaq::RawDataEvent ev("MVD", m_run, m_ev);
			if (!m_running) {
				continue;
			}
			//printf(" m_running ");	}
			//m_mvd->ActiveSequence();
			//printf("activ ");
			//while (  (!m_mvd || !m_running || m_mvd->GetStatTrigger()) ) {printf("wait trig\n");};
			//ev.AddBlock(0, m_mvd->Time(0)); printf("time  0\n");
			//ev.AddBlock(0, m_mvd->Time(0));
			if ( !m_mvd->DataBusy()){
				continue;
			}	//{ printf(" busy\n"); }
			ev.AddBlock(0, m_mvd->Time(0));
			if (!m_mvd->DataReady()){
				continue;
				//printf(" ready\n");
			}
			//ev.AddBlock(2, m_mvd->Time(2));
			for (unsigned adc = 0; adc < m_mvd->NumADCs(); ++adc) {
				//printf("ADC=%d\n", adc);
				for (unsigned sub = 0; sub < 2; ++sub) {
					unsigned i = adc * 2 + sub + 1;
					if (!m_mvd->Enabled(adc, sub))
						continue;
					ev.AddBlock(i, m_mvd->Read(adc, sub));
				}
			}
			//ev.AddBlock(6, m_mvd->Time(3));
			SendEvent(ev);
			m_ev++;
		} while (!done);
	}
	virtual void OnConfigure(const eudaq::Configuration & param) {
		try {
			std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
			if (m_mvd) {
				m_mvd = 0;
			}
			m_mvd = new MVDController();
			m_mvd->Configure(param);
			std::cout << "...Configured (" << param.Name() << ")" << std::endl;
			EUDAQ_INFO("Configured (" + param.Name() + ")");
			SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name()
					+ ")");
		} catch (const std::exception & e) {
			printf("Caught exception: %s\n", e.what());
			SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
		} catch (...) {
			printf("Unknown exception\n");
			SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
		}
	}
	virtual void OnStartRun(unsigned param) {
		try {
			m_run = param;
			m_ev = 0;
			std::cout << "Start Run: " << param << std::endl;
			RawDataEvent ev(RawDataEvent::BORE("MVD", m_run));
			//ev.SetTag("FirmwareID",  to_string(m_tlu->GetFirmwareID()));
			SendEvent(ev);
			eudaq::mSleep(500);
			m_running = true;
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
			m_running = false;
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
		sleep(1);
	}
private:
	unsigned m_run, m_ev;
	bool done, m_running, m_stoppedrun;
	struct timeval tv;
	unsigned int sec;
	unsigned int microsec;
	time_t timer;
	struct tm *date;
	counted_ptr<MVDController> m_mvd;
};

int main(int /*argc*/, const char ** argv) {
	eudaq::OptionParser op("EUDAQ MVD Producer", "1.0",
			"The Producer task for the MVD strip telescope");
	eudaq::Option < std::string > rctrl(op, "r", "runcontrol",
			"tcp://localhost:44000", "address",
			"The address of the RunControl application");
	eudaq::Option < std::string > level(op, "l", "log-level", "NONE", "level",
			"The minimum level for displaying log messages locally");
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());
		MVDProducer producer(rctrl.Value());
		producer.MainLoop();
		std::cout << "Quitting" << std::endl;
		eudaq::mSleep(300);
	} catch (...) {
		return op.HandleMainException();
	}
	return 0;
}
