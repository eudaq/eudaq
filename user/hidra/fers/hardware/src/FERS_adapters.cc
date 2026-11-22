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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "FERS_config.h"
#include "FERSlib.h"	

//extern Config_t* FERScfg[FERSLIB_MAX_NBRD];
//extern int BoardConnected[MAX_NBRD];
//extern int NumBoardConnected;


#define ADAPTER_BIC_MAX_SIZE		256
#define ADAPTER_BIC_START_ADDRESS	0


// #################################################################################
// Global Variables
// #################################################################################
static float CalibratedThrOffset[FERSLIB_MAX_NBRD][FERSLIB_MAX_NCH_5203];
static int ThrCalibLoaded[FERSLIB_MAX_NBRD] = { 0 };
static int ThrCalibIsDisabled[FERSLIB_MAX_NBRD] = { 0 };

//										0A  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
const int A5256_ADA2DAC[17] =          { 0,  0,  1,  3,  2,  9,  8, 11, 10,  6,  7,  5,  4, 14, 15, 13, 12 };
const int A5256_ADA2TDC[17] =          { 1,  2,  4, 20, 22, 38, 40, 58, 62,  3,  5, 23, 25, 39, 41, 59, 61 };
const int A5256_TDC2ADA[64] =          { 0,  0,  1,  9,  2, 10, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC,  3, NC,  4, 11,
										NC, 12, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC,  5, 13,
										 6, 14, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC,  7, 15, NC, 16,  8, NC };

//                                    0   1   2   3   4   5   6   7   8 
static const int A5256_REV0_ADA2DAC_POS[9] = { 0,  1,  9,  6, 14, 16, 25, 22, 30 };
static const int A5256_REV0_ADA2TDC_POS[9] = { 0,  4,  20, 38, 58, 5, 23, 39, 59 };
static const int A5256_REV0_TDC2ADA_POS[64] = { 0,  NC, NC, NC,  1,  5, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC,  2, NC, NC,  6,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC,  3,  7,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC,  4,  8, NC, NC, NC, NC };

//                                    0   1   2   3   4   5   6   7   8 
static const int A5256_REV0_ADA2DAC_NEG[9] = { 0,  0,  8,  7, 15, 17, 24, 23, 31 };
static const int A5256_REV0_ADA2TDC_NEG[9] = { 1,  2, 16, 32, 52, 3,  17, 33, 55 };
static const int A5256_REV0_TDC2ADA_NEG[64] = { NC,  0,  1,  5, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										 2,  6, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										 3,  7, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC, NC, NC, NC, NC,
										NC, NC, NC, NC,  4, NC, NC,  8,
										NC, NC, NC, NC, NC, NC, NC, NC };

// #################################################################################
// Functions
// #################################################################################

// ---------------------------------------------------------------------------------
// Description: Return the number of channels of the adapter
// Return:		num of channels (0 = no adapter)
// ---------------------------------------------------------------------------------
int FERS_AdapterNch(int brd)
{
	if (FERScfg[brd]->AdapterType == ADAPTER_A5256)
		return 18;
	else if ((FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG) || (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS))
		return 9;
	else return 0;
}


// ---------------------------------------------------------------------------------
// Description: choose where to save calibration (if present EEPROM is default)
// Return:		memory code
// ---------------------------------------------------------------------------------
int FERS_FindMemThrDest(int handle)
{
	uint8_t tag;

	FERS_ReadEEPROMBlock(handle, ADAPTER_BIC_START_ADDRESS, 1, &tag);	// Check presence of EEPROM on A5256-Adapter
	if (tag == 'B')
		return A5256_EEPROM;
	else
		return A5203_FLASH;
}


// ---------------------------------------------------------------------------------
// Description: Remap TDC channel into Adapter channel 
// Inputs:		TDC_ch: picoTDC channel number 
// Outputs:		Adapter_ch: Adapter channel number
// Return:		0: map OK, -1: map error
// ---------------------------------------------------------------------------------
int FERS_ChIndex_tdc2ada(int TDC_ch, int* Adapter_ch, int brd) {
	int ich, ret = 0;
	*Adapter_ch = 0;
	if (FERScfg[brd]->AdapterType == ADAPTER_NONE) {
		*Adapter_ch = TDC_ch;
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256) {
		if ((TDC_ch == 1) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_DUAL))
			*Adapter_ch = 17;
		else {
			ich = A5256_TDC2ADA[TDC_ch];
			if (ich == -1) ret = -1;
			else *Adapter_ch = ich;
		}
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS) {
		ich = A5256_REV0_TDC2ADA_POS[TDC_ch];
		if (ich == -1) ret = -1;
		else *Adapter_ch = ich;
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG) {
		ich = A5256_REV0_TDC2ADA_NEG[TDC_ch];
		if (ich == -1) ret = -1;
		else *Adapter_ch = ich;
	}
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Remap Adapter channel into TDC channel 
// Inputs:		Adapter_ch: Adapter channel number
// Outputs:		TDC_ch: TDC channel number
// Return:		0: map OK, -1: map error
// ---------------------------------------------------------------------------------
int FERS_ChIndex_ada2tdc(int Adapter_ch, int *TDC_ch, int brd)
{
	*TDC_ch = -1;
	if (FERScfg[brd]->AdapterType == ADAPTER_NONE) {
		*TDC_ch = Adapter_ch;
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256) {
		if ((Adapter_ch == 0) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_POSITIVE))			*TDC_ch = 1;
		else if ((Adapter_ch == 0) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_NEGATIVE))		*TDC_ch = 0;
		else if ((Adapter_ch < 18) && (Adapter_ch > 0)) *TDC_ch = A5256_ADA2TDC[Adapter_ch];
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS) {
		*TDC_ch = A5256_REV0_ADA2TDC_POS[Adapter_ch];
	} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG) {
		*TDC_ch = A5256_REV0_ADA2TDC_NEG[Adapter_ch];
	}
	return 0;
}

// ---------------------------------------------------------------------------------
// Description: Remap bits of the channel mask
// Inputs:		AdapterMask: Adapter channel mask
// Outputs		ChMask0: TDC channel mask for ch 0..31
//				ChMask1: TDC channel mask for ch 32..63
// Return:		0: OK; -1: error
// ---------------------------------------------------------------------------------
int FERS_ChMask_ada2tdc(uint32_t AdapterMask, uint32_t *ChMask0, uint32_t *ChMask1, int brd)
{
	int i, TDCch, TDCch_d;
	uint64_t TDCmask = 0;
	int nch = FERS_AdapterNch(brd) - 1;
	if (FERScfg[brd]->AdapterType == ADAPTER_NONE)
		return -1;
	for (i = 0; i < nch; i++) {
		TDCch_d = -1;
		if (AdapterMask & (1 << i)) {
			if (FERScfg[brd]->AdapterType == ADAPTER_A5256) {
				if ((i == 0) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_DUAL)) {
					TDCch = 0;
					TDCch_d = 1;
				} else if ((i == 0) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_POSITIVE)) {
					TDCch = 1;
				} else if ((i == 0) && (FERScfg[brd]->A5256_Ch0Polarity == A5256_CH0_NEGATIVE)) {
					TDCch = 0;
				} else {
					TDCch = A5256_ADA2TDC[i];
				}
			} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS) {
				TDCch = A5256_REV0_ADA2TDC_POS[i];
			} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG) {
				TDCch = A5256_REV0_ADA2TDC_NEG[i];
			} else {	// ADAPTER_NONE?
				continue;
			}
			TDCmask |= (uint64_t)1 << TDCch;
			if (TDCch_d != -1) {
				TDCmask |= (uint64_t)1 << TDCch_d;
				TDCch_d = 0;
			}
		}
	}
	*ChMask0 = (uint32_t)(TDCmask & 0xFFFFFFFF);
	*ChMask1 = (uint32_t)((TDCmask >> 32) & 0xFFFFFFFF);
	return 0;
}



// ---------------------------------------------------------------------------------
// Description: Set discriminator threshold
// Inputs:		handle: board handle
//				Adapter_ch: adapter channel number
//				thr_mv: threshold in mv
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int FERS_Set_DiscrThreshold(int handle, int Adapter_ch, float thr_mv, int brd)
{
	int i, ret = 0, b;
	float thr, thr0a, thr0b;
	uint32_t data, addr, dac16;
	int nch = FERS_AdapterNch(brd);

	if (FERScfg[brd]->AdapterType == ADAPTER_NONE)
		return 0;
	if (Adapter_ch >= nch)
		return -1;

	b = FERS_INDEX(handle);

	if (Adapter_ch == 0) {
		thr0a = ((ThrCalibLoaded[b] && !ThrCalibIsDisabled[b])) ? thr_mv + CalibratedThrOffset[b][17] : thr_mv;
		thr0a = (float)max(min(thr0a, 1249), -1249);
		thr0b = ((ThrCalibLoaded[b] && !ThrCalibIsDisabled[b])) ? thr_mv + CalibratedThrOffset[b][0] : thr_mv;
		thr0b = (float)max(min(thr0b, 1249), -1249);
		for (i = 0; i < 2; i++) {
			addr = 0x30 | i;
			dac16 = (i == 0) ? A5256_mV_to_DAC(thr0a) << 4 : A5256_mV_to_DAC(thr0b) << 4;		// DAC uses 16 bit format (4 LSBs = 0)
			data = ((dac16 & 0xFF) << 24) | (((dac16 >> 8) & 0xFF) << 16);
			ret = FERS_I2C_WriteRegister(handle, 0x10, addr, data);
		}

	} else {
		thr = ((ThrCalibLoaded[b] && !ThrCalibIsDisabled[b])) ? thr_mv + CalibratedThrOffset[b][Adapter_ch] : thr_mv;
		thr = (float)max(min(thr, 1249), -1249);
		if (FERScfg[brd]->AdapterType == ADAPTER_A5256) {
			addr = 0x30 | (A5256_ADA2DAC[Adapter_ch] & 0xF);
			dac16 = A5256_mV_to_DAC(thr) << 4;	 // DAC uses 16 bit format (4 LSBs = 0)
			data = ((dac16 & 0xFF) << 24) | (((dac16 >> 8) & 0xFF) << 16);
			ret = FERS_I2C_WriteRegister(handle, 0x0C, addr, data);
		} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS) {
			// not supported anymore
		} else if (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG) {
			// not supported anymore
		}
	}
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Calibrate the threshold offset (zero)
// Inputs:		handle: board handle
//				min_thr: minimun threshold for the scan
//				max_thr: maximum threshold for the scan
// Outputs:		ThrOffset: threshold offsets
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
#define THRCALIB_MAX_NSTEP	100
int FERS_CalibThresholdOffset(int handle, float min_thr, float max_thr, int *done, float *ThrOffset, float *RMSnoise, void(*ptr)(char* msg, int progress))
{
	int ret = 0, b, nb, ch, dtq, ntrg, niter, step, nstep;
	int nch;
	char cback[256];
	ListEvent_t* Event;
	double tstamp_us;
	uint32_t hit_cnt[THRCALIB_MAX_NSTEP][FERSLIB_MAX_NCH_5203], tot_hit, dac_start;   // hit_cnt_max[FERSLIB_MAX_NCH_5203] = { 0 },
	uint64_t st;
	float thr[THRCALIB_MAX_NSTEP];
	Config_t* Fcfg_tmp;
	FILE* calib;

	b = FERS_INDEX(handle);
	if (FERScfg[b]->AdapterType == ADAPTER_NONE)
		return -1;

	//nch = FERS_AdapterNch(b) + 1;  // Ch0 has 2 thresholds
	nch = FERS_AdapterNch(b);  // Ch0 has 2 thresholds
	nstep = (int)((max_thr - min_thr) / A5256_DAC_LSB) + 1;
	if (nstep > THRCALIB_MAX_NSTEP) {
		_setLastLocalError("The scan range from % .1f to % .1f is too high\n", min_thr, max_thr);
		return -1;
	}
	dac_start = A5256_mV_to_DAC(min_thr);
	if (A5256_DAC_to_mV(dac_start) > min_thr) {
		dac_start++;
		nstep++;
	}

	// Reset current calibration
	memset(CalibratedThrOffset[b], 0, nch * sizeof(float));
	ThrCalibLoaded[b] = 0;

	Fcfg_tmp = (Config_t*)malloc(sizeof(Config_t));
	FERScfg[b]->A5256_Ch0Polarity = A5256_CH0_DUAL;
	memcpy(Fcfg_tmp, FERScfg[b], sizeof(Config_t));  // Save configuration
	FERScfg[b]->AcquisitionMode = ACQMODE_TRG_MATCHING;
	FERScfg[b]->TriggerMask = 0x20; // Trigger = Ptrg
	FERScfg[b]->PtrgPeriod = 100000;  // 100 us
	FERScfg[b]->TrgWindowWidth = 50000; // 50 us
	FERScfg[b]->TrgWindowOffset = -50000; // -50 us
	FERScfg[b]->ChEnableMask = (1 << FERS_AdapterNch(b)) - 1; // enable all channels of the adapter
	FERScfg[b]->TDC_ChBufferSize = 5;
	FERScfg[b]->En_Empty_Ev_Suppr = 0;
	FERScfg[b]->Tref_Mask = 0;
	FERScfg[b]->TestMode = 0;
	FERScfg[b]->GlitchFilterMode = GLITCHFILTERMODE_DISABLED;
	FERScfg[b]->StartRunMode = STARTRUN_ASYNC;

	FERS_configure(handle, CFG_HARD);

	//ClearScreen();
	//Con_printf("LCSm", "Starting Threshold Calibration (%d steps)\n\n", nstep);
	//Con_printf("LCSm", "thr(mV)   0A  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 0B\n");
	sprintf(cback, "\nStarting Threshold Calibration (%d steps)\n", nstep);
	(*ptr)(cback, -1);
	sprintf(cback, "thr(mV)   0A  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 0B\n");
	(*ptr)(cback, -2);

	for (step = 0; step < nstep; step++) {

		//float xx = (1250 - (float)(dac_start * ((float)2500 / 4095)));				// Convert Threshold from DAC LSB to mV

		thr[step] = A5256_DAC_to_mV(dac_start - step);
		//Con_printf("LCSm", "%6.2f: ", thr[step]);
		sprintf(cback, "%6.2f:    ", thr[step]);
		//(*ptr)(cback, 0);

		char calib_run[100];
		int per100 = 100 * step / (nstep);
		sprintf(calib_run, "%d Running Threshold Calibration", b);
		(*ptr)(calib_run, per100);

		for (ch = 0; ch < FERS_AdapterNch(b) - 1; ch++)
			ret |= FERS_Set_DiscrThreshold(handle, ch, thr[step], b);
		if (ret < 0) {
			_setLastLocalError("Set Discriminator Threshold A5256 error (ret=%d)\n", ret);
			return ret;
		}

		Sleep(10);
		memset(hit_cnt[step], 0, nch * sizeof(uint32_t));
		ntrg = 0;
		tot_hit = 0;
		niter = 0;


		FERS_StartAcquisition(&handle, 1, STARTRUN_ASYNC, -50);
		st = get_time();
		while (ntrg < 1000) {
			nb = 0;
			ret = FERS_GetEventFromBoard(handle, &dtq, &tstamp_us, (void**)(&Event), &nb); // Read Data from the board
			if (ret < 0) {
				char merr[512];
				sprintf(merr, "Readout error (ret=%d\n)", ret);
				_setLastLocalError(merr);
				return ret;
			}
			if (nb > 0) {
				ntrg++;
				if ((dtq & 0xF) == DTQ_TIMING) {
					for (int i = 0; i < Event->nhits; i++) {
						FERS_ChIndex_tdc2ada(Event->channel[i], &ch, b);
						hit_cnt[step][ch]++;
						tot_hit++;
					}
				}
			} else {  // no data; quit after 1 s
				niter++;
				if ((get_time() - st) > 1000)
					break;
			}
		}
		FERS_StopAcquisition(&handle, 1, STARTRUN_ASYNC, -50);
		for (ch = 0; ch < nch; ch++)
			if (hit_cnt[step][ch] == 0) {
				//Con_printf("LSCm", "  .");
				strcat(cback, ".  ");
				//(*ptr)(cback, 0);
			} else { 
				strcat(cback, "|  "); // or "%s  |"
				//if (step == 0 || step == nstep)
				//	return FERSLIB_ERR_CALIB_FAIL;
				//else {
					//Con_printf("LSCm", "  |");
			}
		strcat(cback, "\n");
		(*ptr)(cback, -2);
		//(*ptr)("\n", 0);
	}

	sprintf(cback, "%d Running Adapter Thresholds Calibration", b);
	(*ptr)(cback, 100);
	// find centroids
	float centr[FERSLIB_MAX_NCH_5203];
	float rms[FERSLIB_MAX_NCH_5203];
	int ntot[FERSLIB_MAX_NCH_5203];
	//Con_printf("LSCm", "\n");
	(*ptr)("\n", -2);
	for (ch = 0; ch < nch; ch++) {
		centr[ch] = 0;
		ntot[ch] = 0;
		rms[ch] = 0;
		for (step = 0; step < nstep; step++) {
			centr[ch] += hit_cnt[step][ch] * thr[step];
			rms[ch] += hit_cnt[step][ch] * (thr[step] * thr[step]);
			ntot[ch] += hit_cnt[step][ch];
		}
		if (ntot[ch] > 0) {
			centr[ch] = centr[ch] / ntot[ch];
			rms[ch] = (float)sqrt(rms[ch] / ntot[ch] - centr[ch] * centr[ch]);
		}
		if (ntot[ch] > 0) {
			if (ch == 0) {
				//Con_printf("LSCm", "Ch 0A: Offset=%7.2f, RMS=%7.2f\n", centr[ch], rms[ch]);
				sprintf(cback, "Ch 0A: Offset=%7.2f, RMS=%7.2f\n", centr[ch], rms[ch]);
				(*ptr)(cback, -2);
				done[ch] = 1;
			} else if (ch == 17) {
				//Con_printf("LSCm", "Ch 0B: Offset=%7.2f, RMS=%7.2f\n", centr[ch], rms[ch]);
				sprintf(cback, "Ch 0B: Offset=%7.2f, RMS=%7.2f\n", centr[ch], rms[ch]);
				(*ptr)(cback, -2);
				done[ch] = 1;
			} else {
				//Con_printf("LSCm", "Ch %2d: Offset=%7.2f, RMS=%7.2f\n", ch, centr[ch], rms[ch]);
				sprintf(cback, "Ch %2d: Offset=%7.2f, RMS=%7.2f\n", ch, centr[ch], rms[ch]);
				(*ptr)(cback, -2);
				done[ch] = 1;
			}
		} else {
			//Con_printf("LSCm", "Ch %2d: UNKNOWN\n", ch);
			sprintf(cback, "Ch %2d: UNKOWN\n", ch);
			(*ptr)(cback, -2);
			done[ch] = 0;
		}
		if (ThrOffset != NULL)
			ThrOffset[ch] = centr[ch];
		if (RMSnoise != NULL)
			RMSnoise[ch] = rms[ch];
	}

	// save data to file (for debug)
	char fname_calib[50];
	sprintf(fname_calib, "ThrCalib_%d.txt", FERS_INDEX(handle));
	calib = fopen(fname_calib, "w");

	for (step = 0; step < nstep; step++) {
		fprintf(calib, "%6.1f : ", thr[step]);
		for (ch = 0; ch < nch; ch++) {
			fprintf(calib, "%10d ", hit_cnt[step][ch]);
		}
		fprintf(calib, "\n");
	}
	fprintf(calib, "\n\nCalibrated Offsets:\n");
	for (ch = 0; ch < nch; ch++) {
		fprintf(calib, "%2d : %7.2f  (RMS=%.2f)\n", ch, ThrOffset[ch], rms[ch]);
	}
	fclose(calib);

	memcpy(CalibratedThrOffset[b], ThrOffset, nch * sizeof(float));
	ThrCalibLoaded[b] = 1;

	memcpy(Fcfg_tmp, &FERScfg[b], sizeof(Config_t)); // Restore configuration
	free(Fcfg_tmp);
	FERS_configure(handle, CFG_HARD);
	return 0;
}


//// --------------------------------------------------------------------------------------------------------- 
//// Description: Write BIC into the EEPROM of the Adapter (if present)
//// Inputs:		handle = board handle 
////				binfo = board info struct
//// Return:		0=OK, negative number = error code
//// --------------------------------------------------------------------------------------------------------- 
//int FERS_WriteAdapterBIC(int handle, FERS_A5256_Info_t binfo)
//{
//	int ret;
//	uint8_t tag;
//	const char BICversion = 0;
//	uint8_t bic[ADAPTER_BIC_MAX_SIZE];
//
//	memset(bic, 0, ADAPTER_BIC_MAX_SIZE);
//	bic[0] = 'B';								// identifier			(1)
//	bic[1] = BICversion;						// BIC version			(1) 
//	memcpy(bic + 2, &binfo.pid, 4);				// PID					(4)
//	memcpy(bic + 6, &binfo.AdapterCode, 2);		// e.g. 5256			(2)
//	memcpy(bic + 8, &binfo.PCBrevision, 1);		// PCB_Rev				(1)
//	memcpy(bic + 9, binfo.ModelCode, 16);		// e.g. WA5256XAAAAA	(16)
//	memcpy(bic + 25, binfo.ModelName, 16);		// e.g. A5256			(16)
//	memcpy(bic + 41, &binfo.NumCh, 2);			// Num Channels			(2)
//	ret = FERS_WriteEEPROMBlock(handle, ADAPTER_BIC_START_ADDRESS, 43, bic);
//	if (ret < 0) return ret;
//	// Check presence of EEPROM
//	ret = FERS_ReadEEPROMBlock(handle, ADAPTER_BIC_START_ADDRESS, 1, &tag);
//	if (ret < 0) return ret;
//	if (tag == 'B')	return 0;
//	else return FERSLIB_ERR_OPER_NOT_ALLOWED;
//}
//
//
//// --------------------------------------------------------------------------------------------------------- 
//// Description: Read BIC from the EEPROM of the Adapter (if present)
//// Inputs:		handle = board handle 
//// Outputs:		binfo = board info struct
//// Return:		0=OK, negative number = error code
//// --------------------------------------------------------------------------------------------------------- 
//int FERS_ReadAdapterBIC(int handle, FERS_A5256_Info_t* binfo)
//{
//	int ret;
//	uint8_t bic[ADAPTER_BIC_MAX_SIZE];
//
//	memset(binfo, 0, sizeof(FERS_A5256_Info_t));
//	ret = FERS_ReadEEPROMBlock(handle, EEPROM_BIC_PAGE, EEPROM_BIC_SIZE, bic);
//	if (ret < 0)
//		return ret;
//	if ((bic[0] != 'B') || (bic[1] != 0))
//		return FERSLIB_ERR_INVALID_BIC;
//	memcpy(&binfo->pid, bic + 2, 4);
//	memcpy(&binfo->AdapterCode, bic + 6, 2);
//	memcpy(&binfo->PCBrevision, bic + 8, 1);
//	memcpy(binfo->ModelCode, bic + 9, 16);
//	memcpy(binfo->ModelName, bic + 25, 16);
//	memcpy(&binfo->NumCh, bic + 41, 2);
//	return 0;
//}


// --------------------------------------------------------------------------------------------------------- 
// Description: Write Threshold offset calibration to flash
//              WARNING: the flash memory contains vital parameters for the board. Overwriting certain pages
//                       can damage the hardware!!! Do not use this function without contacting CAEN first
// Inputs:		handle = board handle 
//				npts = number of values to write
//				MemThrDest = eeprom or flash 
//				ThrOffset = Threshold offsets (use NULL pointer to keep old values)
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_WriteThrCalib(int handle, int npts, int MemThrDest,float *ThrOffset)
{
	int ret, sz=0;
	uint8_t cal[FLASH_PAGE_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int calpage = FLASH_THR_OFFS_PAGE;

	cal[0] = 'T';	// Tag
	cal[1] = 0;		// Format
	*(uint16_t*)(cal + 2) = (uint16_t)(tm.tm_year + 1900);
	cal[4] = (uint8_t)(tm.tm_mon + 1);
	cal[5] = (uint8_t)(tm.tm_mday);
	cal[6] = (uint8_t)(npts);
	if (ThrOffset != NULL) memcpy(cal + 7, ThrOffset, sz);

	if (MemThrDest == A5256_EEPROM)	//eeprom present
		ret = FERS_WriteEEPROMBlock(handle, EEPROM_CAL_PAGE, 7 + sz, cal);
	else
		ret = FERS_WriteFlashPage(handle, calpage, 7 + sz, cal);
	// Update local variables
	memcpy(CalibratedThrOffset, ThrOffset, sz);
	ThrCalibLoaded[FERS_INDEX(handle)] = 1;
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: Read calibrated threshold offsets from flash
// Inputs:		handle = board handle 
//				npts = num of points to read 
//				MemThrDest = eeprom or flash
// Outputs:		date = calibration date	(DD/MM/YYYY)
//				ThrOffset = calibrated offsets
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int FERS_ReadThrCalib(int handle, int npts, int MemThrDest, char *date, float *ThrOffset)
{
	int ret, np;
	int year, month, day;
	uint8_t cal[FLASH_PAGE_SIZE];
	float calthr[FERSLIB_MAX_NCH_5203];
	int calpage = FLASH_THR_OFFS_PAGE;

	if (date != NULL)
		strcpy(date, "");

	if (MemThrDest == A5256_EEPROM)
		ret = FERS_ReadEEPROMBlock(handle, EEPROM_CAL_PAGE, EEPROM_CAL_SIZE, cal);
	else
		ret = FERS_ReadFlashPage(handle, calpage, 256, cal);

	if ((cal[0] != 'T') || (cal[1] != 0)) {
		return FERSLIB_ERR_CALIB_NOT_FOUND;
	}
	year = *(uint16_t*)(cal + 2);
	month = cal[4];
	day = cal[5];
	np = cal[6];
	if (np > npts) {
		_setLastLocalError("Error: Calibration missing\n");
		return FERSLIB_ERR_CALIB_NOT_FOUND;
	}
	if (date != NULL) sprintf(date, "%02d/%02d/%04d", day, month, year);
	memset(calthr, 0, np * sizeof(float));
	memcpy(calthr, cal + 7, np * sizeof(float));
	if (ThrOffset != NULL) memcpy(ThrOffset, calthr, np * sizeof(float));
	memcpy(CalibratedThrOffset[FERS_INDEX(handle)], calthr, np * sizeof(float));
	ThrCalibLoaded[FERS_INDEX(handle)] = 1;
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Disable Threshold calibration
// Inputs:		handle: board handle
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int FERS_DisableThrCalib(int handle) {
	ThrCalibIsDisabled[FERS_INDEX(handle)] = 1;
	return 0;
}

// ---------------------------------------------------------------------------------
// Description: Enable Threshold calibration
// Inputs:		handle: board handle
// Return:		0=ok, <0 error
// ---------------------------------------------------------------------------------
int EnableThrCalib(int handle) {
	ThrCalibIsDisabled[FERS_INDEX(handle)] = 0;
	return 0;
}
