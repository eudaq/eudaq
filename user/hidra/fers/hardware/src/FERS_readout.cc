/*******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/
//#include "MultiPlatform.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>

#include "FERS_MultiPlatform.h"
#include "FERS_LL.h"
#include "FERS_config.h"


#define LLBUFF_SIZE			(16*1024)
#define LLBUFF_CNC_SIZE		(64*1024)
#define EVBUFF_SIZE			(64*1024)			// in bytes
#define MAX_EVENT_SIZE		(64*1024)			// in bytes
#define MAX_NROW_EDTAB		(64*1024)			// Max number of rows in the event descriptor table

#define FERSLIB_QUEUE_SIZE			(64*1024)	// queue size in words
#define FERSLIB_QUEUE_MAX_EVSIZE	4096		// Maximum event size (in words) that can be pushed into the queue
#define FERSLIB_QUEUE_FULL_LEVEL	90			// max queue occupancy (percent) to set the busy condition and stop pushing events
#define FERSLIB_QUEUE_TIMEOUT_MS	100			// timeout in ms 

#define INCR_PNT(p)			((p) = ((p)+1) % LLBUFF_SIZE)
#define MOVE_PNT(p, offs)	((p) = ((p)+(offs)) % LLBUFF_SIZE)
#define BUFF_NB(rp, wp)		(((uint32_t)(wp-rp) % LLBUFF_SIZE))

// *********************************************************************************************************
// Global Variables
// *********************************************************************************************************
static char *LLBuff[FERSLIB_MAX_NBRD] = { NULL };			// buffers for raw data coming from LL protocol. One per board in direct mode (eth/usb), one per concentrator with TDLink
static uint32_t *EvBuff[FERSLIB_MAX_NBRD] = { NULL };		// buffers for event data, before decoding. One per board
static int EvBuff_nb[FERSLIB_MAX_NBRD] = { 0 };				// Number of bytes currently present in EvBuff
static uint32_t* tmp_event = NULL;
static uint64_t Tstamp[FERSLIB_MAX_NBRD] = { 0 };			// Tstamp of the event in EvBuff

static int RO_NumBoards = 0;								// Total num of boards connected for the readout
static int Cnc_NumBoards[FERSLIB_MAX_NCNC] = { 0 };			// Num of boards connected to one concentrator
static int tdl_handle[FERSLIB_MAX_NCNC][8][16] = { -1 };	// TDL handle map (gives the handle of the board connected to a specific cnc/chain/node)
static int Cnc_Flushed[FERSLIB_MAX_NCNC] = { 0 };			// Flag indicating that the concentrator has been flushed
static uint32_t *DescrTable[FERSLIB_MAX_NCNC] = { NULL };	// Event Descriptor Table
static int InitReadout[FERSLIB_MAX_NBRD] = { 0 };			// Readout initialized (buffer allocated)
 
static SpectEvent_t SpectEvent[FERSLIB_MAX_NBRD];			// Decoded event (spectroscopy mode)
static CountingEvent_t CountingEvent[FERSLIB_MAX_NBRD];		// Decoded event (counting mode)
static WaveEvent_t WaveEvent[FERSLIB_MAX_NBRD];				// Decoded event (waveform mode)
static ListEvent_t ListEvent[FERSLIB_MAX_NBRD];				// Decoded event (timing mode)
static TestEvent_t TestEvent[FERSLIB_MAX_NBRD];				// Decoded event (test mode)
static ServEvent_t ServEvent[FERSLIB_MAX_NBRD];				// Decoded event (test mode)

static uint64_t ServTimeStamp[FERSLIB_MAX_NBRD] = { 0 };	// Time stamp of the last service event
static uint32_t CrcErrorCnt[FERSLIB_MAX_NCNC] = { 0 };		// CRC error counter

static int ReadoutMode = 0;									// Readout Mode
static int EnableStartEvent[FERSLIB_MAX_NBRD] = { 0 };		// Enable Start Event 

// Gloabal Variables for the queues (for sorting)
static uint32_t* queue[FERSLIB_MAX_NBRD] = { NULL };	// memory buffers for queues
static uint64_t q_tstamp[FERSLIB_MAX_NBRD];				// tstamp of the oldest event in the queue (0 means empty queue)
static uint64_t q_trgid[FERSLIB_MAX_NBRD];				// trgid of the oldest event in the queue (0 means empty queue)
static int q_wp[FERSLIB_MAX_NBRD];						// Write Pointer
static int q_rp[FERSLIB_MAX_NBRD];						// Read Pointer
static int q_nb[FERSLIB_MAX_NBRD];						// num of words in the queue 
static int q_full[FERSLIB_MAX_NBRD];					// queue full (nb > max occupancy)
static int q_busy = 0;									// num of full queues

//extern uint8_t EnableRawData;								// Enable Low Level data saving
//extern uint8_t ProcessRawData;								// Enable ReadingOut the RawData file saved
//extern uint8_t EnableMaxSizeFile;							// Enable the Max size for Raw Data file saving
//extern float MaxSizeRawDataFile;							// Max Size for LLData saving
//extern char RawDataFilename[FERSLIB_MAX_NBRD][500];			// Rawdata Filename
//extern mutex_t FERS_RoMutex;
int FERS_RunningCnt = 0;								// Num of running boards 
//extern FERS_BoardInfo_t* FERS_BoardInfo[FERSLIB_MAX_NBRD];
//extern float CLK_PERIOD[FERSLIB_MAX_NBRD];

// *********************************************************************************************************
// Queue functions (push and pop)
// *********************************************************************************************************
int q_push(int qi, uint32_t *buff, int size) {
	int tsize = size + 1;  // event data + size
	static FILE *ql = NULL;
	if ((DebugLogs & DBLOG_QUEUES) && (ql == NULL)) ql = fopen("queue_log.txt", "w");
	if (q_full[qi] || (tsize > FERSLIB_QUEUE_MAX_EVSIZE)) {
		if (ql != NULL) fprintf(ql, "Queue Overrun (writing while busy)!!!\n");
		return FERSLIB_ERR_QUEUE_OVERRUN;
	}
	if ((q_wp[qi] < (q_rp[qi] - tsize - 1)) || (q_wp[qi] >= q_rp[qi])) {
		if (q_tstamp[qi] == 0) q_tstamp[qi] = ((uint64_t)buff[4] << 32) | (uint64_t)buff[3];
		if (q_trgid[qi] == 0) q_trgid[qi] = ((uint64_t)buff[2] << 32) | (uint64_t)buff[1];
		*(queue[qi] + q_wp[qi]) = size;
		memcpy(queue[qi] + q_wp[qi] + 1, buff, size * sizeof(uint32_t));
		if (ql != NULL) fprintf(ql, "Push Q%d: %12.3f %8" PRIu64 "\n", qi, (((uint64_t)buff[4] << 32) | (uint64_t)buff[3])*CLK_PERIOD[qi]/1000.0, ((uint64_t)buff[2] << 32) | (uint64_t)buff[1]);
		q_wp[qi] += tsize;
		if ((q_wp[qi] + FERSLIB_QUEUE_MAX_EVSIZE) > FERSLIB_QUEUE_SIZE) q_wp[qi] = 0;

		q_nb[qi] += tsize;
		if (q_nb[qi] > (FERSLIB_QUEUE_SIZE * 100 / FERSLIB_QUEUE_FULL_LEVEL)) {
			q_full[qi] = 1;
			q_busy++;
		}
		return 1;
	}
	return 0;
}

int q_pop(int qi, uint32_t *buff, int *size) {
	uint32_t* nextev;
	if (q_tstamp[qi] == 0) return 0;
	*size = *(queue[qi] + q_rp[qi]);
	memcpy(buff, queue[qi] + q_rp[qi] + 1, *size * sizeof(uint32_t));
	q_rp[qi] += *size + 1;
	if ((q_rp[qi] + FERSLIB_QUEUE_MAX_EVSIZE) > FERSLIB_QUEUE_SIZE) q_rp[qi] = 0;
	q_nb[qi] -= (*size + 1);
	if (q_rp[qi] == q_wp[qi]) { // it was last event => queue is now empty 
		q_tstamp[qi] = 0;  
		q_trgid[qi] = 0;  
	} else {
		nextev = queue[qi] + q_rp[qi] + 1;  // Get tstamp of next event in the queue
		q_tstamp[qi] = ((uint64_t)nextev[4] << 32) | (uint64_t)nextev[3];
		q_trgid[qi] = ((uint64_t)nextev[2] << 32) | (uint64_t)nextev[1];
	}
	if (q_full[qi]) {
		q_full[qi] = 0;
		q_busy--;
	}
	return 1;
}


// *********************************************************************************************************
// Read Raw Event (from board, either Eth or USB)
// Event data are written into the EvBuff of the relevant board.
// *********************************************************************************************************
/* 
Data Format from PIC (new protocol):
0	0x82000000	Start Marker
1	0xDQDSDSDS	DQ = data qualifier, DS: data size (num of 32 bit words in the payload)
2	0xTDTDTDTD	TD: Trigger-id[31:0]
3	0x00TDTDTD	TD: Trigger-id[55:32]
5	0xTSTSTSTS	TS: Time-stamp[31:0]
6	0x00TSTSTS	TS: Time-stamp[55:32]
7	0xPLPLPLPL	PL: Payload
8	0xPLPLPLPL	PL: Payload
	...
	0xF1000000	End Marker

Data Format from PIC (old protocol):
0   0x81000000_00000000
1   0x00000000_DQDSDSDS   DQ = data qualifier, DS: data size (num of 32 bit words in the payload)
2   0x00TDTDTD_TDTDTDTD   TD: Trigger-id
3   0x00TCTCTC_TCTCTCTC	  TC: Time-Stamp
4   0x00PLPLPL_PLPLPLPL   PL: payload -> 32 bit word, aligned to 56 bit
5   0x00PLPLPL_PLPLPLPL
6   0x00PLPLPL_PLPLPLPL
7   0x00PLPLPL_PLPLPLPL   
8   0xC0000000_SZSZSZSZ   SZ: data size in 56 bit words -> can be used to check data integrity
*/

uint32_t get_d32(char *LLBuff_loc, int rp) {
	if (rp == (LLBUFF_SIZE - 1))
		return (*((uint32_t *)(LLBuff_loc + rp)) & 0xFF) | ((*((uint32_t *)LLBuff_loc) & 0xFFFFFF) << 8);
	else if (rp == (LLBUFF_SIZE - 2))
		return (*((uint32_t *)(LLBuff_loc + rp)) & 0xFFFF) | ((*((uint32_t *)LLBuff_loc) & 0xFFFF) << 16);
	else if (rp == (LLBUFF_SIZE - 3))
		return (*((uint32_t *)(LLBuff_loc + rp)) & 0xFFFFFF) | ((*((uint32_t *)LLBuff_loc) & 0xFF) << 24);
	else
		return *((uint32_t *)(LLBuff_loc + rp));
}

static int eth_usb_ReadRawEvent(int handle, int *nb)
{
	static int wp[FERSLIB_MAX_NBRD] = { 0 }, rp[FERSLIB_MAX_NBRD] = { 0 };  // Read and write pointers
	static int wpnt[FERSLIB_MAX_NBRD] = { 0 }, rpp[FERSLIB_MAX_NBRD] = { 0 };
	static int footer_cnt[FERSLIB_MAX_NBRD] = { 0 }, NeedMoreData[FERSLIB_MAX_NBRD] = { 0 };
	static uint32_t size[FERSLIB_MAX_NBRD] = { 0 }, bcnt[FERSLIB_MAX_NBRD] = { 0 };
	static int numev[FERSLIB_MAX_NBRD] = { 0 };
	static int NewPMPprot = 0;  // New PMP protocol 
	static int htag_found = 0;
	int i, rvb, ret, nbb, reqsize;
	uint32_t d32, dtq;
	uint64_t footer, now;
	static FILE* rdlog = NULL;  // readout log file
	int h = FERS_INDEX(handle);
	char *EvBuff8 = (char *)EvBuff[h];

	if ((DebugLogs & DBLOG_RAW_DECODE) && (rdlog == NULL)) {
		char fname[100];
		sprintf(fname, "RawDecodeLog_%d.txt", h);
		rdlog = fopen(fname, "w");
	}

	*nb = 0;
	if (FERS_ReadoutStatus == ROSTATUS_FLUSHING) {
		wp[h] = 0;
		rp[h] = 0;
		size[h] = 0;
		NeedMoreData[h] = 0;
		if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) {
			if (ProcessRawData)	LLeth_ReadData_File(h, LLBuff[h], LLBUFF_SIZE, nb, 1);
			else LLeth_ReadData(h, LLBuff[h], LLBUFF_SIZE, nb);
		} else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB) {
			if (ProcessRawData) LLusb_ReadData_File(h, LLBuff[h], LLBUFF_SIZE, nb, 1);
			else LLusb_ReadData(h, LLBuff[h], LLBUFF_SIZE, nb);
		} 
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][BRD %02d] Flushing Data\n", h);
		return 0;
	}

	while (1) {
		nbb = 0;
		if ((wp[h] == rp[h]) || NeedMoreData[h]) {
			// read data from device 
			if ((rp[h] == 0) && (wp[h] == 0)) reqsize = LLBUFF_SIZE / 2;
			else if (wp[h] < rp[h]) reqsize = rp[h] - wp[h] - 1;
			else if (rp[h] == 0) reqsize = LLBUFF_SIZE - wp[h] - 1;
			else reqsize = LLBUFF_SIZE - wp[h];
			if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) {
				if (ProcessRawData) {
					if (FERS_ReadoutStatus == ROSTATUS_FLUSHING) return 2;
					ret = LLeth_ReadData_File(h, LLBuff[h] + wp[h], reqsize, &nbb, 0);
				} else {
					ret = LLeth_ReadData(h, LLBuff[h] + wp[h], reqsize, &nbb);
				}
			} else {
				if (ProcessRawData) {
					if (FERS_ReadoutStatus == ROSTATUS_FLUSHING) return 2;
					ret = LLusb_ReadData_File(h, LLBuff[h] + wp[h], reqsize, &nbb, 0);
				} else {
					ret = LLusb_ReadData(h, LLBuff[h] + wp[h], reqsize, &nbb);
				}
			}
			if (ret < 0) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Error in reading data from the board\n", h);
				return FERSLIB_ERR_READOUT_ERROR;
			}
			if (ret == 2) return 2;
			if (ret == 4) {
				htag_found = 0;
				return 4; // End of RawData read
			}

			// Check flow of service events (should arrive regularly, about one per second)
			now = get_time();
			if ((ServTimeStamp[h] > 0) && (ServTimeStamp[h] < (now - 5000))) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Missing service events\n", h);
				ServTimeStamp[h] = 0;  // prevent multiple warning messages. Send once. 
			}

			if (nbb > 0) {
				MOVE_PNT(wp[h], nbb);
				NeedMoreData[h] = 0;
				if (rdlog != NULL) {
					fprintf(rdlog, "ReadData: req=%d, rx=%d (bytes). rp=%d, wp=%d\n", reqsize, nbb, rp[h], wp[h]);
					fflush(rdlog);
				}
			} else if (NeedMoreData[h]) 
				continue;
		}
		if (wp[h] == rp[h]) return 0;  // No data

		// get size of the payload and transfer header to EvBuff
		if (size[h] == 0) {
			int skip = 0;
			while (!htag_found) {  // advance read pointer until 0x81000000 (old prot) or 0x82000000 (new prot) is found
				if (BUFF_NB(rp[h], wp[h]) < 4) {
					NeedMoreData[h] = 1;
					if (rdlog != NULL) fprintf(rdlog, "Incomplete word. Waiting for more data... rp=%d, wp=%d\n", rp[h], wp[h]);
					break;  // wait for other data to complete at least one word (4 bytes)
				}
				d32 = get_d32(LLBuff[h], rp[h]);
				if (d32 == 0x00000081) {  // conversion to big endian
					htag_found = 1;
					break;  
				}
				if (d32 == 0x82000000) {
					MOVE_PNT(rp[h], 4);
					NewPMPprot = 1;
					htag_found = 1;
					break;
				}
				if (rdlog != NULL) fprintf(rdlog, "Skip byte %d: %02X\n", skip, (uint8_t)(*(LLBuff[h] + rp[h])));
				INCR_PNT(rp[h]);
				skip++;
			} 
			if (skip > 0) {
				if (rdlog != NULL) fprintf(rdlog, "Skip %d bytes. rp=%d, wp=%d\n", skip, rp[h], wp[h]);
			}
			if (NeedMoreData[h]) continue;
			uint32_t nb_head = NewPMPprot ? 20 : 32;
			if (BUFF_NB(rp[h], wp[h]) < nb_head) {
				NeedMoreData[h] = 1;
				if (rdlog != NULL) fprintf(rdlog, "Incomplete header. Waiting for more data... rp=%d, wp=%d\n", rp[h], wp[h]);
				continue;  // wait for header to complete (read more data if NB<32)
			}
			if (NewPMPprot) {
				bcnt[h] = 0;
				wpnt[h] = 0;
				d32 = get_d32(LLBuff[h], rp[h]);
				size[h] = d32 & 0x00FFFFFF; // payload size (in words)
				if (size[h] > (EVBUFF_SIZE / 4)) {
					if (rdlog != NULL) fprintf(rdlog, "Event Size bigger than memory buffer! (%d)\n", size[h]);
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Event Size bigger than memory buffer! (%d)\n", h, size[h]);
					htag_found = 0;
					return FERSLIB_ERR_READOUT_ERROR;
				}
				dtq = (d32 >> 24) & 0xFF;
				EvBuff[h][wpnt[h]++] = (dtq << 24) | (size[h] + 5);  // 1st word of event: dtq and size
				MOVE_PNT(rp[h], 4);
				for (i = 0; i < 4; i++) {  // trgid + tstamp (4 words)
					d32 = get_d32(LLBuff[h], rp[h]);
					EvBuff[h][wpnt[h]++] = d32;
					if (i == 2) Tstamp[h] = d32;
					if (i == 3) Tstamp[h] |= (uint64_t)d32 << 32;
					MOVE_PNT(rp[h], 4);
				}
				if (rdlog != NULL) fprintf(rdlog, "Header complete: size = %d words. rp=%d, wp=%d\n", size[h], rp[h], wp[h]);
			} else {
				MOVE_PNT(rp[h], 12);
				dtq = (uint32_t)LLBuff[h][rp[h]];
				MOVE_PNT(rp[h], 2);
				size[h] = 256 * (uint32_t)((uint8_t*)LLBuff[h])[rp[h]];
				INCR_PNT(rp[h]);
				size[h] += (uint32_t)((uint8_t*)LLBuff[h])[rp[h]];
				INCR_PNT(rp[h]);
				size[h] *= 4;  // payload size (in bytes)
				bcnt[h] = 0;
				rpp[h] = 0;
				wpnt[h] = 0;
				// event size in 32 bit words = payload + 5 (1 word for event size, 2 words for tstamp, 2 words for trg ID)
				EvBuff8[wpnt[h]++] = (size[h] / 4 + 5) & 0xFF;
				EvBuff8[wpnt[h]++] = ((size[h] / 4 + 5) >> 8) & 0xFF;
				EvBuff8[wpnt[h]++] = 0;
				EvBuff8[wpnt[h]++] = (char)dtq;
				for (i = 0; i < 16; i++) {
					EvBuff8[wpnt[h] + (i / 8) * 8 + (7 - i % 8)] = LLBuff[h][rp[h]];  // get two 32 bit words (tstamp and trigger ID). Convert big/little endian
					INCR_PNT(rp[h]);
				}
				Tstamp[h] = *(uint64_t*)(EvBuff8 + wpnt[h] + 8);
				wpnt[h] += 16;
				if (rdlog != NULL) fprintf(rdlog, "Header complete: size = %d bytes. rp=%d, wp=%d\n", size[h], rp[h], wp[h]);
			}
		}

		// transfer payload to EvBuff
		if (NewPMPprot) {
			while (BUFF_NB(rp[h], wp[h]) >= 4) {
				d32 = get_d32(LLBuff[h], rp[h]);
				MOVE_PNT(rp[h], 4);
				if (wpnt[h] >= EVBUFF_SIZE) {
					if (rdlog != NULL) fprintf(rdlog, "Event Buffer overflow!\n");
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Event Buffer overflow\n", h);
					htag_found = 0;
					return FERSLIB_ERR_READOUT_ERROR;
				}
				if (bcnt[h] < size[h]) {  // Payload
					EvBuff[h][wpnt[h]++] = d32;
					bcnt[h]++;
				} else {  // footer
					if (rdlog != NULL) {
						fprintf(rdlog, "Payload complete: PLsize = %d words. rp=%d, wp=%d\n", bcnt[h], rp[h], wp[h]);
						for (i = 0; i < wpnt[h]; i++)
							fprintf(rdlog, "%3d : %08X\n", i, EvBuff[h][i]);
						if (d32 != 0xF1000000)
							fprintf(rdlog, "Invalid footer: %08X. rp=%d, wp=%d\n", d32, rp[h], wp[h]);
						fflush(rdlog);
					}
					size[h] = 0;
					htag_found = 0;
					EvBuff_nb[h] = wpnt[h] * 4;
					*nb = wpnt[h] * 4;
					return 0;
				}
			}
			NeedMoreData[h] = 1;
		} else {
			for (; BUFF_NB(rp[h], wp[h]) > 0; INCR_PNT(rp[h])) {
				if (bcnt[h] < size[h]) {
					if ((rpp[h]++ & 0x7) == 0) continue;  // skip 1st byte of each quartet (data are 56 bit aligned)
					rvb = 3 - (bcnt[h] & 0x3) * 2;
					if ((wpnt[h] + rvb) > EVBUFF_SIZE) {
						if (rdlog != NULL) fprintf(rdlog, "EvBuffer overflow!\n");
						if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] EvBuffer overflow\n", h);
						return FERSLIB_ERR_READOUT_ERROR;
					}
					EvBuff8[wpnt[h] + rvb] = LLBuff[h][rp[h]];  // convert big/little endian
					bcnt[h]++;
					wpnt[h]++;
				} else if ((bcnt[h] == size[h]) && (footer_cnt[h] == 0)) {
					footer_cnt[h] = (size[h] % 7 == 0) ? 8 : 7 - (size[h] % 7) + 8;
					if (footer_cnt[h] == 8) footer = (uint64_t)LLBuff[h][rp[h]];
					else footer = 0;
					if (rdlog != NULL) {
						fprintf(rdlog, "Payload complete: PLsize = %d bytes. rp=%d, wp=%d\n", bcnt[h], rp[h], wp[h]);
						for (i = 0; i < (wpnt[h] / 4); i++)
							fprintf(rdlog, "%3d : %08X\n", i, EvBuff[h][i]);
						fflush(rdlog);
					}
				} else if (footer_cnt != NULL) {  // g++ > 7.5 ... footer_cnt != NULL
					footer_cnt[h]--;
					if ((footer_cnt[h] <= 9) && (footer_cnt[h] > 0)) footer = (footer << 8) | (uint64_t)LLBuff[h][rp[h]];
					if (footer_cnt[h] == 0) {
						if (rdlog != NULL) {
							fprintf(rdlog, "Footer complete: rp=%d, wp=%d\n", rp[h], wp[h]);
							fflush(rdlog);
						}
						/*if (((footer & 0xFF00000000000000) != 0xC000000000000000) && (ENABLE_FERSLIB_LOGMSG)) {
							FERS_LibMsg("[ERROR][BRD %02d]: unexpected footer = %016X\n", h, footer);
							if (rdlog != NULL) fprintf(rdlog, "footer error. rp=%d, wp=%d\n", rp[h], wp[h]);
						}*/
						size[h] = 0;
						htag_found = 0;
						EvBuff_nb[h] = wpnt[h];
						*nb = wpnt[h];
#if (THROUGHPUT_METER == 2)
						// Rate Meter: print raw data throughput (set nb = 0, thus the data consumer doesn't see any data to process)
						static uint64_t totnb = 0, ct = 0, lt = 0, l0 = 0;
						ct = get_time();
						if (l0 == 0) l0 = ct;
						if (lt == 0) lt = ct;
						totnb += *nb;
						if ((ct - lt) > 1000) {
							printf("%6.1f s: %10.6f MB/s\n", (float)(ct - l0) / 1000, (float)(totnb) / (ct - lt) / 1000);
							totnb = 0;
							lt = ct;
						}
						*nb = 0;
#endif
						return 0;
					}
				}
			}
		}
	}
	return 0;
}



// *********************************************************************************************************
// Read Raw Event (from Concentrator)
// *********************************************************************************************************
/*
Data Format from TDlink:
	0  1  2  3
0   FF FF FF FF    Fixed Header Tag
1   FF FF FF FF    Fixed Header Tag
2   rr rr rr ll    l: link num, r: tab size in rows
r0	pp ss ss ss    ss(24): size
	tt pp pp pp    pp(32): pointer
	tt tt tt tt    tt(56): time code
	id id tt tt    id(56): trigger id
	id id id id
	pk pk pk id    pk(56): packet ID
	pk pk pk pk
	vv vv qq bb    bb(8): board ID, qq(8) qualifier, vv(16): valid
r1  ...
r2  ...
	++ ++ ++ ++    raw data (events are pointed by pointers in the table r0, r1...)
	++ ++ ++ ++
	++ ++ ++ ++
*/

/*
#ifdef _MSC_VER
#define PACK( _Declaration_ ) _pragma( pack(push, 1) ) _Declaration_ _pragma( pack(pop))
#else
#define PACK( _Declaration_ ) _Declaration_ _attribute__((__packed_))
#endif

#pragma PACK (push, 1)
typedef struct t_table_row {
	uint32_t size : 24;
	uint32_t pointer : 32;
	uint64_t timecode : 56;
	uint64_t triggerid : 56;
	uint64_t packetid : 56;
	uint16_t board : 8;
	uint16_t qly : 8;
	uint32_t invalid_space : 15;
	bool valid : 1;
};
*/

static int tdl_ReadRawEvent(int cindex, int* bindex, int* nb)
{
	int nbr, ret, tpnt;
	uint32_t evsize, node, dqf, pnt, crc_error;
	uint64_t now, trgid;
	uint32_t header[3];
	static int bpnt, ngap, chain; // last_point
	static uint32_t last_pnt;
	static int table_nrow = 0, table_pnt = 0;
	static FILE* rdlog = NULL;  // readout log file
	//t_table_row *evdescr;

	if ((DebugLogs & DBLOG_RAW_DECODE) && (rdlog == NULL)) {
		char fname[100];
		sprintf(fname, "ReadDecodeLog_%d.txt", cindex);
		rdlog = fopen(fname, "w");
	}

	if (FERS_ReadoutStatus == ROSTATUS_FLUSHING) {
		table_pnt = 0;
		table_nrow = 0;
		bpnt = 0;
		while (bpnt > 0)  // flush old data (maybe not necessary)
			LLtdl_ReadData(cindex, LLBuff[cindex], LLBUFF_CNC_SIZE, &bpnt);
		if (ProcessRawData) LLtdl_ReadData_File(cindex, LLBuff[cindex], LLBUFF_CNC_SIZE, &bpnt, 1);
		//if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][CNC %02d] Flushing Data\n", cindex);
		return 0;
	}

	*nb = 0;
	*bindex = 0;
	if (table_pnt == table_nrow) {  // all event descriptors consumed
		bpnt = 0;
		while (bpnt < 12) {  // Read header (3 words)
			if (ProcessRawData) 
				ret = LLtdl_ReadData_File(cindex, (char*)header + bpnt, 12 - bpnt, &nbr, 0);
			else
				ret = LLtdl_ReadData(cindex, (char*)header + bpnt, 12 - bpnt, &nbr);
			if (ret == 2) return 2;
			if (ret == 4) return 4;	// Finished reading raw data form file

			// Check flow of service events (should arrive regularly, about one per second)
			now = get_time();
			for (int h = 0; h < NumBoardConnected; h++) {
				if ((ServTimeStamp[h] > 0) && (ServTimeStamp[h] < (now - 5000))) {
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Missing service events\n", h);
					ServTimeStamp[h] = 0;  // prevent multiple warning messages. Send once. 
				}
			}

			if ((nbr == 0) && (bpnt == 0)) return 0;  // No data
			bpnt += nbr;
		}
		if ((header[0] != 0xFFFFFFFF) || (header[1] != 0xFFFFFFFF)) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][CNC %02d] Missing 0xFFFFFFFF tags in header\n", cindex);
			return FERSLIB_ERR_READOUT_ERROR;
		}
		chain = header[2] & 0xFF;  // chain ID
		table_nrow = (header[2] >> 8) & 0xFFFFFF;   // tab size in rows (1 row = 8 words = 256 bits)
		if (rdlog != NULL) fprintf(rdlog, "Data from chain %d: Tab size=%d \n", chain, table_nrow);
		if ((table_nrow > MAX_NROW_EDTAB) || (table_nrow == 0)) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][CNC %02d] Wrong tab size (%d)\n", cindex, table_nrow);
			return FERSLIB_ERR_READOUT_ERROR;
		}

		// Read descriptor table
		bpnt = 0;
		while (bpnt < (table_nrow * 32)) {
			if (ProcessRawData)
				ret = LLtdl_ReadData_File(cindex, (char*)DescrTable[cindex] + bpnt, table_nrow * 32 - bpnt, &nbr, 0);
			else
				ret = LLtdl_ReadData(cindex, (char*)DescrTable[cindex] + bpnt, table_nrow * 32 - bpnt, &nbr);
			if (ret == 2) return 2;
			if (ret == 4) return 4;	// Finished reading raw data form file
			bpnt += nbr;
		}
		table_pnt = 0;
		last_pnt = 0;
	}

	tpnt = table_pnt * 8;
	pnt = ((DescrTable[cindex][tpnt] >> 24) & 0xFF) | ((DescrTable[cindex][tpnt + 1] & 0xFFFFFF) << 8);  // pointer to event in data packet
	evsize = DescrTable[cindex][tpnt] & 0xFFFFFF;		// event size (in 32 bit words)
	node = DescrTable[cindex][tpnt + 7] & 0xFF;			// board id in chain
	dqf = (DescrTable[cindex][tpnt + 7] >> 8) & 0xFF;	// data qualifier
	crc_error = (DescrTable[cindex][tpnt + 7] >> 16) & 0x1;  // crc error (bit 240 of table row)

	if (crc_error) {
		CrcErrorCnt[cindex]++;
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][CNC %02d] CRC error found in data from chain %d, node %d (error count = %d)\n", cindex, chain, node, CrcErrorCnt[cindex]);
	}
	*bindex = FERS_INDEX(tdl_handle[cindex][chain][node]);
	trgid = (((uint64_t)DescrTable[cindex][tpnt + 3] >> 16) & 0xFFFF) | ((uint64_t)DescrTable[cindex][tpnt + 4] << 16) | ((uint64_t)(DescrTable[cindex][tpnt + 5] & 0xFF) << 48);
	Tstamp[*bindex] = (((uint64_t)DescrTable[cindex][tpnt + 1] >> 24) & 0xFF) | ((uint64_t)DescrTable[cindex][tpnt + 2] << 8) | ((uint64_t)(DescrTable[cindex][tpnt + 3] & 0xFFFF) << 40);
	table_pnt++;
	if ((evsize + 5) > MAX_EVENT_SIZE) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][CNC %02d] Event size too big (%d)\n", cindex, evsize);
		return FERSLIB_ERR_READOUT_ERROR;
	}

	// skip fillers (gap between end of previous event and begin of next one)
	if (pnt != last_pnt) {
		ngap = (pnt - last_pnt) * 4;
		bpnt = 0;
		while (bpnt < ngap) {
			if (ProcessRawData)
				ret = LLtdl_ReadData_File(cindex, (char*)EvBuff[*bindex], ngap, &nbr, 0);
			else 
				ret = LLtdl_ReadData(cindex, (char*)EvBuff[*bindex], ngap, &nbr);
			if (ret == 2) return 2;
			if (ret == 4) return 4;	// Finished reading raw data form file
			bpnt += nbr;
		}
	}
	last_pnt += ngap / 4;

	// Read Data
	EvBuff[*bindex][0] = (dqf << 24) | (evsize + 5);
	EvBuff[*bindex][1] = (uint32_t)(trgid & 0xFFFFFFFF);
	EvBuff[*bindex][2] = (uint32_t)((trgid >> 32) & 0xFFFFFFFF);
	EvBuff[*bindex][3] = (uint32_t)(Tstamp[*bindex] & 0xFFFFFFFF);
	EvBuff[*bindex][4] = (uint32_t)((Tstamp[*bindex] >> 32) & 0xFFFFFFFF);
	bpnt = 0;
	int timeout = 0;
	while (bpnt < (int)(evsize * 4)) {
		if (ProcessRawData)
			ret = LLtdl_ReadData_File(cindex, (char*)(EvBuff[*bindex] + 5) + bpnt, (evsize * 4) - bpnt, &nbr, 0);
		else 
			ret = LLtdl_ReadData(cindex, (char*)(EvBuff[*bindex] + 5) + bpnt, (evsize * 4) - bpnt, &nbr);
		if (ret == 2) return 2;
		if (ret == 4) return 4;	// Finished reading raw data form file
		bpnt += nbr;
		if (nbr == 0) {
			timeout++;
			Sleep(1);
			if (timeout == 500) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][CNC %02d] Uncomplete event. Missing data to reach the size of %d bytes\n", cindex, evsize * 4);
				return FERSLIB_ERR_READOUT_ERROR;
			}
		} else {
			timeout = 0;
		}

	}
	last_pnt += evsize;
	evsize += 5;
	if (rdlog != NULL) fprintf(rdlog, "Read Event: TS=%d, TrgID=%d, chain=%d, node=%d, size=%d\n", (int)Tstamp[*bindex], (int)trgid, chain, node, evsize);
	EvBuff_nb[*bindex] = evsize;
	*nb = evsize * 4;
#if (THROUGHPUT_METER == 2)
	// Rate Meter: print raw data throughput (set nb = 0, thus the data consumer doesn't see any data to process)
	static uint64_t totnb = 0, ct = 0, lt = 0, l0 = 0;
	ct = get_time();
	if (l0 == 0) l0 = ct;
	if (lt == 0) lt = ct;
	totnb += *nb;
	if ((ct - lt) > 1000) {
		printf("%6.1f s: %10.6f MB/s\n", (float)(ct - l0) / 1000, (float)totnb / (ct - lt) / 1000);
		totnb = 0;
		lt = ct;
}
	*nb = 0;
#endif
		return 0;
	}


// *********************************************************************************************************
// Decode Event Data
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Decode raw data from a buffer into the relevant event data structure
// Inputs:		handle = device handle
//				EvBuff_d = buffer with raw event data
//				nb = num of bytes in EvBuff
// Outputs:		DataQualifier = data qualifier (type of data, used to determine the struct for event data)
//				tstamp_us = time stamp in us (the information is also reported in the event data struct)
//				Event = pointer to the event data struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// --- DECODE 5202 ---
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static int FERS_DecodeEvent_5202(int handle, uint32_t* EvBuff_d, int nb, int* DataQualifier, double* tstamp_us, void** Event)
{
	uint32_t i, hl = 0, en, pnt = 0, se_err = 0;
	uint16_t size;
	int h = FERS_INDEX(handle);
	static FILE* raw = NULL;
	static int log_service_err = 1;

	if (EvBuff_d == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Readout not initialized\n", FERS_INDEX(handle));
		return FERSLIB_ERR_READOUT_NOT_INIT;
	}
	if (nb == 0) return 0;

	// decode event data structure
	size = EvBuff_d[0] & 0xFFFF;  // in 32bit words
	*DataQualifier = (EvBuff_d[0] >> 24) & 0xFF;

	if (*DataQualifier == DTQ_TEST) {  // test Mode 
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		TestEvent[h].tstamp_us = *tstamp_us;
		TestEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		TestEvent[h].nwords = size - 5;
		for (i = 0; i < TestEvent[h].nwords; i++) {
			if (i > MAX_TEST_NWORDS) break;
			TestEvent[h].test_data[i] = EvBuff_d[i + 5];
		}
		*Event = (void*)&TestEvent[h];
	} else if ((*DataQualifier) == DTQ_SERVICE) {
		int cntstart = 6;
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5202 / 1000.0;
		ServEvent[h].update_time = get_time();
		ServTimeStamp[h] = ServEvent[h].update_time;
		ServEvent[h].tstamp_us = *tstamp_us;
		ServEvent[h].pkt_size = size - 5;
		ServEvent[h].version = (EvBuff_d[5] >> 24) & 0xFF;
		for (i = 0; i < FERSLIB_MAX_NCH_5202; i++)	// Reset counters
			ServEvent[h].ch_trg_cnt[i] = 0;
		ServEvent[h].t_or_cnt = 0;
		ServEvent[h].q_or_cnt = 0;
		if (ServEvent[h].version <= 1) {
			ServEvent[h].format = (EvBuff_d[5] >> 12) & 0xF;
			ServEvent[h].tempFPGA = (float)((((EvBuff_d[5] & 0xFFF) * 503.975) / 4096) - 273.15);
			if (ServEvent[h].format & 1) {  // HV data
				ServEvent[h].hv_Vmon = (float)EvBuff_d[6] / 10000;
				ServEvent[h].hv_Imon = (float)EvBuff_d[7] / 10000;
				ServEvent[h].tempDetector = (float)(EvBuff_d[8] & 0x1FFF) * 256 / 10000;
				ServEvent[h].tempHV = (float)((EvBuff_d[8] >> 13) & 0x1FFF) * 256 / 10000;
				ServEvent[h].hv_status_on = (EvBuff_d[8] >> 26) & 1;
				ServEvent[h].hv_status_ramp = (EvBuff_d[8] >> 27) & 1;
				ServEvent[h].hv_status_ovc = (EvBuff_d[8] >> 28) & 1;
				ServEvent[h].hv_status_ovv = (EvBuff_d[8] >> 29) & 1;
				cntstart = 9;
				if (ServEvent[h].version > 0) {
					ServEvent[h].Status = EvBuff_d[9] & 0xFFFF;
					ServEvent[h].tempBoard = ((EvBuff_d[9] >> 16) & 0x3FF) == 0x3FF ? INVALID_TEMP : (float)((EvBuff_d[9] >> 16) & 0x3FF) / 4;
					cntstart += 1;
				}
			}
			if (ServEvent[h].format & 2) {  // TrgCnt data
				for (i = cntstart; i < size; i++) {
					int ch = (EvBuff_d[i] >> 24) & 0x7F;
					if (ch == 64)
						ServEvent[h].t_or_cnt = EvBuff_d[i] & 0xFFFFFF;
					else if (ch == 65)
						ServEvent[h].q_or_cnt = EvBuff_d[i] & 0xFFFFFF;
					else if (ch < FERSLIB_MAX_NCH_5202)
						ServEvent[h].ch_trg_cnt[ch] = EvBuff_d[i] & 0xFFFFFF;
				}
			}
			se_err = ServEvent[h].Status & (STATUS_FAIL | STATUS_FPGA_OVERTEMP | STATUS_CRC_ERROR | STATUS_BOARD_OVERTEMP);
			if (ENABLE_FERSLIB_LOGMSG && log_service_err && se_err) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][BRD %02d] Possible failure detected: Status = %08X\n", FERS_INDEX(handle), ServEvent[h].Status);
				log_service_err = 0;
			}
		} else {
			ServEvent[h].format = (EvBuff_d[5] >> 12) & 0xF;
			ServEvent[h].tempFPGA = INVALID_TEMP;
			ServEvent[h].t_or_cnt = 0;
			ServEvent[h].q_or_cnt = 0;
			ServEvent[h].hv_Vmon = 0;
			ServEvent[h].hv_Imon = 0;
			ServEvent[h].tempDetector = 0;
			ServEvent[h].tempHV = 0;
			ServEvent[h].hv_status_on = 0;
			ServEvent[h].hv_status_ramp = 0;
			ServEvent[h].hv_status_ovc = 0;
			ServEvent[h].hv_status_ovv = 0;
			ServEvent[h].tempBoard = INVALID_TEMP;
			ServEvent[h].Status = 0;
		}
		*Event = (void*)&ServEvent[h];
	} else if (((*DataQualifier & 0xF) == DTQ_SPECT) || ((*DataQualifier & 0xF) == DTQ_TSPECT)) {
		uint32_t nhits, both_g;
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		SpectEvent[h].tstamp_us = *tstamp_us;
		SpectEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		pnt = 5;
		SpectEvent[h].rel_tstamp_us = 0;
		if (*DataQualifier & 0x80)
			SpectEvent[h].rel_tstamp_us = (double)EvBuff_d[pnt++] * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		SpectEvent[h].chmask = ((uint64_t)EvBuff_d[pnt] << 32) | (uint64_t)EvBuff_d[pnt+1];
		pnt += 2;
		both_g = (*DataQualifier >> 4) & 0x01;  // both gain (LG and HG) are present
		SpectEvent[h].qdmask = 0;
		for (int ch = 0; ch < 64; ch++) {
			int16_t pedhg = EnablePedCal ? CommonPedestal - PedestalHG[h][ch] : 0;
			int16_t pedlg = EnablePedCal ? CommonPedestal - PedestalLG[h][ch] : 0;
			int16_t henergy=0, lenergy=0;
			SpectEvent[h].energyLG[ch] = 0;
			SpectEvent[h].energyHG[ch] = 0;
			SpectEvent[h].tstamp[ch] = 0;
			SpectEvent[h].ToT[ch] = 0;
			if ((SpectEvent[h].chmask >> ch) & 0x1) {
				if (both_g) {
					if (EvBuff_d[pnt] & 0x8000) SpectEvent[h].qdmask |= ((uint64_t)1 << ch);
					henergy = (EvBuff_d[pnt] & 0x3FFF) + pedhg;
					lenergy = ((EvBuff_d[pnt] >> 16) & 0x3FFF) + pedlg;
					pnt++;
				} else {
					en = (hl == 0) ? EvBuff_d[pnt] & 0xFFFF : (EvBuff_d[pnt++] >> 16) & 0xFFFF;
					hl ^= 1;
					if (en & 0x8000) SpectEvent[h].qdmask |= ((uint64_t)1 << ch);
					if (en & 0x4000) lenergy = (en & 0x3FFF) + pedlg;
					else henergy = (en & 0x3FFF) + pedhg;
				}
				// negative numbers (due to pedestal subtraction) are forced to 0 
				// number exceeding max_range (due to pedestal subtraction) are forced to  max_range
				SpectEvent[h].energyLG[ch] = min(max(0, lenergy), MaxEnergyRange);
				SpectEvent[h].energyHG[ch] = min(max(0, henergy), MaxEnergyRange);
			}
		}
		if ((*DataQualifier & DTQ_TIMING) && (pnt < size)) {
			nhits = size - pnt - 1;
			for (i = 0; i < nhits; i++) {
				int ch = (EvBuff_d[pnt + i + 1] >> 25) & 0xFF;
				if (ch >= 64) continue;
				if (SpectEvent[h].tstamp[ch] == 0) SpectEvent[h].tstamp[ch] = EvBuff_d[pnt + i + 1] & 0xFFFF;  // take 1st hit only
				if (SpectEvent[h].ToT[ch] == 0) SpectEvent[h].ToT[ch] = (EvBuff_d[pnt + i + 1] >> 16) & 0x1FF;
			}
		}
		*Event = (void*)&SpectEvent[h];
	} else if ((*DataQualifier & 0x0F) == DTQ_COUNT) {
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		CountingEvent[h].tstamp_us = *tstamp_us;
		CountingEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		CountingEvent[h].chmask = 0;
		for (i = 0; i < FERSLIB_MAX_NCH_5202; i++)
			CountingEvent[h].counts[i] = 0;
		CountingEvent[h].t_or_counts = 0;
		CountingEvent[h].q_or_counts = 0;
		pnt = 5;
		CountingEvent[h].rel_tstamp_us = 0;
		if (*DataQualifier & 0x80)
			CountingEvent[h].rel_tstamp_us = EvBuff_d[pnt++] * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		for (i = pnt; i < size; i++) {
			int ch = (EvBuff_d[i] >> 24) & 0xFF;
			if (ch < 64) {
				CountingEvent[h].counts[ch] = EvBuff_d[i] & 0xFFFFFF;
				CountingEvent[h].chmask |= (uint64_t)(UINT64_C(1) << ch);
			} else if (ch == 64) {
				CountingEvent[h].t_or_counts = EvBuff_d[i] & 0xFFFFFF;
			} else if (ch == 65) {
				CountingEvent[h].q_or_counts = EvBuff_d[i] & 0xFFFFFF;
			}
		}
		*Event = (void*)&CountingEvent[h];

		//} else if ((*DataQualifier == DTQ_TIMING_CSTART) || (*DataQualifier == DTQ_TIMING_CSTOP) || (*DataQualifier == DTQ_TIMING_STREAMING)) {
	} else if ((*DataQualifier & 0x0F) == DTQ_TIMING) {
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		ListEvent[h].tstamp_clk = (((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]);
		ListEvent[h].Tref_tstamp = EvBuff_d[5] & 0x7FFFFFFF;
		ListEvent[h].nhits = size - 6;  // 5 word for header + 1 word for time stamp of Tref CTIN: need to take fine time stamp of Tref
		for (i = 0; i < ListEvent[h].nhits; i++) {
			ListEvent[h].channel[i] = (EvBuff_d[i + 6] >> 25) & 0xFF;
			if ((*DataQualifier & 0xF0) == 0x20) {
				ListEvent[h].tstamp[i] = EvBuff_d[i + 6] & 0x1FFFFFF;
				ListEvent[h].ToT[i] = 0;
			} else {
				ListEvent[h].tstamp[i] = EvBuff_d[i + 6] & 0xFFFF;    // CTIN: if ToT is not present, tstamp takes 25 bits (mask = 0x1FFFFFF)
				ListEvent[h].ToT[i] = (EvBuff_d[i + 6] >> 16) & 0x1FF;  // CTIN: set ToT = 0 if not present
			}
		}
		*Event = (void*)&ListEvent[h];

	} else if (*DataQualifier == DTQ_WAVE) {
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		WaveEvent[h].tstamp_us = *tstamp_us;
		WaveEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		WaveEvent[h].ns = size - 5;
		for (i = 0; i < (uint32_t)(WaveEvent[h].ns); i++) {
			WaveEvent[h].wave_hg[i] = (uint16_t)(EvBuff_d[i + 5] & 0x3FFF);
			WaveEvent[h].wave_lg[i] = (uint16_t)((EvBuff_d[i + 5] >> 14) & 0x3FFF);
			WaveEvent[h].dig_probes[i] = (uint8_t)((EvBuff_d[i + 5] >> 28) & 0xF);
		}
		*Event = (void*)&WaveEvent[h];

	}

	// Dump raw data to file (debug mode)
	if (DebugLogs & DBLOG_RAW_DATA_OUTFILE) {
		if (raw == NULL) raw = fopen("RawEvents.txt", "w");
		if ((*DataQualifier) != DTQ_SERVICE)
			fprintf(raw, "Brd %02d: Tstamp = %.3f us\n", FERS_INDEX(handle), *tstamp_us);
		else
			fprintf(raw, "Brd %02d: Tstamp = %.3f us SERVICE-EVENT\n", FERS_INDEX(handle), *tstamp_us);
		for (i = 0; i < (uint32_t)(nb / 4); i++)
			fprintf(raw, "%08X\n", EvBuff_d[i]);
		fprintf(raw, "\n");
	}
	return 0;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// --- DECODE 5203 ---
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static int FERS_DecodeEvent_5203(int handle, uint32_t* EvBuff_d, int nb, int* DataQualifier, double* tstamp_us, void** Event)
{
	uint32_t i;
	uint16_t size;
	int h = FERS_INDEX(handle), se_err = 0;
	static int log_service_err = 1;
	static FILE* raw = NULL;

	int is128ch = FERS_NumChannels(handle) == 128 ? 1 : 0;  // CTIN: make this individual per board ?

	if (EvBuff_d == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Readout not initialized\n", FERS_INDEX(handle));
		return FERSLIB_ERR_READOUT_NOT_INIT;
	}
	if (nb == 0) return 0;

	// decode event data structure
	size = EvBuff_d[0] & 0xFFFF;
	*DataQualifier = (EvBuff_d[0] >> 24) & 0xFF;

	if (*DataQualifier == DTQ_TEST) {  // test Mode 
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		TestEvent[h].tstamp_us = *tstamp_us;
		TestEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		TestEvent[h].nwords = size - 5;
		for (i = 0; i < TestEvent[h].nwords; i++) {
			if (i > MAX_TEST_NWORDS) break;
			TestEvent[h].test_data[i] = (uint32_t)EvBuff_d[i + 5];
		}
		*Event = (void*)&TestEvent[h];
	} else if ((*DataQualifier) == DTQ_SERVICE) {
		ServEvent[h].update_time = get_time();
		ServTimeStamp[h] = ServEvent[h].update_time;
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		ServEvent[h].tstamp_us = *tstamp_us;
		ServEvent[h].pkt_size = size - 5;
		ServEvent[h].format = (uint8_t)EvBuff_d[5];
		if (ServEvent[h].format < 7) {
			ServEvent[h].Status = ((EvBuff_d[6] >> 12) & 0xF) << 6;
			ServEvent[h].RejTrg_cnt = 0;
			ServEvent[h].TotTrg_cnt = 0;
			ServEvent[h].ChAlmFullFlags[0] = ((uint64_t)EvBuff_d[9] << 32) | EvBuff_d[8];
			ServEvent[h].ChAlmFullFlags[1] = 0;
			ServEvent[h].ReadoutFlags = (uint32_t)(EvBuff_d[10] & 0xFF);
		} else if (ServEvent[h].format >= 7) {
			ServEvent[h].Status = (EvBuff_d[6] >> 12) & 0xFFFF;
			ServEvent[h].TDCROStatus = (EvBuff_d[16] >> 12) & 0xFFFF;
			ServEvent[h].ChAlmFullFlags[0] = ((uint64_t)EvBuff_d[9] << 32) | EvBuff_d[8];
			ServEvent[h].ChAlmFullFlags[1] = ((uint64_t)EvBuff_d[11] << 32) | EvBuff_d[10];
			ServEvent[h].ReadoutFlags = (uint32_t)EvBuff_d[12];
			ServEvent[h].RejTrg_cnt = (uint32_t)EvBuff_d[13];
			ServEvent[h].TotTrg_cnt = (uint32_t)EvBuff_d[14];
			ServEvent[h].SupprTrg_cnt = (ServEvent[h].format == 8) ? (uint32_t)EvBuff_d[15] : 0;
		} else {
			//return FERSLIB_ERR_READOUT_ERROR;
			return 0;
		}
		ServEvent[h].tempFPGA = (float)((((EvBuff_d[6] & 0xFFF) * 503.975) / 4096) - 273.15);
		ServEvent[h].tempBoard = (float)(EvBuff_d[7] & 0x3FF) / 4;
		ServEvent[h].tempTDC[1] = (float)((EvBuff_d[7] >> 20) & 0x3FF) / 4;
		ServEvent[h].tempTDC[0] = (float)((EvBuff_d[7] >> 10) & 0x3FF) / 4;
		*Event = (void*)&ServEvent[h];
		if (ENABLE_FERSLIB_LOGMSG && log_service_err && se_err) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][BRD %02d] Possible failure detected: Status = %08X\n", FERS_INDEX(handle), ServEvent[h].Status);
			log_service_err = 0;
		}

	} else if ((*DataQualifier & 0x3) == DTQ_TIMING) {
		uint32_t port = 0, nwpl, nh;
		uint8_t cn = 0;				//chip number
		// uint32_t ht_nw = 8;				// Num of header and trailer words
		int port_separator, ow_trailer, tdc_suppr[2];
		/*
		for(i = 0; i<8; i++) {
			ListEvent[h].header1[i] = 0;
			ListEvent[h].header2[i] = 0;
			ListEvent[h].trailer[i] = 0;
		}*/
		ListEvent[h].tstamp_clk = ((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3];
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD[FERS_INDEX(handle)] / 1000.0;
		ListEvent[h].tstamp_us = *tstamp_us;
		ListEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		if (size < 5) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Event size too small\n", h);
			return FERSLIB_ERR_READOUT_ERROR;
		}
		nwpl = size - 5;  // num words payload = size - 5 word for packet header 
		port_separator = (*DataQualifier >> 6) & 1;
		ow_trailer = (*DataQualifier >> 7) & 1;
		tdc_suppr[0] = (*DataQualifier >> 2) & 1;
		tdc_suppr[1] = (*DataQualifier >> 3) & 1;
		*DataQualifier &= 0xF3;
		if (tdc_suppr[0]) cn++;  // TDC0 suppressed => read TDC1 only (start from ch64)
		nh = 0;

		for (i = 0; i < nwpl; i++) {
			uint32_t d32 = EvBuff_d[i + 5];
			int dtype = (d32 >> 28) & 0xF;
			if ((dtype & 0x8) == 0) {  // Measurement
				if (nh < MAX_LIST_SIZE) {
					ListEvent[h].channel[nh] = port_separator ? (uint8_t)((port << 4) | ((d32 >> 27) & 0xF)) : (uint8_t)(((d32 >> 25) & 0x3F) | (cn << 6));
					if ((*DataQualifier >> 4 & 0x3) == 0) { //Full Data mode
						ListEvent[h].edge[nh] = port_separator ? (d32 >> 26) & 1 : (d32 >> 24) & 1;
						ListEvent[h].ToA[nh] = port_separator ? d32 & 0x03FFFFFF : d32 & 0x00FFFFFF;
						ListEvent[h].ToT[nh] = 0;
					} else if ((*DataQualifier >> 4 & 0x3) == 1) {	//16 bits leading edge, 11 bits TOT
						ListEvent[h].edge[nh] = EDGE_LEAD;
						ListEvent[h].ToA[nh] = port_separator ? (d32 >> 11) & 0xFFFF : (d32 >> 11) & 0x3FFF;
						ListEvent[h].ToT[nh] = d32 & 0x000007FF;
					} else if ((*DataQualifier >> 4 & 0x3) == 2) {  //19 bits leading edge, 8 bits TOT
						ListEvent[h].edge[nh] = EDGE_LEAD;
						ListEvent[h].ToA[nh] = port_separator ? (d32 >> 8) & 0x7FFFF : (d32 >> 8) & 0x1FFFF;
						ListEvent[h].ToT[nh] = d32 & 0x000000FF;
					}
					// NOTE: when the ToT exceeds the FSR, the picoTDC clips it to 0xFF or 0x7FF. Conversely, the ToA is not clipped
					// and there is no way to know when the time measurement overflows
					nh++;
				}
			} else if (dtype == 0x8) {				// 1st header
				ListEvent[h].header1[port] = d32;
			} else if (dtype == 0x9) {				// 2nd header
				ListEvent[h].header2[port] = d32;
			} else if ((dtype >> 2) == 0x3) {		// one word chip trailer
				if (!is128ch && (i != (nwpl - 1))) {
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Unexpected Trailer\n", h);
					return FERSLIB_ERR_READOUT_ERROR;
				}
				ListEvent[h].ow_trailer = d32;
				cn++;								// Increment chip number counter
			} else if (dtype == 0xA) {				// Trailer
				ListEvent[h].trailer[port] = d32;
				port++;
			} else {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Unknown data type: 0x%08X\n", h, d32);
				return FERSLIB_ERR_READOUT_ERROR;
			}
		}

		ListEvent[h].nhits = (uint16_t)nh;
		*Event = (void*)&ListEvent[h];
	} else {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Error in Data Qualifier: %d\n", h, *DataQualifier);
		return FERSLIB_ERR_READOUT_ERROR;
	}

	// Dump raw data to file (debug mode)
	if (DebugLogs & DBLOG_RAW_DATA_OUTFILE) {
		if (raw == NULL) raw = fopen("RawEvents.txt", "w");
		if ((*DataQualifier) != DTQ_SERVICE)
			fprintf(raw, "Brd %02d: Tstamp = %.3f us\n", FERS_INDEX(handle), *tstamp_us);
		else
			fprintf(raw, "Brd %02d: Tstamp = %.3f us SERVICE-EVENT\n", FERS_INDEX(handle), *tstamp_us);
		for (i = 0; i < (uint32_t)(nb / 4); i++)
			fprintf(raw, "%08X\n", EvBuff_d[i]);
		fprintf(raw, "\n");
	}
	return 0;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// --- DECODE 5204 ---
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static int FERS_DecodeEvent_5204(int handle, uint32_t *EvBuff_d, int nb, int *DataQualifier, double *tstamp_us, void **Event)
{
	uint32_t i, hl=0, en, pnt=0, se_err = 0;
	uint16_t size;
	int ch, h = FERS_INDEX(handle);
	static int log_service_err = 1;
	static FILE *raw = NULL;

	if (EvBuff_d == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Readout not initialized\n", FERS_INDEX(handle));
		return FERSLIB_ERR_READOUT_NOT_INIT;
	}
	if (nb == 0) return 0;

	// decode event data structure
	size = EvBuff_d[0] & 0xFFFF;  // in 32bit words
	*DataQualifier = (EvBuff_d[0] >> 24) & 0xFF;

	if (*DataQualifier == DTQ_TEST) {  // test Mode 
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		TestEvent[h].tstamp_us = *tstamp_us;
		TestEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		TestEvent[h].nwords = size - 5;
		for (i = 0; i < TestEvent[h].nwords; i++) {
			if (i > MAX_TEST_NWORDS) break;
			TestEvent[h].test_data[i] = EvBuff_d[i + 5];
		}
		*Event = (void*)&TestEvent[h];
	} else if ((*DataQualifier) == DTQ_SERVICE) {
		int cntstart = 6;
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		ServEvent[h].update_time = get_time();
		ServTimeStamp[h] = ServEvent[h].update_time;
		ServEvent[h].tstamp_us  = *tstamp_us;
		ServEvent[h].pkt_size   = size - 5; 
		ServEvent[h].version = (EvBuff_d[5] >> 24) & 0xFF;
		for (i = 0; i < FERSLIB_MAX_NCH_5204; i++)	// Reset counters
			ServEvent[h].ch_trg_cnt[i] = 0;
		ServEvent[h].t_or_cnt = 0;
		ServEvent[h].q_or_cnt = 0;
		if (ServEvent[h].version <= 1) {
			ServEvent[h].format = (EvBuff_d[5] >> 12) & 0xF;
			ServEvent[h].tempFPGA = (float)((((EvBuff_d[5] & 0xFFF) * 503.975) / 4096) - 273.15);
			if (ServEvent[h].format & 1) {  // HV data
				ServEvent[h].hv_Vmon = (float)EvBuff_d[6] / 10000;
				ServEvent[h].hv_Imon = (float)EvBuff_d[7] / 10000;
				ServEvent[h].tempDetector = (float)(EvBuff_d[8] & 0x1FFF) * 256 / 10000;
				ServEvent[h].tempHV = (float)((EvBuff_d[8] >> 13) & 0x1FFF) * 256 / 10000;
				ServEvent[h].hv_status_on = (EvBuff_d[8] >> 26) & 1;
				ServEvent[h].hv_status_ramp = (EvBuff_d[8] >> 27) & 1;
				ServEvent[h].hv_status_ovc = (EvBuff_d[8] >> 28) & 1;
				ServEvent[h].hv_status_ovv = (EvBuff_d[8] >> 29) & 1;
				ServEvent[h].Status = EvBuff_d[9] & 0xFFFF;
				ServEvent[h].tempBoard = ((EvBuff_d[9] >> 16) & 0x3FF) == 0x3FF ? INVALID_TEMP : (float)((EvBuff_d[9] >> 16) & 0x3FF) / 4;
				cntstart = 10;
			}
			if (ServEvent[h].format & 2) {  // TrgCnt data
				for (i = cntstart; i < size; i++) {
					ch = (EvBuff_d[i] >> 24) & 0x7F;
					if (ch == 64)
						ServEvent[h].t_or_cnt = EvBuff_d[i] & 0xFFFFFF;
					else if (ch == 65)
						ServEvent[h].q_or_cnt = EvBuff_d[i] & 0xFFFFFF;
					else if (ch < FERSLIB_MAX_NCH_5204)
						ServEvent[h].ch_trg_cnt[ch] = EvBuff_d[i] & 0xFFFFFF;
				}
			}
		} else {
			ServEvent[h].format = (EvBuff_d[5] >> 12) & 0xF;
			ServEvent[h].tempFPGA = INVALID_TEMP;
			ServEvent[h].t_or_cnt = 0;
			ServEvent[h].q_or_cnt = 0;
			ServEvent[h].hv_Vmon = 0;
			ServEvent[h].hv_Imon = 0;
			ServEvent[h].tempDetector = 0;
			ServEvent[h].tempHV = 0;
			ServEvent[h].hv_status_on = 0;
			ServEvent[h].hv_status_ramp = 0;
			ServEvent[h].hv_status_ovc = 0;
			ServEvent[h].hv_status_ovv = 0;
			ServEvent[h].tempBoard = INVALID_TEMP;
			ServEvent[h].Status = 0;
		}
		*Event = (void*)&ServEvent[h];
		if (ENABLE_FERSLIB_LOGMSG && log_service_err && se_err) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][BRD %02d] Possible failure detected: Status = %08X\n", FERS_INDEX(handle), ServEvent[h].Status);
			log_service_err = 0;
		}
	} else if (((*DataQualifier & 0xF) == DTQ_SPECT) || ((*DataQualifier & 0xF) == DTQ_TSPECT)) {
		uint32_t nhits, both_g;
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		SpectEvent[h].tstamp_us = *tstamp_us;
		SpectEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		pnt = 5;
		SpectEvent[h].rel_tstamp_us = 0;
		if (*DataQualifier & 0x80)
			SpectEvent[h].rel_tstamp_us = EvBuff_d[pnt++] * CLK_PERIOD_5204 / 1000.0;
		SpectEvent[h].chmask = ((uint64_t)EvBuff_d[pnt] << 32) | (uint64_t)EvBuff_d[pnt+1];
		pnt += 2;
		both_g = (*DataQualifier >> 4) & 0x01;  // both gain (LG and HG) are present
		SpectEvent[h].qdmask = 0;
		for (ch = 63; ch >= 0; ch--) {  // due to PCB channel swapping, energy values arrive in reverse order
			int16_t pedhg = EnablePedCal ? CommonPedestal - PedestalHG[FERS_INDEX(handle)][ch] : 0;
			int16_t pedlg = EnablePedCal ? CommonPedestal - PedestalLG[FERS_INDEX(handle)][ch] : 0;
			int16_t henergy = 0, lenergy = 0;
			SpectEvent[h].energyLG[ch] = 0;
			SpectEvent[h].energyHG[ch] = 0;
			SpectEvent[h].tstamp[ch] = 0;
			SpectEvent[h].ToT[ch] = 0;
			if ((SpectEvent[h].chmask >> ch) & 0x1) {
				if (both_g) {
					if (EvBuff_d[pnt] & 0x8000) SpectEvent[h].qdmask |= ((uint64_t)1 << ch);
					henergy = (EvBuff_d[pnt] & 0x3FFF) + pedhg;
					lenergy = ((EvBuff_d[pnt] >> 16) & 0x3FFF) + pedlg;
					pnt++;
				} else {
					en = (hl == 0) ? EvBuff_d[pnt] & 0xFFFF : (EvBuff_d[pnt++]>>16) & 0xFFFF;
					hl ^= 1;
					if (en & 0x8000) SpectEvent[h].qdmask |= ((uint64_t)1 << ch);
					if (en & 0x4000) lenergy = (en & 0x3FFF) + pedlg;
					else henergy = (en & 0x3FFF) + pedhg;
				}
				// negative numbers (due to pedestal subtraction) are forced to 0 
				// number exceeding max_range (due to pedestal subtraction) are forced to  max_range
				SpectEvent[h].energyLG[ch] = min(max(0, lenergy), MaxEnergyRange);
				SpectEvent[h].energyHG[ch] = min(max(0, henergy), MaxEnergyRange);
			}
		}
		if ((*DataQualifier & DTQ_TIMING) && (pnt < size)) {
			nhits = size - pnt - 1;  
			for (i=0; i<nhits; i++) { 
				ch = (EvBuff_d[pnt + i + 1] >> 25) & 0xFF;
				if (ch >= 64) continue;
				if (SpectEvent[h].tstamp[ch] == 0) SpectEvent[h].tstamp[ch] = EvBuff_d[pnt + i + 1] & 0xFFFF;  // take 1st hit only
				if (SpectEvent[h].ToT[ch] == 0) SpectEvent[h].ToT[ch] = (EvBuff_d[pnt + i + 1] >> 16) & 0x1FF;  
			}
		}
		*Event = (void *)&SpectEvent[h];
	} else if ((*DataQualifier & 0x0F) == DTQ_COUNT) {
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		CountingEvent[h].tstamp_us = *tstamp_us;
		CountingEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		CountingEvent[h].chmask = 0;
		for (i = 0; i < FERSLIB_MAX_NCH_5204; i++)
			CountingEvent[h].counts[i] = 0;
		CountingEvent[h].t_or_counts = 0;
		CountingEvent[h].q_or_counts = 0;
		pnt = 5;
		CountingEvent[h].rel_tstamp_us = 0;
		if (*DataQualifier & 0x80)
			CountingEvent[h].rel_tstamp_us = EvBuff_d[pnt++] * CLK_PERIOD_5204 / 1000.0;
		for (i=pnt; i<size; i++) {
			ch = (EvBuff_d[i] >> 24) & 0xFF;
			if (ch < 64) {
				CountingEvent[h].counts[ch] = EvBuff_d[i] & 0xFFFFFF;
				CountingEvent[h].chmask |= (uint64_t)(UINT64_C(1) << ch);
			} else if (ch == 64) {
				CountingEvent[h].t_or_counts = EvBuff_d[i] & 0xFFFFFF;
			} else if (ch == 65) {
				CountingEvent[h].q_or_counts = EvBuff_d[i] & 0xFFFFFF;
			}
		}
		*Event = (void *)&CountingEvent[h];

	//} else if ((*DataQualifier == DTQ_TIMING_CSTART) || (*DataQualifier == DTQ_TIMING_CSTOP) || (*DataQualifier == DTQ_TIMING_STREAMING)) {
	} else if ((*DataQualifier & 0x0F) == DTQ_TIMING) {
		uint32_t port = 0, nwpl, nh;
		//uint32_t ht_nw = 8;			// Num of header and trailer words
		int port_separator, ow_trailer;
		ListEvent[h].tstamp_clk = ((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3];
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		ListEvent[h].tstamp_us = *tstamp_us;
		ListEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		if (size < 5) {
			FERS_LibMsg("[ERROR][BRD %02d] Event size too small\n", h);
			return FERSLIB_ERR_READOUT_ERROR;
		}
		nwpl = size - 5;  // num words payload = size - 5 word for packet header 
		port_separator = (*DataQualifier >> 6) & 1;
		ow_trailer = (*DataQualifier >> 7) & 1;
		*DataQualifier &= 0xF3;
		nh = 0;

		for (i = 0; i < nwpl; i++) {
			uint32_t d32 = EvBuff_d[i + 5];
			int dtype = (d32 >> 28) & 0xF;
			if ((dtype & 0x8) == 0) {  // Measurement
				if (nh < MAX_LIST_SIZE) {
					ListEvent[h].channel[nh] = port_separator ? (uint8_t)((port << 4) | ((d32 >> 27) & 0xF)) : (uint8_t)((d32 >> 25) & 0x3F);
#if 1
					ListEvent[h].edge[nh] = port_separator ? (d32 >> 26) & 1 : (d32 >> 24) & 1;
					ListEvent[h].ToA[nh] = port_separator ? d32 & 0x03FFFFFF : d32 & 0x00FFFFFF;
					ListEvent[h].ToT[nh] = 0;
#elif
					if ((*DataQualifier >> 4 & 0x3) == 0) { //Full Data mode
						ListEvent[h].edge[nh] = port_separator ? (d32 >> 26) & 1 : (d32 >> 24) & 1;
						ListEvent[h].ToA[nh] = port_separator ? d32 & 0x03FFFFFF : d32 & 0x00FFFFFF;
						ListEvent[h].ToT[nh] = 0;
					} else if ((*DataQualifier >> 4 & 0x3) == 1) {	//16 bits leading edge, 11 bits TOT
						ListEvent[h].edge[nh] = EDGE_LEAD;
						ListEvent[h].ToA[nh] = port_separator ? (d32 >> 11) & 0xFFFF : (d32 >> 11) & 0x3FFF;
						ListEvent[h].ToT[nh] = d32 & 0x000007FF;
					} else if ((*DataQualifier >> 4 & 0x3) == 2) {  //19 bits leading edge, 8 bits TOT
						ListEvent[h].edge[nh] = EDGE_LEAD;
						ListEvent[h].ToA[nh] = port_separator ? (d32 >> 8) & 0x7FFFF : (d32 >> 8) & 0x1FFFF;
						ListEvent[h].ToT[nh] = d32 & 0x000000FF;
					}
#endif
					// NOTE: when the ToT exceeds the FSR, the picoTDC clips it to 0xFF or 0x7FF. Conversely, the ToA is not clipped
					// and there is no way to know when the time measurement overflows
					nh++;
				}
			} else if (dtype == 0x8) {				// 1st header
				ListEvent[h].header1[port] = d32;
			} else if (dtype == 0x9) {				// 2nd header
				ListEvent[h].header2[port] = d32;
			} else if ((dtype >> 2) == 0x3) {		// one word chip trailer
				if (i != (nwpl - 1)) {
					FERS_LibMsg("[ERROR][BRD %02d] Unexpected Trailer\n", h);
					return FERSLIB_ERR_READOUT_ERROR;
				}
				ListEvent[h].ow_trailer = d32;
			} else if (dtype == 0xA) {				// Trailer
				ListEvent[h].trailer[port] = d32;
				port++;
			} else {
				FERS_LibMsg("[ERROR][BRD %02d] Unknown data type: 0x%08X\n", h, d32);
				return FERSLIB_ERR_READOUT_ERROR;
			}
		}
		ListEvent[h].nhits = (uint16_t)nh;
		*Event = (void*)&ListEvent[h];

	} else if (*DataQualifier == DTQ_WAVE) {
		*tstamp_us = (double)(((uint64_t)EvBuff_d[4] << 32) | (uint64_t)EvBuff_d[3]) * CLK_PERIOD_5204 / 1000.0;
		WaveEvent[h].tstamp_us = *tstamp_us;
		WaveEvent[h].trigger_id = ((uint64_t)EvBuff_d[2] << 32) | (uint64_t)EvBuff_d[1];
		WaveEvent[h].ns = size - 5;
		for (i=0; i < (uint32_t)(WaveEvent[h].ns); i++) {
			WaveEvent[h].wave_hg[i] = (uint16_t)(EvBuff_d[i+5] & 0x3FFF);
			WaveEvent[h].wave_lg[i] = (uint16_t)((EvBuff_d[i+5] >> 14) & 0x3FFF);
			WaveEvent[h].dig_probes[i] = (uint8_t)((EvBuff_d[i+5] >> 28) & 0xF);
		}
		*Event = (void *)&WaveEvent[h];

	}

	// Dump raw data to file (debug mode)
	if (DebugLogs & DBLOG_RAW_DATA_OUTFILE) {
		if (raw == NULL) raw = fopen("RawEvents.txt", "w");
		if ((*DataQualifier) != DTQ_SERVICE)
			fprintf(raw, "Brd %02d: Tstamp = %.3f us\n", FERS_INDEX(handle), *tstamp_us);
		else
			fprintf(raw, "Brd %02d: Tstamp = %.3f us SERVICE-EVENT\n", FERS_INDEX(handle), *tstamp_us);
		for(i=0; i<(uint32_t)(nb/4); i++)
			fprintf(raw, "%08X\n", EvBuff_d[i]);
		fprintf(raw, "\n");
	}
	return 0;
}


int FERS_DecodeEvent(int handle, uint32_t *EvBuff_d, int nb, int *DataQualifier, double *tstamp_us, void **Event)
{
	int h = FERS_INDEX(handle);
	static FILE* raw = NULL;
	int ret = 0;
	if (FERS_BoardInfo[h]->FERSCode == 5202)
		ret = FERS_DecodeEvent_5202(handle, EvBuff_d, nb, DataQualifier, tstamp_us, Event);
	if (FERS_BoardInfo[h]->FERSCode == 5203)
		ret = FERS_DecodeEvent_5203(handle, EvBuff_d, nb, DataQualifier, tstamp_us, Event);
	if (FERS_BoardInfo[h]->FERSCode == 5204)
		ret = FERS_DecodeEvent_5204(handle, EvBuff_d, nb, DataQualifier, tstamp_us, Event);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get the number of CRC errors detected by the concentrator since the last start of run
// Inputs:		cnc_handle = handle of the concentrator
// Outputs:		errcnt: CRC error counter
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_GetCrcErrorCnt(int cnc_handle, uint32_t *errcnt) {
	*errcnt = CrcErrorCnt[FERS_CNCINDEX(cnc_handle)];
	return 0;
}

// *********************************************************************************************************
// Allocate/free memory buffers
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Init readout for one board (allocate buffers and initialize variables)
// Inputs:		handle = device handle
// Outputs:		AllocatedSize = tot. num. of bytes allocated for data buffers and descriptors
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_InitReadout(int handle, int ROmode, int *AllocatedSize) {
	uint32_t ctrl;
	static uint8_t mutex_inizialized = 0;

	if (InitReadout[FERS_INDEX(handle)])
		return 0;
 	*AllocatedSize = 0;
	ReadoutMode = ROmode;
	if (!mutex_inizialized) {
		initmutex(FERS_RoMutex);
		mutex_inizialized = 1;
	}
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		if (Cnc_NumBoards[FERS_CNCINDEX(handle)] == 0) {  // 1st board connected to this cnc
			LLBuff[FERS_CNCINDEX(handle)] = (char *)malloc(LLBUFF_CNC_SIZE);
			if (LLBuff[FERS_CNCINDEX(handle)] == NULL) {
				FERS_LibMsg("ERROR: LL concentrator buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
				_setLastLocalError("ERROR: LL concentrator buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
				return FERSLIB_ERR_MALLOC_BUFFERS;
			}
			*AllocatedSize += LLBUFF_CNC_SIZE;
			FERS_TotalAllocatedMem += LLBUFF_CNC_SIZE;

			DescrTable[FERS_CNCINDEX(handle)] = (uint32_t*)malloc(MAX_NROW_EDTAB * 32);
			if (DescrTable[FERS_CNCINDEX(handle)] == NULL) {
				FERS_LibMsg("ERROR: CNC%02d DescrTable buffer not allocated (ret = %d)\n", FERS_CNCINDEX(handle), FERSLIB_ERR_MALLOC_BUFFERS);
				_setLastLocalError("ERROR: CNC%02d DescrTable buffer not allocated (ret = %d)\n", FERS_CNCINDEX(handle), FERSLIB_ERR_MALLOC_BUFFERS);
				return FERSLIB_ERR_MALLOC_BUFFERS;
			}
			*AllocatedSize += MAX_NROW_EDTAB * 32;
			FERS_TotalAllocatedMem += MAX_NROW_EDTAB * 32;

		}
		tdl_handle[FERS_CNCINDEX(handle)][FERS_CHAIN(handle)][FERS_NODE(handle)] = handle;
		Cnc_NumBoards[FERS_CNCINDEX(handle)]++;
		tdl_handle[FERS_CNCINDEX(handle)][FERS_CHAIN(handle)][FERS_NODE(handle)] = handle;
	} else {
		LLBuff[FERS_INDEX(handle)] = (char *)malloc(LLBUFF_SIZE);
		if (LLBuff[FERS_INDEX(handle)] == NULL) {
			FERS_LibMsg("ERROR: LL boards buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
			_setLastLocalError("ERROR: LL boards buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
			return FERSLIB_ERR_MALLOC_BUFFERS;
		}
		*AllocatedSize += LLBUFF_SIZE;
		FERS_TotalAllocatedMem += LLBUFF_SIZE;
	}
	EvBuff[FERS_INDEX(handle)] = (uint32_t *)malloc(EVBUFF_SIZE);
	tmp_event = (uint32_t*)malloc(EVBUFF_SIZE);
	if (EvBuff[FERS_INDEX(handle)] == NULL) {
		FERS_LibMsg("ERROR: Event boards buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
		_setLastLocalError("ERROR: Event boards buffer not allocated (ret = %d)\n", FERSLIB_ERR_MALLOC_BUFFERS);
		return FERSLIB_ERR_MALLOC_BUFFERS;
	}
	*AllocatedSize += 2*EVBUFF_SIZE;
	FERS_TotalAllocatedMem += 2*EVBUFF_SIZE;
	if (FERS_IsXROC(handle)) {
		WaveEvent[FERS_INDEX(handle)].wave_hg = (uint16_t*)malloc(MAX_WAVEFORM_LENGTH * sizeof(uint16_t));
		WaveEvent[FERS_INDEX(handle)].wave_lg = (uint16_t*)malloc(MAX_WAVEFORM_LENGTH * sizeof(uint16_t));
		WaveEvent[FERS_INDEX(handle)].dig_probes = (uint8_t*)malloc(MAX_WAVEFORM_LENGTH * sizeof(uint8_t));
		*AllocatedSize += (MAX_WAVEFORM_LENGTH * (2 * sizeof(uint16_t) + sizeof(uint8_t)));
		FERS_TotalAllocatedMem += (MAX_WAVEFORM_LENGTH * (2 * sizeof(uint16_t) + sizeof(uint8_t)));
	}

	// If event sorting is required, create queues
	if (ReadoutMode != ROMODE_DISABLE_SORTING) {
		queue[FERS_INDEX(handle)] = (uint32_t*)malloc(FERSLIB_QUEUE_SIZE * sizeof(uint32_t));
		FERS_TotalAllocatedMem += FERSLIB_QUEUE_SIZE * sizeof(uint32_t);
		q_wp[FERS_INDEX(handle)] = 0;
		q_rp[FERS_INDEX(handle)] = 0;
		q_nb[FERS_INDEX(handle)] = 0;
		q_full[FERS_INDEX(handle)] = 0;
		q_tstamp[FERS_INDEX(handle)] = 0;
		q_trgid[FERS_INDEX(handle)] = 0;
	}

	if (!FERS_Offline) {
		FERS_ReadRegister(handle, a_acq_ctrl, &ctrl);
		if (ctrl & (1 << 15))
			EnableStartEvent[FERS_INDEX(handle)] = 1;
	} else
		EnableStartEvent[FERS_INDEX(handle)] = 1;

	RO_NumBoards++;
	InitReadout[FERS_INDEX(handle)] = 1;
	if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][BRD %02d] Init Readout. Tot Alloc. size = %.2f MB\n", FERS_INDEX(handle), (float)(FERS_TotalAllocatedMem)/(1024*1024));
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: De-init readoout (free buffers)
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_CloseReadout(int handle) {
	if (!InitReadout[FERS_INDEX(handle)]) return 0;
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		Cnc_NumBoards[FERS_CNCINDEX(handle)]--;
		tdl_handle[FERS_CNCINDEX(handle)][FERS_CHAIN(handle)][FERS_NODE(handle)] = -1;
		if ((LLBuff[FERS_CNCINDEX(handle)] != NULL) && (Cnc_NumBoards[FERS_CNCINDEX(handle)] == 0)) {  // Last board disconnected
			free(LLBuff[FERS_CNCINDEX(handle)]);
			LLBuff[FERS_CNCINDEX(handle)] = NULL;
			FERS_TotalAllocatedMem -= LLBUFF_CNC_SIZE;
		}
		if (DescrTable[FERS_CNCINDEX(handle)] != NULL) {
			free(DescrTable[FERS_CNCINDEX(handle)]);
			DescrTable[FERS_CNCINDEX(handle)] = NULL;
			FERS_TotalAllocatedMem -= MAX_NROW_EDTAB * 32;
		}
	} else {
		if (LLBuff[FERS_INDEX(handle)] != NULL) {
			free(LLBuff[FERS_INDEX(handle)]);	// DNIN: it trows an acception after usb fw upgrader
			LLBuff[FERS_INDEX(handle)] = NULL;
			FERS_TotalAllocatedMem -= LLBUFF_SIZE;
		}
	}
	if (EvBuff[FERS_INDEX(handle)] != NULL) {
		free(EvBuff[FERS_INDEX(handle)]);
		EvBuff[FERS_INDEX(handle)] = NULL;
		free(tmp_event);
		tmp_event = NULL;
		FERS_TotalAllocatedMem -= 2 * EVBUFF_SIZE;
	}
	if (queue[FERS_INDEX(handle)] != NULL) {
		free(queue[FERS_INDEX(handle)]);
		queue[FERS_INDEX(handle)] = NULL;
		FERS_TotalAllocatedMem -= FERSLIB_QUEUE_SIZE * sizeof(uint32_t);
	}
	RO_NumBoards--;
	InitReadout[FERS_INDEX(handle)] = 0;
	if (FERS_IsXROC(handle)) {
		if (WaveEvent[FERS_INDEX(handle)].wave_hg != NULL) {  // Last board connected => can free waveform buffers. No, wave buffers are for each board
			free(WaveEvent[FERS_INDEX(handle)].wave_hg);
			free(WaveEvent[FERS_INDEX(handle)].wave_lg);
			free(WaveEvent[FERS_INDEX(handle)].dig_probes);
			WaveEvent[FERS_INDEX(handle)].wave_hg = NULL;
			FERS_TotalAllocatedMem -= (MAX_WAVEFORM_LENGTH * (2 * sizeof(uint16_t) + sizeof(uint8_t)));
		}
	}
	return 0;
}


// *********************************************************************************************************
// Start/Stop Acquisition, Flush Data
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Set ReadoutState = RUNNING, wait until all threads are running, then send the run command  
//				to the boards, according to the given start mode
// Inputs:		handle = device handles (af all boards)
// 				NumBrd = number of boards to start
//				StartMode = start mode (Async, T0/T1 chain, TDlink)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_StartAcquisition(int *handle, int NumBrd, int StartMode, int RunNum) {
	int ret=0, b, tdl = 1, rc;
	if (FERS_ReadoutStatus == ROSTATUS_RUNNING)
		FERS_StopAcquisition(handle, NumBrd, StartMode, RunNum);
	for(b = 0; b < NumBrd; b++) {
		if (handle[b] == -1) continue;
		//if (!FERS_Offline) ret |= FERS_FlushData(handle[b]);
		ret |= FERS_FlushData(handle[b]);
		ServTimeStamp[b] = 0;
		if (FERS_CONNECTIONTYPE(handle[b]) != FERS_CONNECTIONTYPE_TDL) tdl = 0;
	}
	// Reset CRC error counter
	for (b = 0; b < FERSLIB_MAX_NCNC; b++) {
		CrcErrorCnt[b] = 0;
	}

	if (!FERS_Offline) {
		// Open Raw Data files (check if they are enabled in the function)
		FERS_OpenRawDataFile(handle, RunNum);

		// Check that all RX-threads are in idle state (not running)	
		for (int i = 0; i < 100; i++) {
			lock(FERS_RoMutex);
			rc = FERS_RunningCnt;
			unlock(FERS_RoMutex);
			if (rc == 0) break;
			Sleep(10);
		}
		if (rc > 0) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] %d RX-thread still running while starting a new run\n", rc);
			return FERSLIB_ERR_START_STOP_ERROR;
		}

		lock(FERS_RoMutex);
		FERS_ReadoutStatus = ROSTATUS_RUNNING;
		unlock(FERS_RoMutex);
		// Check that all RX-threads are running
		for (int i = 0; i < 100; i++) {
			lock(FERS_RoMutex);
			rc = FERS_RunningCnt;
			unlock(FERS_RoMutex);
			if ((tdl && (rc == 1)) || (rc == NumBrd)) break;  // CTIN: count connected concentrators...
			Sleep(10);
		}
		if ((tdl && (rc == 0)) || (!tdl && (rc < NumBrd))) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] %d RX-thread are not running after the run start\n", rc);
			return FERSLIB_ERR_START_STOP_ERROR;
		}

		if (StartMode == STARTRUN_TDL) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Starting Run #%d in TDL mode\n", RunNum);
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Sending Sync commands on TDL\n");
			for (uint16_t chain = 0; chain < FERSLIB_MAX_NTDL; ++chain)
				ret |= LLtdl_ControlChain(0, chain, 0, 0);

			ret |= FERS_SendCommandBroadcast(handle, CMD_TIME_RESET, 0);
			ret |= FERS_SendCommandBroadcast(handle, CMD_RES_PTRG, 0);
			ret |= FERS_SendCommandBroadcast(handle, CMD_ACQ_START, 0);

			for (uint16_t chain = 0; chain < FERSLIB_MAX_NTDL; ++chain)
				ret |= LLtdl_ControlChain(0, chain, 1, 0x100);
		} else if ((StartMode == STARTRUN_CHAIN_T0) || (StartMode == STARTRUN_CHAIN_T1)) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Starting Run #%d in Daisy Chain mode\n", RunNum);
			ret |= FERS_SendCommand(handle[0], CMD_TIME_RESET);
			ret |= FERS_SendCommand(handle[0], CMD_ACQ_START);
		} else {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Starting Run #%d in Async mode\n", RunNum);
			for (b = 0; b < NumBrd; b++) {
				if (handle[b] == -1) continue;
				ret |= FERS_SendCommand(handle[b], CMD_TIME_RESET);
				ret |= FERS_SendCommand(handle[b], CMD_ACQ_START);
				if (FERS_CONNECTIONTYPE(handle[b]) == FERS_CONNECTIONTYPE_USB) {
					f_sem_post(&FERS_StartRunSemaphore[FERS_INDEX(handle[b])]);		// Semaphore posted
				}
			}
		}
	//} else { // READ Pedestal when offline
	//	if (FERS_IsXROC(handle[b])) // DNIN: no 5202/5203 mixed system are allowed at the moment
	//	{
	//		if (FERS_EnableRawdataReadFile) {
	//			ret |= FERS_LoadPedestal();
	//			if (ret != 0) _setLastLocalError("[WARNING] Cannot load the pedestal values from file for repocessing offline the run selected\n");
	//		}
	//	}	
	}
	if (ret < 0) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] Start Command failed. Ret = %d\n", ret);
		return ret;
	} else return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Send the stop command to the boards (according to the same mode used to start them).
//				NOTE: this function stops the run in the hardware (thus stopping the data flow) but it
//				doesn't stop the RX threads because they need to empty the buffers and the pipes.
//				The threads will stop automatically when there is no data or when a flush command is sent.
// Inputs:		handle = device handles (af all boards)
// 				NumBrd = number of boards to stop
//				StartMode = start mode (Async, T0/T1 chain, TDlink)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_StopAcquisition(int *handle, int NumBrd, int StartMode, int RunNum) {
	int ret=0, b;
	if (StartMode == STARTRUN_TDL) {
		if (!FERS_Offline) ret |= FERS_SendCommandBroadcast(handle, CMD_ACQ_STOP, 0);
		int cindex = FERS_CNCINDEX(*handle);
		Cnc_Flushed[cindex] = 0;
	} else if ((StartMode == STARTRUN_CHAIN_T0) || (StartMode == STARTRUN_CHAIN_T1)) {
		if (!FERS_Offline) ret |= FERS_SendCommand(handle[0], CMD_ACQ_STOP);
	} else {
		for(b = 0; b < NumBrd; b++) {
			if (handle[b] == -1) continue;
			if (!FERS_Offline) ret |= FERS_SendCommand(handle[b], CMD_ACQ_STOP);
		}
	}
	if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Run #%d stopped\n", RunNum);
	FERS_ReadoutStatus = ROSTATUS_EMPTYING;
	 // Close RawData file (checks done inside function
	if (!FERS_Offline) FERS_CloseRawDataFile(handle);

	if (ret < 0) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] Stop Command failed. Ret = %d\n", ret);
		return ret;
	} else return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: flush data (Read and discard data until the RX buffer is empty)
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_FlushData(int handle) {
	if (FERS_ReadoutStatus == ROSTATUS_RUNNING) return FERSLIB_ERR_OPER_NOT_ALLOWED;
	FERS_ReadoutStatus = ROSTATUS_FLUSHING;
	if (!FERS_Offline) FERS_SendCommand(handle, CMD_CLEAR);
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		int cindex = FERS_CNCINDEX(handle);
		int bindex = FERS_INDEX(handle);
		int nbf = 0;
		tdl_ReadRawEvent(cindex, &bindex, &nbf);		
		if (!Cnc_Flushed[cindex] && !FERS_Offline) {
			LLtdl_Flush(cindex);  // Flush concentrator 
			Sleep(100);
			Cnc_Flushed[cindex] = 1;
		}
	} else {
		int nb;
		eth_usb_ReadRawEvent(handle, &nb);  // make a read to let all processes to reset buffers
	}
	EvBuff_nb[FERS_INDEX(handle)] = 0;
	if (ReadoutMode != ROMODE_DISABLE_SORTING) {
		q_wp[FERS_INDEX(handle)] = 0;
		q_rp[FERS_INDEX(handle)] = 0;
		q_nb[FERS_INDEX(handle)] = 0;
		q_full[FERS_INDEX(handle)] = 0;
		q_tstamp[FERS_INDEX(handle)] = 0;
		q_trgid[FERS_INDEX(handle)] = 0;
	}
	Sleep(100);
	FERS_ReadoutStatus = ROSTATUS_IDLE;
	return 0;
}


// *********************************************************************************************************
// Get Event 
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Read and decode one event from the readout buffers. There are two readout modes: sorted or unsorted
//              If sorting is requested, the readout init function will allocate queues for sorting
// Inputs:		handle = pointer to the array of handles (of all boards)
// Outputs:		bindex = index of the board from which the event comes
//				DataQualifier = data qualifier (type of data, used to determine the struct for event data)
//				tstamp_us = time stamp in us (the information is also reported in the event data struct)
//				Event = pointer to the event data struct
//				nb = size of the event (in bytes)
// Return:		0=No Data, 1=Good Data 2=Not Running, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_GetEvent(int *handle, int *bindex, int *DataQualifier, double *tstamp_us, void **Event, int *nb) {
	int ret, h, h_found = -1, i, nbb;
	uint64_t oldest_ev = 0;

	*nb = 0;
	*tstamp_us = 0;
	*DataQualifier = 0;
	*bindex = 0;

	// ---------------------------------------------------------------------
	// SORTED
	// ---------------------------------------------------------------------
	if (ReadoutMode != ROMODE_DISABLE_SORTING) {
		int qsel, qi=0;
		static int nodata_cnt[FERSLIB_MAX_NBRD] = { 0 };
		static uint64_t nodata_time[FERSLIB_MAX_NBRD] = { 0 };
		static int timed_out[FERSLIB_MAX_NBRD] = { 0 };

		// Check if there are empty queues and try to fill them
		for (i = 0; i < FERSLIB_MAX_NBRD; i++) {
			if (handle[i] == -1) break;
			qi = FERS_INDEX(handle[i]);
			if ((q_tstamp[FERS_INDEX(handle[i])] == 0) && !q_busy) {  // queue is empty => try to read new data
				if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_TDL) {
					ret = tdl_ReadRawEvent(FERS_CNCINDEX(handle[i]), &qi, &nbb);
				} else {					
					ret = eth_usb_ReadRawEvent(handle[i], &nbb);
				}
				if (ret == 2) return 2;
				if (ret == 4) return 4;
				if (ret < 0) 
					return ret;
				if ((nbb == 0) && !timed_out[qi]) {  // No data from this board; if timeout not reached, continue to wait (the funtion returns 0)
					if (nodata_cnt[qi] == 0) {
						nodata_time[qi] = get_time();
					} else if ((nodata_cnt[qi] & 0xFF) == 0) {
						//uint64_t tnd = get_time() - nodata_time[qi];  // time since no data
						if ((get_time() - nodata_time[qi]) > FERSLIB_QUEUE_TIMEOUT_MS) 
							timed_out[qi] = 1;
					}
					nodata_cnt[qi]++;
					if (!timed_out[qi]) return 0;
				} else {
					nodata_cnt[qi] = 0;
				}
				if (nbb > 0) 
					ret = q_push(qi, EvBuff[qi], EvBuff_nb[qi]);
				if (ret < 0 ) {
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Queue Push error (ret = %d)\n", qi, ret);
					return ret;
				}
				EvBuff_nb[qi] = 0;
			}
		}

		// Search for oldest tstamp
		oldest_ev = (uint64_t)-1;
		qsel = -1;
		for (i = 0; i < FERSLIB_MAX_NBRD; i++) {
			if (handle[i] == -1) break;
			qi = FERS_INDEX(handle[i]);
			if (ReadoutMode == ROMODE_TRGTIME_SORTING) {
				if ((oldest_ev == -1) || (q_tstamp[qi] < oldest_ev)) {
					oldest_ev = q_tstamp[qi];
					qsel = qi;
				}
			} else {
				if ((oldest_ev == -1) || (q_trgid[qi] < oldest_ev)) {
					oldest_ev = q_trgid[qi];
					qsel = qi;
				}
			}
		}

		// get and decode oldest event 
		ret = q_pop(qsel, tmp_event, &EvBuff_nb[qsel]);
		if (ret < 0) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Queue Pop error (ret = %d)\n", qi, ret);
			return ret;
		}
		FERS_DecodeEvent(handle[qsel], tmp_event, EvBuff_nb[qsel], DataQualifier, tstamp_us, Event);
		*nb = EvBuff_nb[qsel];
		*bindex = qsel;
		EvBuff_nb[qsel] = 0;


	// ---------------------------------------------------------------------
	// UNSORTED
	// ---------------------------------------------------------------------
	} else {
		static int init = 1;
		static int NumCnc = 0;				// Total number of concentrators
		static int NumDir = 0;				// Total number of boards with direct connection (no concentrator)
		static int curr_cindex = -1;		// Index of the concentrator being read

		// First call: find number of concentrators and direct connections
		if (init) {
			int ci = -1;
			for(i=0; (i < FERSLIB_MAX_NBRD) && (handle[i] >= 0); i++) {
				if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_TDL) {
					if (FERS_CNCINDEX(handle[i]) > ci) ci = FERS_CNCINDEX(handle[i]);  // highest cnc index
				} else {
					NumDir++;
				}
			}
			NumCnc = ci + 1;
			if (NumCnc > 0) curr_cindex = 0;
			init = 0;
		}

		if (curr_cindex >= 0) {
			for(i=0; i<NumCnc; i++) {  // CTIN: add time sorting between concentrators. For the moment it is a simple round robin
				h = FERS_CNCINDEX(handle[i]);
				ret = tdl_ReadRawEvent(curr_cindex, bindex, nb);
				if (ret < 0) {
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][CNC %02d] Read Raw data error (ret = %d)\n", curr_cindex, ret);
					return ret;
				}
				if (ret == 2) 
					return 2;
				if (ret == 4)
					return 4;
				curr_cindex = (curr_cindex == (NumCnc-1)) ? 0 : curr_cindex + 1;
				if (*nb > 0) {
					ret = FERS_DecodeEvent(handle[*bindex], EvBuff[*bindex], EvBuff_nb[*bindex], DataQualifier, tstamp_us, Event);
					if (ret < 0) {
						if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Decode error (ret = %d)\n", *bindex, ret);
						return ret;
					}
					EvBuff_nb[*bindex] = 0;
					break;
				}
			}
		} else {
			for(i=0; (i < FERSLIB_MAX_NBRD) && (handle[i] >= 0); i++) {
				h = FERS_INDEX(handle[i]);
				if (EvBuff_nb[h] == 0) {
					ret = eth_usb_ReadRawEvent(handle[i], nb);
					if (ret < 0) {
						if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Read Raw data error (ret = %d)\n", i, ret);
						return ret;
					}
					if (ret == 2) 
						return 2;
					if (ret == 4)
						return 4;
				}
				if ((EvBuff_nb[h] > 0) && ((oldest_ev == 0) || (oldest_ev > Tstamp[h]))) {
					oldest_ev = Tstamp[h];
					h_found = h;
				}
			}
			if (h_found >= 0) {
				ret = FERS_DecodeEvent(handle[h_found], EvBuff[h_found], EvBuff_nb[h_found], DataQualifier, tstamp_us, Event);
				if (ret < 0) {
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Decode error (ret = %d)\n", h_found, ret);
					return ret;
				}
				*nb = EvBuff_nb[h_found];
				*bindex = h_found;
				EvBuff_nb[h_found] = 0;
			}
		}
	}

#if (THROUGHPUT_METER == 3)
	// Rate Meter: print raw data throughput (set nb = 0, thus the data consumer doesn't see any data to process)
	static uint64_t totnb = 0, ct = 0, lt = 0, l0 = 0;
	ct = get_time();
	if (l0 == 0) l0 = ct;
	if (lt == 0) lt = ct;
	totnb += *nb;
	if ((ct - lt) > 1000) {
		printf("%6.1f s: %10.6f MB/s\n", (float)(ct - l0) / 1000, (float)totnb / (ct - lt) / 1000);
		totnb = 0;
		lt = ct;
	}
	*nb = 0;
#endif
	return 1;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Read and decode one event from one specific board
// Inputs:		handle = board handle
// Outputs:		DataQualifier = data qualifier (type of data, used to determine the struct for event data)
//				tstamp_us = time stamp in us (the information is also reported in the event data struct)
//				Event = pointer to the event data struct
//				nb = size of the event (in bytes)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_GetEventFromBoard(int handle, int *DataQualifier, double *tstamp_us, void **Event, int *nb) {
	int h, ret=0;
	h = FERS_INDEX(handle);
	if (EvBuff_nb[h] == 0) {
		if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
			int bindex;
			int cindex = FERS_CNCINDEX(handle);
			ret = tdl_ReadRawEvent(cindex, &bindex, nb);
		} else {
			ret = eth_usb_ReadRawEvent(handle, nb);
		}
		if (ret < 0) return ret;
		if (ret == 2) return 2;
	}
	if (EvBuff_nb[h] > 0) {
		ret |= FERS_DecodeEvent(handle, EvBuff[h], EvBuff_nb[h], DataQualifier, tstamp_us, Event);
		*nb = EvBuff_nb[h];
		EvBuff_nb[h] = 0;
	}
	if (*nb == 0) return 0;
	else return 0;
}


