/******************************************************************************
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

#ifndef _FERSUTIL_H
#define _FERSUTIL_H                    // Protect against multiple inclusion

#include "FERSlib.h"
#undef max
#undef min

#define SCAN_THR_FILENAME	"ScanThr.txt"
#define SCAN_HDLY_FILENAME	"ScanHoldDelay.txt"

// simplified version of the GetParam
int FERS_GetParam_int(int handle, char *param_name);
uint32_t FERS_GetParam_uint32(int handle, char* param_name);
uint64_t FERS_GetParam_uint64(int handle, char* param_name);
uint32_t FERS_GetParam_hex(int handle, char* param_name);
uint64_t FERS_GetParam_hex64(int handle, char* param_name);
float FERS_GetParam_float(int handle, char *param_name);

// Channel to Pixel Remapping
int Read_ch2xy_Map (char *filename);
int ch2x(int ch);
int ch2y(int ch);
int xy2ch(int x, int y);
void PrintMap();

// Manual Control Panels
void HVControlPanel(int b_handle);
void CitirocControlPanel(int b_handle);
void ManualController(int b_handle);
void RegAccessTest(int *handle, int nbrd);

// Special runs
int AcquirePedestals(int handle, uint16_t *pedestalLG, uint16_t *pedestalHG);
int ScanThreshold(int handle);
int ScanHoldDelay(int handle);


#endif
