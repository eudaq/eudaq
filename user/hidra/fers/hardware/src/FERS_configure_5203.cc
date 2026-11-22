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

static int SetClockChain(int handle);

int Configure5203(int handle, int mode)
{
	int i, ret = 0;
	int numtdc = 0;
	picoTDC_Cfg_t pcfg;

	uint32_t d32;
	char CfgStep[100];
	int brd = FERS_INDEX(handle);

	int rundelay = 0;
	int deskew = -5;

	int strm_ptrg;

	// ########################################################################################################
	sprintf(CfgStep, "General System Configuration and Acquisition Mode");
	// ########################################################################################################
	if (FERScfg[brd]->HighResClock == HRCLK_DISABLED) {
		// Reset the unit 
		if (mode == CFG_HARD) {
			ret = FERS_SendCommand(handle, CMD_RESET);  // Reset
			if (ret != 0) goto abortcfg;
		}
	} else {  	// If the boards use the external clock, it is necessary to reset the boards and setup all clocks before programming the TDCs
		//if ((brd == 0) && (mode == CFG_HARD)) ret = SetClockChain(handle);
		if (mode == CFG_HARD) ret = SetClockChain(handle);
		if (ret != 0) goto abortcfg;
	}

	// Set USB or Eth communication mode
	if ((FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_ETH) || (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_USB))
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 20, 20, 1); // Force disable of TDlink

	// Acq Mode
	if (mode == CFG_HARD) ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 0, 7, FERScfg[brd]->AcquisitionMode);

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 11, 11, 1); // use new data qualifier format 


	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 26, 26, FERScfg[brd]->TrgIdMode); // Trigger ID: 0=trigger counter, 1=validation counter
	if (FERS_BoardInfo[brd]->NumCh == 128) {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 22, 22, 1); // Enable 128 ch
	} else {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 22, 22, 0); // Enable 64 ch
	}

	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 21, 21, FERScfg[brd]->EnableServiceEvents); // Enable board info: 0=disabled, 1=enabled
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 12, 13, FERScfg[brd]->En_Head_Trail); // 0=Header and trailer enabled, 1=one word trailer, 2=header and trailer suppressed
	if (FERScfg[brd]->AcquisitionMode == ACQMODE_STREAMING)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 14, 14, 1);
	else
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 14, 14, FERScfg[brd]->En_Empty_Ev_Suppr);
	ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 20, 20, FERScfg[brd]->Dis_tdl);

	if (FERScfg[brd]->TestMode) {
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 23, 23, 1); // Enable Test Mode (create events with fixed pattern)
		ret |= FERS_WriteRegisterSlice(handle, a_acq_ctrl, 0, 3, FERScfg[brd]->TestMode); // 1 = 1 hit per channel, 2 = 8 hits per channel, 3 = 32 hit per channel
	}

	// Set ToT reject threshold
	if (FERScfg[brd]->MeasMode == MEASMODE_LEAD_TOT8 || FERScfg[brd]->MeasMode == MEASMODE_LEAD_TOT11) {
		ret |= FERS_WriteRegister(handle, a_tot_rej_lthr, FERScfg[brd]->ToT_reject_low_thr);
		ret |= FERS_WriteRegister(handle, a_tot_rej_hthr, FERScfg[brd]->ToT_reject_high_thr);
	}

	// Set LEMO I/O mode
	ret |= FERS_WriteRegister(handle, a_t0_out_mask, FERScfg[brd]->T0_outMask);
	ret |= FERS_WriteRegister(handle, a_t1_out_mask, FERScfg[brd]->T1_outMask);

	// Set Digital Probe
	ret |= FERS_WriteRegister(handle, a_dprobe_5203, (FERScfg[brd]->DigitalProbe[1] << 8) | FERScfg[brd]->DigitalProbe[0]);
	// Set Digital Probe in concentrator (if present)
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		if (FERScfg[brd]->CncProbe_A >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FA_FN, VR_IO_FUNCTION_ZERO);  // Set FA function = ZERO
		if (FERScfg[brd]->CncProbe_B >= 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_FB_FN, VR_IO_FUNCTION_ZERO);  // Set FB function = ZERO
		FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_IO_DEBUG, FERScfg[brd]->CncProbe_A | (FERScfg[brd]->CncProbe_B << 8));
	}

	// Set Periodic Triggers
	strm_ptrg = FERScfg[brd]->MeasMode == MEASMODE_LEAD_TOT8 ? 128 : 1562;
	if ((FERScfg[brd]->AcquisitionMode == ACQMODE_STREAMING) && (FERS_FPGA_FW_MajorRev(handle) < 1)) {
		ret |= FERS_WriteRegister(handle, a_dwell_time, strm_ptrg);  // about 20 us
	} else {
		ret |= FERS_WriteRegister(handle, a_dwell_time, (uint32_t)(FERScfg[brd]->PtrgPeriod / CLK_PERIOD_5203));
		ret |= FERS_WriteRegister(handle, a_strm_ptrg, strm_ptrg);  // about 20 us
	}

	// Set Trigger Source
	ret |= FERS_WriteRegister(handle, a_trg_mask, FERScfg[brd]->TriggerMask);
	// Set Tref Source
	ret |= FERS_WriteRegister(handle, a_tref_mask, FERScfg[brd]->Tref_Mask);

	// Set Trigger Hold Off (Retrigger Protection Time)
	if (FERScfg[brd]->AcquisitionMode != ACQMODE_STREAMING)
		ret |= FERS_WriteRegister(handle, a_trg_hold_off, (uint32_t)(FERScfg[brd]->TrgHoldOff / CLK_PERIOD_5203));
	else
		ret |= FERS_WriteRegister(handle, a_trg_hold_off, 0);

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

	// Set Veto mask
	FERS_WriteRegister(handle, a_veto_mask, FERScfg[brd]->Veto_Mask);

	// Trigger buffer size and FIFO almost full levels
	if ((FERScfg[brd]->TriggerBufferSize > 0) && (FERScfg[brd]->TriggerBufferSize < 512)) {
		int af_bsy, af_skp, max_evsize;
		const int lsof_size = 16 * 1024;
		int chbuff_size=0;

		// Set almost full level of the trigger fifo; it determines the maximum number of pending triggers (i.e. triggers already sent to the picoTDC but to yet transferred to the LSOF fifo)
		FERS_WriteRegister(handle, a_trgf_almfull, FERScfg[brd]->TriggerBufferSize);

		// Set AlmFull level "as_bsy" of the LSOF FIFO: busy is generated when this level is reached. The busy stops the trigger, but there is no guarantee that the number of pending triggers
		// (still in the pipeline) with the relevant event packets will find place in the LSOF fifo. Therefore, there is a scond almont full level (af_skp); when this is reached, the hit
		// data will be discarded and only headers and trailers will be written into the LSOF fifo. The HLOSS flag is asserted in this case.
		if (FERScfg[brd]->AcquisitionMode != ACQMODE_TEST_MODE) {
			chbuff_size = 1 << (FERScfg[brd]->TDC_ChBufferSize + 2); // Ch buffer size in the picoTDC (0=4, 1=8, ... 7=512)
			max_evsize = (chbuff_size + 2) * FERS_BoardInfo[brd]->NumCh; // +2 is for header and trailer
			if (max_evsize <= 2176)
				af_bsy = lsof_size - (2 * max_evsize + 100);  // keep space for at least 2 events; +100 is for extra margin. 
			else
				af_bsy = lsof_size / 2; // keep half buffer
			af_skp = lsof_size - (FERScfg[brd]->TriggerBufferSize * 2 * (FERS_BoardInfo[brd]->NumCh / 16) + 10);  // keep space at least for header+trailers (one per group of 16 channels) of all pending triggers; +10 is for extra margin. 
		} else {
			if (FERScfg[brd]->TestMode == 1) chbuff_size = 1; // test mode 1 => 1 hit per channel
			else if ((FERScfg[brd]->TestMode == 2) || (FERScfg[brd]->TestMode == 4)) chbuff_size = 8;  // test_mode 2, 4 => 8 hits per channel
			else if ((FERScfg[brd]->TestMode == 3) || (FERScfg[brd]->TestMode == 5)) chbuff_size = 32; // test_mode 3, 5 => 32 hits per channel
			max_evsize = chbuff_size * FERS_BoardInfo[brd]->NumCh + 1; // +1 is for oneword trailer
			af_bsy = lsof_size - (2 * max_evsize + 100);  // keep space for at least 2 events; +100 is for extra margin. 
			af_skp = lsof_size - (FERScfg[brd]->TriggerBufferSize * 2 * (FERS_BoardInfo[brd]->NumCh / 16) + 10);  // keep space at least for header+trailers (one per group of 16 channels) of all pending triggers; +10 is for extra margin. 
		}
		FERS_WriteRegister(handle, a_lsof_almfull_bsy, af_bsy);
		FERS_WriteRegister(handle, a_lsof_almfull_skp, af_skp);

	}
	if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) {
		if (FERScfg[brd]->MaxPck_Train > 0) FERS_WriteRegister(handle, a_pck_maxcnt, FERScfg[brd]->MaxPck_Train);  // Max number of packets (events) that can be appended to a TDL train by one FERS card (default = 16 packets)
		if (FERScfg[brd]->MaxPck_Block > 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_DMA_FLAGS_INFO, FERScfg[brd]->MaxPck_Block);  // Max number of packets accumulated into the Concentrator buffer before switching to the other buffer and flush the current one (default = 500 packets)
		if (FERScfg[brd]->CncBufferSize > 0) FERS_WriteRegister(FERS_CNC_HANDLE(handle), VR_DMA_FLAGS_DATA, FERScfg[brd]->CncBufferSize);  // Size of one data buffer in the concentrator used for the events payload (default = 7340032 x 4 bytes). There are two buffers (ping-pong mode, alternate write/read)
	}

	if (ret) goto abortcfg;

	numtdc = (FERS_BoardInfo[brd]->NumCh == 64) ? 1 : 2;
	int tdc;
	for (tdc = 0; tdc < numtdc; tdc++) {

		// ########################################################################################################
		sprintf(CfgStep, "picoTDC power-up");
		// ########################################################################################################
		ret = 0;

		// Write default cfg
		Set_picoTDC_Default(&pcfg);
		ret |= Write_picoTDC_Cfg(handle, tdc, pcfg, 1);

		// Initialize PLL with AFC
		pcfg.PLL2.bits.pll_afcrst = 1;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);
		pcfg.PLL2.bits.pll_afcrst = 0;
		pcfg.PLL2.bits.pll_afcstart = 1;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);
		Sleep(10); // Wait for lock 
		pcfg.PLL2.bits.pll_afcstart = 0;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);

		// Initialize DLL
		pcfg.DLL_TG.bits.dll_fixctrl = 1;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);
		Sleep(10); // Wait for lock 
		pcfg.DLL_TG.bits.dll_fixctrl = 0;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);

		// Turn everything ON
		pcfg.Control.bits.magic_word = 0x5C;
		pcfg.Control.bits.digital_reset = 1;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, pcfg.Control.reg);
		pcfg.Control.bits.digital_reset = 0;
		ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, pcfg.Control.reg);
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
		double trgwin;
		if (FERScfg[brd]->AcquisitionMode == ACQMODE_COMMON_START || FERScfg[brd]->AcquisitionMode == ACQMODE_COMMON_STOP) {
			if (FERScfg[brd]->GateWidth < 50000 - 7 * TDC_CLK_PERIOD) {	// the max TrgWindow is 50 us: GATE + extra clock (5+2)
				trgwin = (FERScfg[brd]->GateWidth / TDC_CLK_PERIOD) + 2;
			} else {
				trgwin = ((50000 / TDC_CLK_PERIOD) - 7 + 2);
				FERScfg[brd]->GateWidth = (uint32_t)((50000 / TDC_CLK_PERIOD) - 7);
			}
		} else {
			if (FERScfg[brd]->TrgWindowWidth < 50000) {	//the max TrgWindow is 50 us
				trgwin = FERScfg[brd]->TrgWindowWidth / TDC_CLK_PERIOD;
			} else {
				trgwin = 50000 / TDC_CLK_PERIOD;
				FERScfg[brd]->TrgWindowWidth = 50000;
			}
		}

		pcfg.TrgWindow.bits.trigger_window = (uint32_t)trgwin;
		if (FERScfg[brd]->AcquisitionMode == ACQMODE_COMMON_START) {
			ret |= FERS_WriteRegister(handle, a_trg_delay, (uint32_t)(2 * trgwin + 2));
			pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(trgwin + 5);
		} else if (FERScfg[brd]->AcquisitionMode == ACQMODE_COMMON_STOP) {
			ret |= FERS_WriteRegister(handle, a_trg_delay, 0);
			pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(trgwin + 2);
		} else if (FERScfg[brd]->AcquisitionMode == ACQMODE_TRG_MATCHING) {
			if ((FERScfg[brd]->TrgWindowOffset + FERScfg[brd]->TrgWindowWidth) > 0) {  // trg window ends after the trigger => need to delay the trigger (in the FPGA) and shift it to the end of the window
				ret |= FERS_WriteRegister(handle, a_trg_delay, (uint32_t)(2 * (FERScfg[brd]->TrgWindowOffset + FERScfg[brd]->TrgWindowWidth) / TDC_CLK_PERIOD) + 1);
				pcfg.TrgWindow.bits.trigger_latency = (uint32_t)trgwin;
			} else {
				ret |= FERS_WriteRegister(handle, a_trg_delay, 0);
				pcfg.TrgWindow.bits.trigger_latency = (uint32_t)(-FERScfg[brd]->TrgWindowOffset / TDC_CLK_PERIOD);
			}
		} else if (FERScfg[brd]->AcquisitionMode == ACQMODE_STREAMING) {
			pcfg.TrgWindow.bits.trigger_window = strm_ptrg / 2 - 1;
			pcfg.TrgWindow.bits.trigger_latency = strm_ptrg / 2 + 1;
		}

		// TDC pulser
		if (FERScfg[brd]->TDCpulser_Period > 0) {
			pcfg.PulseGen1.bits.pg_run = 1;
			pcfg.PulseGen1.bits.pg_en = 1;
			pcfg.PulseGen1.bits.pg_strength = 3;
			pcfg.PulseGen1.bits.pg_direct = 0;
			pcfg.PulseGen1.bits.pg_rep = 1;
			pcfg.PulseGen1.bits.pg_rising = 1;
			pcfg.PulseGen2.bits.pg_phase = 3;
			pcfg.PulseGen2.bits.pg_falling = (uint32_t)(FERScfg[brd]->TDCpulser_Width / TDC_PULSER_CLK_PERIOD);
			pcfg.PulseGen3.bits.pg_reload = (uint32_t)(FERScfg[brd]->TDCpulser_Period / TDC_PULSER_CLK_PERIOD);
			FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1, pcfg.PulseGen1.reg);
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

		//pcfg.Buffers.bits.channel_buffer_size = 2;


		// Set Measurement mode (Enable ToT or leading edge)
		if (FERScfg[brd]->MeasMode != MEASMODE_LEAD_ONLY) {
			pcfg.Enable.bits.falling_en = 1;
			for (i = 0; i < PICOTDC_NCH; i++) {
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
			for (i = 0; i < PICOTDC_NCH; i++) {
				pcfg.Ch_Control[i].bits.falling_en_tm = 0;
			}
		}

		// Channel offset 
		if (FERScfg[brd]->AdapterType == ADAPTER_NONE) {
			for (i = 0; i < PICOTDC_NCH; i++)
				pcfg.Ch_Control[i].bits.channel_offset = FERScfg[brd]->Ch_Offset[i];
		} else if ((FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_POS) || (FERScfg[brd]->AdapterType == ADAPTER_A5256_REV0_NEG)) {
			for (i = 0; i < 17; i++) {
				int ch;
				FERS_ChIndex_ada2tdc(i, &ch, brd);
				pcfg.Ch_Control[ch].bits.channel_offset = FERScfg[brd]->Ch_Offset[i];
			}
		}

		// Channel RX Enable Mask
		uint32_t ChMask0, ChMask1, ChMask2, ChMask3;
		if (FERScfg[brd]->AdapterType == ADAPTER_NONE) {
			ChMask0 = (uint32_t)(FERScfg[brd]->ChEnableMask & 0xFFFFFFFF);
			ChMask1 = (uint32_t)((FERScfg[brd]->ChEnableMask >> 32) & 0xFFFFFFFF);
			ChMask2 = (uint32_t)(FERScfg[brd]->ChEnableMask_e & 0xFFFFFFFF);
			ChMask3 = (uint32_t)((FERScfg[brd]->ChEnableMask_e >> 32) & 0xFFFFFFFF);
		} else {
			FERS_ChMask_ada2tdc((uint32_t)(FERScfg[brd]->ChEnableMask & 0xFFFFFFFF), &ChMask0, &ChMask1, brd);
			ChMask2 = 0;
			ChMask3 = 0;
		}
		if (tdc == 0) {
			pcfg.Hit_RXen_T.bits.hrx_enable_t = ChMask0; //for TDC0
			pcfg.Hit_RXen_B.bits.hrx_enable_b = ChMask1; //for TDC0
			for (i = 0; i < PICOTDC_NCH; i++) {
				uint32_t mask = (i < 32) ? ChMask0 : ChMask1;
				pcfg.Ch_Control[i].bits.channel_enable = (mask >> (i % 32)) & 1;
			}
		} else if (tdc == 1) {
			pcfg.Hit_RXen_T.bits.hrx_enable_t = ChMask2; //for TDC1
			pcfg.Hit_RXen_B.bits.hrx_enable_b = ChMask3; //for TDC1
			for (i = 0; i < PICOTDC_NCH; i++) {
				uint32_t mask = (i < 32) ? ChMask2 : ChMask3;
				pcfg.Ch_Control[i].bits.channel_enable = (mask >> (i % 32)) & 1;
			}
		}

		// Write final configuration
		ret = Write_picoTDC_Cfg(handle, tdc, pcfg, 0);
		if (ret) goto abortcfg;
	}


	// ########################################################################################################
	sprintf(CfgStep, "Input Adapter");
	// ########################################################################################################
	if (FERScfg[brd]->AdapterType != ADAPTER_NONE) {
		int nch = FERS_AdapterNch(brd);
		if (FERScfg[brd]->DisableThresholdCalib)
			FERS_DisableThrCalib(handle);
		for (i = 0; i < nch; i++) {
			ret |= FERS_Set_DiscrThreshold(handle, i, FERScfg[brd]->DiscrThreshold[i], brd);
		}
		if ((FERScfg[brd]->AdapterType == A5256_CH0_POSITIVE) && (FERScfg[brd]->AdapterType == ADAPTER_A5256))
			FERS_WriteRegisterSlice(handle, a_io_ctrl, 0, 1, 3); // invert polarity of the edge_conn trigger
	}
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

	// Save reg image to output file
	if (DebugLogs & 0x10000)
		Save_picoTDC_Cfg(handle, 0, "picoTDC_RegImg.txt");
	Sleep(10);
	return 0;

abortcfg:
	_setLastLocalError("Error at: %s. Exit Code = %d\n", CfgStep, ret);
	return ret;
}



static int SetClockChain(int handle) {
	int ret = 0;

	int brd = FERS_INDEX(handle);

	if (FERScfg[brd]->HighResClock == HRCLK_FAN_OUT) {
		if (FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) return FERSLIB_ERR_INVALID_CLK_SETTING;
		// Enable ext_clk 
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 4, 0x3); // Set FPGA clk = ext_clk (need to activate clock before swicthing)
		ret |= FERS_SendCommand(handle, CMD_USE_ECLK); // Swicth to Ext Clk; this commands performs also a board reset
	} else {
		// Reset all boards
		ret |= FERS_SendCommand(handle, CMD_RESET);

		if (FERScfg[brd]->HighResClock == HRCLK_DAISY_CHAIN) {
			if (brd == 0) {
				ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 0); // TDC clock sel = internal
				ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 0); // Clock Out Sel = internal
			} else {
				ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 2, 2, 1); // TDC clock sel = external
				ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 3, 3, 1); // Clock Out Sel = external
			}
			ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 4, 4, 1); // Enable Clock Out
		}
		ret |= FERS_WriteRegisterSlice(handle, a_io_ctrl, 5, 6, FERScfg[brd]->TdlClkPhase & 0x3); // Set phase of the divided clock (0=0, 1=90, 2=180, 3=270)
		// need re-sync of chain is clock phase has been changed
		if ((FERS_CONNECTIONTYPE(handle) == FERS_CONNECTIONTYPE_TDL) && ((FERScfg[brd]->HighResClock == HRCLK_DAISY_CHAIN) || (FERScfg[brd]->HighResClock == HRCLK_FAN_OUT)))
			ret |= FERS_InitTDLchains(FERS_CNC_HANDLE(handle), NULL);
	}
	return ret;
}


// ---------------------------------------------------------------------------------
// Description: Configure analog and digital probes
// Inputs:		handle = board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int ConfigureProbe5203(int handle) {
	int i, ret = 0;
	int brd = FERS_INDEX(handle);
	for (i = 0; i < 2; i++) {
		if (FERScfg[brd]->DigitalProbe[i] == DPROBE_OFF)				ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0xFF);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_LG)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x10);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_PEAK_HG)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x11);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_HOLD)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x16);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_START_CONV)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x12);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_COMMIT)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x21);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_DATA_VALID)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x20);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_CLK_1024)		ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x00);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_VAL_WINDOW)	ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x1A);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_T_OR)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x04);
		else if (FERScfg[brd]->DigitalProbe[i] == DPROBE_Q_OR)			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, 0x05);
		else if (FERScfg[brd]->DigitalProbe[i] & 0x80000000) {  // generic setting
			uint32_t blk = (FERScfg[brd]->DigitalProbe[i] >> 16) & 0xF;
			uint32_t sig = FERScfg[brd]->DigitalProbe[i] & 0xF;
			ret |= FERS_WriteRegisterSlice(handle, a_dprobe_5203, i * 8, i * 8 + 7, (blk << 4) | sig);
		}
	}
	Sleep(10);
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: Set default value of picoTDC register (don't write in TDC, just in the struct)
// Outpus:		pfcg: struct with picoTDC regs
// --------------------------------------------------------------------------------------------------------- 
void Set_picoTDC_Default(picoTDC_Cfg_t* pcfg) {
	memset(pcfg, 0, sizeof(picoTDC_Cfg_t));  // set all parames to 0 

	pcfg->Enable.bits.highres_en = 1;
	pcfg->Enable.bits.crossing_en = 1;

	// Set High Res mode
	pcfg->Enable.bits.rx_en_trigger = 1;
	for (int i = 0; i < PICOTDC_NCH; i++)
		pcfg->Ch_Control[i].bits.highres_en_tm = 1;
	pcfg->DLL_TG.bits.tg_bot_nen_fine = 0;
	pcfg->DLL_TG.bits.tg_bot_nen_coarse = 0;
	pcfg->DLL_TG.bits.tg_top_nen_fine = 0;
	pcfg->DLL_TG.bits.tg_top_nen_coarse = 0;

	pcfg->BunchCount.bits.bunchcount_overflow = 0x1FFF;
	pcfg->EventID.bits.eventid_overflow = 0x1FFF;

	pcfg->Buffers.bits.channel_buffer_size = FERScfg[0]->TDC_ChBufferSize & 0x7;
	pcfg->Buffers.bits.readout_buffer_size = 0x7;
	pcfg->Buffers.bits.trigger_buffer_size = 0x7;
	pcfg->Buffers.bits.disable_ro_reject = 1;
	pcfg->Buffers.bits.errorcnts_saturate = 1;

	pcfg->Hit_RX_TX.bits.hrx_bot_bias = 0xF;
	pcfg->Hit_RX_TX.bits.hrx_top_bias = 0xF;
	pcfg->Hit_RX_TX.bits.hrx_top_en = 1;
	pcfg->Hit_RX_TX.bits.hrx_top_en_r = 1;
	pcfg->Hit_RX_TX.bits.hrx_bot_en = 1;
	pcfg->Hit_RX_TX.bits.hrx_bot_en_r = 1;
	pcfg->Hit_RX_TX.bits.tx_strenght = 0x3;

	pcfg->DLL_TG.bits.dll_bias_val = 2;

	pcfg->PLL1.bits.pll_cp_dacset = 0x53;
	pcfg->PLL1.bits.pll_resistor = 0x1E;

	pcfg->PLL2.bits.pll_afcvcal = 0x6;

	pcfg->Clocks.bits.hit_phase = 1;  // must be 1
	pcfg->Clocks.bits.par_speed = 2;  // 00=320, 01=160, 10=80, 11=40
	pcfg->Clocks.bits.par_phase = 4;  // 01=@rising edge
	pcfg->Clocks.bits.sync_clock = 1; // Sync Out; 0=byte sync, 1=half freq clock

	pcfg->ClockShift.bits.shift_clk1G28 = 2;

	pcfg->Enable.bits.falling_en = 0;
	for (int i = 0; i < PICOTDC_NCH; i++) {
		pcfg->Ch_Control[i].bits.falling_en_tm = 0;
	}
}

// --------------------------------------------------------------------------------------------------------- 
// Description: write cfg struct with regs into picoTDC
// Inputs:		handle = device handle
//				tdc: tdc index (0 or 1)
//				pcfg: struct with picoTDC regs
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int Write_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t pcfg, int skipch) {
	int ret = 0, i;
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, pcfg.Control.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Enable, pcfg.Enable.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Header, pcfg.Header.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_TrgWindow, pcfg.TrgWindow.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg0Del_ToT, pcfg.Trg0Del_ToT.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_BunchCount, pcfg.BunchCount.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_EventID, pcfg.EventID.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Buffers, pcfg.Buffers.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RX_TX, pcfg.Hit_RX_TX.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, pcfg.DLL_TG.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL1, pcfg.PLL1.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, pcfg.PLL2.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Clocks, pcfg.Clocks.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ClockShift, pcfg.ClockShift.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_T, pcfg.Hit_RXen_T.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_B, pcfg.Hit_RXen_B.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1, pcfg.PulseGen1.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen2, pcfg.PulseGen2.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen3, pcfg.PulseGen3.reg);
	ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ErrorFlagCtrl, pcfg.ErrorFlagCtrl.reg);
	if (!skipch) {
		for (i = 0; i < PICOTDC_NCH; i++)
			ret |= FERS_I2C_WriteRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Control(i), pcfg.Ch_Control[i].reg);
	}
	return ret;
}


// --------------------------------------------------------------------------------------------------------- 
// Description: read picoTDC regs and update the cfg struct
// Inputs:		handle = device handle
//				tdc: tdc index (0 or 1)
// Outputs		pcfg: struct with picoTDC regs
// Return:		0=OK, negative number = error code
// --------------------------------------------------------------------------------------------------------- 
int Read_picoTDC_Cfg(int handle, int tdc, picoTDC_Cfg_t* pcfg) {
	int ret = 0, i;
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Control, &pcfg->Control.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Enable, &pcfg->Enable.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Header, &pcfg->Header.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_TrgWindow, &pcfg->TrgWindow.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg0Del_ToT, &pcfg->Trg0Del_ToT.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_BunchCount, &pcfg->BunchCount.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_EventID, &pcfg->EventID.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Buffers, &pcfg->Buffers.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RX_TX, &pcfg->Hit_RX_TX.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_DLL_TG, &pcfg->DLL_TG.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL1, &pcfg->PLL1.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL2, &pcfg->PLL2.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Clocks, &pcfg->Clocks.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ClockShift, &pcfg->ClockShift.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_T, &pcfg->Hit_RXen_T.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Hit_RXen_B, &pcfg->Hit_RXen_B.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen1, &pcfg->PulseGen1.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen2, &pcfg->PulseGen2.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PulseGen3, &pcfg->PulseGen3.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_ErrorFlagCtrl, &pcfg->ErrorFlagCtrl.reg);
	ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_PLL_Caps, &pcfg->PLL_Caps.reg);
	for (i = 0; i < PICOTDC_NCH; i++) {
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Control(i), &pcfg->Ch_Control[i].reg);
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Ch_Status(i), &pcfg->Ch_Status[i].reg);
	}
	for (i = 0; i < 4; i++) {
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Trg_Status(i), &pcfg->Trg_Status[i].reg);
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_RO_Status(i), &pcfg->RO_Status[i].reg);
	}
	for (i = 0; i < 3; i++)
		ret |= FERS_I2C_ReadRegister(handle, I2C_ADDR_TDC(tdc), a_pTDC_Cfg_Parity(i), &pcfg->Cfg_Parity[i].reg);
	return ret;
}

// ---------------------------------------------------------------------------------
// Description: Write picoTDC Reg Image into file
// Inputs:		handle = board handle
//				fname = output file name
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
int Save_picoTDC_Cfg(int handle, int tdc, char* fname) {
	picoTDC_Cfg_t pcfg;
	int i, ret;
	FILE* img;

	img = fopen(fname, "w");
	if (img == NULL) return -1;
	ret = Read_picoTDC_Cfg(handle, tdc, &pcfg);
	if (ret < 0) return ret;
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Control, pcfg.Control.reg, "Control");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Enable, pcfg.Enable.reg, "Enable");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Header, pcfg.Header.reg, "Header");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_TrgWindow, pcfg.TrgWindow.reg, "TrgWindow");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Trg0Del_ToT, pcfg.Trg0Del_ToT.reg, "Trg0Del_ToT");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_BunchCount, pcfg.BunchCount.reg, "BunchCount");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_EventID, pcfg.EventID.reg, "EventID");
	for (i = 0; i < PICOTDC_NCH; i++)
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Ch_Control(i), pcfg.Ch_Control[i].reg, "Ch_Control", i);
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Buffers, pcfg.Buffers.reg, "Buffers");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RX_TX, pcfg.Hit_RX_TX.reg, "Hit_RX_TX");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_DLL_TG, pcfg.DLL_TG.reg, "DLL_TG");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL1, pcfg.PLL1.reg, "PLL1");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL2, pcfg.PLL2.reg, "PLL2");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Clocks, pcfg.Clocks.reg, "Clocks");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_ClockShift, pcfg.ClockShift.reg, "ClockShift");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RXen_T, pcfg.Hit_RXen_T.reg, "Hit_RXen_L");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_Hit_RXen_B, pcfg.Hit_RXen_B.reg, "Hit_RXen_H");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen1, pcfg.PulseGen1.reg, "PulseGen1");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen2, pcfg.PulseGen2.reg, "PulseGen2");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PulseGen3, pcfg.PulseGen3.reg, "PulseGen3");
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_ErrorFlagCtrl, pcfg.ErrorFlagCtrl.reg, "ErrorFlagCtrl");
	for (i = 0; i < PICOTDC_NCH; i++)
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Ch_Status(i), pcfg.Ch_Status[i].reg, "Ch_Status", i);
	for (i = 0; i < 4; i++)
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Trg_Status(i), pcfg.Trg_Status[i].reg, "Trg_Status", i);
	for (i = 0; i < 4; i++)
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_RO_Status(i), pcfg.RO_Status[i].reg, "RO_Status", i);
	for (i = 0; i < 3; i++)
		fprintf(img, "0x%04X = 0x%08X - %s(%d)\n", a_pTDC_Cfg_Parity(i), pcfg.Cfg_Parity[i].reg, "CfgParity", i);
	fprintf(img, "0x%04X = 0x%08X - %s\n", a_pTDC_PLL_Caps, pcfg.PLL_Caps.reg, "PLL_Caps");
	fclose(img);
	return 0;
}


// ---------------------------------------------------------------------------------
// Description: Write a text file with the content of all regsiters
// Inputs:		handle: board handle
// Return:		Error code (0=success) 
// ---------------------------------------------------------------------------------
#define NUMREG5203	53

int FERS_DumpBoardRegister5203(int handle, char* filename) {
	uint32_t reg_add = 0;
	uint32_t reg_val = 0;
	int r, i, nreg_ch = 1;
	int brd = FERS_INDEX(handle);

	const char FPGARegAdd[NUMREG5203][2][100] = {
		{"a_acq_ctrl",         "01000000"}, // Acquisition Control Register
		{"a_run_mask",         "01000004"}, // Run mask : bit[0] = SW, bit[1] = T0 - IN
		{"a_trg_mask",         "01000008"}, // Global trigger mask : bit[0] = SW, bit[1] = T1 - IN, bit[2] = Q - OR, bit[3] = Maj, bit[4] = Periodic
		{"a_tref_mask",        "0100000C"}, // Tref mask :
		{"a_t0_out_mask",      "01000014"}, // T0 out mask
		{"a_t1_out_mask",      "01000018"}, // T1 out mask
		{"a_io_ctrl",          "01000020"}, // Clk and I / O control
		{"a_veto_mask",        "0100001C"}, // Veto mask : bit[0] = Cmd from TDL, bit[1] = T0 - IN, bit[2] = T1 - IN,
		{"a_tref_delay",       "01000048"}, // Delay of the time reference for the coincidences
		{"a_tref_window",      "0100004C"}, // Tref coincidence window(for list mode)
		{"a_dwell_time",       "01000050"}, // Dwell time(periodic trigger) in clk cyclces. 0 = > OFF
		{"a_list_size",        "01000054"}, // Number of 32 bit words in a packet in timing mode(list mode)
		{"a_trg_delay",        "01000060"}, // Trigger Delay applied in the FPGA
		{"a_pck_maxcnt",       "01000064"}, // Max num of packets transmitted to the TDL
		{"a_lsof_almfull_bsy", "01000068"}, // Almost Full level of LSOF(Event Data FIFO) to assert Busy
		{"a_lsof_almfull_skp", "0100006C"}, // Almost Full level of LSOF(Event Data FIFO) to skip data
		{"a_trgf_almfull",     "01000070"}, // Almost Full level of Trigger FIFO
		{"a_tot_rej_lthr",     "01000074"}, // ToT Reject Low Threshold(0 = disabled)
		{"a_tot_rej_hthr",     "01000078"}, // ToT Reject High Threshold(0 = disabled)
		{"a_trg_holdoff",      "0100007A"}, // Trigger Hold - off time(early retrigger protection). 0 = disabled
		{"a_strm_ptrg",        "0100007C"}, // Periodic trigger for streaming mode
		{"a_dprobe",           "01000110"}, // Digital probes(signal inspector)
		{"a_i2c_addr",         "01000200"}, // I2C addr(for picoTDC cfg or PLLs)
		{"a_i2c_data",         "01000204"}, // I2C R / W data(for picoTDC cfg or PLLs)
		{"a_spi_data",         "01000224"}, // SPI R / W data(for Flash Memory access)
		{"a_test_led",         "01000228"}, // Test Mode for LEDs
		{"a_tlogic_def",       "01000234"}, // Trigger logic definition
		{"a_fw_rev",           "01000300"}, // Firmware Revision
		{"a_acq_status",       "01000304"}, // Acquisition Status
		{"a_real_time",        "01000308"}, // Real Time in ms
		{"a_dead_time",        "01000310"}, // Dead Time in ms
		{"a_tdl_status",       "01000314"}, // TDLink status
		{"a_trg_cnt",          "01000318"}, // Trigger counter
		{"a_tdcro_status",     "0100031A"}, // TDC readout status
		{"a_rej_trg_cnt",      "0100031C"}, // Rejected Trigger counter
		{"a_zs_trg_cnt",       "01000320"}, // Zero suppressed Trigger counter(empty events)
		{"a_clk_out_phase",    "01000330"}, // clk out phase measurement
		{"a_fpga_temp",        "01000348"}, // FPGA die temperature
		{"a_board_temp",       "01000350"}, // Board temperature near PIC / FPGA
		{"a_TDC0_temp",        "01000354"}, // Board temperature near TDC0
		{"a_TDC1_temp",        "01000358"}, // Board temperature near TDC1
		{"a_tdc_th_temp",      "01000360"}, // PicoTDC overtemperature threshold
		{"a_pid",              "01000400"}, // PID
		{"a_pcb_rev",          "01000404"}, // PCB Revision
		{"a_fers_code",        "01000408"}, // FERS code(5202)
		{"a_tdc0_rtData",      "01000500"}, // TDC0 real - time data output
		{"a_tdc1_rtData",      "01000504"}, // TDC1 real - time data output
		{"a_uC_status",        "01000600"}, // uC Status Reg
		{"a_uC_shutdown",      "01000604"}, // ShutDown uC(PIC)
		{"a_tdc_reset_reg",    "01000608"}, // PicoTDC reset Registers
		{"a_commands",         "01008000"}, // Send Commands(for Ethand USB)
		{"a_rebootfpga",       "0100FFF0"}, // trigger fpga reboot in bootloader
		{"a_offset",           "02000000"}, // Individual channel offset(pos or neg delay)
	};

	FILE* dumpReg = NULL;
	dumpReg = fopen(filename, "w");
	if (dumpReg == NULL)
		return -1;

	for (r = 0; r < NUMREG5203; ++r) {
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

