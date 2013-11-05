#pragma once

#define HIGH 1
#define LOW 0
typedef int parPort;

#include <pthread.h>
//#include "targetver.h"

class TimePixDAQStatus
{
public:
	TimePixDAQStatus(parPort port);
	virtual ~TimePixDAQStatus();

	int parPortAddress;
	int parPortStatusReg;
	int parPortControlReg;
	
	void parPortSetBusyLineHigh();//ParPort Pin 1 (Strobe) is busy line (active low)
	void parPortSetBusyLineLow();//Par Port Pin 13 (Busy) is trigger input
	int parPortCheckBusyLine();

	void parPortUpdateAddress(parPort port);

	int parPortCheckTriggerLine();

protected:
	int _Inp32(short par); //wraps the Inp32 function from
							//inpout32.dll for error handling

	// a mutex to protext the parallel port. It makes this class thread safe
	pthread_mutex_t _parportMutex;
};