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
*
* WDplot is a library that allows WaveDump to plot the waveforms.
* It saves the data read from the digitizer into a text file and sends the 
* plotting commands to gnuplot through a pipe. Thus, WDplot is just a simple 
* server that transfers the data and the commands from WaveDump to gnuplot, 
* which is the actual plotting tool.
******************************************************************************/
#ifdef WIN32
    #include <sys/timeb.h>
    #include <time.h>
#else
	#include <sys/time.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <stdint.h> 
	#include <pthread.h>
#endif

#include "WDplot.h"


/* Defines */
#define PLOT_DATA_FILE   "PlotData.txt"

#ifdef WIN32
	/******************************************************************************
	* Executable file for 'gnuplot'
	* NOTE: use pgnuplot instead of wgnuplot under Windows, otherwise the pipe 
	*       will not work
	******************************************************************************/
    #define GNUPLOT_COMMAND  "pgnuplot"
#else
    #define GNUPLOT_COMMAND  "gnuplot"
#endif

/* Global Variables */
WDPlot_t  PlotVar;
int Busy = 0;
int SetOption = 1;
long Tfinish;
FILE *gnuplot = NULL;
char PlotDataFileName[512];
extern char path[128];

static long get_time()
{
	long time_ms;
#ifdef WIN32
	struct _timeb timebuffer;
	_ftime( &timebuffer );
	time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
#else
	struct timeval t1;    
	struct timezone tz;
	gettimeofday(&t1, &tz);
	time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
#endif
	return time_ms;
}

void ClearPlot(void) {
    if(gnuplot!=NULL)
        fprintf(gnuplot, "clear\n");
}

int SetPlotOptions(void)
{
    fprintf(gnuplot, "set xlabel '%s'\n", PlotVar.Xlabel);
    fprintf(gnuplot, "set ylabel '%s'\n", PlotVar.Ylabel);
    fprintf(gnuplot, "set title '%s'\n", PlotVar.Title);
    fprintf(gnuplot, "Xs = %f\n", PlotVar.Xscale);
    fprintf(gnuplot, "Ys = %f\n", PlotVar.Yscale);
    fprintf(gnuplot, "Xmax = %f\n", PlotVar.Xmax);
    fprintf(gnuplot, "Ymax = %f\n", PlotVar.Ymax);
    fprintf(gnuplot, "Xmin = %f\n", PlotVar.Xmin);
    fprintf(gnuplot, "Ymin = %f\n", PlotVar.Ymin);
    if (PlotVar.Xautoscale)
        fprintf(gnuplot, "set autoscale x\n");
    else
        fprintf(gnuplot, "set xrange [Xmin:Xmax]\n");
    if (PlotVar.Yautoscale)
        fprintf(gnuplot, "set autoscale y\n");
    else
        fprintf(gnuplot, "set yrange [Ymin:Ymax]\n");
    //fprintf(gnuplot, "load 'PlotSettings.cfg'\n");
	fflush(gnuplot);
    SetOption = 0;
    return 0;
}


int PlotWaveforms(void)
{
	int i, s=0, ntr, comma=0, c, npts=0, WaitTime;
	FILE *fplot;
	Busy = 1;	
    if (SetOption)
	    SetPlotOptions();
    sprintf(PlotDataFileName, "%s%s", path, PLOT_DATA_FILE);
	fplot = fopen(PlotDataFileName, "w");
    if (fplot == NULL) {
        Busy = 0;
		return -1;
    }

	ntr = PlotVar.NumTraces;
	while(ntr > 0) {
		fprintf(fplot, "%d\t", s);
		for(i=0; i<PlotVar.NumTraces; i++) {
			if (s < PlotVar.TraceSize[i]) {
                if (PlotVar.DataType == PLOT_DATA_UINT8) {
                    uint8_t *data = (uint8_t *)PlotVar.TraceData[i];
				    fprintf(fplot, "%d\t", data[s]);
                } else if (PlotVar.DataType == PLOT_DATA_UINT16) {
                    uint16_t *data = (uint16_t *)PlotVar.TraceData[i];
				    fprintf(fplot, "%d\t", data[s]);
                } else if (PlotVar.DataType == PLOT_DATA_UINT32) {
                    uint32_t *data = (uint32_t *)PlotVar.TraceData[i];
				    fprintf(fplot, "%d\t", data[s]);
                } else if (PlotVar.DataType == PLOT_DATA_DOUBLE) {
                    double *data = (double *)PlotVar.TraceData[i];
                    fprintf(fplot, "%f\t", data[s]);
               } else if (PlotVar.DataType == PLOT_DATA_FLOAT) {
                    float *data = (float *)PlotVar.TraceData[i];
                    fprintf(fplot, "%f\t", data[s]);
                }
				npts++;
			}
			if (PlotVar.TraceSize[i] == (s-1))
				ntr--;
		}
		s++;
		fprintf(fplot, "\n");
	}
	fclose(fplot);

	/* str contains the plot command for gnuplot */
	fprintf(gnuplot, "plot ");
	c = 2; /* first column of data */
	for(i=0; i<PlotVar.NumTraces; i++) {
		if (comma)
			fprintf(gnuplot, ", ");
		fprintf(gnuplot, "'%s' using ($1*%f):($%d*%f) title '%s' with step linecolor %d ", PlotDataFileName, PlotVar.Xscale, c++, PlotVar.Yscale, PlotVar.TraceName[i], i+1);
		comma = 1;
	}
	fprintf(gnuplot, "\n"); 
	fflush(gnuplot);

	/* set the time gnuplot will have finished */
	WaitTime = npts/20;
	if (WaitTime < 100)
		WaitTime = 100;
	Tfinish = get_time() + WaitTime;
	return 0;
}

/**************************************************************************//**
* \fn      int OpenPlotter(void)
* \brief   Open the plotter (i.e. open gnuplot with a pipe)
* \return  0: Success; -1: Error
******************************************************************************/
WDPlot_t *OpenPlotter(char *Path, int NumTraces, int MaxTraceLenght)
{
    char str[1000];
    int i;

    if (NumTraces > MAX_NUM_TRACES)
        return NULL;

    strcpy(str, Path);
    strcat(str, GNUPLOT_COMMAND);
    if ((gnuplot = popen(str, "w")) == NULL) // open the pipe
        return NULL;

    /* send some plot settings */
    fprintf(gnuplot, "set grid\n");
    fprintf(gnuplot, "set mouse\n");
    fprintf(gnuplot, "bind y 'set yrange [Ymin:Ymax]'\n");
    fprintf(gnuplot, "bind x 'set xrange [Xmin:Xmax]'\n");
    fflush(gnuplot);

    /* set default parameters */
    strcpy(PlotVar.Title, "");
    strcpy(PlotVar.Xlabel, "");
    strcpy(PlotVar.Ylabel, "");
    for(i=0; i<NumTraces; i++)
        strcpy(PlotVar.TraceName[i], "");
    PlotVar.Xscale = 1.0;
    PlotVar.Yscale = 1.0;
    PlotVar.Xmax = 16384;
    PlotVar.Ymax = 16384;
    PlotVar.Xmin = 0;
    PlotVar.Ymin = 0;
    PlotVar.NumTraces = 0;

    /* allocate data buffers (use 4 byte per point) */
    for (i=0; i<NumTraces; i++) {
        PlotVar.TraceData[i] = malloc(MaxTraceLenght * sizeof(double));  // double is the maximum size
        if (PlotVar.TraceData[i] == NULL) {
            int j;
            for(j=0; j<i; i++)
                free(PlotVar.TraceData[j]);
            break;
        }
    }
    if (i < NumTraces)
        return NULL;
    
    return &PlotVar;
}



/**************************************************************************//**
* \fn      void ClosePlotter(void)
* \brief   Close the plotter (gnuplot pipe)
******************************************************************************/
int ClosePlotter()
{
    if (gnuplot != NULL)
        pclose(gnuplot);
    return 0;
}

/**************************************************************************//**
* \brief   Check if plot has finished 
******************************************************************************/
int IsPlotterBusy ()
{
	if (get_time() > Tfinish)
		Busy = 0;
	return Busy;
}
