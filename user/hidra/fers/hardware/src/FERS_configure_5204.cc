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
#include "FERSlib.h"


static int Configure_Radioroc(int handle);
static int Configure_Psiroc(int handle);
static int Configure_XROC_from_file(char* fname, int handle);
static int save_XROC_Config_to_file(char* fname, int handle);



int Configure5204(int handle, int mode) {
	int i, ret = 0;
	picoTDC_Cfg_t pcfg;
	double trgwin=0;

	uint32_t d32, Tpulse_ctrl = 0;
	char CfgStep[100];
	int brd = FERS_INDEX(handle);

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


	// Enable Zero suppression in counting mode (only with FW >= 4.00)
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 30, 30, FERScfg[brd]->EnableCntZeroSuppr);
	// Service event Mode
	if (FERScfg[brd]->AcquisitionMode == ACQMODE_COUNT)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 18, 19, FERScfg[brd]->EnableServiceEvents & 0x1);
	else
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 18, 19, FERScfg[brd]->EnableServiceEvents);

	// Disable Autoscan
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 31, 31, 1);

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 24, 25, FERScfg[brd]->Validation_Mode); // 0=disabled, 1=accept, 2=reject

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 27, 29, FERScfg[brd]->Counting_Mode); // 0=singles, 1=paired_AND
	ret |= FERS_WriteRegister(handle, a_hit_width, (uint32_t)(FERScfg[brd]->ChTrg_Width / CLK_PERIOD[brd])); // Monostable on Citiroc Self triggers => Coinc Window for Trigger logic and counting in paired-AND mode
	ret |= FERS_WriteRegister(handle, a_tlogic_width, (uint32_t)(FERScfg[brd]->Tlogic_Width / CLK_PERIOD[brd])); // Monostable on Trigger Logic Output (0=linear)

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
	if (FERScfg[brd]->T0_outMask == 0x2000) // XROC_TRG_ASYN
		ret |= FERS_WriteRegister(handle, 0x0100006C, FERScfg[brd]->ProbeChannel[0]);  // HACK CTIN: manage channel index from the same reg of the DPROBE (???)
	ret |= FERS_WriteRegister(handle, a_t1_out_mask, FERScfg[brd]->T1_outMask);
	if (FERScfg[brd]->T1_outMask == 0x2000) // XROC_TRG_ASYN
		ret |= FERS_WriteRegister(handle, 0x0100006C, FERScfg[brd]->ProbeChannel[1]);

	// Waveform length
	if (FERScfg[brd]->WaveformLength > 0) {
		ret |= FERS_WriteRegister(handle, a_wave_length, FERScfg[brd]->WaveformLength);
	}
	// Set Periodic Trigger
	ret |= FERS_WriteRegister(handle, a_dwell_time, (uint32_t)(FERScfg[brd]->PtrgPeriod / CLK_PERIOD[brd]));
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
		//if (FERScfg[brd]->T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T0-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (FERScfg[brd]->StartRunMode == STARTRUN_CHAIN_T1) {
		if (FERS_INDEX(handle) == 0) {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x01);
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1 << 10);  // set T1-OUT = RUN_SYNC
		} else {
			ret |= FERS_WriteRegister(handle, a_run_mask, (deskew << 16) | (rundelay << 8) | 0x84);
			ret |= FERS_WriteRegister(handle, a_t1_out_mask, 1);  // set T1-OUT = T1-IN
		}
		//if (FERScfg[brd]->T0_outMask != (1 << 4)) Con_printf("LCSw", "WARNING: T1-OUT setting has been overwritten for Start daisy chaining\n");
	} else if (FERScfg[brd]->StartRunMode == STARTRUN_TDL) {
		ret |= FERS_WriteRegister(handle, a_run_mask, 0x01);
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
	} else {
		Tpulse_ctrl = FERScfg[brd]->TestPulseSource;
		ret |= FERS_WriteRegister(handle, a_tpulse_ctrl, Tpulse_ctrl);
		ret |= FERS_WriteRegister(handle, a_tpulse_dac, FERScfg[brd]->TestPulseAmplitude);
	}
	ConfigureProbe(handle);
	// Set Digital Probe in concentrator (if present)
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		if (FERScfg[brd]->CncProbe_A >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FA_FN, VR_IO_FUNCTION_ZERO);  // Set FA function = ZERO
		if (FERScfg[brd]->CncProbe_B >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FB_FN, VR_IO_FUNCTION_ZERO);  // Set FB function = ZERO
		FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_DEBUG, FERScfg[brd]->CncProbe_A | (FERScfg[brd]->CncProbe_B << 8));
	}


	// Set trigger mask in FPGA Tlogic, not in XROC ASIC
	ret |= FERS_WriteRegister(handle, a_tlogic_mask_0, (uint32_t)(FERScfg[brd]->Tlogic_Mask & 0xFFFFFFFF));
	ret |= FERS_WriteRegister(handle, a_tlogic_mask_1, (uint32_t)((FERScfg[brd]->Tlogic_Mask >> 32) & 0xFFFFFFFF));

	ret |= FERS_WriteRegister(handle, a_hold_delay, (uint32_t)(FERScfg[brd]->HoldDelay / CLK_PERIOD[brd]));  // Time between trigger and peak sensing Hold

	if (FERScfg[brd]->MuxClkPeriod > 0)
		ret |= FERS_WriteRegisterSlice(handle, a_amux_seq_ctrl, 0, 6, (uint32_t)(FERScfg[brd]->MuxClkPeriod / CLK_PERIOD[brd]));		// Period of the mux clock
	ret |= FERS_WriteRegisterSlice(handle, a_amux_seq_ctrl, 8, 9, FERScfg[brd]->MuxNSmean);

	// Set Trigger Hold-off (for channel triggers)
	FERS_WriteRegister(handle, a_trgho, (uint32_t)(FERScfg[brd]->TrgHoldOff / CLK_PERIOD[brd]));

	if (ret) goto abortcfg;

	// ########################################################################################################
	sprintf(CfgStep, "Channel pedestal and ZS settings");
	// ########################################################################################################	
	for (i = 0; i < FERSLIB_MAX_NCH_5204; i++) {
		uint16_t PedHG, PedLG, zs_thr_lg, zs_thr_hg, ediv;

		FERS_GetChannelPedestalBeforeCalib(handle, i, &PedLG, &PedHG);
		//if (FERScfg[brd]->EHistoNbin > 0)
		//	ediv = FERScfg[brd]->Range_14bit ? ((1 << 14) / FERScfg[brd]->EHistoNbin) : ((1 << 13) / FERScfg[brd]->EHistoNbin);
		//else
		//	ediv = 1;
		ediv = 1; // HACK CTIN: ediv tiene conto del rebinning, dato che la libreria non conosce. Capire come gestire la cosa (vedi anche A5202)
		if (FERScfg[brd]->AcquisitionMode == ACQMODE_SPECT) {  // CTIN: ZS doesn't work in TSPEC mode (indeed ZS applies to energy only, not timing data. It would be a partial suppression)
			zs_thr_lg = (FERScfg[brd]->ZS_Threshold_LG[i] > 0) ? FERScfg[brd]->ZS_Threshold_LG[i] * ediv - FERScfg[brd]->Pedestal + PedLG : 0;
			ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_zs_lgthr, i), zs_thr_lg);
			zs_thr_hg = ((FERScfg[brd]->ZS_Threshold_HG[i] > 0)) ? FERScfg[brd]->ZS_Threshold_HG[i] * ediv - FERScfg[brd]->Pedestal + PedHG : 0;
			ret |= FERS_WriteRegister(handle, INDIV_ADDR(a_zs_hgthr, i), zs_thr_hg);
		}
		if (ret) break;
	}
	if (ret) goto abortcfg;

	// ########################################################################################################
	sprintf(CfgStep, "Configure weeroc ASIC");
	// ########################################################################################################
	if (FERS_BoardInfo[brd]->FERSCode == 5204)
		ret = Configure_Radioroc(handle);
	if (FERS_BoardInfo[brd]->FERSCode == 5205)
		ret = Configure_Psiroc(handle);

	//ret = Configure_XROC_from_file("weeroc_cfg.txt", handle);
	save_XROC_Config_to_file("xroc_cfg.txt", handle);
	if (ret) goto abortcfg;

	// ########################################################################################################
	sprintf(CfgStep, "Configure picoTDC");
	// ########################################################################################################
	ret = 0;
	
	// Write default cfg
	Set_picoTDC_Default(&pcfg);
	Write_picoTDC_Cfg(handle, 0, pcfg, 1);
	
	// Initialize PLL with AFC
	pcfg.PLL2.bits.pll_afcrst = 1;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_PLL2, pcfg.PLL2.reg);
	pcfg.PLL2.bits.pll_afcrst = 0;
	pcfg.PLL2.bits.pll_afcstart = 1;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_PLL2, pcfg.PLL2.reg);
	Sleep(10); // Wait for lock 
	pcfg.PLL2.bits.pll_afcstart = 0;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_PLL2, pcfg.PLL2.reg);
	
	// Initialize DLL
	pcfg.DLL_TG.bits.dll_fixctrl = 1;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);
	Sleep(10); // Wait for lock 
	pcfg.DLL_TG.bits.dll_fixctrl = 0;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);
	
	// Turn everything ON
	pcfg.Control.bits.magic_word = 0x5C;
	pcfg.Control.bits.digital_reset = 1;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_Control, pcfg.Control.reg);
	pcfg.Control.bits.digital_reset = 0;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(0), a_pTDC_Control, pcfg.Control.reg);
	if (ret < 0) goto abortcfg;
	
	// ########################################################################################################
	sprintf(CfgStep, "picoTDC setting");
	// ########################################################################################################
	// Header 
	pcfg.Header.bits.relative = 1;			// Time stamp relative to trigger window
	pcfg.Header.bits.header_fields0 = FERScfg[brd]->HeaderField0;	// Header field0 = 4 = near full flags (ch 0..7 of the port) - 00000CCCCCCCC
	pcfg.Header.bits.header_fields1 = FERScfg[brd]->HeaderField1;	// Header field1 = 5 = near full flags (ch 8..15 of the port + RO_buff, TrgBuff) - RT000CCCCCCCC
	pcfg.Header.bits.untriggered = 0;

	// Acquisition Mode
	//double trgwin=0;
	if (FERScfg[brd]->AcquisitionMode == ACQMODE_TIMING_GATED) {
		if (FERScfg[brd]->GateWidth < 50000 - 7 * TDC_CLK_PERIOD) {	// the max TrgWindow is 50 us: GATE + extra clock (5+2)
			trgwin = (FERScfg[brd]->GateWidth / TDC_CLK_PERIOD) + 2;
		} else {
			trgwin = ((50000 / TDC_CLK_PERIOD) - 7 + 2);
			FERScfg[brd]->GateWidth = (uint32_t)((50000 / TDC_CLK_PERIOD) - 7);
		}
	}

	pcfg.TrgWindow.bits.trigger_window = (uint32_t)trgwin;
	if (FERScfg[brd]->AcquisitionMode == ACQMODE_TIMING_GATED) {
		ret |= FERS_WriteRegister(handle, a_trg_delay, (uint32_t)(2 * trgwin + 2));  // trg_delay is expressed in FPGA clock cycles = tdc_clk * 2
		pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(trgwin + 5);  // latency is expressed in tdc_clk cycles
	} else if (FERScfg[brd]->AcquisitionMode == ACQMODE_TIMING_STREAMING) {
		//pcfg.TrgWindow.bits.trigger_window = strm_ptrg / 2 - 1;  // HACK CTIN: sistemare modalità streaming
		//pcfg.TrgWindow.bits.trigger_latency = strm_ptrg / 2 + 1;
	}


	// Hit RX filters (Glitch Filter)
	if (FERScfg[brd]->GlitchFilterMode != GLITCHFILTERMODE_DISABLED) {
		pcfg.Hit_RX_TX.bits.hrx_top_delay = FERScfg[brd]->GlitchFilterDelay & 0xF;
		pcfg.Hit_RX_TX.bits.hrx_bot_delay = FERScfg[brd]->GlitchFilterDelay & 0xF;
		pcfg.Hit_RX_TX.bits.hrx_top_filter_leading = (FERScfg[brd]->GlitchFilterMode >> 1) & 1;
		pcfg.Hit_RX_TX.bits.hrx_top_filter_trailing = FERScfg[brd]->GlitchFilterMode & 1;
		pcfg.Hit_RX_TX.bits.hrx_bot_filter_leading = (FERScfg[brd]->GlitchFilterMode >> 1) & 1;
		pcfg.Hit_RX_TX.bits.hrx_bot_filter_trailing = FERScfg[brd]->GlitchFilterMode & 1;
	}

	pcfg.Buffers.bits.channel_buffer_size = 2;

	// Set Measurement mode (Enable ToT or leading edge)
	if (FERScfg[brd]->MeasMode != MEASMODE_LEAD_ONLY) {
		pcfg.Enable.bits.falling_en = 1;
		for (i = 0; i < 64; i++) {
			pcfg.Ch_Control[i].bits.falling_en_tm = 1;
		}
		if (MEASMODE_OWLT(FERScfg[brd]->MeasMode)) {
			pcfg.Trg0Del_ToT.bits.tot = 1;
			pcfg.Trg0Del_ToT.bits.tot_8bit = (FERScfg[brd]->MeasMode == MEASMODE_LEAD_TOT8) ? 1 : 0;
			pcfg.Trg0Del_ToT.bits.tot_saturate = 1;  //TOT will saturate if value bigger than dynamic range (otherwise overflow occurs)
			pcfg.Trg0Del_ToT.bits.tot_startbit = FERScfg[brd]->ToT_LSB;
			pcfg.Trg0Del_ToT.bits.tot_leadingstartbit = FERScfg[brd]->LeadTrail_LSB;
		}
	} else {
		pcfg.Enable.bits.falling_en = 0;
		for (i = 0; i < 64; i++) {
			pcfg.Ch_Control[i].bits.falling_en_tm = 0;
		}
	}

	// Channel offset 
	/*for (i = 0; i < 64; i++)
		pcfg.Ch_Control[i].bits.channel_offset = FERScfg[brd]->Ch_Offset[brd][i]; */

	// Channel RX Enable Mask
	uint32_t ChMask0, ChMask1;
	ChMask0 = FERScfg[brd]->ChEnableMask & 0xFFFFFFFF;
	ChMask1 = (FERScfg[brd]->ChEnableMask >> 32) & 0xFFFFFFFF;
	pcfg.Hit_RXen_T.bits.hrx_enable_t = ChMask0;
	pcfg.Hit_RXen_B.bits.hrx_enable_b = ChMask1;
	for (i = 0; i < 64; i++) {
		uint32_t mask = (i < 32) ? ChMask0 : ChMask1;
		pcfg.Ch_Control[i].bits.channel_enable = (mask >> (i % 32)) & 1;
	}

	// Write final configuration
	ret = Write_picoTDC_Cfg(handle, 0, pcfg, 0);
	//if (DebugLogs & 0x10000)
	//	Save_picoTDC_Cfg(handle, 0, "picoTDC_RegImg.txt");
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
	_setLastLocalError("Error at: %s. Exit Code = %d\n", CfgStep, ret);
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Configure analog and digital probes
// Inputs:		handle = board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int ConfigureProbe5204(int handle) {
	int brd = FERS_INDEX(handle);
	int i, ret = 0, en_aprobe;
	static int prev_aprobe = -1, prev_probch = -1;

	// Set Analog Probe (only probe0, probe1 doesn't exist for analog probe)
	if (((int)FERScfg[brd]->AnalogProbe[0] != prev_aprobe) || ((int)FERScfg[brd]->ProbeChannel[0] != prev_probch)) {  // do not apply settings if not changed (save time)
		en_aprobe = (FERScfg[brd]->AnalogProbe[0] > 0) && (FERScfg[brd]->AnalogProbe[0] <= 5) ? 1 : 0;
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl2, 3, 3, en_aprobe);
		for (i = 0; i < 64; i++) {
			if (en_aprobe && (i == (int)FERScfg[brd]->ProbeChannel[0])) {
				ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 0x42, 1 << (FERScfg[brd]->AnalogProbe[0] - 1)); // set probe enable mask 
			} else {
				ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 0x42, 0); // disable probe
			}
		}
		prev_aprobe = FERScfg[brd]->AnalogProbe[0];
		prev_probch = FERScfg[brd]->ProbeChannel[0];
	}

	// Set Digital Probe (NOTE: address 'd_probe' is the same for both 5202 and 5204 => use a_dprobe_5202)
	for (i = 0; i < 2; i++) {
		if (FERScfg[brd]->DigitalProbe[i] == DPROBE_OFF)				ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0xFF);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_START_CONV)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x12);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_COMMIT)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x21);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_VALID)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x20);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_CLK_1024)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x00);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_VAL_WINDOW)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x1A);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_HOLD) {
			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x10);
			ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 0x42, 0x1); // set XROC digital probe = Hold
		} else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_XROC_TRG) {
			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, 0x10);
			ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 0x42, 0x2); // set XROC digital probe = Trg
		} else if (FERScfg[brd]->DigitalProbe[i] & 0x80000000) {  // generic setting
			uint32_t blk = (FERScfg[brd]->DigitalProbe[i] >> 16) & 0xF;
			uint32_t sig = FERScfg[brd]->DigitalProbe[i] & 0xF;
			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8, i * 8 + 7, (blk << 4) | sig);
		}
		ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5202, i * 8 + 16, i * 8 + 21, FERScfg[brd]->ProbeChannel[i]);
	}

	Sleep(10);
	return ret;
}




// --------------------------------------------------------------------------------------------------------- 
// Description: configure Radioroc ASIC
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int Configure_Radioroc(int handle) {
	int ret = 0, i;
	int brd = FERS_INDEX(handle);
	uint8_t enmask;

	// some hard coded settings
	uint8_t PATcomp = 0;  // higher values correspond to slower response of the PAT; Weeroc suggests to keep it at 0. 
	uint8_t trg_hysteresis_T1 = 0;  
	uint8_t trg_hysteresis_T2 = 0;
	uint8_t hold_select = 1; // 0=internal, 1=external
	uint8_t en_delay = 0; 
	uint8_t trg_select = 0; // 0=glob_external, 1=local_T1, 2=local_T2, 3=local_TQ, 4=glob_OR_T1, 8=glob_OR_T2, 12=glob_OR_TQ

	// Common settings
	ret |= FERS_XROC_WriteRegister(handle, a_XR_ASIC_bias, 2, 0x74); //  trigger threshold calibration bias = 7 (suggested by Weeroc)

	uint32_t th1 = min(FERScfg[brd]->TD1_CoarseThreshold, 1023);
	uint32_t th2 = min(FERScfg[brd]->TD2_CoarseThreshold, 1023);
	uint32_t thq = min(FERScfg[brd]->QD_CoarseThreshold, 1023);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 1, th1 & 0xFF);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 2, ((th2 & 0x3F) << 2) | (th1 >> 8) & 0x3);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 3, ((thq & 0xF) << 4) | (th2 >> 6) & 0xF);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 4, (thq >> 4) & 0x3F);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 0xC, trg_select | (hold_select << 4) | (en_delay << 5) | (trg_hysteresis_T1 << 6) | (trg_hysteresis_T2 << 6));

	// Channel Settings (probe setting will be applied by the function ConfigureProbe5204)
	for (i = 0; i < 64; i++) {
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 0, (uint8_t)(min(FERScfg[brd]->HV_IndivAdj[i], 0xFF)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 1, (uint8_t)((PATcomp << 6) | min(FERScfg[brd]->T_Gain[i], 0x3F)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 2, (uint8_t)((min(FERScfg[brd]->LG_Gain[i], 0xF) << 4) | min(FERScfg[brd]->HG_Gain[i], 0xF)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 3, (uint8_t)((min(FERScfg[brd]->LG_ShapingTime_ind[i], 0xF) << 4) | min(FERScfg[brd]->HG_ShapingTime_ind[i], 0xF)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 4, (uint8_t)(min(FERScfg[brd]->TD1_FineThreshold[i], 0x3F)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 5, (uint8_t)(min(FERScfg[brd]->TD2_FineThreshold[i], 0x3F)));
		uint8_t en_TD1 = (uint8_t)((FERScfg[brd]->TD1_Mask >> i) & 1);
		uint8_t en_TD2 = (uint8_t)((FERScfg[brd]->TD2_Mask >> i) & 1);
		uint8_t en_QD =  (uint8_t)((FERScfg[brd]->QD_Mask >> i) & 1);
		uint8_t en_PA =  (uint8_t)((FERScfg[brd]->ChEnableMask >> i) & 1);
		enmask = 0x80 | (0x60 * en_PA) | (en_TD1 << 4) | (en_TD2 << 3) | (en_QD << 2) | (0x03 * en_PA);
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 6, enmask);
		uint8_t en_ctest = (uint8_t)((FERScfg[brd]->TestPulseDestination == i));
		enmask = (en_ctest << 4) | 0xF;
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 7, enmask); // enable mask (need to set bit 4 = test input)
		ret |= FERS_XROC_WriteRegister(handle, a_XR_out_cfg, 46, 0x1F); // enable mux out
	}

	// Outing settings
	for (i = 0; i < 64; i++) {
		ret |= FERS_XROC_WriteRegister(handle, a_XR_out_cfg, 63 - i, 0x10); // Out = T1 LVDS (NOTE: channels are swapped on PCB: 0 -> 63, 1 -> 62, etc...)
	}
	ret |= FERS_XROC_WriteRegister(handle, a_XR_out_cfg, 0x46, 0x1F); // Enable ON_outing, ON_aBuffer, EN_probe, EN_MuxOut

	// Force ValEvent
	ret |= FERS_XROC_WriteRegister(handle, a_XR_event_val, 0x00, 0x01); // 

	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: configure Psiroc ASIC
// Inputs:		handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int Configure_Psiroc(int handle) {
	int ret = 0, i;
	int brd = FERS_INDEX(handle);
	uint8_t enmask;


	uint32_t th_td = min(FERScfg[brd]->TD_CoarseThreshold, 1023);
	uint32_t th_totd = min(FERScfg[brd]->TOTD_CoarseThreshold, 1023);
	uint32_t thq = min(FERScfg[brd]->QD_CoarseThreshold, 1023);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 1, th_td & 0xFF);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 2, ((th_totd & 0x3F) << 2) | (th_td >> 8) & 0x3);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 3, ((thq & 0xF) << 4) | (th_totd >> 6) & 0xF);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 4, (thq >> 4) & 0x3F);
	ret |= FERS_XROC_WriteRegister(handle, a_XR_common_cfg, 0xC, 0x10);  // use external trigger and external hold


	// Channel Settings (probe setting will be applied by the function ConfigureProbe5204)
	for (i = 0; i < 64; i++) {
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 0, (uint8_t)(min(FERScfg[brd]->PAQ_Gain[i], 0x3F) | ((FERScfg[brd]->InputPolarity[i] & 0x1) << 6)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 1, (uint8_t)(min(FERScfg[brd]->PAQ_Comp[i], 0x3F)));
		// ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 2, (uint8_t)()); HACK CTIN to do... (shaper Tau range, detector leakage current, fast chaper compens)
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 3, (uint8_t)((min(FERScfg[brd]->LG_ShapingTime_ind[i], 0xF) << 4) | min(FERScfg[brd]->HG_ShapingTime_ind[i], 0xF)));

		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 4, (uint8_t)(min(FERScfg[brd]->TD_FineThreshold[i], 0x3F)));
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 5, (uint8_t)(min(FERScfg[brd]->TOTD_FineThreshold[i], 0x3F)));
		uint8_t en_TD = (uint8_t)((FERScfg[brd]->TD_Mask >> i) & 1);
		uint8_t en_TOTD = (uint8_t)((FERScfg[brd]->TOTD_Mask >> i) & 1);
		uint8_t en_QD = (uint8_t)((FERScfg[brd]->QD_Mask >> i) & 1);
		uint8_t en_PA = (uint8_t)((FERScfg[brd]->ChEnableMask >> i) & 1);
		uint8_t en_FS = (uint8_t)((FERScfg[brd]->ChEnableMask >> i) & 1);
		enmask = (en_FS << 4) | (en_PA << 3) | (en_TD << 2) | (en_TOTD << 1) | en_QD;
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 6, enmask);
		uint8_t en_ctest = (uint8_t)((FERScfg[brd]->TestPulseDestination == i));
		enmask = (en_ctest << 4) | 0xF;
		ret |= FERS_XROC_WriteRegister(handle, a_XR_ChControl(i), 7, enmask); // enable mask (need to set bit 4 = test input)
	}

	// Outing settings
	for (i = 0; i < 64; i++) {
		ret |= FERS_XROC_WriteRegister(handle, a_XR_out_cfg, 63 - i, 0x10); // Out = T1 LVDS (NOTE: channels are swapped on PCB: 0 -> 63, 1 -> 62, etc...)
	}
	ret |= FERS_XROC_WriteRegister(handle, a_XR_out_cfg, 0x46, 0x1F); // Enable ON_outing, ON_aBuffer, EN_probe, EN_MuxOut

	// Force ValEvent
	ret |= FERS_XROC_WriteRegister(handle, a_XR_event_val, 0x00, 0x01); // 

	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: configure XROC ASIC taking registers from file (3 columns, hex_addr, hex_subddr, hex_data)
// Inputs:		fname = register file name
//				handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int Configure_XROC_from_file(char* fname, int handle) {
	int ret = 0, nn;
	int addr, subaddr, data;
	uint8_t d8;
	FILE* fin;

	fin = fopen(fname, "r");
	if (fin == NULL) return -1;

	while (!feof(fin)) {
		//nn = fscanf(fin, "%x %x %x", &addr, &subaddr, &data);
		//nn = fscanf(fin, "%d, %d, %s", &addr, &subaddr, str);
		nn = fscanf(fin, "%x %x %x", &addr, &subaddr, &data);
		//data = strtol(str, NULL, 2);
		if (nn != 3) break;
		if ((addr < 0) || (addr > 0x43))
			continue;
		d8 = (uint8_t)data;
		ret |= FERS_XROC_WriteRegister(handle, addr, subaddr, d8);
	}
	fclose(fin);
	return ret;
}

// --------------------------------------------------------------------------------------------------------- 
// Description: save XROC ASIC registers to file (3 columns, hex_addr, hex_subddr, hex_data)
// Inputs:		fname = register file name
//				handle = device handle
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
static int save_XROC_Config_to_file(char* fname, int handle) {
	int ret = 0, addr, subaddr, num_subaddr = 0;
	uint8_t data;
	FILE* fout;

	fout = fopen(fname, "w");
	if (fout == NULL) return -1;

	for (addr = 0; addr <= 0x43; addr++) {
		if (addr <= 0x3F) num_subaddr = 9;
		else if (addr == 0x40) num_subaddr = 15;
		else if (addr == 0x41) num_subaddr = 14;
		else if (addr == 0x42) num_subaddr = 71;
		else if (addr == 0x43) num_subaddr = 1;
		for (int i = 0; i < num_subaddr; i++) {
			if ((addr <= 0x3F) && (i == 8))			subaddr = 0x42;
			else if ((addr <= 0x41) && (i == 14))	subaddr = 0x42;
			else									subaddr = i;
			ret |= FERS_XROC_ReadRegister(handle, addr, subaddr, &data);
			fprintf(fout, "%02X\t%02X\t%02X\n", addr, subaddr, data);
		}
	}
	return 0;
}



// ---------------------------------------------------------------------------------
// Description: Write a text file with the content of all regsiters
// Inputs:		handle: board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
#define NUMREG5204	58

int FERS_DumpBoardRegister5204(int handle, char* filename) {
	uint32_t reg_add = 0;
	uint32_t reg_val = 0;
	int r, i, nreg_ch = 1;
	int brd = FERS_INDEX(handle);

	const char FPGARegAdd[NUMREG5204][2][100] = {
	{"a_acq_ctrl",        "01000000"},   // Acquisition Control Register
	{"a_run_mask",        "01000004"},   // Run mask: bit[0]=SW, bit[1]=T0-IN
	{"a_trg_mask",        "01000008"},   // Global trigger mask: bit[0]=SW, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=Maj, bit[4]=Maj, bit[4]=Periodic
	{"a_tref_mask",       "0100000C"},   // Tref mask: bit[0]=T0-IN, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=T-OR, bit[5]=Periodit[5]=Periodic
	{"a_validation_mask", "01000010"},   // Validation mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN, 
	{"a_t0_out_mask",     "01000014"},   // T0 out mask
	{"a_t1_out_mask",     "01000018"},   // T1 out mask
	{"a_veto_mask",       "0100001C"},   // Veto mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN, 
	{"a_acq_ctrl2",       "01000020"},   // 2nd Acquisition Control Register (extension of acq_ctrl)
	{"a_dwell_time",      "01000050"},   // Dwell time (periodic trigger) in clk cyclces. 0 => OFF
	{"a_trg_delay",       "01000060"},   // Trigger Delay applied in the FPGA
	{"a_pck_maxcnt",      "01000064"},   // Max num of packets transmitted to the TDL
	{"a_dprobe",          "01000068"},   // Digital probes (signal inspector)
	{"a_channel_mask_0",  "01000100"},   // Input Channel Mask (ch  0 to 31)
	{"a_channel_mask_1",  "01000104"},   // Input Channel Mask (ch 32 to 63)
	{"a_trg2hold_time",   "01000124"},   // Time from gtrg to peak hold
	{"a_amux_seq_ctrl",   "01000128"},	 // Timing parameters for the analog mux readout and conversion
	{"a_wave_length",     "0100012C"},	 // Waveform Length
	{"a_trg_mask_0",      "01000138"},	 // RR trg Mask (ch  0 to 31)
	{"a_trg_mask_1",      "0100013C"},	 // RR trg Mask (ch 32 to 63)
	{"a_tpulse_ctrl",     "01000200"},   // Test pulse mask
	{"a_hv_regaddr",      "01000210"},   // HV Register Address and Data Type (I2C master)
	{"a_hv_regdata",      "01000214"},   // HV Register Data (I2C master)
	{"a_trgho",           "01000218"},   // channel trigger hold off
	{"a_trefho",          "0100021C"},   // Tref hold off
	{"a_dc_offset",       "01000220"},   // DC offset (bit[15:14] = ch, bit[11:0] = value)
	{"a_spi_data",        "01000224"},   // SPI R/W data (for Flash Memory access)
	{"a_test_led",        "01000228"},   // Test Mode for LEDs
	{"a_tlogic_def",      "01000234"},   // Trigger logic definition
	{"a_hit_width",       "01000238"},   // Monostable for CR triggers
	{"a_tlogic_width",    "0100023C"},   // Monostable for Tlogic output
	{"a_i2c_addr",        "01000240"},   // I2C addr (for PLL)
	{"a_i2c_data",        "01000244"},   // I2C R/W data (for PLL)
	{"a_fw_rev",          "01000300"},   // Firmware Revision 
	{"a_acq_status",      "01000304"},   // Acquisition Status
	{"a_real_time",       "01000308"},   // Real Time in ms
	{"a_dead_time",       "01000310"},   // Dead Time in ms
	{"a_tdl_status",      "01000314"},   // TDLink status
	{"a_trg_cnt",         "01000318"},	 // Trigger (or validation) counter
	{"a_or_t1_cnt",       "01000350"},	 // T-OR counter
	{"a_or_t2_cnt",       "01000354"},	 // Q-OR counter
	{"a_hv_Vmon",         "01000356"},	 // HV Vmon
	{"a_hv_Imon",         "01000358"},	 // HV Imon
	{"a_hv_status",       "01000360"},	 // HV Status
	{"a_pid",             "01000400"},   // PID 
	{"a_pcb_rev",         "01000404"},   // PCB Revision
	{"a_fers_code",       "01000408"},   // FERS code (5202)
	{"a_fpga_temp",       "01000450"},   // FPGA die temperature 
	{"a_board_temp",      "01000454"},   // Board temperature near PIC/FPGA 
	{"a_TDC_temp",        "01000458"},   // Board temperature near TDC0 
	{"a_uC_status",       "01000600"},   // uC Status Reg
	{"a_uC_shutdown",     "01000604"},   // ShutDown uC (PIC)
	{"a_sw_compatib",     "01004000"},   // SW compatibility
	{"a_commands",        "01008000"},   // Send Commands (for Eth and USB)
	{"a_rebootfpga",      "0100FFF0"},   // trigger fpga reboot in bootloader
	{"a_zs_lgthr",        "02000000"},   // Threshold for zero suppression (LG)
	{"a_zs_hgthr",        "02000004"},   // Threshold for zero suppression (HG)
	{"a_hitcnt",          "02000800"},   // Hit counters 
	};

	FILE* dumpReg = NULL;
	dumpReg = fopen(filename, "w");
	if (dumpReg == NULL)
		return -1;

	for (r = 0; r < NUMREG5204; ++r) {
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

