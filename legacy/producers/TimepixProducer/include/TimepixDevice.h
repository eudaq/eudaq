/*
 * Time.h
 *
 *  Created on: May 7, 2013
 *      Author: mbenoit
 */

#ifndef TIMEPIXDEVICE_H_
#define TIMEPIXDEVICE_H_

#include <iostream>
#include <fstream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>    /* POSIX Threads */
#include "mpxmanagerapi.h"
#include "mpxhw.h"

using namespace std ;

void AcquisitionPreStarted(CBPARAM /*par*/,INTPTR /*aptr*/);
void AcquisitionStarted(CBPARAM /*par*/,INTPTR /*aptr*/);
void AcquisitionFinished(int /*stuff*/,int /*stuff2*/);
void FrameIsReady(int /*stuff*/,int /*stuff2*/);

class TimepixDevice {

public:

	TimepixDevice();
	void Configure(const std::string & binary_config, const std::string & ascii_config);
	int  ReadFrame(char * Filename, char* buffer);
	int SetTHL(int THL);
	int SetIkrum(int IKrum);
	int ReadDACs();
	int PerformAcquisition(char *output);
	int GetFrameData(byte *buffer);
	int GetFrameData2(char * Filename, char* buffer);
	int GetFrameDatadAscii(char * Filename, u32* buffer);
	u32 GetBufferSize(){return size;};
	int Abort ();
   	int SetAcqTime(double time);
   	char* GetChipID(){return chipBoardID;};
	
private:
		int control; // Control int for verifying success of operation
		DEVID devId; // Timepix Device ID
		int count; // Number of chip connected to FitPix
		int numberOfChips;
		int numberOfRows;
		char chipBoardID[MPX_MAX_CHBID];
		char ifaceName[MPX_MAX_IFACENAME];
		double acqTime ;
		bool acqFinished;

		int hwInfoCount;
		int mode;
		DACTYPE dacVals[14];
		int chipnumber;
		u32 size;
		FRAMEID FrameID;
		string ascii_config,binary_config;
};

#endif /* MIMTLU_H_ */
