#ifndef DRS4PRODUCER_HH
#define DRS4PRODUCER_HH

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>

#include "strlcpy.h"
#include "DRS.h"

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"

#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <vector>
#include <time.h>
#include <chrono>


class DRS4Producer: public eudaq::Producer  {
public:
	DRS4Producer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
	virtual void OnConfigure(const eudaq::Configuration & config);
	virtual void OnStartRun(unsigned runnumber);
	virtual void OnStopRun();
	virtual void OnTerminate();
	void ReadoutLoop();
	//  virtual ~DRS4Producer();
private:
	virtual void SendRawEvent();
	void SetTimeStamp();
	unsigned m_run, m_ev;
	unsigned m_tlu_waiting_time;
	std::string m_verbosity, m_producerNamem,m_event_type, m_producerName;
	bool m_terminated, m_running, triggering,m_self_triggering;;
	double m_n_self_trigger;
	float m_inputRange;
	eudaq::Configuration m_config;
	std::uint64_t m_timestamp;
	DRS *m_drs;
	int m_serialno;
	DRSBoard *m_b;
	bool is_initalized;
	float time_array[8][1024];
	unsigned short raw_wave_array[8][1024];
	float wave_array[8][1024];
	int n_channels;
	bool m_chnOn[8]; //todo fill with active channels
	std::map<int,std::string> names;
	unsigned char activated_channels;
};
int main(int /*argc*/, const char ** argv);
#endif /*DRS4PRODUCER_HH*/
