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

#ifndef _OUTPUTFILES_H
#define _OUTPUTFILES_H                    // Protect against multiple inclusion

#include <stdio.h>
#include <inttypes.h>


//#include "FERSlib.h"
#include "JanusC.h"
#include "console.h"
//#include "configure.h"


int OpenOutputFiles(int RunNumber);
int WriteListfileHeader();
int CloseOutputFiles();
int SaveRawData(uint32_t *buff, int nw);
int SaveList(int brd, double ts, uint64_t trgid, void *generic_ev, int dtq);
int SaveHistos();
int WriteTempHV(uint64_t pc_tstamp, ServEvent_t sev[MAX_NBRD]);
int SaveRunInfo();
//int SaveMeasurement();

#endif