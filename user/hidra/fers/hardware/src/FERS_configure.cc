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

#include "FERS_MultiPlatform.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FERSlib.h"	
#include "FERS_config.h"


// **********************************************
//  CONFIGURE FERS
// **********************************************
int FERS_configure(int handle, int mode)
{
	int brd = FERS_INDEX(handle);
	int ret = 0;
	lock(FERScfg[brd]->bmutex);
	if (FERS_BoardInfo[brd]->FERSCode == 5202)
		ret |= Configure5202(handle, mode);
	if (FERS_BoardInfo[brd]->FERSCode == 5203)
		ret |= Configure5203(handle, mode);
	if (FERS_BoardInfo[brd]->FERSCode == 5204)
		ret |= Configure5204(handle, mode);
	unlock(FERScfg[brd]->bmutex);

	if (DebugLogs && DBLOG_CONFIG)
		FERS_DumpBoardRegister(handle);

	return ret;
}


int ConfigureProbe(int handle) {
	int brd = FERS_INDEX(handle);
	int ret = 0;
	if (FERS_BoardInfo[brd]->FERSCode == 5202)
		ret |= ConfigureProbe5202(handle);
	if (FERS_BoardInfo[brd]->FERSCode == 5203)
		ret |= ConfigureProbe5203(handle);
	if (FERS_BoardInfo[brd]->FERSCode == 5204)
		ret |= ConfigureProbe5204(handle);

	return ret;
}


// **********************************************
//  REGISTER DUMP
// **********************************************
int FERS_DumpBoardRegister(int handle) {
	int brd = FERS_INDEX(handle);
	int ret = 0;
	char filename[128];
	sprintf(filename, "FERSLIB_brd_registers_brd%02d_PID%d.txt", brd, FERS_BoardInfo[brd]->pid);
	if (FERS_BoardInfo[brd]->FERSCode == 5202)
		ret |= FERS_DumpBoardRegister5202(handle, filename);
	if (FERS_BoardInfo[brd]->FERSCode == 5203)
		ret |= FERS_DumpBoardRegister5203(handle, filename);
	if (FERS_BoardInfo[brd]->FERSCode == 5204)
		ret |= FERS_DumpBoardRegister5204(handle, filename);
	return ret;
}
