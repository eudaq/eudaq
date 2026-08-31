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

#ifdef _WIN32
#include "FERS_MultiPlatform.h"
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>

#ifndef _WIN32
#include <dirent.h>
#endif

#include "FERS_LL.h"
#include "FERS_config.h"
#include "FERSlib.h"


#define EVBUFF_SIZE			(16*1024)

// *********************************************************************************************************
// Global Variables
// *********************************************************************************************************
int BoardConnected[FERSLIB_MAX_NBRD] = { 0 };			// Board connection status
int NumBoardConnected = 0;								// Number of boards 
FERS_BoardInfo_t* FERS_BoardInfo[FERSLIB_MAX_NBRD] = { NULL };	// pointers to the board info structs 
FERS_CncInfo_t* FERS_CncInfo[FERSLIB_MAX_NCNC] = { NULL };	// pointers to the cnc info structs
uint16_t MaxEnergyRange = (1 << 13) - 1;
int CncConnected[FERSLIB_MAX_NCNC] = { 0 };				// Concentrator connection status
char BoardPath[FERSLIB_MAX_NBRD][20];					// Path of the FE boards
char CncPath[FERSLIB_MAX_NCNC][20];						// Path of the concentrator
char PedestalsFilename[500];
int CncOpenHandles[FERSLIB_MAX_NCNC] = { 0 };			// Number of handles currently open for the concentrator (slave boards or concentrator itself)
int HVinit[FERSLIB_MAX_NBRD] = { 0 };					// HV init flags
uint16_t PedestalLG[FERSLIB_MAX_NBRD][FERSLIB_MAX_NCH_5202];	// LG Pedestals (calibrate PHA Mux Readout)      64
uint16_t PedestalHG[FERSLIB_MAX_NBRD][FERSLIB_MAX_NCH_5202];	// HG Pedestals (calibrate PHA Mux Readout)
uint16_t CommonPedestal = 100;							// Common pedestal added to all channels
int PedestalBackupPage[FERSLIB_MAX_NBRD] = { 0 };		// Enable pedestal backup page
int EnablePedCal= 1;									// 0 = disable calibration, 1 = enable calibration
Config_t* FERScfg[FERSLIB_MAX_NBRD];
int FERS_TotalAllocatedMem = 0;							// Total allocated memory 
int FERS_ReadoutStatus = 0;								// Status of the readout processes (idle, running, flushing, etc...)
#ifdef _WIN32
mutex_t FERS_RoMutex = NULL;							// Mutex for the access to FERS_ReadoutStatus
#else
mutex_t FERS_RoMutex;									// Mutex for the access to FERS_ReadoutStatus
#endif
f_sem_t FERS_StartRunSemaphore[FERSLIB_MAX_NBRD];	// Semaphore for sync the start of the run with the data receiver thread
int DebugLogs = 0;									// Debug Logs
//uint8_t EnableRawData = 0;							// Enable LowLevel data saving
uint8_t ProcessRawData = 0;							// Enable ReadingOut the RawData file saved	- Is the same of FERS_Offline, redundant
uint8_t EnableSubRun = 1;							// Enable sub run increasing while reading Raw Data file
//uint8_t EnableMaxSizeFile;							// Enable the Max size for Raw Data file saving
//float MaxSizeRawDataFile;							// Max Size for LLData saving
char RawDataFilename[FERSLIB_MAX_NBRD][500];		// Rawdata Filename (eth, usb connection)
char RawDataFilenameTdl[FERSLIB_MAX_NBRD][500];		// Rawdata Filename (tdl connection)

int FERS_Offline = 0;

float CLK_PERIOD[FERSLIB_MAX_NBRD];  				// clock period in ns (for reg settings)

static THREAD_LOCAL char lastError[1024];
static int FERS_OpenOffline(char* path, int* handle);

// *********************************************************************************************************
// Messaging and errors
// *********************************************************************************************************
static void _getLastLocalError(char description[1024]) {
	strncpy(description, lastError, 1024);
	description[1024 - 1] = '\0';
}

static void _resetLastLocalError(void) {
	lastError[0] = '\0';
}

void _setLastLocalError(const char* description, ...) {
	va_list args;
	va_start(args, description);
	vsnprintf(lastError, ARRAY_SIZE(lastError), description, args);
	va_end(args);
}

int FERS_LibMsg(char *fmt, ...) 
{
	char msg[1000];
	char FERS_MsgString[1024];
	static FILE *LibLog;
	static int openfile = 1;
	static uint64_t t0;
	uint64_t elapsed_time; 
	va_list args;

	uint64_t tstamp = get_time();

	if (openfile) {
		char mytime[100];
		time_t startt;
		time(&startt);
		strcpy(mytime, asctime(gmtime(&startt)));
		mytime[strlen(mytime) - 1] = 0;

		LibLog = fopen("FERSlib_Log.txt", "w");
		if (LibLog != NULL) {
			fprintf(LibLog, "FERSlib Log created on %s UTC\n", mytime); 
			fflush(LibLog);
		}
		t0 = tstamp;
		openfile = 0;
	}

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

	//printf("%s", msg); // Write to console
	elapsed_time = tstamp - t0;
	uint64_t ms = elapsed_time % 1000;
	uint64_t s = (elapsed_time / 1000) % 60;
	uint64_t m = (elapsed_time / 60000) % 60;
	uint64_t h = (elapsed_time / 3600000);
	sprintf(FERS_MsgString, "[%02dh:%02dm:%02ds:%03dms]%s", (int)h, (int)m, (int)s, (int)ms, msg); // Added timestamp to FERSlib Log	
	//sprintf(FERS_MsgString, "[%" PRIu64 "]%s", tstamp, msg); // Added timestamp to FERSlib Log	
	if (LibLog != NULL) {
		fprintf(LibLog, "%s", FERS_MsgString); // Write to Log File
		fflush(LibLog);
	}
	return 0;
}

int FERS_GetLastError(char description[1024]) {
	_getLastLocalError(description);
	_resetLastLocalError();
	return 0;
}

// Get if Raw data filename is FERSlib like
int isRawDataFilename(const char* rawdata_filename, int *a0, int *a1) {
	const char* end_str = strrchr(rawdata_filename, '_');

	//char* end_str = strrchr(rawdata_filename, '_');
	if (!end_str) {
		return 0;
	}
	size_t len = strlen(end_str);
	const char* suffix = ".frd";
	size_t suffixLen = strlen(suffix);

	const char* pattern = "_Run%d.%d.frd";
	int matches = sscanf(end_str, pattern, a0, a1);

	// Check if the the string matches 
	if (matches == 2 && strncmp(end_str, "_Run", 4) == 0 && strncmp(end_str + len - suffixLen, suffix, suffixLen) == 0) {
		return 1;
	} else
		return 0;
}


// --------------------------------------------------------------------
// Get Release num and date
// --------------------------------------------------------------------
char* FERS_GetLibReleaseNum()
{
	return FERSLIB_RELEASE_STRING;
}


char* FERS_GetLibReleaseDate()
{
	return FERSLIB_RELEASE_DATE;
}


// --------------------------------------------------------------------
// Macros for getting main parameters of the FERS 
// --------------------------------------------------------------------
//int FERS_Model(int handle) {
//	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->Model : 0)
//}
uint32_t FERS_pid(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->pid : 0);
}

uint16_t FERS_Code(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode : 0);
}

char* FERS_ModelName(int handle) {
	char nu[10] = "";
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->ModelName : nu);
}

uint32_t FERS_FPGA_FWrev(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev : 0);
}

uint32_t FERS_FPGA_FW_MajorRev(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? ((FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev) >> 8) & 0xFF : 0);
}

uint32_t FERS_FPGA_FW_MinorRev(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? (FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev) & 0xFF : 0);
}

uint32_t FERS_uC_FWrev(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->uC_FWrev : 0);
}

uint16_t FERS_NumChannels(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->NumCh : 0);
}

uint8_t FERS_PCB_Rev(int handle) {
	return ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->PCBrevision : 0);
}

uint16_t FERS_GetNumBrdConnected() {
	return (uint16_t)NumBoardConnected;
}

bool FERS_IsXROC(int handle) {
	if ((FERS_INDEX(handle) >= 0) && ((FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5202) || 
									  (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5204) ||
									  (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5205)))
		return true;
	else
		return false;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Enable Raw Data output file writing, with open and close file
// Inputs:		DebugEnableMask: RawData Writing enable, Max size of RawData file, DataFilePath, RunNumber
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 


// 
// --------------------------------------------------------------------------------------------------------- 
// Description: Set Raw data filename for re-processing
// Inputs:		DataRawFilePath: filename with its path
//				brd: board of index
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int FERS_SetRawdataReadFile(char DataRawFilePath[500], int brd)
{
	int ret = 0;
	if (strlen(DataRawFilePath) == 0) {
		_setLastLocalError("ERROR: not Raw Data file provided");
		return FERSLIB_ERR_INVALID_PATH;
	}

	char off_fname[500];
	strcpy(off_fname, DataRawFilePath);

	char* end_string = strrchr(off_fname, '_');

	if (!end_string) {  // Filename does not contain the the Run part. Take the filename as it is
		EnableSubRun = 0;
		sprintf(RawDataFilename[brd], "%s", off_fname);
		sprintf(RawDataFilenameTdl[brd], "%s", off_fname);
		return 0;
	}
	int run = 0, srun = 0;
	int isOk = isRawDataFilename(off_fname, &run, &srun);

	if (isOk == 0) {	// Filename with no subrun, just take the filename as it is
		EnableSubRun = 0;
		sprintf(RawDataFilename[brd], "%s", off_fname);
		sprintf(RawDataFilenameTdl[brd], "%s", off_fname);
		return 0;
	} else {
		EnableSubRun = 1;
		strncpy(RawDataFilename[brd], off_fname, strlen(off_fname) - 6);
		strncpy(RawDataFilenameTdl[brd], off_fname, strlen(off_fname) - 6);
		//char* tRaw = DataRawFilePath[brd];
		//char tint[10]; // subrun part. It should start from 0, since that's the file containing the Board info
		//sprintf(tint, ".%d", srun);
		//sprintf(RawDataFilename[i], "%s", tRaw);// (DataRawFilePath - strlen(tint) - strlen(".frd")));
		//sprintf(RawDataFilenameTdl[i], "%s", (DataRawFilePath[i] - strlen(tint) - strlen(".frd")));
	}
	return ret;
}


// Open raw data file to dump raw data
int FERS_OpenRawDataFile(int *handle, int RunNum) 
{
	uint16_t brd_conn = FERS_GetNumBrdConnected();
	uint8_t tdl_opened = 0;

	for (int i = 0; i < brd_conn; ++i) {
		// Open if RawData saving is enabled
		if (!FERScfg[FERS_INDEX(handle[i])]->OF_RawData) continue;

		// DNIN: at the moment just one concentrator connection is handled
		if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_TDL && !tdl_opened) {
			sprintf(RawDataFilenameTdl[i], "%sRawData_cnc%d_Run%d", FERScfg[i]->OF_RawDataPath, i, RunNum); // This is not usefull
			LLtdl_OpenRawOutputFile(handle);
			tdl_opened = 1;
		} else {
			sprintf(RawDataFilename[i], "%sRawData_b%d_Run%d", FERScfg[i]->OF_RawDataPath, i, RunNum);
			if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_ETH)
				LLeth_OpenRawOutputFile(handle[i]);
			else if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_USB)
				LLusb_OpenRawOutputFile(handle[i]);
		}
	}
	return 0;
}

// Close raw data file
int FERS_CloseRawDataFile(int *handle)
{
	uint16_t brd_conn = FERS_GetNumBrdConnected();
	uint8_t tdl_closed = 0;
	for (int i = 0; i < brd_conn; ++i) {
		if (!FERScfg[FERS_INDEX(handle[i])]->OF_RawData) continue;

		if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_TDL && !tdl_closed) {
			LLtdl_CloseRawOutputFile(handle[i]);
			tdl_closed = 1;
		}
		else if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_ETH)
			LLeth_CloseRawOutputFile(handle[i]);
		else if (FERS_CONNECTIONTYPE(handle[i]) == FERS_CONNECTIONTYPE_USB)
			LLusb_CloseRawOutputFile(handle[i]);
	}

	return 0;
}

// Load pedestal values from file
static int FERS_SetPedestalOffline(uint16_t PedLG[FERSLIB_MAX_NCH_5202], uint16_t PedHG[FERSLIB_MAX_NCH_5202], int handle)
{
	for (int i = 0; i < FERSLIB_MAX_NCH_5202; ++i) {
		PedestalLG[FERS_INDEX(handle)][i] = PedLG[i];
		PedestalHG[FERS_INDEX(handle)][i] = PedHG[i];
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Set Lib Clock period depending on board code
// Input:		handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int FERS_SetClockPeriodLib(int handle)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5202) {
		CLK_PERIOD[FERS_INDEX(handle)] = (float)CLK_PERIOD_5202;
	} else if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203) {
		CLK_PERIOD[FERS_INDEX(handle)] = (float)CLK_PERIOD_5203;
	} else if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5204) {
		CLK_PERIOD[FERS_INDEX(handle)] = (float)CLK_PERIOD_5204;
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Get the Clock period depending on board code
// Input:		handle
// Return:		Clock period in ns
// --------------------------------------------------------------------------------------------------------- 
float FERS_GetClockPeriod(int handle)
{
	return CLK_PERIOD[FERS_INDEX(handle)];
}


// ---------------------------------------------------------------------------------------------------------
// Description: Set FERS_Offline variable
// Inputs:		value of offline variable
// Return:		0
// ---------------------------------------------------------------------------------------------------------
static int FERS_SetOffline(int offline)
{
	FERS_Offline = offline;
	ProcessRawData = (uint8_t)offline;
	return 0;
}


static int FERS_SetBoardInfo(FERS_BoardInfo_t* BrdInfo, int handle)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)] == NULL) {
		FERS_BoardInfo[FERS_INDEX(handle)] = (FERS_BoardInfo_t*)malloc(sizeof(FERS_BoardInfo_t));
	}
	memcpy(FERS_BoardInfo[FERS_INDEX(handle)], BrdInfo, sizeof(FERS_BoardInfo_t));
	FERS_SetClockPeriodLib(handle);
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Read Board info from the relevant flash memory page of the FERS unit
// Inputs:		handle = board handle 
// Outputs:		binfo = board info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int FERS_ReadBoardInfo(int handle, FERS_BoardInfo_t* binfo)
{
	int ret;
	uint8_t bic[FLASH_PAGE_SIZE];

	memset(binfo, 0, sizeof(FERS_BoardInfo_t));
	for (int i = 0; i < 10; i++) {
		ret = FERS_ReadFlashPage(handle, FLASH_BIC_PAGE, 46, bic);
		if (ret < 0) return ret;
		if ((bic[0] != 'B') || (bic[1] != 0)) {
			if (i == 3)	return FERSLIB_ERR_INVALID_BIC;
		} else break;
	}
	memcpy(&binfo->pid, bic + 2, 4);
	memcpy(&binfo->FERSCode, bic + 6, 2);
	memcpy(&binfo->PCBrevision, bic + 8, 1);
	memcpy(binfo->ModelCode, bic + 9, 16);
	memcpy(binfo->ModelName, bic + 25, 16);
	memcpy(&binfo->FormFactor, bic + 41, 1);
	memcpy(&binfo->NumCh, bic + 42, 2);
	//memcpy(&PedestalCalibPage,	bic + 44, 2);
	FERS_ReadRegister(handle, a_fw_rev, &binfo->FPGA_FWrev);
	FERS_ReadRegister(handle, 0xF0000000, &binfo->uC_FWrev);
	// Manage possible inconsistency in num of channels
	if  (binfo->FERSCode == 5202 || binfo->FERSCode == 5204) {
		if (binfo->NumCh != FERSLIB_MAX_NCH_5202) {
			FERS_LibMsg("[WARNING] Inconsistent NumCh read from board %d(PID=%d): NumCh Read=%d, library forces NumCh to %d\n", FERS_INDEX(handle), binfo->pid, binfo->NumCh, FERSLIB_MAX_NCH_5202);
			binfo->NumCh = FERSLIB_MAX_NCH_5202;
		}
	} else if (binfo->FERSCode == 5203) {
		if (binfo->NumCh != FERSLIB_MAX_NCH_5203 && binfo->NumCh != FERSLIB_MAX_NCH_5202) {
			FERS_LibMsg("[WARNING] Inconsistent NumCh read from board %d(PID=%d): NumCh Read=%d, library forces NumCh to %d\n", FERS_INDEX(handle), binfo->pid, binfo->NumCh, FERSLIB_MAX_NCH_5202);
			binfo->NumCh = FERSLIB_MAX_NCH_5202;
		}
	}
	return 0;
}


// *********************************************************************************************************
// Open/Close
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Open a device (either FERS board or concentrator). Further operation might be performed
//              after the board has been opened, such as calibration, data flush, etc...
//              NOTE: when opening a FE board connected to a concentrator that has not been opened before,
//              the function automatically opens the concentrator first (without any notice to the caller)
// Inputs:		path = device path (e.g. eth:192.168.50.125:tdl:0:0)
// Outputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_OpenDevice(char *path, int *handle) 
{
	int BoardIndex, CncIndex, i, ret, ns;
	int cnc_handle=-1;
	char *s, * sep, ss[10][20], cpath[50];
	uint32_t fwrev;

	if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO] Opening Device with path %s\n", path);
	if (strstr(path, "offline") != NULL) { // Open an offline connection
		ret = FERS_OpenOffline(path, handle);
		ret |= FERS_SetOffline(1);
		return ret;
	}
	else {
		// split path into strings separated by ':'
		ns = 0;
		s = path;
		while (s < path + strlen(path)) {
			sep = strchr(s, ':');
			if (sep == NULL) break;
			strncpy(ss[ns], s, sep - s);
			ss[ns][sep - s] = 0;
			s += sep - s + 1;
			ns++;
			if (ns == 9) break;
		}
		strcpy(ss[ns++], s);
		if (ns < 2) return FERSLIB_ERR_INVALID_PATH;
	}

	if (((strstr(path, "cnc") != NULL) || (strstr(path, "tdl") != NULL)) && !FERS_Offline) {  // Connection through concentrator
		// Find cnc already opened with this path or find a free cnc index and open it
		sprintf(cpath, "%s:%s:cnc", ss[0], ss[1]);
		for(i=0; i < FERSLIB_MAX_NCNC; i++) {
			if (CncConnected[i]) {
				if (strcmp(cpath, CncPath[i]) == 0) {
					cnc_handle = FERS_CONNECTIONTYPE_CNC | i;
					break;
				}
			} else break;
		}
		if (cnc_handle == -1) {
			if (i == FERSLIB_MAX_NCNC) return FERSLIB_ERR_MAX_NBOARD_REACHED;
			CncIndex = i;
			if (strstr(path, "eth") != NULL) {  // Ethernet
				ret = LLtdl_OpenDevice(ss[1], CncIndex);
			} else if (strstr(path, "usb") != NULL) {  // Usb
				ret = LLtdl_OpenDevice(ss[1], CncIndex);
			} else
				ret = -1;
			if (ret < 0) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] Can't open CNC n. %d with IP_addr = %s\n", CncIndex, ss[1]);
				_setLastLocalError("Can't open Concentrator");
				return FERSLIB_ERR_COMMUNICATION;
			}
			CncConnected[CncIndex] = 1;
			strcpy(CncPath[CncIndex], cpath);
			cnc_handle = FERS_CONNECTIONTYPE_CNC | CncIndex;
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][CNC %02d] Connected CNC with index %d\n", CncIndex, CncIndex);
		}
	}

	if (((strstr(path, "cnc") != NULL) && (ns == 3)) && !FERS_Offline) {  // Open Concentrator only
		if (cnc_handle == -1) return FERSLIB_ERR_INVALID_PATH;
		*handle = cnc_handle;
		CncOpenHandles[FERS_INDEX(cnc_handle)]++;
		return 0;

	} else {  // Open FE board

	 // Find free board index 
		for (i = 0; i < FERSLIB_MAX_NBRD; i++)
			if (!BoardConnected[i]) break;
		if (i == FERSLIB_MAX_NBRD) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR] Max num of boards (%d) reached!\n", FERSLIB_MAX_NBRD);
			_setLastLocalError("Max num of boards reached!");
			return FERSLIB_ERR_MAX_NBOARD_REACHED;
		}
		BoardIndex = i;
		//strcpy(FERScfg[i]->ConnPath, path);

		if ((strstr(path, "tdl") != NULL) && (ns == 5)) {  // Concentrator => TDlink => FE-board
			int chain, node, cindex = FERS_INDEX(cnc_handle);
			sscanf(ss[3], "%d", &chain);
			sscanf(ss[4], "%d", &node);
			if ((chain < 0) || (chain >= FERSLIB_MAX_NTDL) || (node < 0) || (node >= FERSLIB_MAX_NNODES)) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Invalid path TDL to node (%d:%d:%d)\n", BoardIndex, cindex, chain, node);
				_setLastLocalError("Invalid TDL path");
				return FERSLIB_ERR_INVALID_PATH;
			}
			//ret = LLtdl_InitTDLchains(cindex, NULL);
			//if (ret < 0) return ret;
			if (node >= (int)TDL_NumNodes[cindex][chain]) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Invalid path to TDL node (%d:%d:%d). Chain %d has %d nodes\n", BoardIndex, cindex, chain, node, chain, TDL_NumNodes[cindex][chain]);
				_setLastLocalError("Invalid TDL path");
				return FERSLIB_ERR_INVALID_PATH;
			}
			CncOpenHandles[cindex]++;
			*handle = (cindex << 30) | (chain << 24) | (node << 20) | FERS_CONNECTIONTYPE_TDL | BoardIndex;

		} else if (ns == 2) {  // Direct to FE-board
			if (strstr(path, "eth") != NULL) {  // Ethernet
				ret = LLeth_OpenDevice(ss[1], BoardIndex);
				if (ret < 0) {
					FERS_LibMsg("[ERROR][BRD %02d] Failed to open the device at %s\n", BoardIndex, ss[1]);
					_setLastLocalError("Can't open the Eth device");
					return FERSLIB_ERR_COMMUNICATION;
				}
				*handle = FERS_CONNECTIONTYPE_ETH | BoardIndex;
			} else if (strstr(path, "usb") != NULL) { // USB
				int pid;
				sscanf(ss[1], "%d", &pid);
				ret = LLusb_OpenDevice(pid, BoardIndex);
				if (ret < 0) {
					FERS_LibMsg("[ERROR][BRD %02d] Failed to open the USB device %d\n", BoardIndex, pid);
					_setLastLocalError("Can't open the USB device");
					return FERSLIB_ERR_COMMUNICATION;
				}
				*handle = FERS_CONNECTIONTYPE_USB | BoardIndex;
			} else {
				return FERSLIB_ERR_INVALID_PATH;
			}
		} else {
			return FERSLIB_ERR_INVALID_PATH;
		}

		BoardConnected[BoardIndex] = 1;
		strcpy(BoardPath[BoardIndex], path);
		FERS_BoardInfo[BoardIndex] = (FERS_BoardInfo_t*)malloc(sizeof(FERS_BoardInfo_t));
		FERS_TotalAllocatedMem += sizeof(FERS_BoardInfo_t);

		FERScfg[i] = (Config_t*)malloc(sizeof(Config_t));  // Further, allocate ch arrays dynamically
		memset(FERScfg[i], 0, sizeof(Config_t));
		FERS_TotalAllocatedMem += sizeof(Config_t);

		++NumBoardConnected;

		// Read FW revision to verify that the board is responding
		ret = FERS_ReadRegister(*handle, a_fw_rev, &fwrev);
		if (ret != 0) {  
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Can't access board registers. The board is not communicating\n", BoardIndex);
			char tmpDesc[1024];
			sprintf(tmpDesc, "Can't access board %d registers", BoardIndex);
			_setLastLocalError(tmpDesc);
			free(FERS_BoardInfo[BoardIndex]);
			FERS_BoardInfo[BoardIndex] = NULL;
			return FERSLIB_ERR_INVALID_FW;
		}
		if (fwrev == 0) {  // fwrev = 0 means that the board is responding but there is not a valid FW. 
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Board is not running a valid firmware\n", BoardIndex);
			char tmpDesc[1024];
			sprintf(tmpDesc, "Board %d is not running a valid firmware", BoardIndex);
			_setLastLocalError(tmpDesc);
			free(FERS_BoardInfo[BoardIndex]);
			FERS_BoardInfo[BoardIndex] = NULL;
			return FERSLIB_ERR_INVALID_FW;
		}

		// Read board info (BIC) from the flash memory via SPI bus
		ret = FERS_ReadBoardInfo(*handle, FERS_BoardInfo[BoardIndex]);
		// Check if board is already connected
		//for (int bb = 0; bb < BoardIndex; ++bb) {
		//	if (FERS_BoardInfo[BoardIndex]->pid == FERS_BoardInfo[bb]->pid) {
		//		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Board is already open\n", BoardIndex);
		//		char tmpDesc[1024];
		//		sprintf(tmpDesc, "Board %d is already open", FERS_BoardInfo[BoardIndex]->pid);
		//		_setLastLocalError(tmpDesc);
		//		return FERSLIB_ERR_DEVICE_ALREADY_OPENED;
		//	}
		//}

		FERScfg[BoardIndex]->handle = *handle;
		FERS_SetClockPeriodLib(*handle);
		if (ret != 0) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD %02d] Can't read board info or invalid BIC\n", BoardIndex);
			_setLastLocalError("Can't read board info or invalid BIC");
			free(FERS_BoardInfo[BoardIndex]);
			FERS_BoardInfo[BoardIndex] = NULL;
			return FERSLIB_ERR_INVALID_BIC;
		}

		// Read pedestal calibration and DC offset from flash memory
		if (FERS_IsXROC(*handle)) {
			uint16_t DCoffset[4];
			uint32_t dco = (FERS_BoardInfo[BoardIndex]->FERSCode == 5202) ? 2750 : 0xB80;  // default value in case of missing DC offset in flash memory
			ret = FERS_ReadPedestalsFromFlash(*handle, NULL, PedestalLG[BoardIndex], PedestalHG[BoardIndex], DCoffset);
			if (FERS_BoardInfo[BoardIndex]->FERSCode == 5202) {
				for (i = 0; i < 4; i++) {  // 0=LG0, 1=HG0, 2=LG1, 3=HG1
					if ((DCoffset[i] > 0) && (DCoffset[i] < 4095))
						FERS_WriteRegister(*handle, a_dc_offset, (i << 14) | DCoffset[i]);
					else
						FERS_WriteRegister(*handle, a_dc_offset, (i << 14) | dco);
					Sleep(1);
				}
			}
			else if (FERS_BoardInfo[BoardIndex]->FERSCode == 5204) {
				for (i = 0; i < 2; i++) {  // 0=LG, 1=HG
					if ((DCoffset[i] > 0) && (DCoffset[i] < 4095))
						FERS_WriteRegister(*handle, a_dc_offset, (i << 14) | DCoffset[i]);
					else
						FERS_WriteRegister(*handle, a_dc_offset, (i << 14) | dco);
					Sleep(1);
				}
			}
			if (ret != 0) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][BRD %02d] Failed to set DC offset for MuxOut\n", BoardIndex);
			}
		}

		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][BRD %02d] Connected board with index %d: Type=%s, Code=%" PRIu16 ", PID=%" PRIu32 ", Ch=%" PRIu16 ", FW_Rev=% 08X\n", BoardIndex, BoardIndex, 
			FERS_BoardInfo[BoardIndex]->ModelName,
			FERS_BoardInfo[BoardIndex]->FERSCode,
			FERS_BoardInfo[BoardIndex]->pid,
			FERS_BoardInfo[BoardIndex]->NumCh,
			FERS_BoardInfo[BoardIndex]->FPGA_FWrev);

		// Send a Stop command (in case the board is still running from a previous connection)
		FERS_SendCommand(*handle, CMD_ACQ_STOP);
	}
	return 0;
}


// *********************************************************************************************************
// Offline Open/Close
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Open a Raw file (either FERS board or concentrator). 
//				The raw data file must contain an header as:
//				Header_Size Header_Marker Cnc_Info(If Tdl) Brd_Handle Brd_Info Pedestal_LG Pedestal HG             
// Inputs:		path = device path (e.g. eth:192.168.50.125:tdl:0:0)
// Outputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_OpenOffline(char* path, int *handle) {
	int ret = 0;
	if (strstr(path, "offline") == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][OFFLINE] Filename provided is not correct. Please, insert the path as 'offline:path_to_filename'\n");
		_setLastLocalError("Filename provided is not correct. Please, insert the path as 'offline:path_to_filename'");
		return FERSLIB_ERR_INVALID_PATH;
	}
	FERS_Offline = 1;
	ProcessRawData = (uint8_t)FERS_Offline;

	char tmpPath[100];
	sprintf(tmpPath, "%s", path);
	char* filename = strtok(tmpPath, ":");
	filename = strtok(NULL, "");
	if (filename == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][OFFLINE] No filename provided. Insert path as 'offline:path_to_filename'\n");
		_setLastLocalError("No filename provided. Insert path as 'offline:path_to_filename'");
		return FERSLIB_ERR_INVALID_PATH;
	}

	// Open Board

	FILE* tmp_info;
	tmp_info = fopen(filename, "rb");
	if (tmp_info == NULL) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][OFFLINE] Cannot open Raw Data file %s\n", filename);
		_setLastLocalError("Cannot open rawdata file %s", filename);
		return FERSLIB_ERR_INVALID_PATH;
	}
	size_t header_size = 0;
	char file_header[33];
	uint16_t PedeLG[64], PedeHG[64];
	FERS_BoardInfo_t BoardInfo;
	//fscanf(tmp_info, "%s", &file_header);
	int fret = fread(&file_header, 32, 1, tmp_info);
	if (strcmp(file_header, "$$$$$$$FERSRAWDATAHEADER$$$$$$$") != 0) { // No header mark found
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][OFFLINE] No valid keyword header found\n");
		_setLastLocalError("No valid keyword header found");
		return FERSLIB_ERR_GENERIC;
	}

	//uint8_t rere;
	//fread(&rere, sizeof(uint8_t), 1, tmp_info);
	//fread(&rere, sizeof(uint8_t), 1, tmp_info);
	//fread(&rere, sizeof(uint8_t), 1, tmp_info);
	//fread(&rere, sizeof(uint8_t), 1, tmp_info);

	fret = fread(&header_size, sizeof(header_size), 1, tmp_info);
	if (header_size == 0) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][OFFLINE] Cannot read header of Raw Data file %s\n", filename);
		_setLastLocalError("Cannot read header of RawData file %s", filename);
		return FERSLIB_ERR_GENERIC;
	}

	header_size -= sizeof(header_size);

	// if header size is larger then handle+brdInfo, a tdl connection was established.
	// Cannot be retrieved from handle since it is not yet read
	if (header_size > sizeof(*handle) + sizeof(FERS_BoardInfo_t) + 2 * sizeof(PedeLG)) { // For 5203, sizeof CncInfo is > 2*Pedestal. The inequality is still true.
		FERS_CncInfo[0] = (FERS_CncInfo_t*)malloc(sizeof(FERS_CncInfo_t));
		//fread(&cnc_handle, sizeof(int), 1, tmp_info);
		fret = fread(FERS_CncInfo[0], sizeof(FERS_CncInfo_t), 1, tmp_info);
		header_size -= sizeof(FERS_CncInfo_t);
		//FERS_SetConcentratorInfo(cnc_handle, &tmpCInfo);
	}

	int b = 0;
	// Find first free board index 
	for (b = 0; b < FERSLIB_MAX_NBRD; b++)
		if (!BoardConnected[b]) break;
	if (b == FERSLIB_MAX_NBRD) return FERSLIB_ERR_MAX_NBOARD_REACHED;

	// Here the file has been checked, can be set as file for reprocessing
	ret = FERS_SetRawdataReadFile(filename, b);
	if (ret < 0)
		return ret;

	while (header_size > 0) {
		if (b >= FERSLIB_MAX_NBRD) {
			_setLastLocalError("ERROR: Exceeded max number of board connected");
			return FERSLIB_ERR_MAX_NBOARD_REACHED;
		}
		FERScfg[b] = (Config_t*)calloc(1, sizeof(Config_t));
		//strcpy(FERScfg[b]->ConnPath, filename);

		int tmp_handle = 0;
		fread(&tmp_handle, sizeof(int), 1, tmp_info);
		handle[b] = tmp_handle;

		fread(&BoardInfo, sizeof(FERS_BoardInfo_t), 1, tmp_info);		if (BoardInfo.FERSCode == 5202) {
			fret = fread(&PedeLG, sizeof(PedeLG), 1, tmp_info);
			fret = fread(&PedeHG, sizeof(PedeHG), 1, tmp_info);
		}

		// Open LL connection
		int connection_type = FERS_CONNECTIONTYPE(*handle);
		if (connection_type == FERS_CONNECTIONTYPE_TDL) {  // Concentrator => TDlink => FE-board
			int cindex = FERS_CNCINDEX(*handle);
			CncOpenHandles[cindex]++;
		} else if (connection_type == FERS_CONNECTIONTYPE_ETH) {  // Direct to FE-board - Ethernet
			ret |= LLeth_OpenDevice(filename, b);
			if (ret < 0) return FERSLIB_ERR_COMMUNICATION;
		} else if (connection_type == FERS_CONNECTIONTYPE_USB) { // USB
			int pid = FERS_INDEX(handle[b]);
			ret |= LLusb_OpenDevice(pid, b);
			if (ret < 0) return FERSLIB_ERR_COMMUNICATION;
		} else {
			return FERSLIB_ERR_INVALID_PATH;
		}

		FERS_SetBoardInfo(&BoardInfo, handle[b]);
		if (FERS_IsXROC(handle[b]))
			FERS_SetPedestalOffline(PedeLG, PedeHG, handle[b]);

		header_size -= (sizeof(*handle) + sizeof(FERS_BoardInfo_t));
		if (FERS_IsXROC(handle[b]))
			header_size -= 2 * sizeof(PedeLG);

		BoardConnected[b] = 1;
		++NumBoardConnected;

		if (header_size > 0) ++b;
	}

	fclose(tmp_info);

	return ret;
}


// For offline. Board info should contain at least the FERSCode and ModelName


int FERS_GetBoardInfo(int handle, FERS_BoardInfo_t* BrdInfo)
{
	if ((FERS_BoardInfo[FERS_INDEX(handle)] == NULL) || (FERS_BoardInfo[FERS_INDEX(handle)]->ModelCode == 0))
		return FERSLIB_ERR_COMMUNICATION;  // the board info is read in the FERS_OpenDevice function; if the board info in not availabe or null, we assume that the board is not communicating
	memcpy(BrdInfo, FERS_BoardInfo[FERS_INDEX(handle)], sizeof(FERS_BoardInfo_t));
	return 0;
}

int FERS_GetCncInfo(int handle, FERS_CncInfo_t* BrdInfo) 
{
	int cnc_index = FERS_CNCINDEX(handle);
	if (FERS_CncInfo[cnc_index] == NULL)
		return 0;  // Maybe should return something else
	memcpy(BrdInfo, FERS_CncInfo[cnc_index], sizeof(FERS_CncInfo_t));
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Check if a device is already open
// Inputs:		path = device path
// Return:		0=not open, 1=open
// --------------------------------------------------------------------------------------------------------- 
int FERS_IsOpen(char *path) 
{
	int i;
	for(i=0; i<FERSLIB_MAX_NBRD; i++) 
		if (BoardConnected[i] && (strcmp(BoardPath[i], path) == 0)) return 1;
	for(i=0; i<FERSLIB_MAX_NCNC; i++) 
		if (CncConnected[i] && (strcmp(CncPath[i], path) == 0)) return 1;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Close device (either FERS board or concentrator)
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_CloseDevice(int handle) 
{
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_CNC) {
		CncOpenHandles[FERS_CNCINDEX(handle)]--;
		if (CncOpenHandles[FERS_CNCINDEX(handle)] == 0) {
			LLtdl_CloseDevice(FERS_CNCINDEX(handle));
			CncConnected[FERS_CNCINDEX(handle)] = 0;
			if (FERS_CncInfo[FERS_CNCINDEX(handle)] != NULL) {
				free(FERS_CncInfo[FERS_CNCINDEX(handle)]);
				FERS_CncInfo[FERS_CNCINDEX(handle)] = NULL;
			}
		}
	} else {
		if ((handle < 0) || (FERS_INDEX(handle) >= FERSLIB_MAX_NBRD)) return FERSLIB_ERR_INVALID_HANDLE;
		if (BoardConnected[FERS_INDEX(handle)]  && !FERS_Offline) {
			if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) {
				LLeth_CloseDevice(FERS_INDEX(handle));
			} else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB) {
				LLusb_CloseDevice(FERS_INDEX(handle));
			} else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
				CncOpenHandles[FERS_CNCINDEX(handle)]--;
				if (CncOpenHandles[FERS_CNCINDEX(handle)] == 0)
					LLtdl_CloseDevice(FERS_CNCINDEX(handle));
			}
		} else if (((FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) || (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_CNC)) && !FERS_Offline) {
			CncOpenHandles[FERS_INDEX(handle)]--;
			if (CncOpenHandles[FERS_INDEX(handle)] == 0)
				LLtdl_CloseDevice(FERS_INDEX(handle));
		}
		free(FERS_BoardInfo[FERS_INDEX(handle)]); // free the board info struct
		FERS_BoardInfo[FERS_INDEX(handle)] = NULL;
		FERS_TotalAllocatedMem -= sizeof(FERS_BoardInfo[FERS_INDEX(handle)]);

		BoardConnected[FERS_INDEX(handle)] = 0;
		--NumBoardConnected;

		free(FERScfg[FERS_INDEX(handle)]);
		FERScfg[FERS_INDEX(handle)] = NULL;
		FERS_TotalAllocatedMem -= sizeof(FERScfg[FERS_INDEX(handle)]);
	}
	if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[INFO][BRD %02d] Device closed\n", FERS_INDEX(handle));
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Return the total size of the allocated buffers for the readout of all boards
// Return:		alloctaed memory (bytes)
// --------------------------------------------------------------------------------------------------------- 
int FERS_TotalAllocatedMemory() {
	return FERS_TotalAllocatedMem;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Restore the factory IP address (192.168.50.3) of the device. The board must be connected 
//              through the USB port
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Reset_IPaddress(int handle) 
{
	if ((handle < 0) || (FERS_INDEX(handle) >= FERSLIB_MAX_NBRD) || (FERS_CONNECTIONTYPE(handle) != FERS_CONNECTIONTYPE_USB)) return FERSLIB_ERR_INVALID_HANDLE;
	return (LLusb_Reset_IPaddress(FERS_INDEX(handle)));
}


// --------------------------------------------------------------------------------------------------------- 
// Description: find the path of the concentrator to which a device is connected
// Inputs:		dev_path = device path
// Outputs:		cnc_path = concentrator path
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Get_CncPath(char *dev_path, char *cnc_path) 
{
	char *cc = strstr(dev_path, "tdl");
	strcpy(cnc_path, "");
	if (cc == NULL) return -1;
	strncpy(cnc_path, dev_path, cc - dev_path);
	cnc_path[cc - dev_path] = 0;
	strcat(cnc_path, "cnc");
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Initialize the TDL chains 
// Inputs:		handle = concentrator handle
//				DelayAdjust = individual fiber delay adjust
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_InitTDLchains(int handle, float DelayAdjust[FERSLIB_MAX_NTDL][FERSLIB_MAX_NNODES])
{
	//int cnc = FERS_CNCINDEX(handle);
	return LLtdl_InitTDLchains(FERS_CNCINDEX(handle), DelayAdjust);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Check if TDL chains are initialized
// Inputs:		handle = concentrator handle
// Return:		false = not init, true = init done
// --------------------------------------------------------------------------------------------------------- 
bool FERS_TDLchainsInitialized(int handle)
{
	return LLtdl_TDLchainsInitialized(FERS_CNCINDEX(handle));
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Set the number of bits the energy is given from ADCs
// Inputs:		EnergyRangeBits = boolean
// Return:		0
// --------------------------------------------------------------------------------------------------------- 
int FERS_SetEnergyBitsRange(uint16_t EnergyRange)
{
	if (!EnergyRange) // 13 bits
		MaxEnergyRange = (1 << 13) - 1;
	else // 14 bits
		MaxEnergyRange = (1 << 14) - 1;
	return 0;
}



// *********************************************************************************************************
// Register Read/Write, Send Commands
// *********************************************************************************************************
// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of the board
// Inputs:		handle = device handle
//				address = reg address 
// Outputs:		data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadRegister(int handle, uint32_t address, uint32_t *data) {
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) 
		return LLeth_ReadRegister(FERS_INDEX(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
		return LLusb_ReadRegister(FERS_INDEX(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL)
		return LLtdl_ReadRegister(FERS_CNCINDEX(handle), FERS_CHAIN(handle), FERS_NODE(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_CNC)
		return LLtdl_CncReadRegister(FERS_CNCINDEX(handle), address, data);
	else 
		return FERSLIB_ERR_INVALID_HANDLE;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of the board
// Inputs:		handle = device handle
//				address = reg address 
//				data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteRegister(int handle, uint32_t address, uint32_t data) {
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH)
		return LLeth_WriteRegister(FERS_INDEX(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
		return LLusb_WriteRegister(FERS_INDEX(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL)
		return LLtdl_WriteRegister(FERS_CNCINDEX(handle), FERS_CHAIN(handle), FERS_NODE(handle), address, data);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_CNC)
		return LLtdl_CncWriteRegister(FERS_CNCINDEX(handle), address, data);
	else 
		return FERSLIB_ERR_INVALID_HANDLE;
}

int FERS_MultiWriteRegister(int handle, uint32_t* address, uint32_t* data, int ncycle) {
	return LLtdl_MultiWriteRegister(FERS_CNCINDEX(handle), FERS_CHAIN(handle), FERS_NODE(handle), address, data, ncycle);
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Send a command to the board
// Inputs:		handle = device handle
//				cmd = command opcode
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_SendCommand(int handle, uint32_t cmd) {
	if (cmd == CMD_ACQ_START) FERS_ReadoutStatus = ROSTATUS_RUNNING;
	if (cmd == CMD_ACQ_STOP) FERS_ReadoutStatus = ROSTATUS_IDLE;
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL)
		return (LLtdl_SendCommand(FERS_CNCINDEX(handle), FERS_CHAIN(handle), FERS_NODE(handle), cmd, TDL_COMMAND_DELAY));
	else
		return (FERS_WriteRegister(handle, a_commands, cmd));
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Send a broadcast command to multiple boards connected to a concentrator
// Inputs:		handle = device handles of all boards that should receive the command
//				cmd = command opcode
//				delay = execution delay (0 for automatic). 
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_SendCommandBroadcast(int *handle, uint32_t cmd, uint32_t delay) {
	if (cmd == CMD_ACQ_START) FERS_ReadoutStatus = ROSTATUS_RUNNING;
	if (cmd == CMD_ACQ_STOP) FERS_ReadoutStatus = ROSTATUS_IDLE;
	if (FERS_CONNECTIONTYPE(*handle) == FERS_CONNECTIONTYPE_TDL) {
		if (delay == 0) delay = TDL_COMMAND_DELAY;  // CTIN: manage auto delay mode (the minimum depends on the num of boards in the TDL chain)
		return (LLtdl_SendCommandBroadcast(FERS_CNCINDEX(handle[0]), cmd, delay));  // CTIN: manage multiple concentrators
	}
	else return FERSLIB_ERR_NOT_APPLICABLE;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a slice of a register 
// Inputs:		handle = device handle
//				address = reg address 
//				start_bit = fisrt bit of the slice (included)
//				stop_bit = last bit of the slice (included)
//				data = slice data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteRegisterSlice(int handle, uint32_t address, uint32_t start_bit, uint32_t stop_bit, uint32_t data) {
	int ret=0;
	uint32_t i, reg, mask=0;

	ret |= FERS_ReadRegister(handle, address, &reg);
	for(i=start_bit; i<=stop_bit; i++)
		mask |= 1<<i;
	reg = (reg & ~mask) | ((data << start_bit) & mask);
	ret |= FERS_WriteRegister(handle, address, reg);   
	return ret;
}

// *********************************************************************************************************
// I2C Register R/W 
// *********************************************************************************************************
static int Wait_i2c_busy(int handle) 
{
	uint32_t stat;
	for (int i=0; i<50; i++) {
		FERS_ReadRegister(handle, a_acq_status, &stat); 
		if ((stat & (1 << 17)) == 0) break;
		Sleep(1);
	}
	return (stat & STATUS_I2C_FAIL);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of an I2C device (picoTDC, PLL, etc...)
// Inputs:		handle = device handle
//				i2c_dev_addr = I2C device address (7 bit)
//				reg_addr = register address (in the device)
//				reg_data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_I2C_WriteRegister(int handle, uint32_t i2c_dev_addr, uint32_t reg_addr, uint32_t reg_data) {
	int ret = 0, ack = 0;
	int a_i2c_data = 0;
	int a_i2c_addr = 0;
	uint8_t AckBit;
	uint32_t AcqStatus;

	if (FERS_IsXROC(handle)) {
		a_i2c_addr = a_i2c_addr_5202;
		a_i2c_data = a_i2c_data_5202;
	} else if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203) {
		a_i2c_addr = a_i2c_addr_5203;
		a_i2c_data = a_i2c_data_5203;
	}

	if ((i2c_dev_addr == I2C_ADDR_PLL0) || (i2c_dev_addr == I2C_ADDR_PLL1) || (i2c_dev_addr == I2C_ADDR_PLL2)) {
		ret |= FERS_WriteRegister(handle, a_i2c_data, (reg_addr >> 8) & 0xFF);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 16 | 0x01);
		ack |= Wait_i2c_busy(handle);
		ret |= FERS_WriteRegister(handle, a_i2c_data, reg_data);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 16 | (reg_addr & 0xFF));
	} else if (i2c_dev_addr == I2C_ADDR_EEPROM_MEM) {
		if (FERS_BoardInfo[FERS_INDEX(handle)]->PCBrevision <= 2) {					//PcbRev > 2 has I2C aux. bus				
			uint32_t device_address_25bit = (i2c_dev_addr & 0x7F) << 17;
			ret |= FERS_WriteRegister(handle, a_i2c_data, (reg_data << 24));
			ret |= FERS_WriteRegister(handle, a_i2c_addr, device_address_25bit | reg_addr);
		} else {
			uint32_t device_address_25bit = (1 << 24) | ((i2c_dev_addr & 0x7F) << 17);
			ret |= FERS_WriteRegister(handle, a_i2c_data, (reg_data << 24));
			ret |= FERS_WriteRegister(handle, a_i2c_addr, device_address_25bit | reg_addr);
		}
	} else if ((i2c_dev_addr == 0x10 || i2c_dev_addr == 0x0C) && FERS_BoardInfo[FERS_INDEX(handle)]->PCBrevision > 2) {
		ret |= FERS_WriteRegister(handle, a_i2c_data, reg_data);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, 1 << 24 | i2c_dev_addr << 17 | reg_addr);
	} else {
		ret |= FERS_WriteRegister(handle, a_i2c_data, reg_data);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 17 | reg_addr);
	}
	ack |= Wait_i2c_busy(handle);

	//Check ACK from slave
	ret |= FERS_ReadRegister(handle, a_acq_status, &AcqStatus);
	AckBit = (AcqStatus >> 18) & 1;

	if (AckBit | ack) {
		return FERSLIB_ERR_I2C_NACK;
	}

	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of an I2C device (picoTDC, PLL, etc...)
// Inputs:		handle = device handle
//				i2c_dev_addr = I2C device address (7 bit)
//				reg_addr = register address (in the device)
// Outputs:		reg_data = reg data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_I2C_ReadRegister(int handle, uint32_t i2c_dev_addr, uint32_t reg_addr, uint32_t* reg_data) {
	int ret = 0, ack = 0;
	int a_i2c_data = 0;
	int a_i2c_addr = 0;
	uint8_t AckBit;
	uint32_t AcqStatus;

	if (FERS_IsXROC(handle)) {
		a_i2c_addr = a_i2c_addr_5202;
		a_i2c_data = a_i2c_data_5202;
	} else if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203) {
		a_i2c_addr = a_i2c_addr_5203;
		a_i2c_data = a_i2c_data_5203;
	}

	if ((i2c_dev_addr == I2C_ADDR_PLL0) || (i2c_dev_addr == I2C_ADDR_PLL1) || (i2c_dev_addr == I2C_ADDR_PLL2)) {
		ret |= FERS_WriteRegister(handle, a_i2c_data, (reg_addr >> 8) & 0xFF);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 17 | 0x01);
		ack |= Wait_i2c_busy(handle);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 17 | 0x10000 | (reg_addr & 0xFF));
	} else if (i2c_dev_addr == I2C_ADDR_EEPROM_MEM) {
		if (FERS_BoardInfo[FERS_INDEX(handle)]->PCBrevision <= 2) {				//PcbRev > 2 has I2C aux. bus
			uint32_t device_address_25bit = ((i2c_dev_addr & 0x7F) << 17) | 1 << 16;
			ret |= FERS_WriteRegister(handle, a_i2c_addr, device_address_25bit | reg_addr);
		} else {
			uint32_t device_address_25bit = (1 << 24) | ((i2c_dev_addr & 0x7F) << 17 | 1 << 16);
			ret |= FERS_WriteRegister(handle, a_i2c_addr, device_address_25bit | reg_addr);
		}
	} else if ((i2c_dev_addr == 0x10 || i2c_dev_addr == 0x0C) && FERS_BoardInfo[FERS_INDEX(handle)] -> PCBrevision > 2) {
		ret |= FERS_WriteRegister(handle, a_i2c_data, *reg_data);
		ret |= FERS_WriteRegister(handle, a_i2c_addr, 1 << 24 | i2c_dev_addr << 17 | reg_addr);
	} else {
		ret |= FERS_WriteRegister(handle, a_i2c_addr, i2c_dev_addr << 17 | 0x10000 | reg_addr);
	}

	if (i2c_dev_addr == I2C_ADDR_EEPROM_MEM) {
		ack |= Wait_i2c_busy(handle);
		uint32_t* reg_data_shifted = (uint32_t*)malloc(sizeof(uint32_t));
		ret |= FERS_ReadRegister(handle, a_i2c_data, reg_data_shifted);
		*reg_data = *reg_data_shifted >> 24;
		free(reg_data_shifted);
	} else {
		ack |= Wait_i2c_busy(handle);
		ret |= FERS_ReadRegister(handle, a_i2c_data, reg_data);
	}

	//Check ACK from slave
	ret |= FERS_ReadRegister(handle, a_acq_status, &AcqStatus);
	AckBit = (AcqStatus >> 18) & 1;

	if (AckBit | ack) {
		return FERSLIB_ERR_I2C_NACK;
	}
	return ret;
}

// *********************************************************************************************************
// EEPROM Read/Write (M24C64)
// *********************************************************************************************************

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a data block to the EEPROM
// Inputs:		handle = board handle 
//				start_address = memory address of the 1st byte
//				size = num of bytes to write
//				data = data to write
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteEEPROMBlock(int handle, int start_address, int size, uint8_t* data) {
	int ret = 0;
	uint8_t i = 0;
	uint32_t reg_addr = 0;

	for (i = 0; i < size - 1; i++) {
		reg_addr = start_address + i;
		ret = FERS_I2C_WriteRegister(handle, I2C_ADDR_EEPROM_MEM, reg_addr, *data);
		data++;
		Sleep(5);				//M24C64 needs 5 ms to complete the write cycle safely
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a data block from the EEPROM
// Inputs:		handle = board handle 
//				start_address = memory address of the 1st byte
//				size = num of bytes to read
//				data = read data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadEEPROMBlock(int handle, int start_address, int size, uint8_t* data) {
	int ret = 0;
	uint32_t reg_addr = 0;
	uint32_t reg_data;

	for (int i = 0; i < size; i++) {
		reg_addr = start_address + i;
		ret = FERS_I2C_ReadRegister(handle, I2C_ADDR_EEPROM_MEM, reg_addr, &reg_data);
		data[i] = (uint8_t)(reg_data & 0xFF);
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write Adapter A5256 info into the relevant EEPROM memory page
// 
// Inputs:		handle = board handle 
//				binfo = board info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteA5256EEPROMInfo(int handle, FERS_A5256_Info_t binfo) {
	int ret;
	const char BICversion = 0;
	uint8_t bic[EEPROM_BIC_SIZE];

	memset(bic, 0, EEPROM_BIC_SIZE);
	bic[0] = 'B';								// identifier			(1)
	bic[1] = BICversion;						// BIC version			(1) 
	memcpy(bic + 2, &binfo.pid, 4);				// PID					(4)
	memcpy(bic + 6, &binfo.AdapterCode, 2);		// e.g. 5256			(2)
	memcpy(bic + 8, &binfo.PCBrevision, 1);		// PCB_Rev				(1)
	memcpy(bic + 9, binfo.ModelCode, 16);		// e.g. WA5256XAAAAA	(16)
	memcpy(bic + 25, binfo.ModelName, 16);		// e.g. A5256			(16)
	memcpy(bic + 41, &binfo.NumCh, 2);			// Num Channels			(2)
	ret = FERS_WriteEEPROMBlock(handle, EEPROM_BIC_PAGE, EEPROM_BIC_SIZE, bic);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read Adapter A5256 info into the relevant EEPROM memory page
// Inputs:		handle = board handle 
// Outputs:		binfo = board info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadA5256EEPROMInfo(int handle, FERS_A5256_Info_t* binfo) {
	int ret;
	uint8_t bic[EEPROM_BIC_SIZE];

	memset(binfo, 0, sizeof(FERS_A5256_Info_t));

	ret = FERS_ReadEEPROMBlock(handle, EEPROM_BIC_PAGE, EEPROM_BIC_SIZE, bic);

	if (ret < 0) return ret;
	if ((bic[0] != 'B') || (bic[1] != 0)) {
		return FERSLIB_ERR_INVALID_BIC;
	}
	memcpy(&binfo->pid, bic + 2, 4);
	memcpy(&binfo->AdapterCode, bic + 6, 2);
	memcpy(&binfo->PCBrevision, bic + 8, 1);
	memcpy(binfo->ModelCode, bic + 9, 16);
	memcpy(binfo->ModelName, bic + 25, 16);
	memcpy(&binfo->NumCh, bic + 41, 2);

	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Check A5256 presence by checking EEPROM or DAC presence
// Inputs:		handle = board handle 
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_checkA5256presence(int handle, FERS_A5256_Info_t* tinfo) {
	int ret = 0;
	uint32_t data = 0, addr = 0;

	ret = FERS_I2C_WriteRegister(handle, 0x10, addr, data);		//check presence (with ack bit) of A5256 DAC
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Write a slice of a register of an I2C device
// Inputs:		handle = device handle
//				i2c_dev_addr = I2C device address (7 bit) 
//				address = reg address 
//				start_bit = fisrt bit of the slice (included)
//				stop_bit = last bit of the slice (included)
//				data = slice data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_I2C_WriteRegisterSlice(int handle, uint32_t i2c_dev_addr, uint32_t address, uint32_t start_bit, uint32_t stop_bit, uint32_t data) {
	int ret=0;
	uint32_t i, reg, mask=0;

	ret |= FERS_I2C_ReadRegister(handle, i2c_dev_addr, address, &reg);
	for(i=start_bit; i<=stop_bit; i++)
		mask |= 1<<i;
	reg = (reg & ~mask) | ((data << start_bit) & mask);
	ret |= FERS_I2C_WriteRegister(handle, i2c_dev_addr, address, reg);   
	return ret;
}



// *********************************************************************************************************
// Flash Mem Read/Write (AT54DB321)
// *********************************************************************************************************
// Parameters for the access to the Flash Memory
#define MAIN_MEM_PAGE_READ_CMD          0xD2
#define MAIN_MEM_PAGE_PROG_TH_BUF1_CMD  0x82
#define STATUS_READ_CMD                 0x57
#define FLASH_KEEP_ENABLED				0x100 // Keep Chip Select active during SPI operation

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a page of the flash memory 
// Inputs:		handle = board handle 
//				page = page number
// Outputs		data = page data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadFlashPage(int handle, int pagenum, int size, uint8_t *data)  
{
	int i, ret = 0, nb;
	uint32_t rdata, page_addr;

	if ((size == 0) || (size > FLASH_PAGE_SIZE)) nb = FLASH_PAGE_SIZE;
	else nb = size;
	page_addr = (pagenum & 0x1FFF) << 10;

	// Enable flash and write Opcode
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | MAIN_MEM_PAGE_READ_CMD);
	// Write Page address
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | ((page_addr >> 16) & 0xFF));
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | ((page_addr >> 8)  & 0xFF));
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | (page_addr         & 0xFF));
	// additional don't care bytes
	for (i = 0; i < 4; i++) {
		ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED);
	}
	if (ret) return ret;
	Sleep(10);
	// Read Data (full page with the relevant size for that flash)
	for (i = 0; i < nb; i++) {
		if (i == (nb-1))
			ret |= FERS_WriteRegister(handle, a_spi_data, 0);
		else
			ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED);
		ret |= FERS_ReadRegister(handle, a_spi_data, &rdata);
		data[i] = (uint8_t)rdata;
		if (ret) return ret;
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Write a page of the flash memory. 
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
//				page: page number
//				data = data to write
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteFlashPage(int handle, int pagenum, int size, uint8_t *data)
{
	int i, ret = 0, nb;
	uint32_t stat, page_addr;

	if ((size == 0) || (size > FLASH_PAGE_SIZE)) nb = FLASH_PAGE_SIZE;
	else nb = size;
	page_addr = (pagenum & 0x1FFF) << 10;
	// Enable flash and write Opcode
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | MAIN_MEM_PAGE_PROG_TH_BUF1_CMD);
	// Page address
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | ((page_addr >> 16) & 0xFF));
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | ((page_addr >> 8)  & 0xFF));
	ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | ( page_addr        & 0xFF));
	if (ret)
		return ret;
	// Write Data
	for (i = 0; i < nb; i++) {
		if (i == (nb-1))
			ret |= FERS_WriteRegister(handle, a_spi_data, (uint32_t)data[i]);
		else
			ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | (uint32_t)data[i]);
		if (ret)
			return ret;
	}

	// wait for Tep (Time of erase and programming)
	do {
		ret |= FERS_WriteRegister(handle, a_spi_data, FLASH_KEEP_ENABLED | STATUS_READ_CMD); // Status read Command
		ret |= FERS_WriteRegister(handle, a_spi_data, 0);
		ret |= FERS_ReadRegister(handle, a_spi_data, &stat); // Status read
		if (ret)
			return ret;
	} while (!(stat & 0x80));
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write Board info into the relevant flash memory page
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
//				binfo = board info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteBoardInfo(int handle, FERS_BoardInfo_t binfo)
{
	int ret;
	const char BICversion = 0;
	const int PedCalibPage = FLASH_PEDCALIB_PAGE;
	uint8_t bic[FLASH_PAGE_SIZE];

	memset(bic, 0, FLASH_PAGE_SIZE);
	bic[0] = 'B';								// identifier			(1)
	bic[1] = BICversion;						// BIC version			(1) 
	memcpy(bic + 2, &binfo.pid, 4);				// PID					(4)
	memcpy(bic + 6, &binfo.FERSCode, 2);		// e.g. 5202			(2)
	memcpy(bic + 8, &binfo.PCBrevision, 1);		// PCB_Rev				(1)
	memcpy(bic + 9, binfo.ModelCode, 16);		// e.g. WA5202XAAAAA	(16)
	memcpy(bic + 25, binfo.ModelName, 16);		// e.g. A5202			(16)
	memcpy(bic + 41, &binfo.FormFactor, 1);		// 0=naked, 1=boxed		(1)
	memcpy(bic + 42, &binfo.NumCh, 2);			// Num Channels			(2)
	memcpy(bic + 44, &PedCalibPage, 2);			// Pedestal Calib page	(2)  (pedestal in A5202, threshold offset in A5203)
	ret = FERS_WriteFlashPage(handle, FLASH_BIC_PAGE, 46, bic);
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Set Concentrator info from offline RawData file
// Inputs:		handle = cnc handle 
// 				cinfo = concentrator info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int FERS_SetConcentratorInfo(int handle, FERS_CncInfo_t* cinfo)
{
	if (FERS_CncInfo == NULL) {
		FERS_CncInfo[FERS_CNCINDEX(handle)] = (FERS_CncInfo_t*)malloc(sizeof(FERS_CncInfo_t));
		memcpy(FERS_CncInfo[FERS_CNCINDEX(handle)], cinfo, sizeof(FERS_CncInfo_t));
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Write Concentrator info 
// Inputs:		handle = cnc handle 
// 				cinfo = concentrator info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
// CTIN: to do....
/*int FERS_WriteConcentratorInfo(int handle, FERS_CncInfo_t cinfo)
{
	return 0;
}*/

// --------------------------------------------------------------------------------------------------------- 
// Description: Read Concentrator info 
// Inputs:		handle = cnc handle 
// Outputs:		cinfo = concentrator info struct
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadConcentratorInfo(int handle, FERS_CncInfo_t* cinfo)
{
	int ret;
	uint16_t i;
	ret = LLtdl_GetCncInfo(FERS_CNCINDEX(handle), cinfo);
	if (ret) return ret;
	// CTIN: the following info are hard-coded for the moment. Will be read from the concentrator...
	//strcpy(cinfo->ModelCode, "WDT5215XAAAA");
	//strcpy(cinfo->ModelName, "DT5215");
	//cinfo->PCBrevision = 1;
	//cinfo->NumLink = 8;
	for (i = 0; i < 8; i++) {
		ret = LLtdl_GetChainInfo(FERS_CNCINDEX(handle), i, &cinfo->ChainInfo[i]);
	}

	if (FERS_CncInfo[FERS_CNCINDEX(handle)] == NULL) {
		FERS_CncInfo[FERS_CNCINDEX(handle)] = (FERS_CncInfo_t*)malloc(sizeof(FERS_CncInfo_t));
		FERS_TotalAllocatedMem += sizeof(FERS_CncInfo_t);
	}
	memcpy(FERS_CncInfo[FERS_CNCINDEX(handle)], cinfo, sizeof(FERS_CncInfo_t));

	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get FPGA Die Temperature
// Inputs:		handle = board handle 
// Outputs:     temp = FPGA temperature (Celsius)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Get_FPGA_Temp(int handle, float *temp) {
	uint32_t data;
	int i, ret;
	for (i = 0; i < 5; i++) {
		ret = FERS_ReadRegister(handle, a_fpga_temp, &data);
		*temp = (float)(((data * 503.975) / 4096) - 273.15);
		if ((*temp > 0) && (*temp < 125)) break;
	}
	if (i == 5) *temp = INVALID_TEMP;
	return ret;
}



// --------------------------------------------------------------------------------------------------------- 
// Description: Get board Temperature (between PIC and FPGA)
// Inputs:		handle = board handle 
// Outputs:     temp = PIC board temperature (Celsius)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Get_Board_Temp(int handle, float* temp) {
	uint32_t data;
	int i, ret;
	for (i = 0; i < 5; i++) {
		ret = FERS_ReadRegister(handle, a_board_temp, &data);
		*temp = (float)(data / 4.);
		if ((*temp > -20) && (*temp < 125)) break;
	}
	if (i == 5) *temp = INVALID_TEMP;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get TDC0 Temperature
// Inputs:		handle = board handle 
// Outputs:     temp = TDC temperature (Celsius)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Get_TDC0_Temp(int handle, float* temp) {
	if (FERS_IsXROC(handle)) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t data;
	int ret;
	for (int i = 0; i < 5; i++) {
		ret = FERS_ReadRegister(handle, a_tdc0_temp, &data);
		*temp = (float)(data / 4.);
		if ((*temp > 0) && (*temp < 125)) break;
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get TDC1 Temperature
// Inputs:		handle = board handle 
// Outputs:     temp = TDC temperature (Celsius)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_Get_TDC1_Temp(int handle, float* temp) {
	if (FERS_IsXROC(handle)) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t data;
	int ret;
	for (int i = 0; i < 5; i++) {
		ret = FERS_ReadRegister(handle, a_tdc1_temp, &data);
		*temp = (float)(data / 4.);
		if ((*temp > 0) && (*temp < 125)) break;
	}
	return ret;
}



// --------------------------------------------------------------------------------------------------------- 
// Description: Write Pedestal calibration
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
//				PedLG, PedHG = pedestals (64+64 values)
//				dco = DCossfet (use NULL pointer to keep old values)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WritePedestals(int handle, uint16_t *PedLG, uint16_t *PedHG, uint16_t *dco)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret, sz = 64*sizeof(uint16_t);
	uint8_t ped[FLASH_PAGE_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int pedpage = PedestalBackupPage[FERS_INDEX(handle)] ? FLASH_PEDCALIB_BCK_PAGE : FLASH_PEDCALIB_PAGE;

	ret = FERS_ReadFlashPage(handle, pedpage, 16 + sz*2, ped);
	if (ret < 0) return ret;
	ped[0] = 'P';	// Tag
	ped[1] = 0;		// Format
	*(uint16_t *)(ped+2) = (uint16_t)(tm.tm_year + 1900);
	ped[4] = (uint8_t)(tm.tm_mon + 1);
	ped[5] = (uint8_t)(tm.tm_mday);
	if (dco != NULL) memcpy(ped+6, dco, 4 * sizeof(uint16_t));  // 0=LG0, 1=HG0, 2=LG1, 3=HG1
	// Note: 10 to 15 are unused bytes (spares). Calib data start from 16
	memcpy(ped+16, PedLG, sz);
	memcpy(ped+16+sz, PedHG, sz);
	ret = FERS_WriteFlashPage(handle, pedpage, 16 + sz*2, ped);

	// Update local pedestals and DC offset (used in the library)
	memcpy(PedestalLG, PedLG, sz);
	memcpy(PedestalHG, PedHG, sz);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read Pedestal calibration and DC offset
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
// Outputs:		date = calibration date	(DD/MM/YYYY)
//				PedLG, PedHG = pedestals (64+64 values)
//				dco = DCoffset (DAC). 4 values. Use NULL pointer if not requested
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadPedestalsFromFlash(int handle, char *date, uint16_t *PedLG, uint16_t *PedHG, uint16_t *dco)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret, sz = 64*sizeof(uint16_t);
	int year, month, day;
	uint8_t ped[FLASH_PAGE_SIZE];
	int pedpage = PedestalBackupPage[FERS_INDEX(handle)] ? FLASH_PEDCALIB_BCK_PAGE : FLASH_PEDCALIB_PAGE;

	if (date != NULL) strcpy(date, "");
	ret = FERS_ReadFlashPage(handle, pedpage, 16 + sz*2, ped);
	if ((ped[0] != 'P') || (ped[1] != 0)) {
		EnablePedCal= 0;
		return FERSLIB_ERR_PEDCALIB_NOT_FOUND;
	}
	year = *(uint16_t *)(ped+2);
	month = ped[4];
	day = ped[5];
	if (date != NULL) sprintf(date, "%02d/%02d/%04d", day, month, year);
	if (dco != NULL) memcpy(dco, ped+6, 4 * sizeof(uint16_t));
	memcpy(PedLG, ped+16, sz);
	memcpy(PedHG, ped+16+sz, sz);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Switch to pedesatl backup page
// Inputs:		handle = board handle 
//				EnBckPage: 0=normal page, 1=packup page
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_PedestalBackupPage(int handle, int EnBckPage)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	PedestalBackupPage[FERS_INDEX(handle)] = EnBckPage;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set a common pedestal (applied to all channels after pedestal calibration)
//				WARNING: this function enables pedestal calibration
// Inputs:		handle = board handle 
//				Ped = fixed pedestals 
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_SetCommonPedestal(int handle, uint16_t Pedestal)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	CommonPedestal = Pedestal;
	EnablePedCal = 1;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Enable/Disable pedestal calibration
// Inputs:		handle = board handle 
//				enable = 0: disabled, 1=enabled
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_EnablePedestalCalibration(int handle, int enable)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	EnablePedCal = enable;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get Channel pedestal
// Inputs:		handle = board handle 
//				enable = 0: disabled, 1=enabled
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_GetChannelPedestalBeforeCalib(int handle, int ch, uint16_t *PedLG, uint16_t *PedHG)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	if ((ch < 0) || (ch > 63)) return FERSLIB_ERR_INVALID_PARAM;
	*PedLG = PedestalLG[FERS_INDEX(handle)][ch];
	*PedHG = PedestalHG[FERS_INDEX(handle)][ch];
	return 0;
}

// *********************************************************************************************************
// ASIC Register R/W
// *********************************************************************************************************

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of the XROC ASCI chip via I2C
// Inputs:		handle = board handle 
//				page_addr = page address (identifies the register group)
//				sub_addr = sub address (identifies the register in the page)
//				data = data to write
// Return:		0=OK, negative number = error code (missing ACK on I2C will be reported with a specific error code)
// --------------------------------------------------------------------------------------------------------- 
int FERS_XROC_WriteRegister(int handle, int page_addr, int sub_addr, uint8_t data)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = FERS_I2C_WriteRegister(handle, I2C_ADDR_XR, ((page_addr & 0xFF) << 8) | sub_addr & 0xFF, (int)data);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of the XROC ASCI chip via I2C
// Inputs:		handle = board handle 
//				page_addr = page address (identifies the register group)
//				sub_addr = sub address (identifies the register in the page)
//				data = read data 
// Return:		0=OK, negative number = error code (missing ACK on I2C will be reported with a specific error code)
// --------------------------------------------------------------------------------------------------------- 
int FERS_XROC_ReadRegister(int handle, int page_addr, int sub_addr, uint8_t *data)
{
	uint32_t d32;
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = FERS_I2C_ReadRegister(handle, I2C_ADDR_XR, ((page_addr & 0xFF) << 8) | sub_addr & 0xFF, &d32);
	*data = (uint8_t)d32;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register slice of the XROC ASCI chip via I2C
// Inputs:		handle = board handle 
//				page_addr = page address (identifies the register group)
//				sub_addr = sub address (identifies the register in the page)
//				start_bit = first bit of the slice (included)
//				stop_bit = last bit of the slice (included)
//				data = data to write
// Return:		0=OK, negative number = error code (missing ACK on I2C will be reported with a specific error code)
// --------------------------------------------------------------------------------------------------------- 
int FERS_XROC_WriteRegisterSilce(int handle, int page_addr, int sub_addr, uint32_t start_bit, uint32_t stop_bit, uint8_t data)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = FERS_I2C_WriteRegisterSlice(handle, I2C_ADDR_XR, ((page_addr & 0xFF) << 8) | sub_addr & 0xFF, start_bit, stop_bit, (int)data);
	return ret;
}


// *********************************************************************************************************
// High Voltage Control
// *********************************************************************************************************

// --------------------------------------------------------------------------------------------------------- 
// Description: Initialize the HV communication bus (I2C)
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Init(int handle)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret=0;
	// Set Digital Control
	ret |= FERS_WriteRegister(handle, a_hv_regaddr, 0x02001);  
	Wait_i2c_busy(handle);
	ret |= FERS_WriteRegister(handle, a_hv_regdata, 0);  
	Wait_i2c_busy(handle);
	HVinit[FERS_INDEX(handle)] = 1;

	// Set PID = 1 (for more precision)
	FERS_HV_WriteReg(handle, 30, 2, 1);

	// Set Ramp Speed = 10 V/s
	//HV_WriteReg(handle, 3, 1, 50000);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of the HV 
// Inputs:		handle = device handle
//				reg_addr = register address
//				dtype = data type (0=signed int, 1=fixed point (Val*10000), 2=unsigned int, 3=float)
//				reg_data = register data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_WriteReg(int handle, uint32_t reg_addr, uint32_t dtype, uint32_t reg_data)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret=0;
	if (!HVinit[FERS_INDEX(handle)]) FERS_HV_Init(handle);
	ret |= FERS_WriteRegister(handle, a_hv_regaddr, 0x00000 | (dtype << 8) | reg_addr);  
	Wait_i2c_busy(handle);
	ret |= FERS_WriteRegister(handle, a_hv_regdata, reg_data);  
	Wait_i2c_busy(handle);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of the HV 
// Inputs:		handle = device handle
//				reg_addr = register address
//				dtype = data type (0=signed int, 1=fixed point (Val*10000), 2=unsigned int, 3=float)
// Outputs:		reg_data = register data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_ReadReg(int handle, uint32_t reg_addr, uint32_t dtype, uint32_t *reg_data)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret=0;
	if (!HVinit[FERS_INDEX(handle)]) FERS_HV_Init(handle);
	for(int i=0; i<5; i++) {
		ret |= FERS_WriteRegister(handle, a_hv_regaddr, 0x10000 | (dtype << 8) | reg_addr);  
		Wait_i2c_busy(handle);
		ret |= FERS_ReadRegister(handle, a_hv_regdata, reg_data);
		Wait_i2c_busy(handle);
		if (*reg_data != 0xFFFFFFFF) break;  // sometimes, the I2C access fails and returns 0xFFFFFFFF. In this case, make another read
	}
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Switch HV output on or off
// Inputs:		handle = device handle
//				OnOff = 0:OFF, 1:ON
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Set_OnOff(int handle, int OnOff)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	return FERS_HV_WriteReg(handle, 0, 2, OnOff);
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get HV Status
// Inputs:		handle = device handle
// Outputs:		OnOff = 0:OFF, 1:ON
//				Ramping = HV is rumping up
//				OvC = over current
//				OvV = over voltage
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_Status(int handle, int *OnOff, int *Ramping, int *OvC, int *OvV)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int i, ret = 0;
	uint32_t d32;
	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		ret = FERS_ReadRegister(handle, a_hv_status, &d32);
		*OnOff =   (d32 >> 26) & 0x1;
		*Ramping = (d32 >> 27) & 0x1;
		*OvC     = (d32 >> 28) & 0x1;
		*OvV     = (d32 >> 29) & 0x1;
	} else {
		for (i=0; i<5; i++) {
			ret |= FERS_HV_ReadReg(handle, 0, 2, (uint32_t *)OnOff);
			if ((*OnOff == 0) || (*OnOff == 1)) break;
		}
		ret |= FERS_HV_ReadReg(handle, 32, 2, (uint32_t *)Ramping);
		ret |= FERS_HV_ReadReg(handle, 250, 2, (uint32_t *)OvC);
		ret |= FERS_HV_ReadReg(handle, 249, 2, (uint32_t *)OvV);
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get HV Serial Number
// Inputs:		handle = device handle
// Outputs:		sernum = serial number
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_SerNum(int handle, int *sernum)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = 0;
	ret |= FERS_HV_ReadReg(handle, 254, 2, (uint32_t*)sernum);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set HV output voltage (bias)
// Inputs:		handle = device handle
//				vbias = output voltage in Volts
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Set_Vbias(int handle, float vbias)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	return FERS_HV_WriteReg(handle, 2, 1, (uint32_t)(vbias * 10000));
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get HV output voltage setting
// Inputs:		handle = device handle
// Outputs:		vbias = output voltage setting in Volts (read from register)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_Vbias(int handle, float *vbias)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret;
	ret = FERS_HV_ReadReg(handle, 2, 1, &d32);
	*vbias = (float)d32 / 10000;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get HV output voltage (HV read back with internal ADC)
// Inputs:		handle = device handle
// Outputs:		vmon = output voltage in Volts (read-back)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_Vmon(int handle, float *vmon)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret = 0;
	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		ret = FERS_ReadRegister(handle, a_hv_Vmon, &d32);
	} else {
		ret = FERS_HV_ReadReg(handle, 231, 1, &d32);
	}
	*vmon = (float)d32 / 10000;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set maximum output current 
// Inputs:		handle = device handle
//				imax = max output current in mA
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Set_Imax(int handle, float imax)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	return FERS_HV_WriteReg(handle, 5, 1, (uint32_t)(imax * 10000));
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get maximum output current setting
// Inputs:		handle = device handle
// Outputs:		imax = max output current in mA (read from register)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_Imax(int handle, float *imax)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret = 0;
	ret = FERS_HV_ReadReg(handle, 5, 1, &d32);
	*imax = (float)d32 / 10000;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get output current flowing into the detector
// Inputs:		handle = device handle
// Outputs:		imon = detector current
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_Imon(int handle, float *imon)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret = 0;
	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		ret = FERS_ReadRegister(handle, a_hv_Imon, &d32);
	} else {
		ret = FERS_HV_ReadReg(handle, 232, 1, &d32);
	}
	*imon = (d32>>31) ? 0 : (float)d32 / 10000;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get internal temperature of the HV module
// Inputs:		handle = device handle
// Outputs:		temp = temperature (degC)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_IntTemp(int handle, float *temp)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret = 0;
	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		ret = FERS_ReadRegister(handle, a_hv_status, &d32);
		*temp = (float)((d32 >> 13) & 0x1FFF) * 256 / 10000;
	} else {
		ret = FERS_HV_ReadReg(handle, 228, 1, &d32);
		*temp = (float)(d32 & 0x1FFFFF)/10000;
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Get external temperature (of the detector). Temp sensor must be connected to dedicated lines
// Inputs:		handle = device handle
// Outputs:		temp = detector temperature (degC)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Get_DetectorTemp(int handle, float *temp)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	uint32_t d32;
	int ret = 0;
	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		ret = FERS_ReadRegister(handle, a_hv_status, &d32);
		*temp = (float)(d32 & 0x1FFF) * 256 / 10000;
	} else {
		ret = FERS_HV_ReadReg(handle, 234, 1, &d32);
		*temp = (float)(d32 & 0x1FFFFF)/10000;
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set coefficients for external temperature sensor. T = V*V*c2 + V*c1 + c0
// Inputs:		Tsens_coeff = coefficients (0=offset, 1=linear, 2=quadratic)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Set_Tsens_Coeff(int handle, float Tsens_coeff[3])
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = 0;
	for(int i=0; i<2; i++) {
		ret |= FERS_HV_WriteReg(handle, 7, 1, (uint32_t)(Tsens_coeff[2] * 10000));
		ret |= FERS_HV_WriteReg(handle, 8, 1, (uint32_t)(Tsens_coeff[1] * 10000));
		ret |= FERS_HV_WriteReg(handle, 9, 1, (uint32_t)(Tsens_coeff[0] * 10000));
	}
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set coefficients for Vbias temperature feedback 
// Inputs:		Tsens_coeff = coefficient (mV/C)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_HV_Set_TempFeedback(int handle, int enable, float Tsens_coeff)
{
	if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == (uint16_t)5203) return FERSLIB_ERR_NOT_APPLICABLE;
	int ret = 0;
	for(int i=0; i<2; i++) {
		ret |= FERS_HV_WriteReg(handle, 28, 1, (uint32_t)(-Tsens_coeff * 10000));
		if (enable) ret |= FERS_HV_WriteReg(handle, 1, 0, 2);
		else ret |= FERS_HV_WriteReg(handle, 1, 0, 0);
	}
	return ret;
}


// *********************************************************************************************************
// TDC Config and readout (for test)
// *********************************************************************************************************
uint32_t calib_periods = 10; 
// --------------------------------------------------------------------------------------------------------- 
// Description: Write a register of the TDC
// Inputs:		handle = device handle
//				addr = register address
//				data = register data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_TDC_WriteReg(int handle, int tdc_id, uint32_t addr, uint32_t data)
{
	if (addr <= 0x09)
		return FERS_WriteRegister(handle, a_tdc_data, ((tdc_id & 1) << 24) | 0x4000 | ((addr & 0xFF) << 8) | (data & 0xFF));
	else 
		return FERSLIB_ERR_INVALID_PARAM;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read a register of the TDC
// Inputs:		handle = device handle
//				addr = register address
// Outputs:		data = register data
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_TDC_ReadReg(int handle, int tdc_id, uint32_t addr, uint32_t *data)
{
	int ret = 0;
	int size_bit = (addr <= 0x09) ? 0 : 1;  // 0 = 8 bits, 1 = 24 bits
	FERS_WriteRegister(handle, a_tdc_data, (size_bit << 25) | ((tdc_id & 1) << 24) | ((addr & 0xFF) << 8));
	ret |= FERS_ReadRegister(handle, a_tdc_data, data);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Configure TDC
// Inputs:		handle = device handle
//				tdc_id = TDC index (0 or 1)
//				StartSrc = start source (0=bunch_trg, 1=t0_in, 2=t1_in, 3=T-OR, 4=ptrg)
//				StopSrc = stop source (0=bunch_trg, 1=t0_in, 2=t1_in, 3=T-OR, 4=delayed ptrg)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_TDC_Config(int handle, int tdc_id, int StartSrc, int StopSrc)
{
	int ret = 0;
	int meas_mode = 0;		// 0=Mode1, 1=Mode2
	int startEdge = 0;		// 0=rising, 1=falling
	int stopEdge = 0;		// 0=rising, 1=falling
	int num_stops = 1;		// num of stops (1 to 5)
	int cal_mode = 1;		// 0=2 periods, 1=10 periods, 2=20 periods, 3=40 periods

	FERS_WriteRegister(handle, a_tdc_mode, ((StopSrc << 4) | StartSrc) << tdc_id * 8);
	calib_periods = (cal_mode == 0) ? 2 : cal_mode * 10;
	ret |= FERS_TDC_WriteReg(handle, tdc_id, 0x00, (stopEdge << 4) | (startEdge << 3) | (meas_mode << 1)); // Config1
	ret |= FERS_TDC_WriteReg(handle, tdc_id, 0x01, (cal_mode << 6) | num_stops); // Config2
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Make a start-stop measurements
// Inputs:		handle = device handle
//				tdc_id = TDC index (0 or 1)
// Outputs:		tof_ns = measured start-stop time (ns)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_TDC_DoStartStopMeasurement(int handle, int tdc_id, double *tof_ns)
{
	int ret = 0, i;
	double calCount, ClockPeriod = 125;  // 8 MHz, T = 125 ns
	uint32_t calib1, calib2, tmeas, int_status;

	ret |= FERS_TDC_WriteReg(handle, tdc_id, 0x00, 0x01); // start measurement (mode = 0)
	for (i=0; i<10; i++) {
		ret |= FERS_TDC_ReadReg(handle, tdc_id, 0x02, &int_status);
		if (int_status & 0x1) break;
	} 
	if (ret) return ret;
	if (!(int_status & 0x1)) {
		*tof_ns = 0;
		return 0;
	}
	ret |= FERS_TDC_WriteReg(handle, tdc_id, 0x02, 0x01); // clear interrupt
	ret |= FERS_TDC_ReadReg(handle, tdc_id, 0x1B, &calib1);  
	ret |= FERS_TDC_ReadReg(handle, tdc_id, 0x1C, &calib2);  
	ret |= FERS_TDC_ReadReg(handle, tdc_id, 0x10, &tmeas);  
	calCount = (double)(calib2 - calib1) / (calib_periods - 1);
	*tof_ns = (double)tmeas * ClockPeriod / calCount;
	return ret;
}


// *********************************************************
// Firmware Upgrade
// *********************************************************

static int waitFlashfree(int handle)
{
	uint32_t reg_add;
	uint32_t status;
	int8_t tout = -1;
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL)
		reg_add = FUP_BA + FUP_RESULT_REG;
	else
		reg_add = 8189; 
	do {
		//Inserire qui un timeout a 5 secondi
		if (tout >= 0)
			Sleep(1);
		FERS_ReadRegister(handle, reg_add, &status);
		++tout;
	} while (status != 1 && tout < 300);
	if (tout == 300)
		return FERSLIB_ERR_UPGRADE_ERROR;
	return 0;
}

static int WriteFPGAFirmwareOnFlash(int handle, char *pageText, int textLen, void(*ptr)(char *msg, int progress))
{
	//uint32_t temp = 0;
	uint32_t* datarow; // [8192] ;
	//uint32_t rw = 0;
	//uint32_t rd = 0;
	uint32_t *i_pageText;
	const uint32_t chunk_size_byte = 60 * 512 ;
	const uint32_t n_chunks = (uint32_t)ceil((double)textLen / chunk_size_byte);
	int ret = 0;

	char sBrd[8];
	sprintf(sBrd, "%d", FERS_INDEX(handle));

	datarow = (uint32_t*)calloc(8192, sizeof(uint32_t));

	for (int cnk = 0; cnk < (int)n_chunks; cnk++) {
		i_pageText = (uint32_t*)&(pageText[cnk*chunk_size_byte]);
		for (int i = 0; i < 8192; i++)
			datarow[i] = 0;

		for (int i = 0; i < (int)(chunk_size_byte/4); i++)
			datarow[i] = i_pageText[i];
	
		uint32_t cmd = 0xA;
		datarow[8190] = cnk*chunk_size_byte;
		datarow[8191] = cmd;
		const uint16_t ChunkSize = 64;
		for (int i = 0; i < 8192; i += ChunkSize) {
			if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH)
				ret |= LLeth_WriteMem(FERS_INDEX(handle), i, (char *)(&datarow[i]), ChunkSize * 4);
			else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
				ret |= LLusb_WriteMem(FERS_INDEX(handle), i, (char *)(&datarow[i]), ChunkSize * 4);
		}
		ret |= waitFlashfree(handle);
		if (ret < 0) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade: timeout in waitFlashFree function\n");
			char err[1024];
			sprintf(err, "ERROR: failed firmware upgrade: timeout in waitFlashFree function\n");
			(*ptr)(err, -1);
			return ret;
		}

		if (ptr != NULL) {
			(*ptr)(sBrd, cnk * 100 / n_chunks);
		}
	}
	(*ptr)(sBrd, 100);
	
	free(datarow);
	return 0;
}


uint32_t crc32c(uint32_t crc, const unsigned char* buf, size_t len)
{
	int k;

	crc = ~crc;
	while (len--) {
		crc ^= *buf++;
		for (k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}


static int FUP_WriteFPGAFirmwareOnFlash(int handle, char* pageText, int textLen, void(*ptr)(char* msg, int progress))
{
	uint32_t temp = 0;
	uint32_t datarow[1024];
	uint32_t addrow[1024];
	//uint32_t rw = 0;
	//uint32_t rd = 0;
	uint32_t* i_pageText;
	const uint32_t chunk_size_byte = 4 * 512;
	const uint32_t n_chunks = (uint32_t)ceil((double)textLen / chunk_size_byte);
	uint32_t crc_R, crc_T;
	int onepercent = n_chunks / 100;
	int next_p = 0;
	char sBrd[8];
	sprintf(sBrd, "%d", FERS_INDEX(handle));
	//for (int cnk = (int)n_chunks - 1; cnk >=0; --cnk) {
	for (int cnk = 0; cnk < (int)n_chunks; cnk++) {
		i_pageText = (uint32_t*)&(pageText[cnk * chunk_size_byte]);
		for (int i = 0; i < 1024; i++)
			datarow[i] = 0;

		for (int i = 0; i < (int)(chunk_size_byte / 4); i++)
			datarow[i] = i_pageText[i];

		for (int i = 0; i < 512; i ++) {
			addrow[i] = FUP_BA + i;
		}

		crc_R = 0;
		int cnt = 0, cnt1 = 0;
		do {
			FERS_MultiWriteRegister(handle, addrow, datarow, 512);

			crc_T = crc32c(0, (unsigned char*)i_pageText, 512 * 4);
			crc_R = 0;
			// Ask for CRC 
			cnt1 = 0;
			do {
				FERS_WriteRegister(handle, FUP_BA + FUP_CONTROL_REG, 0x1F);
				int cnt3 = 0;
				do {
					Sleep(5);
					FERS_ReadRegister(handle, FUP_BA + FUP_CONTROL_REG, &temp);
					++cnt3;
				} while ((cnt3 < 20) && (temp == 0x1F));

				++cnt1;
			} while ((temp == 0x1F) && (cnt1 < 100));
			if (cnt1 == 100) {
				if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade via TDL: cannot read CRC Rx\n");
				char err[1024];
				sprintf(err, "ERROR: failed firmware upgrade via TDL: cannot read CRC Rx\n");
				(*ptr)(err, -1);
				return FERSLIB_ERR_UPGRADE_ERROR;
			}
			// rileggo il crc
			FERS_ReadRegister(handle, FUP_BA + FUP_RESULT_REG, &crc_R);
			++cnt;
			if (crc_R != crc_T) {
				printf("*** CRC TX error: %08X %08X\n", crc_R, crc_T);
			}
		} while ((crc_R != crc_T) && (cnt < 20)); // ripeto la lettura del CRC (10 volte) finch\E9 non \E8 uguale a quello calcolato per essere sicura che il pacchetto \E8 tutto nella memoria del uBlaze
		if (cnt == 20) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade via TDL: CRC TX (%08X) and RX (%08X) does not match\n", crc_T, crc_R);
			char err[1024];
			sprintf(err, "ERROR: failed firmware upgrade via TDL: CRC TX (%08X) and RX (%08X) does not match\n", crc_T, crc_R);
			(*ptr)(err, -1);
			return FERSLIB_ERR_UPGRADE_ERROR;
		}
		cnt = -1;
		do {
			FERS_WriteRegister(handle, FUP_BA + FUP_PARAM_REG, cnk * chunk_size_byte);
			FERS_ReadRegister(handle, FUP_BA + FUP_PARAM_REG, &temp);
			++cnt;
			//printf("Read size attempt %d\n", cnt);
		} while ((temp != cnk * chunk_size_byte) && (cnt < 10));
		if (cnt == 10) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade via TDL: cannot write the correct firmware size in uBlaze\n");
			char err[1024];
			sprintf(err, "ERROR: failed firmware upgrade via TDL: cannot write the correct firmware size in uBlaze\n");
			(*ptr)(err, 0);
			return FERSLIB_ERR_UPGRADE_ERROR;
		}
		FERS_WriteRegister(handle, FUP_BA + FUP_CONTROL_REG, 0xA);  // comando la scrittura della pagina nella flash

		int ret = waitFlashfree(handle);
		if (ret < 0) {
			if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade: timeout in waitFlashFree function\n");
			char err[1024];
			sprintf(err, "ERROR: failed firmware upgrade: timeout in waitFlashFree function\n");
			(*ptr)(err, -1);
			return ret;
		}

		if (ptr != NULL) {
			if (cnk > next_p * onepercent) {
				(*ptr)(sBrd, cnk * 100 / n_chunks);
				++next_p;
			}
		}
	}
	(*ptr)(sBrd, 100);
	return 0;
}

static int EraseFPGAFirmwareFlash(int handle, uint32_t size_in_byte, void(*ptr)(char* msg, int progress)) {
	uint32_t reg_add0, reg_add1;
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		reg_add0 = FUP_BA + FUP_PARAM_REG;
		reg_add1 = FUP_BA + FUP_CONTROL_REG;
	} else {
		reg_add0 = 8190;
		reg_add1 = 8191;
	}
	//FERS_WriteRegister(handle, reg_add0, size_in_byte);
	int cnt = -1;
	uint32_t temp = 0;
	do {
		FERS_WriteRegister(handle, reg_add0, size_in_byte);
		FERS_ReadRegister(handle, reg_add0, &temp);
		++cnt;
		//printf("Erase attempt %d\n", cnt);
	} while ((temp != size_in_byte) && (cnt < 10));
	if (cnt == 10) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade via TDL: cannot write the correct size to erase in uBlaze\n");
		char err[1024];
		sprintf(err, "ERROR: failed firmware upgrade via TDL: cannot write the correct firmware size in uBlaze\n");
		(*ptr)(err, -1);
		return FERSLIB_ERR_UPGRADE_ERROR;
	}
	Sleep(10);
	FERS_WriteRegister(handle, reg_add1, 0x1);
	Sleep(10);
	int ret = waitFlashfree(handle);
	if (ret < 0)
		return ret;
	return 0;
}

static int RebootFromFWuploader(int handle)
{
	FERS_WriteRegister(handle, a_rebootfpga, 1);
	Sleep(10);
	return 0;
}


int FERS_FirmwareBootApplication_tdl(int *handle) {
	int ret = 0;
	uint32_t reg_add0;
	for (int i = 0; i < NumBoardConnected; ++i)
	{
		if (FERS_CONNECTIONTYPE(handle[i]) != FERS_CONNECTIONTYPE_TDL) continue;
		reg_add0 = a_rebootfpga;
		ret |= FERS_WriteRegister(handle[i], reg_add0, 0x2);
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%d] Error in setting reboot address application via tdl\n", i);
	}
	ret |= FERS_SendCommandBroadcast(handle, 0xFA, 0); // DNIN: it works only if Board 0 is tdl connected
	if (ret != 0) _setLastLocalError("ERROR: Reboot from application via TDL failed\n");

	return ret;
}


int FERS_FirmwareBootApplication_ethusb(int handle) {
	uint32_t reg_add0 = 8191;
	int ret = FERS_WriteRegister(handle, reg_add0, 0xFE);
	return ret;
}


// DISMISSED
static int FirmwareBootApplication(int handle) {
	uint32_t reg_add0;
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		reg_add0 = a_rebootfpga;
		FERS_WriteRegister(handle, reg_add0, 0x2); // set reboot address: application 
		FERS_SendCommandBroadcast(&handle, 0xFA, 0); // Reboot of all the board 	   //FERS_SendCommand(handle, 0xFA);            // reboot CMD
	} else {
		reg_add0 = 8191;
		FERS_WriteRegister(handle, reg_add0, 0xFE);
	}

	Sleep(10);
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Check if the FPGA is in "bootloader mode" and read the bootloader version
// Inputs:		handle = board handle 
// Outputs:		isInBootloader = if 1, the FPGA is in bootloader mode 
//				version = BL version
//				release = BL release
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_CheckBootloaderVersion(int handle, int *isInBootloader, uint32_t *version, uint32_t *release) 
{
	char buffer[32];
	uint32_t *p_intdata;
	int res;

	*isInBootloader = 0;
	*version = 0;
	*release = 0;

	for (int i = 0; i < 3; i++) {
		res = FERS_WriteRegister(handle, 8191, 0xFF);
		if (res) return res;
		Sleep(5);
	}
	
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) 
		res = LLeth_ReadMem(FERS_INDEX(handle), 0, buffer, 16);
	else if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
		res = LLusb_ReadMem(FERS_INDEX(handle), 0, buffer, 16);
	if (res) return res;
	p_intdata = (uint32_t *)buffer;

	if (p_intdata[0] == 0xB1FFAA1B) {
		*isInBootloader = 1;
		*version = p_intdata[1];
		*release = p_intdata[2];
	}
	return 0;
}

int FUP_CheckControllerVersion(int handle, int* isValid, uint32_t* version, uint32_t* release, void(*ptr)(char* msg, int progress))
{
	*isValid = 0;
	*version = 0;
	*release = 0;
	FERS_WriteRegister(handle, 0xFF000000 + FUP_CONTROL_REG, FUP_CMD_READ_VERSION);

	uint32_t key;
	FERS_ReadRegister(handle, 0xFF000000 + 0, &key);
	FERS_ReadRegister(handle, 0xFF000000 + 1, version);
	FERS_ReadRegister(handle, 0xFF000000 + 2, release);

	char uBlaze_version[500];
	sprintf(uBlaze_version, "uBlaze key: %x\n", key);
	(*ptr)(uBlaze_version, -1);


	if (key == 0xA1FFAA1B) {
		printf("unlock key ok\n");
		*isValid = 1;
	} else {
		printf("key not ok\n");
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Upgrade the FPGA firmware 
// Inputs:		handle = board handle 
//				fp = pointer to the configuration file being loaded
//				ptr = call back function (used for messaging during the upgrade)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_FirmwareUpgrade(int handle, char filen[200], void(*ptr)(char *msg, int progress)) //FILE *fp
{
	int isInBootloader, isValid, msize, retfsf;
	uint32_t BLversion;
	uint32_t BLrelease;
	uint32_t FUPversion, FUPrelease;
	char msg[100];
	char header[5][100]; // header reading
	uint16_t board_family[20] = { 0 };
	int valid_board = 0;
	int board_compatibility = 0;
	char *firmware;
	char b0;
	int firmware_size_byte = 0;

	FILE *fp = fopen(filen, "r");
	if (fp == NULL) {
		_setLastLocalError("Cannot open %s\n", filen);
		return FERSLIB_ERR_INVALID_FWFILE;
	}
	//if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) return FERSLIB_ERR_OPER_NOT_ALLOWED;
	// check for header
	FERS_LibMsg("[INFO][BRD %02d] Upgrading FPGA Firmware\n", FERS_INDEX(handle));

	retfsf = fscanf(fp, "%s", header[0]);
	if (strcmp(header[0], "$$$$CAEN-Spa") == 0) { // header is found in FW_File
		for (int i = 0; i < 15; ++i) {	// 15 is > 10, # of lines used in the header. not while since I didn't find a proper exit condition
			retfsf = fscanf(fp, "%s", header[0]);
			if (strcmp(header[0], "Header:") == 0)
				retfsf = fscanf(fp, "%s", header[1]);
			else if (strcmp(header[0], "Rev:") == 0)
				retfsf = fscanf(fp, "%s", header[2]);
			else if (strcmp(header[0], "Build:") == 0)
				retfsf = fscanf(fp, "%s", header[3]);
			else if (strcmp(header[0], "Board:") == 0) {
				for (int j = 0; j < 50; ++j) {
					retfsf = fscanf(fp, "%s", header[4]);
					if (strcmp(header[4], "$$$$") == 0) {
						printf("\n");
						i = 15;
						break;
					} else {
						sscanf(header[4], "%" SCNu16, &board_family[j]);
						++valid_board;
					}
				}
			}
		}
		fclose(fp);
		fp = fopen(filen, "rb");
		fseek(fp, 0x40, SEEK_CUR);
		char read_bit;
		while (!feof(fp)) {	// find the last carriage of header and return byte written in the header - last byte '0x0a'
			int fret = fread(&read_bit, sizeof(read_bit), 1, fp);
			if (read_bit == 0x24) { // Char == $, end of the header
				while (1) {
					fret = fread(&read_bit, sizeof(read_bit), 1, fp);
					if (read_bit == 0xa) // or read_bit == -1, with SEEK(fp, -1, SEEK_CUR)
						break;
				}
				break;
			}
		}
		// Check for firmware compatibility
		FERS_BoardInfo_t BInfo;
		retfsf = FERS_GetBoardInfo(handle, &BInfo);
		if (retfsf == 0) {
			// NOTE: if the firmware is corrupted, then the board info cannot be read and it is not possible to check the compatibility.
			//       Nevertheless, the upgrade cannot be skipped, otherwise it won't be possible to recover
			char tmpAllBrd[256] = "";
			for (int i = 0; i < valid_board; ++i) {
				sprintf(tmpAllBrd, "%s%" PRIu16 ",", tmpAllBrd, board_family[i]);
				if (BInfo.FERSCode == board_family[i]) {
					board_compatibility = 1;
					break;
				}
			}
			if (board_compatibility == 0) { // raise an error, the firmware is not compatible with the board
				char merr[128];
				sprintf(merr, "ERROR! The firmware version valid for FERS %s is not compatible with the model of board %d (%" PRIu16 " / %" PRIu16 ")\n", tmpAllBrd, FERS_INDEX(handle), BInfo.FERSCode, FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode);
				FERS_LibMsg("[ERROR][BRD %02d] %s", FERS_INDEX(handle), merr);
				(*ptr)(merr, -1); 
				return FERSLIB_ERR_UPGRADE_ERROR;
			}
		}
	} else
		fseek(fp, 0, SEEK_SET);	// back to the begin of the file

	FERS_LibMsg("[INFO][BRD %02d] New Firmware: Rev %s (Build %s)\n", FERS_INDEX(handle), header[2], header[3], header[4]);

	int firmware_start = ftell(fp); // in case of header, this offset point to begin of the firmware anyway (0 or byte_of_header)
	// Check 1st byte (must be -1 in Xilinx .bin files)
 	int fret = fread(&b0, 1, 1, fp);
	if (b0 != -1) {
		FERS_LibMsg("[ERROR][BRD %02d] Error: invalid firmware\n", FERS_INDEX(handle));
		return FERSLIB_ERR_INVALID_FWFILE;
	}
	//Get file length
	fseek(fp, 0, SEEK_END);
	firmware_size_byte = ftell(fp) - firmware_start; // offset in case of header
	fseek(fp, firmware_start, SEEK_SET);
	
	// Read file contents into buffer
	msize = firmware_size_byte + (8192*4);
	firmware = (char*)malloc(msize);
	if (!firmware)	return FERSLIB_ERR_UPGRADE_ERROR;
	fret = fread(firmware, firmware_size_byte, 1, fp);

	//char infoFW[1024];
	//sprintf(infoFW, "New Firmware: %s - %s for board %hu\n", firmware_size_byte, header[2], header[3], ALTTABINFO);
	//(*ptr)(infoFW, -1);

	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		FERS_FlushData(handle);

		//enable uBlaze
		FERS_WriteRegister(handle, a_rebootfpga, 0x0);
		Sleep(100);

		for (int i = 0; i < 20; i++) {
			FUP_CheckControllerVersion(handle, &isValid, &FUPversion, &FUPrelease, ptr);
			if (isValid) break;
			Sleep(100);
		}
		if (!isValid) {
			FERS_LibMsg("[ERROR][BRD %02d] ERROR: Fiber uploader not installed!\n", FERS_INDEX(handle));
			(*ptr)("ERROR: Fiber uploader not installed!\n", -1);
			return FERSLIB_ERR_UPGRADE_ERROR;
		}
		sprintf(msg, "Fiber uploader version %X, build: %8X\n", FUPversion, FUPrelease);
		(*ptr)(msg, -1);
	} else {
		// Reboot from FWloader
		(*ptr)("Reboot from Firmware loader\n", -1);
		RebootFromFWuploader(handle);
		if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
			LLusb_StreamEnable(FERS_INDEX(handle), false);
		FERS_FlushData(handle);

		for (int i = 0; i < 20; i++) {
			FERS_CheckBootloaderVersion(handle, &isInBootloader, &BLversion, &BLrelease);
			if (isInBootloader) break;
			Sleep(100);
		}
		if (!isInBootloader) {
			FERS_LibMsg("[ERROR][BRD %02d] ERROR: FW loader not installed!\n", FERS_INDEX(handle));
			(*ptr)("ERROR: FW loader not installed!\n", -1);
			return FERSLIB_ERR_UPGRADE_ERROR;
		}
		sprintf(msg, "FW loader version %X, build: %8X\n", BLversion, BLrelease);
		(*ptr)(msg, -1);
	}

	// Erase FPGA
	(*ptr)("Erasing FPGA...", -1);
	int ret = EraseFPGAFirmwareFlash(handle, firmware_size_byte, ptr);
	if (ret < 0) {
		if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[ERROR][BRD%02d] Failed firmware upgrade: timeout in waitFlashFree function\n", FERS_INDEX(handle));
		char err[1024];
		sprintf(err, "ERROR: failed firmware upgrade: timeout in waitFlashFree function\n");
		(*ptr)(err, -1);
		return ret;
	}

	// Write FW
	(*ptr)("Writing new firmware\n", -1);
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL)
		ret |= FUP_WriteFPGAFirmwareOnFlash(handle, firmware, firmware_size_byte, ptr);
	else {
		ret |= WriteFPGAFirmwareOnFlash(handle, firmware, firmware_size_byte, ptr);
		if (ret < 0) return ret;
		ret |= FERS_FirmwareBootApplication_ethusb(handle);  // FirmwareBootApplication(handle);
	}
	
	//if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB)
	//	LLusb_StreamEnable(FERS_INDEX(handle), true);

	FERS_LibMsg("[INFO][BRD %02d] Firmware upgrade completed\n", FERS_INDEX(handle));

	return 0;
}



