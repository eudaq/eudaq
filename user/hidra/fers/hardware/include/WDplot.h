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

#ifndef __WDPLOT_H
#define __WDPLOT_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAENDigitizerType.h"

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    #define popen  _popen    /* redefine POSIX 'deprecated' popen as _popen */
    #define pclose  _pclose  /* redefine POSIX 'deprecated' pclose as _pclose */
#else

#endif


//#define MAX_NUM_TRACES    8+1     /* Maximum number of traces in a plot */
#define MAX_NUM_TRACES    MAX_V1730_CHANNEL_SIZE     /* Maximum number of traces in a plot */

typedef enum {
    PLOT_DATA_UINT8		= 0,
    PLOT_DATA_UINT16	= 1,
    PLOT_DATA_UINT32	= 2,
    PLOT_DATA_DOUBLE	= 3,
    PLOT_DATA_FLOAT		= 4, 
} PlotDataType_t;

typedef struct {
    char              Title[100];
    char              TraceName[MAX_NUM_TRACES][100];
    char              Xlabel[100];
    char              Ylabel[100];
    int               Xautoscale;
    int               Yautoscale;
    float             Xscale;
    float             Yscale;
    float             Xmax;
    float             Ymax;
    float             Xmin;
    float             Ymin;
    int               NumTraces;
    int               TraceSize[MAX_NUM_TRACES];
    void              *TraceData[MAX_NUM_TRACES];
    PlotDataType_t    DataType;
} WDPlot_t;


/* Functions */
WDPlot_t *OpenPlotter(char *Path, int NumTraces, int MaxTraceLenght);
int SetPlotOptions(void);
int PlotWaveforms(void);
int IsPlotterBusy(void);
int ClosePlotter(void);
void ClearPlot(void);

#endif // __WDPLOT_H
