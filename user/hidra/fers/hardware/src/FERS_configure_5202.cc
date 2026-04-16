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

#include "FERS_config.h"

#define NW_SCBS (1144/32+1)  // Num of 32 bit word to contain the SC bit stream

// Local functions
static int ReadSCbsFromChip(int handle, int chip, uint32_t* SCbs);
static int WriteSCbsToChip(int handle, int chip, uint32_t* SCbs);
static int ReadSCbsFromFile(char* filename, uint32_t* SCbs);
static int WriteCStoFile(char* filename, uint32_t* SCbs);
static uint16_t CSslice(uint32_t* SCbs, int start, int stop);
static int WriteCStoFileFormatted(char* filename, uint32_t* SCbs);


int Configure5202(int handle, int mode) {
	int i, ret = 0;

	uint32_t HV_adj_range = 0;

	uint32_t d32, CitirocCfg, Tpulse_ctrl = 0;
	char CfgStep[100];
	int brd = FERS_INDEX(handle);
	if (!BoardConnected[brd])
		return 2;	// BOARD NOT CONNECTED
	uint32_t SCbs[2][NW_SCBS];  // Citiroc SC bit stream

	int rundelay = 0;
	int deskew = -5;

	// ########################################################################################################
	sprintf(CfgStep, "General System Configuration and Acquisition Mode");
	// ########################################################################################################
	// Reset the unit 
	if (mode == CFG_HARD) {
		ret = FERS_SendCommand(handle, CMD_RESET);  // Reset
		if (ret != 0) goto abortcfg;
	}
	// Channel Enable Mask
	ret |= FERS_WriteRegister(handle, a_channel_mask_0, (uint32_t)(FERScfg[brd]->ChEnableMask & 0xFFFFFFFF));
	ret |= FERS_WriteRegister(handle, a_channel_mask_1, (uint32_t)((FERScfg[brd]->ChEnableMask >> 32) & 0xFFFFFFFF));

	// Set USB or Eth communication mode
	if ((FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) || (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB))
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 20, 20, 1); // Force disable of TDlink
	else if (FERS_FPGA_FW_MajorRev(handle) >= 4)
		ret |= FERS_WriteRegister(handle, a_uC_shutdown, 0xDEAD); // Shut down uC (PIC) if not used (readout via TDL)

	// Acq Mode
	if (mode == CFG_HARD) ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 0, 3, FERScfg[brd]->AcquisitionMode);

	if (FERS_FPGA_FW_MajorRev(handle) >= 4) {
		// Enable Zero suppression in counting mode (only with FW >= 4.00)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 30, 30, FERScfg[brd]->EnableCntZeroSuppr);
		// Service event Mode
		if (FERScfg[brd]->AcquisitionMode == ACQMODE_COUNT)
			ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 18, 19, FERScfg[brd]->EnableServiceEvents & 0x1);
		else
			ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 18, 19, FERScfg[brd]->EnableServiceEvents);
	}

	//ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 4, 7, FERScfg->TimingMode); // timing mode: 0=common start, 1=common stop, 2=streaming  
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 8, 9, FERScfg[brd]->EnableToT); // timing info: 0=leading edge only, 1=leading edge + ToT

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 24, 25, FERScfg[brd]->Validation_Mode); // 0=disabled, 1=accept, 2=reject

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 27, 29, FERScfg[brd]->Counting_Mode); // 0=singles, 1=paired_AND
	ret |= FERS_WriteRegister(handle, a_hit_width, (uint32_t)(FERScfg[brd]->ChTrg_Width / CLK_PERIOD[FERS_INDEX(handle)])); /// Monostable on Citiroc Self triggers => Coinc Window for Trigger logic and counting in paired-AND mode
	ret |= FERS_WriteRegister(handle, a_tlogic_width, (uint32_t)(FERScfg[brd]->Tlogic_Width / CLK_PERIOD[FERS_INDEX(handle)])); // Monostable on Trigger Logic Output (0=linear)

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 26, 26, FERScfg[brd]->TrgIdMode); // Trigger ID: 0=trigger counter, 1=validation counter

	//ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 17, 17, 1); // enable blank conversion (citiroc conversion runs also when the board is busy, but data are not saved)
	//ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 16, 16, 1); // set ADC test mode (ADC chips generate fixed patterns = 0x2AAA - 0x1555)
	//ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 23, 23, 1); // set FPGA test mode (generate event with fixed data pattern)
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 12, 13, FERScfg[brd]->GainSelect); // 0=auto, 1=high gain, 2=low gain, 3=both

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 21, 21, FERScfg[brd]->Range_14bit); // use 14 bit for the A/D conversion of the peak (energy)

	// Set fixed pedestal (applied to PHA after subtraction of the calibrated pedestals)
	ret |= FERS_SetCommonPedestal(handle, FERScfg[brd]->Pedestal);
	ret |= FERS_EnablePedestalCalibration(handle, 1);

	// Set LEMO I/O mode
	ret |= FERS_WriteRegister(handle, a_t0_out_mask, FERScfg[brd]->T0_outMask);
	ret |= FERS_WriteRegister(handle, a_t1_out_mask, FERScfg[brd]->T1_outMask);

	// Waveform length
	if (FERScfg[brd]->WaveformLength > 0) {
		ret |= FERS_WriteRegister(handle, a_wave_length, FERScfg[brd]->WaveformLength);
	}
	// Set Periodic Trigger
	ret |= FERS_WriteRegister(handle, a_dwell_time, (uint32_t)(FERScfg[brd]->PtrgPeriod / CLK_PERIOD[FERS_INDEX(handle)]));
	// Set Trigger Source
	ret |= FERS_WriteRegister(handle, a_trg_mask, FERScfg[brd]->TriggerMask);

	// enable 2nd time stamp (relative to Tref)
	if (FERScfg[brd]->Enable_2nd_tstamp)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl2, 0, 0, 1);

	// Set Run Mask
	rundelay = (NumBoardConnected - FERS_INDEX(handle) - 1) * 4;
	if (FERScfg[brd]->StartRunMode == STARTRUN_ASYNC) {
		ret |= FERS_WriteRegister(handle, a_run_mask, 0x01);
	} else if (FERScfg[brd]->StartRunMode == STARTRUN_CHAIN_T0) {
		if (FERS_INDEX(handle) == 0) {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x01);
			ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1 << 10);  // set T0-OUT = RUN_SYNC
		} else {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x82);
			//ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1);  // set T0-OUT = T0-IN
			ret |= FERS_WriteRegister(handle, a_t0_out_mask, 1 << 10);  // set T0-OUT = RUN_SYNC
		}
		//if (FERScfg->T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T0-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (FERScfg[brd]->StartRunMode == STARTRUN_CHAIN_T1) {
		if (FERS_INDEX(handle) == 0) {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x01);
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1 << 10);  // set T1-OUT = RUN_SYNC
		} else {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x84);
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1);  // set T1-OUT = T1-IN
		}
		//if (FERScfg->T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T1-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (FERScfg[brd]->StartRunMode == STARTRUN_TDL) {
		ret |= FERS_WriteRegister(handle, a_run_mask, 0x01);
	}

	// Set Tref mask
	FERS_WriteRegister(handle, a_tref_mask, FERScfg[brd]->Tref_Mask);
	// Set Tref window
	FERS_WriteRegister(handle, a_tref_window, (uint32_t)(FERScfg[brd]->TrefWindow / ((float)CLK_PERIOD[FERS_INDEX(handle)] / 16)));
	//uint32_t trf_w = (uint32_t)(FERScfg->TrefWindow / ((float)CLK_PERIOD / 16));
	// Set Tref delay
	if ((FERScfg[brd]->AcquisitionMode == ACQMODE_TIMING_CSTART) || (FERScfg[brd]->AcquisitionMode == ACQMODE_TSPECT))
		FERS_WriteRegister(handle, a_tref_delay, (uint32_t)(FERScfg[brd]->TrefDelay / (float)CLK_PERIOD[FERS_INDEX(handle)]));
	else if (FERScfg[brd]->AcquisitionMode == ACQMODE_TIMING_CSTOP) {
		float td = (float)(FERScfg[brd]->TrefDelay / CLK_PERIOD[FERS_INDEX(handle)]) - (float)(FERScfg[brd]->TrefWindow / ((float)CLK_PERIOD[FERS_INDEX(handle)]));
		//int32_t td = ((int32_t)(-FERScfg->TrefWindow / ((float)CLK_PERIOD)) + (int32_t)(FERScfg->TrefDelay / ((float)CLK_PERIOD)));
		FERS_WriteRegister(handle, a_tref_delay, (uint32_t)td);
	}


	// Set Trigger Logic
	FERS_WriteRegister(handle, a_tlogic_def, (FERScfg[brd]->MajorityLevel << 8) | (FERScfg[brd]->TriggerLogic & 0xFF));
	// Set Veto mask
	FERS_WriteRegister(handle, a_veto_mask, FERScfg[brd]->Veto_Mask);
	// Set Validation mask
	FERS_WriteRegister(handle, a_validation_mask, FERScfg[brd]->Validation_Mask);

	// Set Test pulse Source and Amplitude
	if (FERScfg[brd]->TestPulseSource == -1) {  // OFF
		ret |= FERS_WriteRegister(handle, a_tpulse_ctrl, 0);  // Set Tpulse = external 
		ret |= FERS_WriteRegister(handle, a_tpulse_dac, 0);
		//else if (FERScfg->TestPulseDestination == TEST_PULSE_DEST_NONE) ??? this is not available;
	} else {
		Tpulse_ctrl = FERScfg[brd]->TestPulseSource;
		if (FERScfg[brd]->TestPulseDestination == TEST_PULSE_DEST_ALL)  Tpulse_ctrl |= 0x00;
		else if (FERScfg[brd]->TestPulseDestination == TEST_PULSE_DEST_EVEN) Tpulse_ctrl |= 0x10;
		else if (FERScfg[brd]->TestPulseDestination == TEST_PULSE_DEST_ODD)  Tpulse_ctrl |= 0x20;
		//else if (FERScfg->TestPulseDestination == TEST_PULSE_DEST_NONE) ??? this is not available;
		else Tpulse_ctrl |= 0x30 | (FERScfg[brd]->TestPulseDestination << 6);
		Tpulse_ctrl |= ((FERScfg[brd]->TestPulsePreamp & 0x3) << 12);
		ret |= FERS_WriteRegister(handle, a_tpulse_ctrl, Tpulse_ctrl);
		ret |= FERS_WriteRegister(handle, a_tpulse_dac, FERScfg[brd]->TestPulseAmplitude);
	}
	ConfigureProbe5202(handle);
	// Set Digital Probe in concentrator (if present)
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		if (FERScfg[brd]->CncProbe_A >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FA_FN, VR_IO_FUNCTION_ZERO);  // Set FA function = ZERO
		if (FERScfg[brd]->CncProbe_B >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FB_FN, VR_IO_FUNCTION_ZERO);  // Set FB function = ZERO
		FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_DEBUG, FERScfg[brd]->CncProbe_A | (FERScfg[brd]->CncProbe_B << 8));
	}


	// ---------------------------------------------------------------------------------
	// Set Citiroc Parameters
	// ---------------------------------------------------------------------------------
	ret |= FERS_WriteRegister(handle, a_qd_coarse_thr, FERScfg[brd]->QD_CoarseThreshold);	// Threshold for Q-discr
	ret |= FERS_WriteRegister(handle, a_td_coarse_thr, FERScfg[brd]->TD_CoarseThreshold);	// Threshold for T-discr
	//ret |= FERS_WriteRegister(handle, a_td_coarse_thr, FERScfg->TD_CoarseThreshold);	// Threshold for T-discr

	ret |= FERS_WriteRegister(handle, a_lg_sh_time, FERScfg[brd]->LG_ShapingTime);		// Shaping Time LG
	ret |= FERS_WriteRegister(handle, a_hg_sh_time, FERScfg[brd]->HG_ShapingTime);		// Shaping Time HG

	// Set charge discriminator mask
	ret |= FERS_WriteRegister(handle, a_qdiscr_mask_0, (uint32_t)(FERScfg[brd]->QD_Mask & 0xFFFFFFFF));
	ret |= FERS_WriteRegister(handle, a_qdiscr_mask_1, (uint32_t)((FERScfg[brd]->QD_Mask >> 32) & 0xFFFFFFFF));

	// Set time discriminator mask (applied in FPGA, not in Citiroc)
	ret |= FERS_WriteRegister(handle, a_tdiscr_mask_0, (uint32_t)(FERScfg[brd]->Tlogic_Mask & 0xFFFFFFFF));
	ret |= FERS_WriteRegister(handle, a_tdiscr_mask_1, (uint32_t)((FERScfg[brd]->Tlogic_Mask >> 32) & 0xFFFFFFFF));

	// Citiroc Modes
	CitirocCfg = 0;
	CitirocCfg |= FERScfg[brd]->EnableQdiscrLatch << crcfg_qdiscr_latch;	// Qdiscr output: 1=latched, 0=direct                        (bit 258 of SR) 
	CitirocCfg |= FERScfg[brd]->PeakDetectorMode << crcfg_pdet_mode_hg;	// Peak_det mode HighGain: 0=peak detector, 1=T&H            (bit 306 of SR)
	CitirocCfg |= FERScfg[brd]->PeakDetectorMode << crcfg_pdet_mode_lg;	// Peak_det mode LowGain:  0=peak detector, 1=T&H            (bit 307 of SR)
	CitirocCfg |= FERScfg[brd]->FastShaperInput << crcfg_pa_fast_sh;	// Fast shaper connection: 0=high gain pa, 1=low gain pa     (bit 328 of SR)
	//uint32_t HV_adj_range = 0;
	if ((int)FERScfg[brd]->HV_Adjust_Range >= 0) HV_adj_range = (uint32_t)FERScfg[brd]->HV_Adjust_Range << crcfg_8bit_dac_ref;
	else HV_adj_range = 0x1 << crcfg_8bit_dac_ref; // Disabled option means same voltage set as vmon, DAC set to 4.5 as default
	CitirocCfg |= HV_adj_range;					// HV adjust DAC reference: 0=2.5V, 1=4.5V                   (bit 330 of SR)
	CitirocCfg |= FERScfg[brd]->EnableChannelTrgout << crcfg_enable_chtrg;	// 0 = Channel Trgout Disabled, 1 = Enabled
	// Hard coded settings
	CitirocCfg |= 0 << crcfg_sca_bias;			// SCA bias: 0=high (5MHz readout speed), 1=weak             (bit 301 of SR)
	CitirocCfg |= 0 << crcfg_ps_ctrl_logic;		// Peak Sens Ctrl Logic: 0=internal, 1=use ext. PS_modeb_ext (bit 308 of SR)
	CitirocCfg |= 1 << crcfg_ps_trg_source;		// Peak Sens Trg source: 0=internal, 1=external              (bit 309 of SR)
	CitirocCfg |= 0 << crcfg_lg_pa_bias;		// LG Preamp bias: 0=normal, 1=weak                          (bit 323 of SR)
	CitirocCfg |= 1 << crcfg_ota_bias;			// Output OTA buffer bias auto off: 0=auto, 1=force on       (bit 1133 of SR)
	CitirocCfg |= 1 << crcfg_trg_polarity;		// Trigger polarity: 0=pos, 1=neg                            (bit 1141 of SR)
	CitirocCfg |= 1 << crcfg_enable_gtrg;		// Enable propagation of gtrg to the Citiroc pin global_trig
	CitirocCfg |= 1 << crcfg_enable_veto;		// Enable propagation of gate (= not veto) to the Citiroc pin val_evt
	CitirocCfg |= 1 << crcfg_repeat_raz;		// Enable REZ iterations when Q-latch is not reset (bug in Citiroc chip ?)
	ret |= FERS_WriteRegister(handle, a_citiroc_cfg, CitirocCfg);

	//ret |= FERS_WriteRegisterBit(handle, a_citiroc_en, 26, 0); // disable trgouts

	ret |= FERS_WriteRegister(handle, a_hold_delay, (uint32_t)(FERScfg[brd]->HoldDelay / CLK_PERIOD[FERS_INDEX(handle)]));  // Time between trigger and peak sensing Hold

	if (FERScfg[brd]->MuxClkPeriod > 0)
		ret |= FERS_WriteRegisterSlice(handle, a_amux_seq_ctrl, 0, 6, (uint32_t)(FERScfg[brd]->MuxClkPeriod / CLK_PERIOD[FERS_INDEX(handle)]));		// Period of the mux clock
	ret |= FERS_WriteRegisterSlice(handle, a_amux_seq_ctrl, 8, 9, FERScfg[brd]->MuxNSmean);

	// Set Trigger Hold-off (for channel triggers)
	FERS_WriteRegister(handle, a_trgho, (uint32_t)(FERScfg[brd]->TrgHoldOff / CLK_PERIOD[FERS_INDEX(handle)]));

	if (ret) goto abortcfg;

	// ########################################################################################################
	sprintf(CfgStep, "Channel settings");
	// ########################################################################################################	
	for (i = 0; i < FERSLIB_MAX_NCH_5202; i++) {
		uint16_t PedHG, PedLG, zs_thr_lg, zs_thr_hg, ediv;
		ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_lg_gain, i), FERScfg[brd]->LG_Gain[i]);	// Gain (low gain)
		ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_hg_gain, i), FERScfg[brd]->HG_Gain[i]);	// Gain (high gain)
		ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_qd_fine_thr, i), FERScfg[brd]->QD_FineThreshold[i]);
		ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_td_fine_thr, i), FERScfg[brd]->TD_FineThreshold[i]);

		FERS_GetChannelPedestalBeforeCalib(handle, i, &PedLG, &PedHG);
		//if (FERScfg[brd]->EHistoNbin > 0)
		//	ediv = FERScfg[brd]->Range_14bit ? ((1 << 14) / FERScfg[brd]->EHistoNbin) : ((1 << 13) / FERScfg[brd]->EHistoNbin);
		//else
		ediv = 1;
		if (FERScfg[brd]->AcquisitionMode == ACQMODE_SPECT) {  // CTIN: ZS doesn't work in TSPEC mode (indeed ZS applies to energy only, not timing data. It would be a partial suppression)
			zs_thr_lg = (FERScfg[brd]->ZS_Threshold_LG[i] > 0) ? FERScfg[brd]->ZS_Threshold_LG[i] * ediv - FERScfg[brd]->Pedestal + PedLG : 0;
			ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_zs_lgthr, i), zs_thr_lg);
			zs_thr_hg = ((FERScfg[brd]->ZS_Threshold_HG[i] > 0)) ? FERScfg[brd]->ZS_Threshold_HG[i] * ediv - FERScfg[brd]->Pedestal + PedHG : 0;
			ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_zs_hgthr, i), zs_thr_hg);
		}

		if ((int)FERScfg[brd]->HV_Adjust_Range >= 0) ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_hv_adj, i), 0x100 | FERScfg[brd]->HV_IndivAdj[i]);	// DAC-ON | DAC[7..0]
		else ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_hv_adj, i), 0x1FF);	// DAC-ON | DAC[7..0], all ON to simulate 'DISABLED' effect
		if (ret) break;
	}
	if (ret) goto abortcfg;

	// ########################################################################################################
	sprintf(CfgStep, "Configure Citiroc chips");
	// ########################################################################################################

	if (FERScfg[brd]->CitirocCfgMode == CITIROC_CFG_FROM_FILE) {
		FERS_WriteRegister(handle, a_scbs_ctrl, 0x100);  // enable manual loading of SCbs (chip 0)
		ReadSCbsFromFile("weerocGUI.txt", SCbs[0]);
		WriteCStoFileFormatted("weerocGUI_formatted.txt", SCbs[0]);
		WriteSCbsToChip(handle, 0, SCbs[0]);
		ReadSCbsFromChip(handle, 0, SCbs[0]);
		//WriteCStoFile("CitirocCfg_bitstream_0.txt", SCbs[0]);
		char path_file[256];
		sprintf(path_file, "B%d_CitirocCfg_0.txt", brd);
		WriteCStoFileFormatted(path_file, SCbs[0]);

		FERS_WriteRegister(handle, a_scbs_ctrl, 0x300);  // enable manual loading of SCbs (chip 1)
		ReadSCbsFromFile("weerocGUI.txt", SCbs[1]);
		WriteSCbsToChip(handle, 1, SCbs[1]);
		ReadSCbsFromChip(handle, 1, SCbs[1]);
		//WriteCStoFile("CitirocCfg_bitstream_1.txt", SCbs[1]);
		sprintf(path_file, "B%d_CitirocCfg_1.txt", brd);
		WriteCStoFileFormatted(path_file, SCbs[1]);
	} else {
		FERS_WriteRegister(handle, a_scbs_ctrl, 0x000);  // set citiroc index = 0
		FERS_SendCommand(handle, CMD_CFG_ASIC);
		ReadSCbsFromChip(handle, 0, SCbs[0]);
		//WriteCStoFile("CitirocCfg_bitstream_0.txt", SCbs[0]);
		char path_file[256];
		sprintf(path_file, "B%d_CitirocCfg_0.txt", brd);
		WriteCStoFileFormatted(path_file, SCbs[0]);

		FERS_WriteRegister(handle, a_scbs_ctrl, 0x200);  // set citiroc index = 1
		FERS_SendCommand(handle, CMD_CFG_ASIC);
		ReadSCbsFromChip(handle, 1, SCbs[1]);
		//WriteCStoFile("CitirocCfg_bitstream_1.txt", SCbs[1]);
		sprintf(path_file, "B%d_CitirocCfg_1.txt", brd);
		WriteCStoFileFormatted(path_file, SCbs[1]);
	}
	if (ret) goto abortcfg;


	// ########################################################################################################
	sprintf(CfgStep, "Configure HV module");
	// ########################################################################################################
	ret |= FERS_HV_Set_Vbias(handle, FERScfg[brd]->HV_Vbias);  // CTIN: the 1st access to Vset doesn't work (to understand why...). Make it twice
	ret |= FERS_HV_Set_Vbias(handle, FERScfg[brd]->HV_Vbias);
	if (FERScfg[brd]->HV_Imax >= 10)
		FERScfg[brd]->HV_Imax = (float)9.999;  // Warning 10mA is not accepted!!!
	ret |= FERS_HV_Set_Imax(handle, FERScfg[brd]->HV_Imax); // same for Imax...
	ret |= FERS_HV_Set_Imax(handle, FERScfg[brd]->HV_Imax);

	FERS_HV_Set_Tsens_Coeff(handle, FERScfg[brd]->TempSensCoeff);
	FERS_HV_Set_TempFeedback(handle, FERScfg[brd]->EnableTempFeedback, FERScfg[brd]->TempFeedbackCoeff);
	if (ret) goto abortcfg;


	// ########################################################################################################
	sprintf(CfgStep, "Generic Write accesses with mask");
	// ########################################################################################################
	for (i = 0; i < FERScfg[brd]->GWn; i++) {
		ret |= FERS_ReadRegister(handle, FERScfg[brd]->GWaddr[i], &d32);
		d32 = (d32 & ~FERScfg[brd]->GWmask[i]) | (FERScfg[brd]->GWdata[i] & FERScfg[brd]->GWmask[i]);
		ret |= FERS_WriteRegister(handle, FERScfg[brd]->GWaddr[i], d32);
	}
	if (ret) goto abortcfg;

	Sleep(10);
	return 0;

abortcfg:
	//sprintf(ErrorMsg, "Error at: %s. Exit Code = %d\n", CfgStep, ret);
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Configure analog and digital probes
// Inputs:		handle = board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int ConfigureProbe5202(int handle) {
	int brd = FERS_INDEX(handle);
	int a_dprobe;
	if (FERS_IsXROC(handle))
		a_dprobe = a_dprobe_5202;
	else if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203)
		a_dprobe = a_dprobe_5203;
	else return FERSLIB_ERR_NOT_APPLICABLE;

	int i, ret = 0;
	for (i = 0; i < 2; i++) {
		// Set Analog Probe
		if ((FERScfg[brd]->AnalogProbe[i] >= 0) && (FERScfg[brd]->AnalogProbe[i] <= 5)) {
			uint32_t ctp = (FERScfg[brd]->AnalogProbe[i] == 0) ? 0x00 : 0x80 | (FERScfg[brd]->ProbeChannel[i] & 0x3F) | ((FERScfg[brd]->AnalogProbe[i] - 1) << 9);
			ret |= FERS_WriteRegister(handle, a_scbs_ctrl, i << 9);
			ret |= FERS_WriteRegister(handle, a_citiroc_probe, ctp);
			Sleep(10);
		}
		// Set Digital Probe
		if (FERS_FPGA_FW_MajorRev(handle) >= 5 || FERS_Code(handle) == 5204 || FERS_Code(handle) == 5205) {
			ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0);
			if (FERScfg[brd]->DigitalProbe[i] == DPROBE_OFF)				ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0xFF);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_LG)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x10);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_HG)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x11);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_HOLD)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x16);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_START_CONV)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x12);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_COMMIT)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x21);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_VALID)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x20);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_CLK_1024)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x00);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_VAL_WINDOW)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x1A);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_T_OR)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x04);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_Q_OR)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, 0x05);
			else if (FERScfg[brd]->DigitalProbe[i] & 0x80000000) {  // generic setting
				uint32_t blk = (FERScfg[brd]->DigitalProbe[i] >> 16) & 0xF;
				uint32_t sig = FERScfg[brd]->DigitalProbe[i] & 0xF;
				ret |= FERS_WriteRegisterSlice(handle, a_dprobe, i * 8, i * 8 + 7, (blk << 4) | sig);
			}
		} else {
			if (FERScfg[brd]->DigitalProbe[i] == DPROBE_OFF)				ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0xFF);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_LG)		ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x10);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_HG)		ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x11);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_HOLD)			ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x16);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_START_CONV)	ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x12);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_COMMIT)	ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x21);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_VALID)	ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x20);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_CLK_1024)		ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x00);
			else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_VAL_WINDOW)	ret |= FERS_WriteRegisterSlice(handle, a_citiroc_probe, 12, 18, 0x1A);
			else if (FERScfg[brd]->DigitalProbe[i] & 0x80000000)			ret |= FERS_WriteRegister(handle, a_citiroc_probe, FERScfg[brd]->DigitalProbe[i] & 0x7FFFFFFF);  // generic setting
		}
	}
	Sleep(10);
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Read the SC bit stream (1144 bits) from the board (referred to last programmed chip)
// Inputs:		handle = board handle
// Outputs:		SCbs: bit stream (36 words of 32 bits)
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
static int ReadSCbsFromChip(int handle, int chip, uint32_t *SCbs)
{
	int ret = 0, i;
	uint32_t d32; 
	uint32_t src = 0; // 0=from chip, 1=from FPGA
	uint32_t man;
	int brd = FERS_INDEX(handle);

	ret |= FERS_ReadRegister(handle, a_scbs_ctrl, &d32);
	man = (FERScfg[brd]->CitirocCfgMode == CITIROC_CFG_FROM_FILE) ?	0x100 : 0;
	ret |= FERS_WriteRegister(handle, a_scbs_ctrl, (src << 10) | (chip << 9) | man);
	FERS_SendCommand(handle, CMD_CFG_ASIC);  // rewrite bit stream to read it out from chip
	for(i=0; i<NW_SCBS; i++) {
		ret |= FERS_WriteRegister(handle, a_scbs_ctrl, (src << 10) | (chip << 9) | man | i);
		ret |= FERS_ReadRegister(handle, a_scbs_data, &SCbs[i]);
	}
	ret |= FERS_WriteRegister(handle, a_scbs_ctrl, d32);
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Write the SC bit stream (1144 bits) into one Citiroc chip
// Inputs:		handle = board handle
//              chip = Citiroc index (0 or 1)
// Outputs:		SCbs: bit stream (36 words of 32 bits)
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
static int WriteSCbsToChip(int handle, int chip, uint32_t *SCbs)
{
	int ret = 0, i;
	uint32_t d32; 
	ret |= FERS_ReadRegister(handle, a_scbs_ctrl, &d32);
	for(i=0; i<NW_SCBS; i++) {
		ret |= FERS_WriteRegister(handle, a_scbs_ctrl, chip << 9 | 0x100 | i);
		ret |= FERS_WriteRegister(handle, a_scbs_data, SCbs[i]);
	}
	ret |= FERS_SendCommand(handle, CMD_CFG_ASIC);  // Configure Citiroc (0 or 1 depending on bit 9 of a_scbs_ctrl
	ret |= FERS_WriteRegister(handle, a_scbs_ctrl, d32);
	Sleep(10);
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Read the SC bit stream (1144 bits) from text file (sequence of 0 and 1)
// Inputs:		filename = input file
// Outputs:		SCbs: bit stream (36 words of 32 bits)
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
static int ReadSCbsFromFile(char *filename, uint32_t *SCbs)
{
	FILE *scf;
	char c;
	int i;
	if ((scf = fopen(filename, "r")) == NULL) return -1;
	memset(SCbs, 0, NW_SCBS * sizeof(uint32_t));
	for(i=0; i<1144; i++) {
		int sret = fscanf(scf, "%c", &c);
		if ((c != '0') && (c != '1')) {
			//if (!SockConsole) printf("Invalid CS file (%s)\n", filename);
			return -1;
		}
		if (c == '1')
			SCbs[i/32] |= (1 << (i%32));
	}
	fclose(scf);
	return 0;
}


// ---------------------------------------------------------------------------------
// Description: Read the SC bit stream (1144 bits) from text file (sequence of 0 and 1)
// Inputs:		filename = input file
//				SCbs: bit stream (36 words of 32 bits)
// Outputs:		-
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
static int WriteCStoFile(char *filename, uint32_t *SCbs)
{
	FILE *scf;
	char c;
	int i;
	if ((scf = fopen(filename, "w")) == NULL) return -1;
	for(i=0; i<1144; i++) {
		c = '0' + ((SCbs[i/32] >> (i%32)) & 1);
		fprintf(scf, "%c", c);
	}
	fclose(scf);
	return 0;
}


static uint16_t CSslice(uint32_t *SCbs, int start, int stop)
{
	int a = start/32;
	uint64_t d64;
	uint64_t one = 1;

	if ((start < 0) || (start > 1143) || (stop < 0) || (stop > 1143)) return 0; 
	if (a < (NW_SCBS-1))
		d64 = ((uint64_t)SCbs[a+1] << 32) | (uint64_t)SCbs[a];
	else
		d64 = (uint64_t)SCbs[a];
	d64 = d64 >> (start % 32);
	d64 = d64 & ((one << (stop-start+1)) - 1);
	return (uint16_t)d64;
}


// ---------------------------------------------------------------------------------
// Description: Write a text file with the Citiroc Settings
// Inputs:		filename: output file name
//				SCbs: configuration bit stream
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
static int WriteCStoFileFormatted(char *filename, uint32_t *SCbs)
{
	FILE *of;
	int i, a, b;
	of = fopen(filename, "w");
	for(i=0; i<32; i++) {
		a = i*4;
		b = a+3;
		fprintf(of, "[%4d : %4d] (%2d) TD-FineThr[%02d] = %-6d (0x%X)\n", b, a, b-a+1, i, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	}
	for(i=0; i<32; i++) {
		a = 128+i*4;
		b = a+3;
		fprintf(of, "[%4d : %4d] (%2d) QD-FineThr[%02d] = %-6d (0x%X)\n", b, a, b-a+1, i, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	}
	a =  256; b =  256; fprintf(of, "[%4d : %4d] (%2d) -PWen QD       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  257; b =  257; fprintf(of, "[%4d : %4d] (%2d) -PWpm QD       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  258; b =  258; fprintf(of, "[%4d : %4d] (%2d) QDiscr latch   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  259; b =  259; fprintf(of, "[%4d : %4d] (%2d) -PWen TD       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  260; b =  260; fprintf(of, "[%4d : %4d] (%2d) -PWpm TD       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  261; b =  261; fprintf(of, "[%4d : %4d] (%2d) -PWen fineQthr = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  262; b =  262; fprintf(of, "[%4d : %4d] (%2d) -PWpm fineQthr = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  263; b =  263; fprintf(of, "[%4d : %4d] (%2d) -PWen fineTthr = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  264; b =  264; fprintf(of, "[%4d : %4d] (%2d) -PWpm fineTthr = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  265; b =  296; fprintf(of, "[%4d : %4d] (%2d) Discr Mask     = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  297; b =  297; fprintf(of, "[%4d : %4d] (%2d) -PWpm T&H HG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  298; b =  298; fprintf(of, "[%4d : %4d] (%2d) -PWen T&H HG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  299; b =  299; fprintf(of, "[%4d : %4d] (%2d) -PWpm T&H LG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  300; b =  300; fprintf(of, "[%4d : %4d] (%2d) -PWen T&H LG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  301; b =  301; fprintf(of, "[%4d : %4d] (%2d) SCA bias       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  302; b =  302; fprintf(of, "[%4d : %4d] (%2d) -PWpm PkD HG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  303; b =  303; fprintf(of, "[%4d : %4d] (%2d) -PWen PkD HG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  304; b =  304; fprintf(of, "[%4d : %4d] (%2d) -PWpm PkD LG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  305; b =  305; fprintf(of, "[%4d : %4d] (%2d) -PWen PkD LG   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  306; b =  306; fprintf(of, "[%4d : %4d] (%2d) Pk-det mode HG = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  307; b =  307; fprintf(of, "[%4d : %4d] (%2d) Pk-det mode LG = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  308; b =  308; fprintf(of, "[%4d : %4d] (%2d) PkS Ctrl Logic = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  309; b =  309; fprintf(of, "[%4d : %4d] (%2d) PkS Trg Source = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  310; b =  310; fprintf(of, "[%4d : %4d] (%2d) -PWpm Fast ShF = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  311; b =  311; fprintf(of, "[%4d : %4d] (%2d) -PWen Fast Sh  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  312; b =  312; fprintf(of, "[%4d : %4d] (%2d) -PWpm Fast Sh  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  313; b =  313; fprintf(of, "[%4d : %4d] (%2d) -PWpm LG Sl Sh = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  314; b =  314; fprintf(of, "[%4d : %4d] (%2d) -PWen LG Sl Sh = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  315; b =  317; fprintf(of, "[%4d : %4d] (%2d) LG Shap Time   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  318; b =  318; fprintf(of, "[%4d : %4d] (%2d) -PWpm HG Sl Sh = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  319; b =  319; fprintf(of, "[%4d : %4d] (%2d) -PWen HG Sl Sh = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  320; b =  322; fprintf(of, "[%4d : %4d] (%2d) HG Shap Time   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  323; b =  323; fprintf(of, "[%4d : %4d] (%2d) LG preamp bias = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  324; b =  324; fprintf(of, "[%4d : %4d] (%2d) -PWpm HG PA    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  325; b =  325; fprintf(of, "[%4d : %4d] (%2d) -PWen HG PA    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  326; b =  326; fprintf(of, "[%4d : %4d] (%2d) -PWpm LG PA    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  327; b =  327; fprintf(of, "[%4d : %4d] (%2d) -PWen LG PA    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  328; b =  328; fprintf(of, "[%4d : %4d] (%2d) PA Fast Shaper = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  329; b =  329; fprintf(of, "[%4d : %4d] (%2d) En Input DAC   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a =  330; b =  330; fprintf(of, "[%4d : %4d] (%2d) 8bit Dac Ref   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	for(i=0; i<32; i++) {
		a = 331+i*9;
		b = a+8;
		fprintf(of, "[%4d : %4d] (%2d) HV-adjust[%02d]  = %-6d (0x%X)\n", b, a, b-a+1, i, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	}
	for(i=0; i<32; i++) {
		a = 619+i*15;
		b = a+14;
		fprintf(of, "[%4d : %4d] (%2d) Preamp-Cfg[%02d] = %-6d (0x%X)\n", b, a, b-a+1, i, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	}
	a = 1099; b = 1099; fprintf(of, "[%4d : %4d] (%2d) -PWpm TempSens = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1100; b = 1100; fprintf(of, "[%4d : %4d] (%2d) -PWen TempSens = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1101; b = 1101; fprintf(of, "[%4d : %4d] (%2d) -PWpm Bandgap  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1102; b = 1102; fprintf(of, "[%4d : %4d] (%2d) -PWen Bandgap  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1103; b = 1103; fprintf(of, "[%4d : %4d] (%2d) -PWen Q-DAC    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1104; b = 1104; fprintf(of, "[%4d : %4d] (%2d) -PWpm Q-DAC    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1105; b = 1105; fprintf(of, "[%4d : %4d] (%2d) -PWen T-DAC    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1106; b = 1106; fprintf(of, "[%4d : %4d] (%2d) -PWpm T-DAC    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1107; b = 1116; fprintf(of, "[%4d : %4d] (%2d) QD coarse Thr  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1117; b = 1126; fprintf(of, "[%4d : %4d] (%2d) TD coarse Thr  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1127; b = 1127; fprintf(of, "[%4d : %4d] (%2d) -PWen HG OTA   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1128; b = 1128; fprintf(of, "[%4d : %4d] (%2d) -PWpm HG OTA   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1129; b = 1129; fprintf(of, "[%4d : %4d] (%2d) -PWen LG OTA   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1130; b = 1130; fprintf(of, "[%4d : %4d] (%2d) -PWpm LG OTA   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1131; b = 1131; fprintf(of, "[%4d : %4d] (%2d) -PWen Prb OTA  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1132; b = 1132; fprintf(of, "[%4d : %4d] (%2d) -PWpm Prb OTA  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1133; b = 1133; fprintf(of, "[%4d : %4d] (%2d) OTA bias       = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1134; b = 1134; fprintf(of, "[%4d : %4d] (%2d) -PWen Val Evt  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1135; b = 1135; fprintf(of, "[%4d : %4d] (%2d) -PWpm Val Evt  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1136; b = 1136; fprintf(of, "[%4d : %4d] (%2d) -PWen Raz Chn  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1137; b = 1137; fprintf(of, "[%4d : %4d] (%2d) -PWpm Raz Chn  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1138; b = 1138; fprintf(of, "[%4d : %4d] (%2d) -PWen Out Dig  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1139; b = 1139; fprintf(of, "[%4d : %4d] (%2d) -PWen Out32    = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1140; b = 1140; fprintf(of, "[%4d : %4d] (%2d) -PWen Out32OC  = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1141; b = 1141; fprintf(of, "[%4d : %4d] (%2d) Trg Polarity   = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1142; b = 1142; fprintf(of, "[%4d : %4d] (%2d) -PWen Out32TOC = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	a = 1143; b = 1143; fprintf(of, "[%4d : %4d] (%2d) -PWen ChTrgOut = %-6d (0x%X)\n", b, a, b-a+1, CSslice(SCbs, a, b), CSslice(SCbs, a, b));
	fclose(of);
	return 0;
}


// ---------------------------------------------------------------------------------
// Description: Write a text file with the content of all regsiters
// Inputs:		handle: board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
#define NUMREG5202				77

int FERS_DumpBoardRegister5202(int handle, char *filename) {
	uint32_t reg_add = 0;
	uint32_t reg_val = 0;
	int r, i, nreg_ch = 1;
	int brd = FERS_INDEX(handle);

	const char FPGARegAdd[NUMREG5202][2][100] = {
	{"a_acq_ctrl",         "01000000"},   //  Acquisition Control Register
	{"a_run_mask",         "01000004"},   //  Run mask: bit[0]=SW, bit[1]=T0-IN
	{"a_trg_mask",         "01000008"},   //  Global trigger mask: bit[0]=SW, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=Maj, bit[4]=Periodic
	{"a_tref_mask",        "0100000C"},   //  Tref mask: bit[0]=T0-IN, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=T-OR, bit[5]=Periodic
	{"a_t0_out_mask",      "01000014"},   //  T0 out mask
	{"a_t1_out_mask",      "01000018"},   //  T1 out mask
	{"a_veto_mask",        "0100001C"},   //  Veto mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN, 
	{"a_tref_delay",       "01000048"},   //  Delay of the time reference for the coincidences
	{"a_tref_window",      "0100004C"},   //  Tref coincidence window (for list mode)
	{"a_dwell_time",       "01000050"},   //  Dwell time (periodic trigger) in clk cyclces. 0 => OFF
	{"a_list_size",        "01000054"},   //  Number of 32 bit words in a packet in timing mode (list mode)
	{"a_pck_maxcnt",       "01000064"},   //  Max num of packets transmitted to the TDL
	{"a_channel_mask_0",   "01000100"},   //  Input Channel Mask (ch  0 to 31)
	{"a_channel_mask_1",   "01000104"},   //  Input Channel Mask (ch 32 to 63)
	{"a_fw_rev",           "01000300"},   //  Firmware Revision 
	{"a_acq_status",       "01000304"},   //  Acquisition Status
	{"a_real_time",        "01000308"},   //  Real Time in ms
	{"a_dead_time",        "01000310"},   //  Dead Time in ms
	{"a_fpga_temp",        "01000348"},   //  FPGA die Temperature 
	{"a_pid",              "01000400"},	  //  PID
	{"a_pcb_rev",          "01000404"},	  //  PCB revision
	{"a_fers_code",        "01000408"},	  //  Fers CODE (5202)
	{"a_rebootfpga",       "0100FFF0"},	  //  reboot FPGA from FW uploader
	{"a_test_led",         "01000228"},   //  LED test mode
	{"a_tdc_mode",         "0100022C"},   //  TDC Mode
	{"a_tdc_data",         "01000230"},   //  TDC Data
	{"a_sw_compatib",      "01004000"},   //  SW compatibility
	{"a_commands",         "01008000"},   //  Send Commands (for Eth and USB)
	{"a_validation_mask",  "01000010"},   //  Validation mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN
	{"a_acq_ctrl2",        "01000020"},   //  Acquisition Control Register 2
	{"a_dprobe_5202",      "01000068"},   //  Digital probes (signal inspector)
	{"a_citiroc_cfg",      "01000108"},   //  Citiroc common configuration bits
	{"a_citiroc_en",       "0100010C"},   //  Citiroc internal parts enable mask 
	{"a_citiroc_probe",    "01000110"},   //  Citiroc probes (signal inspector)
	{"a_qd_coarse_thr",    "01000114"},   //  Coarse threshold for the Citiroc Qdiscr (10 bit DAC)
	{"a_td_coarse_thr",    "01000118"},   //  Coarse threshold for the Citiroc Tdiscr (10 bit DAC)
	{"a_lg_sh_time",       "0100011C"},   //  Low gain shaper time constant
	{"a_hg_sh_time",       "01000120"},   //  High gain shaper time constant
	{"a_hold_delay",       "01000124"},   //  Time from gtrg to peak hold
	{"a_amux_seq_ctrl",    "01000128"},   //  Timing parameters for the analog mux readout and conversion
	{"a_wave_length",      "0100012C"},   //  Waveform Length
	{"a_scbs_ctrl",        "01000130"},	  //  Citiroc SC bit stream index and select
	{"a_scbs_data",        "01000134"},	  //  Citiroc SC bit stream data
	{"a_qdiscr_mask_0",    "01000138"},	  //  Charge Discriminator mask
	{"a_qdiscr_mask_1",    "0100013C"},	  //  Charge Discriminator mask
	{"a_tdiscr_mask_0",    "01000140"},	  //  Time Discriminator mask
	{"a_tdiscr_mask_1",    "01000144"},	  //  Time Discriminator mask
	{"a_tpulse_ctrl",      "01000200"},   //  Test pulse mask
	{"a_tpulse_dac",       "01000204"},   //  Internal Test Pulse amplitude
	{"a_hv_regaddr",       "01000210"},   //  HV Register Address and Data Type
	{"a_hv_regdata",       "01000214"},   //  HV Register Data
	{"a_trgho",            "01000218"},   //  Trigger Hold off
	{"a_dc_offset",        "01000220"},   //  DAC for DC offset
	{"a_spi_data",         "01000224"},   //  SPI R/W data (for Flash Memory access)
	{"a_test_led",         "01000228"},   //  Test Mode for LEDs
	{"a_tdc_mode",         "0100022C"},   //  R/W   C   32    TDC modes
	{"a_tdc_data",         "01000230"},   //  R/W   C   32    Regs of TDC
	{"a_tlogic_def",       "01000234"},   //  Trigger Logic Definition
	{"a_tlogic_width",     "0100023C"},   //  Monostable for Trigger logic output
	{"a_hit_width",        "01000238"},   //  Monostable for CR triggers
	{"a_i2c_addr_5202",    "01000240"},   //  I2C Addr
	{"a_i2c_data_5202",    "01000244"},   //  I2C Data
	{"a_t_or_cnt",         "01000350"},	  //  T-OR counter
	{"a_q_or_cnt",         "01000354"},	  //  Q-OR counter
	{"a_hv_Vmon",          "01000356"},   //  HV Vmon
	{"a_hv_Imon",          "01000358"},   //  HV Imon
	{"a_hv_status",        "01000360"},   //  HV Status
	{"a_uC_status",        "01000600"},   //  uC status
	{"a_uC_shutdown",      "01000604"},   //  uC shutdown
	{"a_zs_lgthr",         "02000000"},   //  Threshold for zero suppression (LG)
	{"a_zs_hgthr",         "02000004"},   //  Threshold for zero suppression (HG)
	{"a_qd_fine_thr",      "02000008"},   //  Fine individual threshold for the Citiroc Qdiscr (4 bit DAC)
	{"a_td_fine_thr",      "0200000C"},   //  Fine individual threshold for the Citiroc Tdiscr (4 bit DAC)
	{"a_lg_gain",          "02000010"},   //  Preamp Low Gain Setting
	{"a_hg_gain",          "02000014"},   //  Preamp High Gain Setting
	{"a_hv_adj",           "02000018"},   //  HV individual adjust (8 bit DAC)
	{"a_hitcnt",           "02000800"}    //  Hit counters 
	};

	FILE* dumpReg = NULL;
	dumpReg = fopen(filename, "w");
	if (dumpReg == NULL)
		return -1;

	for (r = 0; r < NUMREG5202; r++) {
		uint32_t base_add;
		sscanf(FPGARegAdd[r][1], "%x", &base_add);
		if ((base_add & 0xFF000000) == 0x02000000) nreg_ch = FERS_BoardInfo[brd]->NumCh;
		else nreg_ch = 1;
		for (i = 0; i < nreg_ch; i++) {
			reg_add = base_add + (i << 16);
			FERS_ReadRegister(handle, reg_add, &reg_val);
			fprintf(dumpReg, "0x%08X : 0x%08X  (%s) \n", reg_add, reg_val, FPGARegAdd[r][0] + 2);
		}
	}
	fclose(dumpReg);
	return 0;
}
