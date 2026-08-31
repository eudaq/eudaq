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

#ifndef _PLOT_H
#define _PLOT_H                    // Protect against multiple inclusion

#include "JanusC.h"
#include "FERSlib.h"
#include "Statistics.h"

#define MAX_NTRACES   8

int OpenPlotter();
int ClosePlotter();
int PlotSpectrum();
int PlotCntHisto();
int PlotWave(WaveEvent_t *wev, char *title);
int Plot2Dmap(int StatIntegral);
int PlotStaircase();
int PlotScanHoldDelay(int *newrun);

#endif