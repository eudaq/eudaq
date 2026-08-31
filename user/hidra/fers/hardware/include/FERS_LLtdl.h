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

// -----------------------------------------------------------------------------------
// Connect 
// -----------------------------------------------------------------------------------
int LLtdl_OpenDevice_Eth(char *board_ip_addr, int bindex);
int LLtdl_CloseDevice(int bindex);
int LLtdl_GetChainNodes(int bindex, int node_cnt[8]);

// -----------------------------------------------------------------------------------
// R/W data to REGS memory 
// -----------------------------------------------------------------------------------
//int LLtdl_WriteMem(int cindex, int chain, int node, uint32_t address, char *data, uint16_t size);
//int LLtdl_ReadMem(int cindex, int chain, int node, uint32_t address, char *data, uint16_t size);
int LLtdl_WriteRegister(int cindex, int chain, int node, uint32_t address, uint32_t data);
int LLtdl_ReadRegister(int cindex, int chain, int node, uint32_t address, uint32_t *data);
int LLtdl_SendCommand(int cindex, int chain, int node, uint32_t cmd, uint32_t delay);
int LLtdl_SendCommandBroadcast(int cindex, uint32_t cmd, uint32_t delay);

// -----------------------------------------------------------------------------------
// Read raw data
// -----------------------------------------------------------------------------------
int LLtdl_ReadData(int cindex, char *buff, int size, int *nb);
int LLtdl_ReadRawEvent(int cindex, uint32_t *RawEvent, int *chain, int *node, int *last, int *nb);
int LLtdl_FlushData(int cindex);


#endif