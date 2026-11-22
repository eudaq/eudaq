/////////////////////////////////////////////////////////////////////
//                         2024 June 07                            //
//                   authors: Jordan Damgov                        //
//                email: Jordan.Damgov@ttu.edu                     //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#ifndef _FERS_EUDAQ_SHM_h
#define _FERS_EUDAQ_SHM_h

#include "CAENDigitizer.h"
//#include "FERSlib.h"
#define MAX_NBRD                       20
#define MAX_NGR                         8
//#include <vector>
//#include "paramparser.h"
//#include <map>
// shared structure
#include <iostream>
#include <fstream>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>

#include <chrono>


#define SHM_KEY 0x12345
#define MAXCHAR 30 // max size of chars in following struct. DON'T MAKE IT TOO BIG!!!
struct shmseg {
        // FERS
        int connectedboards[MAX_NGR] = {0}; // number of connected FERS boards
        //int nchannels[MAX_NGR][MAX_NBRD];
        int handle[MAX_NGR][MAX_NBRD]; // handle is given by FERS_OpenDevice()
	int flat_idx[MAX_NGR][MAX_NBRD];
	int FERS_TDLink[MAX_NGR][MAX_NBRD];
	uint64_t FERS_last_trigID[MAX_NGR][MAX_NBRD]={{0}};

        //char IP[MAX_NBRD][MAXCHAR]; // IP address
        //char producer[MAX_NBRD][MAXCHAR]; // title of producer
        float HVbias[MAX_NGR][MAX_NBRD]; // HV bias
        //char collector[MAX_NBRD][MAXCHAR]; // title of data collector
        std::chrono::high_resolution_clock::time_point FERS_Aqu_start_time_us ;
        std::chrono::high_resolution_clock::time_point FERS_last_event_time_us ;
	//int FERS_offset_us = 0;

	// FERS monitoring
        std::chrono::high_resolution_clock::time_point FERS_LastSrvEvent_us[MAX_NGR][MAX_NBRD];
	float tempFPGA[MAX_NGR][MAX_NBRD];
	float tempDet[MAX_NGR][MAX_NBRD];
        float hv_Vmon[MAX_NGR][MAX_NBRD];
	float hv_Imon[MAX_NGR][MAX_NBRD];
	uint8_t hv_status_on[MAX_NGR][MAX_NBRD];

	uint16_t SUMenergyLG[24][64];
	uint16_t SUMenergyHG[24][64];
	int SUMevt;

        // DRS
        int connectedboardsDRS = 0; // number of connected DRS boards
        std::chrono::high_resolution_clock::time_point DRS_Aqu_start_time_us ;
        std::chrono::high_resolution_clock::time_point DRS_last_event_time_us ;
        int nevtDRS[20];
	uint32_t DRS_last_trigID[20]={0};

        // QTP
        int connectedboardsQTP = 0; // number of connected QTP boards

};

std::chrono::time_point<std::chrono::high_resolution_clock> get_midnight_today();

#endif
