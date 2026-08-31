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

#ifndef _FERS_LLTDL_H
#define _FERS_LLTDL_H				// Protect against multiple inclusion

#include "FERSlib.h"

#include "FERS_MultiPlatform.h"
#include "FERS_Registers_520X.h"
#include "FERS_Registers_5215.h"
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "FERS_config.h"

extern mutex_t FERS_RoMutex;		// Mutex for the access to FERS_ReadoutStatus
extern f_sem_t FERS_StartRunSemaphore[FERSLIB_MAX_NBRD];	// Semaphore for sync the start of the run with the data receiver thread

// TDL fiber delay setting
#define FIBER_DELAY(length_m) (22.f + 0.781f * (length_m))  // Delay ~= 22 + 0.781 * length (in m)
#define DEFAULT_FIBER_LENGTH  (0.3f)  // default fiber length = 0.3 m


// -----------------------------------------------------------------------------------
// Connect 
// -----------------------------------------------------------------------------------
int LLtdl_OpenDevice(char *board_ip_addr, int cindex);
int LLtdl_CloseDevice(int cindex);
int LLtdl_InitTDLchains(int cindex, float DelayAdjust[FERSLIB_MAX_NTDL][FERSLIB_MAX_NNODES]);
bool LLtdl_TDLchainsInitialized(int cindex);
int LLtdl_ControlChain(int cindex, int chain, bool enable, uint32_t token_interval);
int LLtdl_GetChainInfo(int cindex, int chain, FERS_TDL_ChainInfo_t *tdl_info);

int LLeth_OpenDevice(char *board_ip_addr, int bindex);
int LLeth_CloseDevice(int bindex);

int LLusb_OpenDevice(int PID, int bindex);
int LLusb_CloseDevice(int bindex);
int LLusb_StreamEnable(int bindex, bool Enable);
int LLusb_Reset_IPaddress(int bindex);

// -----------------------------------------------------------------------------------
// R/W data to REGS memory 
// -----------------------------------------------------------------------------------
//int LLtdl_WriteMem(int cindex, int chain, int node, uint32_t address, char *data, uint16_t size);
//int LLtdl_ReadMem(int cindex, int chain, int node, uint32_t address, char *data, uint16_t size);
int LLtdl_MultiWriteRegister(int cindex, int chain, int node, uint32_t* address, uint32_t* data, int ncycles);
int LLtdl_WriteRegister(int cindex, int chain, int node, uint32_t address, uint32_t data);
int LLtdl_ReadRegister(int cindex, int chain, int node, uint32_t address, uint32_t *data);
int LLtdl_SendCommand(int cindex, int chain, int node, uint32_t cmd, uint32_t delay);
int LLtdl_SendCommandBroadcast(int cindex, uint32_t cmd, uint32_t delay);
int LLtdl_CncWriteRegister(int cindex, uint32_t address, uint32_t data);
int LLtdl_CncReadRegister(int cindex, uint32_t address, uint32_t *data);
int LLtdl_GetCncInfo(int cindex, FERS_CncInfo_t *CncInfo);

int LLeth_WriteMem(int bindex, uint32_t address, char *data, uint16_t size);
int LLeth_ReadMem(int bindex, uint32_t address, char *data, uint16_t size);
int LLeth_WriteRegister(int bindex, uint32_t address, uint32_t data);
int LLeth_ReadRegister(int bindex, uint32_t address, uint32_t *data);

int LLusb_WriteMem(int bindex, uint32_t address, char *data, uint16_t size);
int LLusb_ReadMem(int bindex, uint32_t address, char *data, uint16_t size);
int LLusb_WriteRegister(int bindex, uint32_t address, uint32_t data);
int LLusb_ReadRegister(int bindex, uint32_t address, uint32_t *data);

// -----------------------------------------------------------------------------------
// Read raw data
// -----------------------------------------------------------------------------------
int LLtdl_ReadData(int cindex, char *buff, int size, int *nb);
int LLeth_ReadData(int bindex, char *buff, int size, int *nb);
int LLusb_ReadData(int bindex, char *buff, int size, int *nb);
int LLtdl_ReadData_File(int cindex, char* buff, int size, int* nb, int flushing);
int LLeth_ReadData_File(int bindex, char* buff, int size, int* nb, int flushing);
int LLusb_ReadData_File(int bindex, char* buff, int size, int* nb, int flushing);
int LLtdl_Flush(int cindex);

// -----------------------------------------------------------------------------------
// Save raw data files
// -----------------------------------------------------------------------------------
int LLeth_OpenRawOutputFile(int handle);
int LLeth_CloseRawOutputFile(int handle);
int LLusb_OpenRawOutputFile(int handle);
int LLusb_CloseRawOutputFile(int handle);
int LLtdl_OpenRawOutputFile(int *handle);
int LLtdl_CloseRawOutputFile(int handle);

#ifdef __cplusplus
}
#endif

#endif
