#include "stdafx.h"
#include "afxwin.h"
#include "TimePixDAQStatus.h"

#ifndef WINVER
#define WINVER = 0x0501
#endif


short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

TimePixDAQStatus::TimePixDAQStatus(parPort port)
{
	parPortAddress = port;
	parPortStatusReg = parPortAddress+1;
	parPortControlReg = parPortAddress+2;

	pthread_mutex_init( &_parportMutex, 0 );
}

TimePixDAQStatus::~TimePixDAQStatus()
{
    pthread_mutex_destroy( &_parportMutex);		
}

void TimePixDAQStatus::parPortSetBusyLineHigh()
{
    pthread_mutex_lock( &_parportMutex);		

	int controlRegister = _Inp32( parPortControlReg );
	//int new_register = 0;
	controlRegister &= 0xFE;
	
	Out32(parPortControlReg, controlRegister);

	pthread_mutex_unlock( &_parportMutex);		
}

void TimePixDAQStatus::parPortSetBusyLineLow()
{
	pthread_mutex_lock( &_parportMutex);		

	int controlRegister = _Inp32( parPortControlReg );
	//int new_register = 0;		
	//controlRegister &= 0xFE;
	controlRegister |= 0x01;

	
	Out32(parPortControlReg, controlRegister);

	pthread_mutex_unlock( &_parportMutex);		
	
}

//checks Status of Trigger Line, 1 is active
int TimePixDAQStatus::parPortCheckBusyLine()
{
	int retval;

	pthread_mutex_lock( &_parportMutex);		

	int controlRegister = _Inp32( parPortControlReg );

	if (controlRegister & 0x01)
		retval = LOW;
	else
		retval = HIGH;

	pthread_mutex_unlock( &_parportMutex);		

	return retval;
}

int TimePixDAQStatus::parPortCheckTriggerLine()
{
	int retval;

	pthread_mutex_lock( &_parportMutex);		

	int statusRegister = _Inp32( parPortStatusReg );
	
	if (statusRegister & 0x80)
		retval = LOW;
	else 
		retval = HIGH;

	pthread_mutex_unlock( &_parportMutex);		

	return retval;
}


void TimePixDAQStatus::parPortUpdateAddress(parPort port)
{
	pthread_mutex_lock( &_parportMutex);		

	parPortAddress = port;
	parPortStatusReg = parPortAddress+1;
	parPortControlReg = parPortAddress+2;

	pthread_mutex_unlock( &_parportMutex);		
}

// do not protext this by mutex. It's internally only called by mutex protected functions
int TimePixDAQStatus::_Inp32(short par)
{	static int retval;
	retval = Inp32(par);
	if(retval == 0xFF)
		return AfxMessageBox("Please enter valid Port Address", MB_ICONERROR, 0);//MessageBox(NULL, "Please enter valid Port Address", "I/O Error", MB_ICONERROR);
	else
		return retval;
}