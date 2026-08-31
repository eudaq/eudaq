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

#include <stdio.h>
#include <stdlib.h>
#include "JanusC.h"
#include "Statistics.h"
//#include "FERSlib.h"
#include "console.h"
//#include "MultiPlatform.h"

// ****************************************************************************************
// Global Variables
// ****************************************************************************************
Stats_t Stats;			// Statistics (histograms and counters)

//int HistoCreated[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int StcCreated[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedE[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedT[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoCreatedM[STATS_MAX_NBRD][STATS_MAX_NCH] = { 0 };
int HistoFileCreated = 0;

// ****************************************************************************************
// Create/destroy/initialize histograms
// ****************************************************************************************
int CreateHistogram1D(int Nbin, char *Title, char *Name, char *Xunit, char *Yunit, Histogram1D_t *Histo) {
	Histo->H_data = (uint32_t *)malloc(Nbin*sizeof(uint32_t));
	strcpy(Histo->title, Title);
	strcpy(Histo->spectrum_name, Name);
	strcpy(Histo->x_unit, Xunit);
	strcpy(Histo->y_unit, Yunit);
	Histo->Nbin = Nbin;
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	Histo->Bin_set = 0;
	Histo->mean = 0;
	Histo->rms = 0;
	Histo->real_time = 0;
	Histo->live_time = 0;
	Histo->n_ROI = 0;
	Histo->n_calpnt = 0;
	Histo->A[0] = 0;
	Histo->A[1] = 1;
	Histo->A[2] = 0;
	Histo->A[3] = 0;
	return 0;
}

int CreateHistogram2D(int NbinX, int NbinY, char *Title, char *Name, char *Xunit, char *Yunit, Histogram2D_t *Histo) {
	Histo->H_data = (uint32_t *)malloc(NbinX * NbinY * sizeof(uint32_t));
	strcpy(Histo->title, Title);
	strcpy(Histo->spectrum_name, Name);
	strcpy(Histo->x_unit, Xunit);
	strcpy(Histo->y_unit, Yunit);
	Histo->NbinX = NbinX;
	Histo->NbinY = NbinY;
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	return 0;
}

int CreateStaircase_Offline(Staircase_t *stc) {
	stc->nsteps = 0;
	stc->threshold = (uint32_t*)malloc(STAIRCASE_NBIN * sizeof(uint32_t));
	stc->counts = (float*)malloc(STAIRCASE_NBIN * sizeof(float));
	stc->nsteps = 0;
	memset(stc->threshold, 0, STAIRCASE_NBIN * sizeof(uint32_t));
	memset(stc->counts, 0, STAIRCASE_NBIN * sizeof(float));
	return 0;
}

int DestroyHistogram2D(Histogram2D_t Histo) {
	if (Histo.H_data != NULL) {
		free(Histo.H_data);
		Histo.H_data = NULL;
	}
	return 0;
}

int DestroyHistogram1D(Histogram1D_t Histo) {
	if (Histo.H_data != NULL) {
		free(Histo.H_data);
		Histo.H_data = NULL;
	}	// DNIN error in memory access
	return 0;
}

int DestroyStaircase_Offline(Staircase_t stc) {
	if (stc.threshold != NULL) {
		free(stc.threshold);
		stc.threshold = NULL;
	}
	if (stc.counts != NULL) {
		free(stc.counts);
		stc.counts = NULL;
	}
	return 0;
}

int ResetHistogram1D(Histogram1D_t *Histo) {
	memset(Histo->H_data, 0, Histo->Nbin * sizeof(uint32_t));
	Histo->H_cnt = 0;
	Histo->H_p_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	Histo->Bin_set = 0;
	Histo->rms = 0;
	Histo->p_rms = 0;
	Histo->mean = 0;
	Histo->p_mean = 0;
	Histo->real_time = 0;
	Histo->live_time = 0;
	return 0;
}

int ResetHistogram2D(Histogram2D_t *Histo) {
	memset(Histo->H_data, 0, Histo->NbinX * Histo->NbinY * sizeof(uint32_t));
	Histo->H_cnt = 0;
	Histo->Ovf_cnt = 0;
	Histo->Unf_cnt = 0;
	return 0;
}

int ResetStaircase_Offline(Staircase_t *stc) {
	stc->nsteps = 0;
	memset(stc->threshold, 0, STAIRCASE_NBIN * sizeof(uint32_t));
	memset(stc->counts, 0, STAIRCASE_NBIN * sizeof(float));
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Add one count to the histogram 1D
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_AddCount(Histogram1D_t *Histo, int Bin)
{
	if (Bin < 0) {
		Histo->Unf_cnt++;
		return -1;
	} else if (Bin >= (int)(Histo->Nbin-1)) {
		Histo->Ovf_cnt++;
		return -1;
	}
	Histo->H_data[Bin]++;
	Histo->H_cnt++;
	Histo->mean += (double)Bin;
	Histo->rms += (double)Bin * (double)Bin;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set number of counts in a bin to the histogram 1D
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_SetBinsCount(Histogram1D_t* Histo, int Bin, int counts)
{
	if (Bin < 0) {
		Histo->Unf_cnt++;
		return -1;
	}
	else if (Bin >= (int)(Histo->Nbin - 1)) {
		Histo->Ovf_cnt++;
		return -1;
	}
	Histo->H_data[Bin] = counts;
	Histo->H_cnt += counts;
	Histo->mean += (double)Bin * (double)counts;
	if (counts != 0)
		Histo->rms += (double)Bin * (double)Bin * (double)counts;
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set number of counts by increasing the bin number to the histogram 1D [MultiChannel Scaler]
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_SetCount(Histogram1D_t* Histo, int counts)
{
	// mean and rms are computed on Y-axis
	uint32_t Bin = Histo->Bin_set % (Histo->Nbin-1);
	Histo->H_data[Bin] = counts;
	Histo->Bin_set = Bin + 1;	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set bin contents for staircase
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int GetNumLine(char* filename) {	// To initialize Histogram
	FILE* hfile;
	if (!(hfile = fopen(filename, "r"))) {
		Con_printf("LCSw", "Warning: the histogram file %s does not exists\n", filename);
		return -1;
	}
	char tmp_str[100];
	int mline = 0;
	while (!feof(hfile)) {
		fgets(tmp_str, 100, hfile);
		++mline;
	}
	fclose(hfile);

	return (mline - 1);
};

// --------------------------------------------------------------------------------------------------------- 
// Description: Set bin contents for staircase
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int SetStaircase_FromFile(Staircase_t *stc, int sbin, int threshold, float counts)
{
	if (sbin < 1000) {
		stc->nsteps = sbin;
		stc->threshold[sbin] = threshold;
		stc->counts[sbin] = counts;
		//Stats.Staircase_file[num_of_trace][sbin] = counts;
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Fill an 1D histogram from file - Offline
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
//int H1_File_Fill(int num_of_trace, char* type, char *filename)
//{
//	if (strcmp(type, "Staircase") != 0) ResetHistogram1D(&Stats.H1_File[num_of_trace]);
//	else ResetStaircase_Offline(&Stats.Staircase_offline[num_of_trace]);
//	
//	//char filename[200];
//	//sprintf(filename, "%s\\Run%d_%s_%d_%d.txt", "DataFiles", num_r, type, num_brd, num_ch);
//
//	FILE* savedtrace;
//	if (!(savedtrace = fopen(filename, "r")))
//	{
//		// DNIN: send an error message?
//		return -1;
//	}
//
//	int sbin = 0;
//	int scounts = 0;
//	float value_th = 0;
//	while (!feof(savedtrace))
//	{
//		if (strcmp(type, "Staircase") != 0) {
//			fscanf(savedtrace, "%d", &scounts);
//			Histo1D_SetBinsCount(&Stats.H1_File[num_of_trace], sbin, scounts);
//			sbin++;
//		}
//		else {
//			int threshold = 0;
//			fscanf(savedtrace, "%d %f", &threshold, &value_th);	// thresholds and values must be saved both for a correct offline visualization of StairCase
//			SetStaircase_FromFile(&Stats.Staircase_offline[num_of_trace], sbin, threshold, value_th);
//			sbin++;
//		}
//
//	}
//	//if (strcmp(type, "Staircase") != 0) Stats.H1_File[num_of_trace].Nbin = sbin;
//	fclose(savedtrace);
//
//	return 0;
//}

// --------------------------------------------------------------------------------------------------------- 
// Description: Set bin contents in the histogram 1D - Browse from PlotTraces
// Return:		0=OK, -1=under/over flow
// --------------------------------------------------------------------------------------------------------- 
int H1_File_Fill_FromFile(int num_of_trace, char *type, char* name_of_file)
{
	if (strcmp(type, "Staircase") != 0) ResetHistogram1D(&Stats.H1_File[num_of_trace]);
	else ResetStaircase_Offline(&Stats.Staircase_offline[num_of_trace]);
	FILE* savedtrace;
	if (!(savedtrace = fopen(name_of_file, "r")))
	{
		Con_printf("LCSw", "Warning: the histogram file %s does not exists\n", name_of_file);
		return -1;
	}

	int sbin = 0;
	int scounts = 0;
	float value_th = 0;
	while (!feof(savedtrace))
	{
		if (strcmp(type, "Staircase") != 0) {
			char tmpstr[50];
			fgets(tmpstr, 50, savedtrace);	// Since in the offline visualization you can have whatever file
			sscanf(tmpstr, "%d", &scounts);
			Histo1D_SetBinsCount(&Stats.H1_File[num_of_trace], sbin, scounts);
			//Stats.H1_File[num_of_trace].Nbin = sbin;
			sbin++;
		}
		else {
			int threshold = 0;
			char tmpstr[50];
			char ctrl[50];
			fgets(tmpstr, 50, savedtrace);
			int nen = sscanf(tmpstr, "%d %f %s", &threshold, &value_th, ctrl);	// thresholds and values must be saved both for a correct offline visualization of StairCase
			if (nen != 2) {
				threshold = 150;
				value_th = 1;
			}
			SetStaircase_FromFile(&Stats.Staircase_offline[num_of_trace], sbin, threshold, value_th);
			sbin++;
		}


	}
	//if (strcmp(type, "Staircase") != 0) Stats.H1_File[num_of_trace].Nbin = sbin;
	fclose(savedtrace);

	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Manage offline histograms (F & S) filling
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int Histo1D_Offline(int num_of_trace, int num_brd, int num_ch, char *infos, int type_p) {

	char xunit_off[20];
	char yunit_off[20];
	char plot_name[50];
	if (type_p == PLOT_E_SPEC_LG) {
		sprintf(plot_name, "PHA_LG");
		sprintf(xunit_off, "%s", "fC");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_E_SPEC_HG) {
		sprintf(plot_name, "PHA_HG");
		sprintf(xunit_off, "%s", "fC");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_TOA_SPEC) {
		sprintf(plot_name, "ToA");
		sprintf(xunit_off, "%s", "ns");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_TOT_SPEC) {
		sprintf(plot_name, "ToT");
		sprintf(xunit_off, "%s", "ns");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_MCS_TIME) {
		sprintf(plot_name, "MCS");
		sprintf(xunit_off, "%s", "# Trg");
		sprintf(yunit_off, "%s", "Cnt");
	} else if (type_p == PLOT_SCAN_THR) {
		sprintf(plot_name, "Staircase");
	}

	char filename[200] = "";
	if (infos[0] == 'F') {
		int num_r;
		sscanf(infos + 1, "%d", &num_r);
		sprintf(filename, "%s\\Run%d_%s_%d_%d.txt", J_cfg.DataFilePath, num_r, plot_name, num_brd, num_ch);
	}
	else if (infos[0] == 'S') {
		sscanf(infos + 1, "%s", filename);
	}
		
	if (strcmp(plot_name, "Staircase") == 0) {
		if (Stats.offline_bin <= 0) {
			Stats.offline_bin = GetNumLine(filename);
			if (Stats.offline_bin <= 0) {// empty or not existing, set the bin content to 0
				memset(Stats.H1_File[num_of_trace].H_data, 0, sizeof(uint32_t)*Stats.H1_File[num_of_trace].Nbin);
				return -1;
			}
		}
		char h_name[50];
		sprintf(h_name, "%s[%d][%d]", plot_name, num_brd, num_ch);
		DestroyHistogram1D(Stats.H1_File[num_of_trace]);
		CreateHistogram1D(Stats.offline_bin, plot_name, h_name, xunit_off, yunit_off, &Stats.H1_File[num_of_trace]);
	}
	H1_File_Fill_FromFile(num_of_trace, plot_name, filename);
	//}
	//else if (infos[0] == 'S') {
	//	sscanf(infos + 1, "%s", filename);
	//	if (strcmp(plot_name, "Staircase") != 0) {
	//		if (Stats.offline_bin <= 0) {
	//			Stats.offline_bin = GetNumLine(filename);
	//			if (Stats.offline_bin <= 0) // empty or not existing
	//				return -1;
	//		}

	//		char h_name[50];
	//		sprintf(h_name, "ExtFile_%s[%d][%d]", plot_name, num_brd, num_ch);
	//		DestroyHistogram1D(Stats.H1_File[num_of_trace]);
	//		CreateHistogram1D(Stats.offline_bin, plot_name, h_name, xunit_off, yunit_off, &Stats.H1_File[num_of_trace]);
	//	}
	//	H1_File_Fill_FromFile(num_of_trace, plot_name, filename);

	//}

	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Add one count to the histogram 1D
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int Histo2D_AddCount(Histogram2D_t *Histo, int BinX, int BinY)
{
	if ((BinX >= (int)(Histo->NbinX-1)) || (BinY >= (int)(Histo->NbinY-1))) {
		Histo->Ovf_cnt++;
		return -1;
	} else if ((BinX < 0) || (BinY < 0)) {
		Histo->Unf_cnt++;
		return -1;
	}
	Histo->H_data[BinX + HISTO2D_NBINY * BinY]++;
	Histo->H_cnt++;
	return 0;
}




// ****************************************************************************************
// Create Structures and histograms for the specific application
// ****************************************************************************************

// --------------------------------------------------------------------------------------------------------- 
// Description: Create all histograms and allocate the memory
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int CreateStatistics(int nb, int nch, int *AllocatedSize)
{
	int b, ch, i;

	Stats.offline_bin = -1;

	//*AllocatedSize = sizeof(Stats_t);
	*AllocatedSize = 0;
	for(b=0; b < nb; b++) {
		for(ch=0; ch < nch; ch++) {
			char hname[100];
			
			if (J_cfg.EHistoNbin > 0) {
				sprintf(hname, "PHA-LG[%d][%d]", b, ch);
				CreateHistogram1D(J_cfg.EHistoNbin, "PHA-LG", hname, "fC", "Cnt", &Stats.H1_PHA_LG[b][ch]);
				*AllocatedSize += J_cfg.EHistoNbin * sizeof(uint32_t);
				Stats.H1_PHA_LG[b][ch].A[0] = 0;
				Stats.H1_PHA_LG[b][ch].A[1] = 1;
				Stats.H1_PHA_LG[b][ch].A[2] = 0;
				Stats.H1_PHA_LG[b][ch].A[3] = 0;
				sprintf(hname, "PHA-HG[%d][%d]", b, ch);
				CreateHistogram1D(J_cfg.EHistoNbin, "PHA-HG", hname, "fC", "Cnt", &Stats.H1_PHA_HG[b][ch]);
				*AllocatedSize += J_cfg.EHistoNbin * sizeof(uint32_t);
				Stats.H1_PHA_HG[b][ch].A[0] = 0;
				Stats.H1_PHA_HG[b][ch].A[1] = 1;
				Stats.H1_PHA_HG[b][ch].A[2] = 0;
				Stats.H1_PHA_HG[b][ch].A[3] = 0;
				HistoCreatedE[b][ch] = 1;
			}
			
			if (J_cfg.ToAHistoNbin) {
				sprintf(hname, "ToA[%d][%d]", b, ch);
				CreateHistogram1D(J_cfg.ToAHistoNbin, "ToA", hname, "Time (ns)", "Cnt", &Stats.H1_ToA[b][ch]);
				*AllocatedSize += J_cfg.ToAHistoNbin * sizeof(uint32_t);
				Stats.H1_ToA[b][ch].A[0] = J_cfg.ToAHistoMin;
				Stats.H1_ToA[b][ch].A[1] = 0.5 * J_cfg.ToARebin; // Conversion from bin to ns
				sprintf(hname, "ToT[%d][%d]", b, ch);
				CreateHistogram1D(J_cfg.ToTHistoNbin, "ToT", hname, "Time (ns)", "Cnt", &Stats.H1_ToT[b][ch]);
				*AllocatedSize += J_cfg.ToTHistoNbin * sizeof(uint32_t);
				Stats.H1_ToT[b][ch].A[0] = 0;
				//Stats.H1_ToT[b][ch].A[1] = 0.5 * ((1 << TOT_NBIT) / J_cfg.ToTHistoNbin);  // conversion from bin to ns
				Stats.H1_ToT[b][ch].A[1] = 0.5;

				HistoCreatedT[b][ch] = 1;
			}

			if (J_cfg.MCSHistoNbin) {
				sprintf(hname, "MCS[%d][%d]", b, ch);
				CreateHistogram1D(J_cfg.MCSHistoNbin, "MCS", hname, "Time (s)", "Cnt", &Stats.H1_MCS[b][ch]);
				*AllocatedSize += J_cfg.MCSHistoNbin * sizeof(uint32_t);
				Stats.H1_MCS[b][ch].A[0] = 0;	// DNIN set the correct values
				if (J_cfg.TriggerMask == 0x21)
					Stats.H1_MCS[b][ch].A[1] = J_cfg.PtrgPeriod/1e9;
				else
					Stats.H1_MCS[b][ch].A[1] = 1;

				HistoCreatedM[b][ch] = 1;
			}
			// allocate stair case (similar to histogram 1D but threated in a different way)
			int scsize = STAIRCASE_NBIN * sizeof(float);
			Stats.Staircase[b][ch] = (float *)malloc(scsize);
			memset(Stats.Staircase[b][ch], 0, scsize);
			StcCreated[b][ch] = 1;
			*AllocatedSize += scsize;

			//HistoCreated[b][ch] = 1;
		}
	}

	for(i=0; i<MAX_NTRACES; i++) {
		
		// Allocate histograms form file 
		int maxnbin = J_cfg.EHistoNbin;
		maxnbin = max(maxnbin, J_cfg.ToAHistoNbin);
		maxnbin = max(maxnbin, J_cfg.ToTHistoNbin);
		CreateHistogram1D(maxnbin, "", "", "", "Cnt", &Stats.H1_File[i]);
		*AllocatedSize += maxnbin*sizeof(uint32_t);
		// Allocate stair case from file (similar to histogram 1D but threated in a different way)
		int scsize = STAIRCASE_NBIN * sizeof(float);
		int stsize = STAIRCASE_NBIN * sizeof(uint32_t);
		CreateStaircase_Offline(&Stats.Staircase_offline[i]);
		*AllocatedSize += scsize;
		*AllocatedSize += stsize;
		}

	HistoFileCreated = 1;
	ResetStatistics();	// DNIN: Is it needed here? Too Many Reset Histograms at the begin

	// This is related to histogram filling, not histogram creation
	//for (i = 0; i < MAX_NTRACES; i++) {
	//	char tmpchar[100] = "";
	//	int tmp0, tmp1; // , rn;
	//	sscanf(RunVars.PlotTraces[i], "%d %d %s", &tmp0, &tmp1, tmpchar);
	//	if (tmpchar[0] == 'F' || tmpchar[0] == 'S')
	//		Histo1D_Offline(i, tmp0, tmp1, tmpchar, RunVars.PlotType);
	//}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Destroy (free memory) all histograms
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int DestroyStatistics()
{
	int b, ch, i;
	for(b=0; b< J_cfg.NumBrd; b++) {
		for(ch=0; ch<FERSLIB_MAX_NCH_5202; ch++) {
			if (HistoCreatedE[b][ch]) {
				DestroyHistogram1D(Stats.H1_PHA_LG[b][ch]);
				DestroyHistogram1D(Stats.H1_PHA_HG[b][ch]);
				HistoCreatedE[b][ch] = 0;
			}
			if (HistoCreatedT[b][ch]) {
				DestroyHistogram1D(Stats.H1_ToA[b][ch]);
				DestroyHistogram1D(Stats.H1_ToT[b][ch]);
				HistoCreatedT[b][ch] = 0;
			}
			if (HistoCreatedM[b][ch]) {
				DestroyHistogram1D(Stats.H1_MCS[b][ch]);
				HistoCreatedM[b][ch] = 0;
			}
			if (StcCreated[b][ch]) {
				free(Stats.Staircase[b][ch]);
				Stats.Staircase[b][ch] = NULL;
				StcCreated[b][ch] = 0;
			}
		}
	}

	if (HistoFileCreated) {
		for (i = 0; i < MAX_NTRACES; i++) {
			DestroyHistogram1D(Stats.H1_File[i]);
			DestroyStaircase_Offline(Stats.Staircase_offline[i]);
		}
		HistoFileCreated = 0;
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Reset all statistics and histograms
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
int ResetStatistics()	// Have H1_File to be reset?
{
	int b, ch;

	Stats.start_time = j_get_time(); 
	Stats.current_time = Stats.start_time; 
	Stats.previous_time = Stats.start_time; 
	memset(&Stats.BuiltEventCnt, 0, sizeof(Counter_t));
	for(b=0; b<J_cfg.NumBrd; b++) {
		Stats.current_trgid[b] = 0;
		Stats.previous_trgid[b] = 0;
		Stats.current_tstamp_us[b] = 0;
		Stats.previous_tstamp_us[b] = 0;
		Stats.trgcnt_update_us[b] = 0;
		Stats.previous_trgcnt_update_us[b] = 0;
		Stats.LostTrgPerc[b] = 0;
		Stats.BuildPerc[b] = 0;
		memset(&Stats.LostTrg[b], 0, sizeof(Counter_t));
		memset(&Stats.Q_OR_Cnt[b], 0, sizeof(Counter_t));
		memset(&Stats.T_OR_Cnt[b], 0, sizeof(Counter_t));
		memset(&Stats.GlobalTrgCnt[b], 0, sizeof(Counter_t));
		memset(&Stats.ByteCnt[b], 0, sizeof(Counter_t));
		for(ch=0; ch<FERSLIB_MAX_NCH_5202; ch++) {
			if (HistoCreatedE[b][ch]) {
				ResetHistogram1D(&Stats.H1_PHA_LG[b][ch]);
				ResetHistogram1D(&Stats.H1_PHA_HG[b][ch]);
			}
			if (HistoCreatedT[b][ch]) {
				ResetHistogram1D(&Stats.H1_ToA[b][ch]);
				ResetHistogram1D(&Stats.H1_ToT[b][ch]);
			}
			if (HistoCreatedM[b][ch]) ResetHistogram1D(&Stats.H1_MCS[b][ch]);
			
			memset(&Stats.ChTrgCnt[b][ch], 0, sizeof(Counter_t));
			memset(&Stats.HitCnt[b][ch], 0, sizeof(Counter_t));
			memset(&Stats.PHACnt[b][ch], 0, sizeof(Counter_t));
		}
	}
	for (int i = 0; i < 8; i++) {
		ResetStaircase_Offline(&Stats.Staircase_offline[i]);
		//ResetHistogram1D(&Stats.H1_File[i]);	// DNIN:It prevent to initialize the histograms
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Update statistics 
// Return:		0=OK, -1=error
// --------------------------------------------------------------------------------------------------------- 
#define MEAN(m, ns)		((ns) > 0 ? (m)/(ns) : 0)
#define RMS(r, m, ns)	((ns) > 0 ? sqrt((r)/(ns) - (m)*(m)) : 0)

void UpdateCounter(Counter_t *Counter, uint32_t cnt) {
	// counters in FPGA are 24 bit => extend to 64
	if (cnt < (uint32_t)(Counter->cnt & 0xFFFFFF)) 
		Counter->cnt = ((Counter->cnt & 0xFFFFFFFFFF000000) + 0x1000000) | (cnt & 0xFFFFFF);
	else 
		Counter->cnt = (Counter->cnt & 0xFFFFFFFFFF000000) | (cnt & 0xFFFFFF);
}

void UpdateCntRate(Counter_t *Counter, double elapsed_time_us, int RateMode) {
	if (elapsed_time_us <= 0) {
		//Counter->rate = 0;
		return;
	}
	else if (RateMode == 1)
		Counter->rate = Counter->cnt / (elapsed_time_us * 1e-6);
	else 
		Counter->rate = (Counter->cnt - Counter->pcnt) / (elapsed_time_us * 1e-6);
	Counter->pcnt = Counter->cnt;
}

int UpdateStatistics(int RateMode)
{
	int b, ch;
	double pc_elapstime = (RateMode == 1) ? 1e3 * (Stats.current_time - Stats.start_time) : 1e3 * (Stats.current_time - Stats.previous_time);  // us
	Stats.previous_time = Stats.current_time;

	for(b=0; b < J_cfg.NumBrd; b++) {
		double brd_elapstime = (RateMode == 1) ? Stats.current_tstamp_us[b]: Stats.current_tstamp_us[b] - Stats.previous_tstamp_us[b];  // - Stats.start_time 
		double elapstime = (brd_elapstime > 0) ? brd_elapstime : pc_elapstime;
		double trgcnt_elapstime = (RateMode == 1) ? Stats.trgcnt_update_us[b] - Stats.start_time*1000: Stats.trgcnt_update_us[b] - Stats.previous_trgcnt_update_us[b];  // - Stats.start_time 
		Stats.previous_tstamp_us[b] = Stats.current_tstamp_us[b];
		Stats.previous_trgcnt_update_us[b] = Stats.trgcnt_update_us[b];
		for(ch=0; ch<FERSLIB_MAX_NCH_5202; ch++) {
			/*if (HistoCreated[b][ch]) {
				Stats.H1_PHA_HG[b][ch].mean = MEAN(Stats.H1_PHA_HG[b][ch].mean, Stats.H1_PHA_HG[b][ch].H_cnt);
				Stats.H1_PHA_LG[b][ch].mean = MEAN(Stats.H1_PHA_LG[b][ch].mean, Stats.H1_PHA_LG[b][ch].H_cnt);
			}*/
			UpdateCntRate(&Stats.ChTrgCnt[b][ch], trgcnt_elapstime, RateMode);
			UpdateCntRate(&Stats.HitCnt[b][ch], elapstime, RateMode);
			UpdateCntRate(&Stats.PHACnt[b][ch], elapstime, RateMode);
		}
		if (Stats.current_trgid[b] == 0) 
			Stats.LostTrgPerc[b] = 0;
		else if (RateMode == 1) 
			Stats.LostTrgPerc[b] = (float)(100.0 * Stats.LostTrg[b].cnt / Stats.current_trgid[b]);
		else if (Stats.current_trgid[b] > Stats.previous_trgid_p[b]) 
			Stats.LostTrgPerc[b] = (float)(100.0 * (Stats.LostTrg[b].cnt - Stats.LostTrg[b].pcnt) / (Stats.current_trgid[b] - Stats.previous_trgid_p[b]));
		else 
			Stats.LostTrgPerc[b] = 0;
		if (Stats.LostTrgPerc[b] > 100) 
			Stats.LostTrgPerc[b] = 100;
		Stats.previous_trgid_p[b] = Stats.current_trgid[b];

		if (Stats.BuiltEventCnt.cnt == 0) 
			Stats.BuildPerc[b] = 0;
		else if (RateMode == 1) 
			Stats.BuildPerc[b] = (float)(100.0 * Stats.GlobalTrgCnt[b].cnt / Stats.BuiltEventCnt.cnt);
		else if (Stats.BuiltEventCnt.cnt > Stats.BuiltEventCnt.pcnt) 
			Stats.BuildPerc[b] = (float)(100.0 * (Stats.GlobalTrgCnt[b].cnt - Stats.GlobalTrgCnt[b].pcnt) / (Stats.BuiltEventCnt.cnt - Stats.BuiltEventCnt.pcnt));
		else 
			Stats.BuildPerc[b] = 0;
		if (Stats.BuildPerc[b] > 100) 
			Stats.BuildPerc[b] = 100;
		UpdateCntRate(&Stats.LostTrg[b], elapstime, RateMode);
		UpdateCntRate(&Stats.T_OR_Cnt[b], trgcnt_elapstime, RateMode);
		UpdateCntRate(&Stats.Q_OR_Cnt[b], trgcnt_elapstime, RateMode);
		UpdateCntRate(&Stats.GlobalTrgCnt[b], elapstime, RateMode);
		UpdateCntRate(&Stats.ByteCnt[b], pc_elapstime, RateMode);
	}
	Stats.BuiltEventCnt.pcnt = Stats.BuiltEventCnt.cnt;
	return 0;
}

