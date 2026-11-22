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

#ifndef _STATISTICS_H
#define _STATISTICS_H              // Protect against multiple inclusion

#include <stdint.h>
#include "MultiPlatform.h"

#define MAX_NUM_ROI					16
#define MAX_NUM_CALPNT				16
#define MAX_NUM_SPECTRA				8

#define HISTO2D_NBINX				8
#define HISTO2D_NBINY				8
#define STAIRCASE_NBIN				1024

#define STATS_MAX_NBRD				128
#define STATS_MAX_NCH				64


//****************************************************************************
// Histogram Data Structure (1 dimension)
//****************************************************************************
// ROI 
typedef struct {
	int begin;
	int end;
	int gross_counts;
	int net_counts;
	double centroid;
	double fwhm;
	double amplitude;
	double unc;
	double calibr;
	double live_time_fit;		// live time in s at the time of the fit
} ROI_t;

typedef struct
{
	char spectrum_name[100];	// spectrum name
	char title[20];				// plot title
	char x_unit[20];			// X axis unit name (e.g. keV)
	char y_unit[20];			// Y axis unit name (e.g. counts)
	uint32_t *H_data;			// pointer to the histogram data
	uint32_t Nbin;				// Number of bins (channels) in the histogram
	uint32_t H_cnt;				// Number of entries (sum of the counts in each bin)
	uint32_t H_p_cnt;			// Previous Number of entries 
	uint32_t Ovf_cnt;			// Overflow counter
	uint32_t Unf_cnt;			// Underflow counter
	uint32_t Bin_set;			// Last bin set (for MCS popruse)
	double rms;					// rms
	double mean;				// mean
	double p_rms;				// previous rms
	double p_mean;				// previous mean
	double real_time;			// real time in s
	double live_time;			// live time in s

	// calibration coefficients
	double A[4];

	// calibration points
	int n_calpnt;
	double calpnt_ch[MAX_NUM_CALPNT];
	double calpnt_kev[MAX_NUM_CALPNT];

	// ROIs
	int n_ROI;
	ROI_t ROI[MAX_NUM_ROI];
} Histogram1D_t;

//****************************************************************************
// Histogram Data Structure (2 dimensions)
//****************************************************************************
typedef struct
{
	char spectrum_name[100];	// spectrum name
	char title[20];				// plot title
	char x_unit[20];			// X axis unit name (e.g. keV)
	char y_unit[20];			// Y axis unit name (e.g. counts)
	uint32_t *H_data;			// pointer to the histogram data
	uint32_t NbinX;				// Number of bins (channels) in the X axis
	uint32_t NbinY;				// Number of bins (channels) in the Y axis
	uint32_t H_cnt;				// Number of entries (sum of the counts in each bin)
	uint32_t Ovf_cnt;			// Overflow counter
	uint32_t Unf_cnt;			// Underflow counter
} Histogram2D_t;

//****************************************************************************
// Counter Data Structure
//****************************************************************************
typedef struct
{
	uint64_t cnt;		// Current Counter value
	uint64_t pcnt;		// Previous Counter value (last update)
	uint64_t ptime;		// Last update time (ms from start time)
	double rate;		// Rate (Hz)
} Counter_t;

//****************************************************************************
// Staircase histogram offline
//****************************************************************************
typedef struct
{
	int nsteps;
	uint32_t* threshold;
	float* counts;
} Staircase_t;

//****************************************************************************
// struct containig the statistics amd histograms of each channel
//****************************************************************************
typedef struct Stats_t {
	int offline_bin;											// Number of bin for offline histograms
	time_t time_of_start;										// Start Time in UnixEpoch (as time_t type)
	uint64_t start_time;										// Start Time from computer (epoch ms)
	uint64_t stop_time;											// Stop Time from computer (epoch ms)
	uint64_t current_time;										// Current Time from computer (epoch ms)
	uint64_t previous_time;										// Previous Time from computer (epoch ms)
	Counter_t BuiltEventCnt;									// Counter of Built events
	double current_tstamp_us[STATS_MAX_NBRD];					// Current Time from board (oldest time stamp in us)
	double previous_tstamp_us[STATS_MAX_NBRD];					// Previous Time from board (used to calculate elapsed time => rates)
	uint64_t current_trgid[STATS_MAX_NBRD];						// Current Trigger ID
	uint64_t previous_trgid[STATS_MAX_NBRD];					// Previous Trigger ID (used to calculate lost triggers)
	uint64_t previous_trgid_p[STATS_MAX_NBRD];					// Previous Trigger ID (used to calculate percent of lost triggers)
	double trgcnt_update_us[STATS_MAX_NBRD];					// update time of the trg counters
	double previous_trgcnt_update_us[STATS_MAX_NBRD];			// Previous update time of the trg counters
	Counter_t ChTrgCnt[STATS_MAX_NBRD][STATS_MAX_NCH];			// Channel Trigger counter from fast discriminators (when counting mode is enabled)
	Counter_t HitCnt[STATS_MAX_NBRD][STATS_MAX_NCH];			// Recorded Hit counter (when timing mode is enabled)
	Counter_t PHACnt[STATS_MAX_NBRD][STATS_MAX_NCH];			// PHA event counter HG or LG (when spectroscopy mode is enabled)
	Counter_t T_OR_Cnt[STATS_MAX_NBRD];							// T-OR counter
	Counter_t Q_OR_Cnt[STATS_MAX_NBRD];							// Q-OR counter
	Counter_t GlobalTrgCnt[STATS_MAX_NBRD];						// Global Trigger Counter
	Counter_t LostTrg[STATS_MAX_NBRD];							// Lost Trigger
	Counter_t ByteCnt[STATS_MAX_NBRD];							// Byte counter (read data)
	float LostTrgPerc[STATS_MAX_NBRD];							// Percent of lost triggers
	float BuildPerc[STATS_MAX_NBRD];							// Percent of built events
	Histogram1D_t H1_PHA_HG[STATS_MAX_NBRD][STATS_MAX_NCH];		// Energy Histograms (High Gain)
	Histogram1D_t H1_PHA_LG[STATS_MAX_NBRD][STATS_MAX_NCH];		// Energy Histograms (Low Gain)
	Histogram1D_t H1_ToA[STATS_MAX_NBRD][STATS_MAX_NCH];		// ToA Histograms 
	Histogram1D_t H1_ToT[STATS_MAX_NBRD][STATS_MAX_NCH];		// ToT Histograms 
	Histogram1D_t H1_MCS[STATS_MAX_NBRD][STATS_MAX_NCH];		// MultiChannel Scaler Histograms
	float *Staircase[STATS_MAX_NBRD][STATS_MAX_NCH];			// Staircase (Cnt vs Threshold)
	uint32_t *Hold_PHA_2Dmap[STATS_MAX_NBRD][STATS_MAX_NCH];	// Hold Delay Scan (PHA vs Hold delay)
	Histogram1D_t H1_File[MAX_NUM_SPECTRA];							// Histograms read from file (off line)
	//float* Staircase_file[MAX_NTRACES];						// Staircase read from file (offline)
	Staircase_t Staircase_offline[MAX_NUM_SPECTRA];					// Staircase structure, offline
} Stats_t;

//****************************************************************************
// Global variables
//****************************************************************************
extern Stats_t Stats;		// Statistics (histograms and counters)

//****************************************************************************
// Function prototypes
//****************************************************************************
int Histo1D_AddCount(Histogram1D_t *Histo, int Bin);
int Histo1D_SetBinsCount(Histogram1D_t* Histo, int Bin, int counts);
int Histo1D_SetCount(Histogram1D_t* Histo, int counts);
int Histo2D_AddCount(Histogram2D_t *Histo, int BinX, int BinY);
int Histo1D_Offline(int num_of_trace, int num_brd, int num_ch, char *infos, int type_p);
int H1_File_Fill(int num_of_trace, char* type, char* filename);
int H1_File_Fill_FromFile(int num_of_trace, char* type, char* name_of_file);
int SetStaircase_FromFile(Staircase_t *stc, int sbin, int threshold, float counts);
int CreateHistogram1D(int Nbin, char* Title, char* Name, char* Xunit, char* Yunit, Histogram1D_t* Histo);
int DestroyHistogram1D(Histogram1D_t Histo);
int CreateStatistics(int nb, int nch, int *AllocatedSize);
int DestroyStatistics();
int ResetStatistics();
void UpdateCounter(Counter_t *Counter, uint32_t cnt);
int UpdateStatistics(int RateMode);

#endif