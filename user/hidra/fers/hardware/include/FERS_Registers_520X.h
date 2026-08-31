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

#ifndef _REGISTERS_H
#define _REGISTERS_H				// Protect against multiple inclusion


// ############################################################################################
// FPGA REGISTERS
// ############################################################################################


// *****************************************************************
// Individual Channel and Broadcast address converter
// *****************************************************************
#define INDIV_ADDR(addr, ch)		(0x02000000 | ((addr) & 0xFFFF) | ((ch)<<16))
#define BCAST_ADDR(addr)			(0x03000000 | ((addr) & 0xFFFF))

/*!
* @ingroup FERS_Registers
* @brief FERS configuration registers
* @{
*/
// *****************************************************************
// FPGA Register Address Map common
// *****************************************************************
#define a_acq_ctrl         0x01000000   //!< Acquisition Control Register
#define a_run_mask         0x01000004   //!< Run mask: bit[0]=SW, bit[1]=T0-IN
#define a_trg_mask         0x01000008   //!< Global trigger mask: bit[0]=SW, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=Maj, bit[4]=Periodic
#define a_tref_mask        0x0100000C   //!< Tref mask: bit[0]=T0-IN, bit[1]=T1-IN, bit[2]=Q-OR, bit[3]=T-OR, bit[5]=Periodic
#define a_t0_out_mask      0x01000014   //!< T0 out mask
#define a_t1_out_mask      0x01000018   //!< T1 out mask
#define a_veto_mask        0x0100001C   //!< Veto mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN, 
#define a_tref_delay       0x01000048   //!< Delay of the time reference for the coincidences
#define a_tref_window      0x0100004C   //!< Tref coincidence window (for list mode)
#define a_dwell_time       0x01000050   //!< Dwell time (periodic trigger) in clk cyclces. 0 => OFF
#define a_list_size        0x01000054   //!< Number of 32 bit words in a packet in timing mode (list mode)
#define a_pck_maxcnt       0x01000064   //!< Max num of packets transmitted to the TDL
#define a_channel_mask_0   0x01000100   //!< Input Channel Mask (ch  0 to 31)
#define a_channel_mask_1   0x01000104   //!< Input Channel Mask (ch 32 to 63)
#define a_fw_rev           0x01000300   //!< Firmware Revision 
#define a_acq_status       0x01000304   //!< Acquisition Status
#define a_real_time        0x01000308   //!< Real Time in ms
#define a_dead_time        0x01000310   //!< Dead Time in ms
#define a_fpga_temp        0x01000348   //!< FPGA die Temperature 
#define a_pid              0x01000400	//!< PID
#define a_pcb_rev          0x01000404	//!< PCB revision
#define a_fers_code        0x01000408	//!< Fers CODE (5202)
#define a_rebootfpga       0x0100FFF0	//!< reboot FPGA from FW uploader
#define a_test_led         0x01000228   //!< LED test mode
#define a_tdc_mode         0x0100022C   //!< TDC Mode
#define a_tdc_data         0x01000230   //!< TDC Data

#define a_sw_compatib      0x01004000   //!< SW compatibility
#define a_commands         0x01008000   //!< Send Commands (for Eth and USB)

// *****************************************************************
// FPGA Register Address Map 5202 Only
// *****************************************************************
#define a_validation_mask  0x01000010   //!< Validation mask: bit[0]=Cmd from TDL, bit[1]=T0-IN, bit[2]=T1-IN
#define a_acq_ctrl2        0x01000020   //!< Acquisition Control Register 2
#define a_dprobe_5202      0x01000068   //!< Digital probes (signal inspector)
#define a_citiroc_cfg      0x01000108   //!< Citiroc common configuration bits
#define a_citiroc_en       0x0100010C   //!< Citiroc internal parts enable mask 
#define a_citiroc_probe    0x01000110   //!< Citiroc probes (signal inspector)
#define a_qd_coarse_thr    0x01000114   //!< Coarse threshold for the Citiroc Qdiscr (10 bit DAC)
#define a_td_coarse_thr    0x01000118   //!< Coarse threshold for the Citiroc Tdiscr (10 bit DAC)
#define a_lg_sh_time       0x0100011C   //!< Low gain shaper time constant
#define a_hg_sh_time       0x01000120   //!< High gain shaper time constant
#define a_hold_delay       0x01000124   //!< Time from gtrg to peak hold
#define a_amux_seq_ctrl    0x01000128   //!< Timing parameters for the analog mux readout and conversion
#define a_wave_length      0x0100012C   //!< Waveform Length
#define a_scbs_ctrl        0x01000130	//!< Citiroc SC bit stream index and select
#define a_scbs_data        0x01000134	//!< Citiroc SC bit stream data
#define a_qdiscr_mask_0    0x01000138	//!< Charge Discriminator mask
#define a_qdiscr_mask_1    0x0100013C	//!< Charge Discriminator mask
#define a_tdiscr_mask_0    0x01000140	//!< Time Discriminator mask
#define a_tdiscr_mask_1    0x01000144	//!< Time Discriminator mask
#define a_tpulse_ctrl      0x01000200   //!< Test pulse mask
#define a_tpulse_dac       0x01000204   //!< Internal Test Pulse amplitude
#define a_hv_regaddr       0x01000210   //!< HV Register Address and Data Type
#define a_hv_regdata       0x01000214   //!< HV Register Data
#define a_trgho            0x01000218   //!< Trigger Hold off
#define a_dc_offset        0x01000220   //!< DAC for DC offset
#define a_spi_data         0x01000224   //!< SPI R/W data (for Flash Memory access)
#define a_test_led         0x01000228   //!< Test Mode for LEDs
#define a_tdc_mode         0x0100022C   //!< R/W   C   32    TDC modes
#define a_tdc_data         0x01000230   //!< R/W   C   32    Regs of TDC
#define a_tlogic_def       0x01000234   //!< Trigger Logic Definition
#define a_tlogic_width     0x0100023C   //!< Monostable for Trigger logic output
#define a_hit_width        0x01000238   //!< Monostable for CR triggers
#define a_i2c_addr_5202    0x01000240   //!< I2C Addr
#define a_i2c_data_5202    0x01000244   //!< I2C Data
#define a_t_or_cnt         0x01000350	//!< T-OR counter
#define a_q_or_cnt         0x01000354	//!< Q-OR counter
#define a_hv_Vmon          0x01000356   //!< HV Vmon
#define a_hv_Imon          0x01000358   //!< HV Imon
#define a_hv_status        0x01000360   //!< HV Status
#define a_uC_status        0x01000600   //!< uC status
#define a_uC_shutdown      0x01000604   //!< uC shutdown
#define a_zs_lgthr         0x02000000   //!< Threshold for zero suppression (LG)
#define a_zs_hgthr         0x02000004   //!< Threshold for zero suppression (HG)
#define a_qd_fine_thr      0x02000008   //!< Fine individual threshold for the Citiroc Qdiscr (4 bit DAC)
#define a_td_fine_thr      0x0200000C   //!< Fine individual threshold for the Citiroc Tdiscr (4 bit DAC)
#define a_lg_gain          0x02000010   //!< Preamp Low Gain Setting
#define a_hg_gain          0x02000014   //!< Preamp High Gain Setting
#define a_hv_adj           0x02000018   //!< HV individual adjust (8 bit DAC)
#define a_hitcnt           0x02000800   //!< Hit counters

// *****************************************************************
// FPGA Register Address Map 5203 Only
// *****************************************************************
#define a_io_ctrl          0x01000020   //!< I/O control
#define a_trg_delay        0x01000060   //!< Trigger delay
#define a_dprobe_5203	   0x01000110   //!< Digital probe selection	
#define a_lsof_almfull_bsy 0x01000068	//!< Almost Full level of LSOF (Event Data FIFO) to assert Busy
#define a_lsof_almfull_skp 0x0100006C   //!< Almost Full level of LSOF (Event Data FIFO) to skip data
#define a_trgf_almfull     0x01000070   //!< Almost Full level of Trigger FIFO
#define a_tot_rej_lthr	   0x01000074	//!< ToT Reject Lower Threshold (0=disabled)
#define a_tot_rej_hthr	   0x01000078	//!< ToT Reject Higer Threshold (0=disabled)
#define a_trg_hold_off     0x0100007A   //!< Retrigger protection time. Set the busy active for N clock cycles (0=disabled)
#define a_strm_ptrg        0x0100007C   //!< Periodic trigger for streaming mode
#define a_i2c_addr_5203    0x01000200   //!< I2C Addr
#define a_i2c_data_5203    0x01000204   //!< I2C Data
#define a_trg_cnt          0x01000318   //!< Trigger counter
#define a_tdcro_status     0x0100031A   //!< TDC readout Status
#define a_rej_trg_cnt      0x0100031C   //!< Rejected Trigger counter
#define a_zs_trg_cnt       0x01000320   //!< Zero Suppressed Trigger counter
#define a_clk_out_phase    0x01000330   //!< Phase between TDC clock and FPGA clock (0x0000 = 0 deg; 0xFFFF = 180 deg)
#define a_board_temp       0x01000340	//!< Board Temp
#define a_tdc0_temp		   0x01000354	//!< TDC0 Temperature
#define a_tdc1_temp		   0x01000358	//!< TDC1 Temperature
#define a_spi_data         0x01000224   //!< SPI R/W data (for Flash Memory access)

// *****************************************************************
// FPGA Register Address Map 5204 Only
// *****************************************************************
#define a_tlogic_mask_0    0x01000140	//!< Trigger FPGA-Tlogic mask (in A5202 it corresponds to a_tdiscr_mask_0)
#define a_tlogic_mask_1    0x01000144	//!< Trigger FPGA-Tlogic mask (in A5202 it corresponds to a_tdiscr_mask_1)

/*! @} */

// *****************************************************************
// FPGA Register Bit Fields 
// *****************************************************************
// Status Register 
#define STATUS_READY				(1 <<  0)
#define STATUS_FAIL					(1 <<  1)
#define STATUS_RUNNING				(1 <<  2)
#define STATUS_TDL_SYNC				(1 <<  3)
#define STATUS_FPGA_OVERTEMP		(1 <<  4)
#define STATUS_TDC_RO_ERR			(1 <<  5)
#define STATUS_TDLINK_LOL			(1 <<  6)
#define STATUS_TDC0_LOL				(1 <<  7)
#define STATUS_TDC1_LOL				(1 <<  8)
#define STATUS_RO_CLK_LOL			(1 <<  9)
#define STATUS_TDL_DISABLED			(1 << 10)
#define STATUS_TDC0_OVERTEMP		(1 << 11)
#define STATUS_TDC1_OVERTEMP		(1 << 12)
#define STATUS_BOARD_OVERTEMP		(1 << 13)
#define STATUS_CRC_ERROR			(1 << 14)
#define STATUS_UNUSED_15			(1 << 15)
#define STATUS_SPI_BUSY				(1 << 16)
#define STATUS_I2C_BUSY				(1 << 17)
#define STATUS_I2C_FAIL				(1 << 18)


// *****************************************************************
// FPGA Commands
// *****************************************************************
#define CMD_TIME_RESET     0x11  //!< Absolute Time reset
#define CMD_ACQ_START      0x12  //!< Start acquisition
#define CMD_ACQ_STOP       0x13  //!< Stop acquisition
#define CMD_TRG            0x14  //!< Send software trigger
#define CMD_RESET          0x15  //!< Global Reset (clear data, set all regs to default)
#define CMD_TPULSE         0x16  //!< Send a test pulse
#define CMD_RES_PTRG       0x17  //!< Reset periodic trigger counter (and rearm PTRG in single pulse mode)
#define CMD_CLEAR	       0x18  //!< Clear Data
#define CMD_VALIDATION	   0x19  //!< Trigger Validation (either positive = accept or negative = reject)
#define CMD_SET_VETO	   0x1A  //!< Set Veto
#define CMD_CLEAR_VETO	   0x1B  //!< Clear Veto
#define CMD_TDL_SYNC	   0x1C  //!< Sync signal from TDL
#define CMD_USE_ICLK	   0x1E  //!< Use internal CLK for FPGA
#define CMD_USE_ECLK	   0x1F  //!< Use external CLK for FPGA
#define CMD_CFG_ASIC	   0x20  //!< Configure ASIC (load shift register)


// ############################################################################################
// CITIROC REGISTERS
// ############################################################################################

#define crcfg_qdiscr_latch				0		//!< Qdiscr output: 0=latched, 1=direct                        (bit 258 of SR) 
#define crcfg_sca_bias        			1		//!< SCA bias: 0=high (5MHz readout speed), 1=weak             (bit 301 of SR)
#define crcfg_pdet_mode_hg    			2		//!< Peak_det mode HighGain: 0=peak detector, 1=T&H            (bit 306 of SR)
#define crcfg_pdet_mode_lg    			3		//!< Peak_det mode LowGain:  0=peak detector, 1=T&H            (bit 307 of SR)
#define crcfg_ps_ctrl_logic   			4		//!< Peak Sens Ctrl Logic: 0=internal, 1=external=PS_modeb_ext (bit 308 of SR)
#define crcfg_ps_trg_source   			5		//!< Peak Sens Trg source: 0=internal, 1=external              (bit 309 of SR)
#define crcfg_lg_pa_bias      			6		//!< LG Preamp bias: 0=normal, 1=weak                          (bit 323 of SR)
#define crcfg_pa_fast_sh      			7		//!< Fast shaper connection: 0=high gain pa, 1=low gain pa     (bit 328 of SR)
#define crcfg_8bit_dac_ref    			8		//!< HV adjust DAC reference: 0=2.5V, 1=4.5V                   (bit 330 of SR)
#define crcfg_ota_bias        			9		//!< Output OTA buffer bias auto off: 0=auto, 1=force on       (bit 1133 of SR)
#define crcfg_trg_polarity    			10		//!< Trigger polarity: 0=pos, 1=neg                            (bit 1141 of SR)
#define crcfg_enable_chtrg     			11		//!< Enable channel triggers                                   (bit 1143 of SR)
#define crcfg_enable_gtrg     			16		//!< Enable propagation of gtrg to the Citiroc pin global_trig
#define crcfg_enable_veto     			17		//!< Enable propagation of gate (= not veto) to the Citiroc pin val_evt
#define crcfg_repeat_raz      			18		//!< Enable loop asserting raz_chn until nor_charge stays active 

#define SHAPING_TIME_12_5NS				6
#define SHAPING_TIME_25NS				5
#define SHAPING_TIME_37_5NS				4
#define SHAPING_TIME_50NS				3
#define SHAPING_TIME_62_5NS				2
#define SHAPING_TIME_75NS				1
#define SHAPING_TIME_87_5NS				0

#define TEST_PULSE_SOURCE_EXT			0
#define TEST_PULSE_SOURCE_T0_IN			1
#define TEST_PULSE_SOURCE_T1_IN			2
#define TEST_PULSE_SOURCE_PTRG			3
#define TEST_PULSE_SOURCE_SW_CMD		4

#define TEST_PULSE_PREAMP_LG 			1
#define TEST_PULSE_PREAMP_HG 			2
#define TEST_PULSE_PREAMP_BOTH			3

#define TEST_PULSE_DEST_ALL 			-1
#define TEST_PULSE_DEST_EVEN			-2
#define TEST_PULSE_DEST_ODD				-3
#define TEST_PULSE_DEST_NONE			-4

#define DPROBE_OFF						0
#define DPROBE_PEAK_LG					1
#define DPROBE_PEAK_HG					2
#define DPROBE_HOLD						3
#define DPROBE_START_CONV				4
#define DPROBE_DATA_COMMIT				5
#define DPROBE_DATA_VALID				6
#define DPROBE_CLK_1024					7
#define DPROBE_VAL_WINDOW				8
#define DPROBE_T_OR						9
#define DPROBE_Q_OR						10
#define DPROBE_XROC_TRG					11

#define GAIN_SEL_AUTO					0
#define GAIN_SEL_HIGH					1
#define GAIN_SEL_LOW					2
#define GAIN_SEL_BOTH					3

#define FAST_SHAPER_INPUT_HGPA			0
#define FAST_SHAPER_INPUT_LGPA			1




// ############################################################################################
// RADIOROC REGISTERS
// ############################################################################################

// *****************************************************************
// XROC register map
// *****************************************************************
#define a_XR_ChControl(i)		(0x0000 + (63-i))	//!< Channel Settings (XROC channels are swapped on PCB => ch0 goes in ch63...)
#define a_XR_ASIC_bias			0x0040				//!< ASIC biasing
#define a_XR_common_cfg			0x0041				//!< Common settings
#define a_XR_out_cfg			0x0042				//!< Output settings
#define a_XR_event_val			0x0043				//!< Event validation gating

// *****************************************************************
// XROC reg data struct
// *****************************************************************
typedef struct {
	struct {							// Addr - SubAddr
		uint8_t r0_inDAC;				// 0+CH - 0
		uint8_t r1_pat_Gain_Comp;		// 0+CH - 1
		uint8_t r2_LG_HG_Gain;			// 0+CH - 2
		uint8_t r3_LG_HG_Tau;			// 0+CH - 3
		uint8_t r4_calibDacT1;			// 0+CH - 4
		uint8_t r5_calibDacT2;			// 0+CH - 5
		uint8_t r6_EnMask1;				// 0+CH - 6
		uint8_t r7_EnMask2;				// 0+CH - 7
		uint8_t r66_ProbeSwitchCmd;		// 0+CH - 66
	} ChControl[64];
	struct {							// Addr - SubAddr
		uint8_t r0_inDAC0;				// 64   - 0
		uint8_t r1_inDAC1;				// 64   - 1
		uint8_t r2_calDAC_paT;			// 64   - 2
		uint8_t r3_pa_LG_HG;			// 64   - 3
		uint8_t r4_sh_HG;				// 64   - 4
		uint8_t r5_sh_LG;				// 64   - 5
		uint8_t r6_pdbuff_pdetect;		// 64   - 6
		uint8_t r7_FCNpdet_FCPpdet;		// 64   - 7
		uint8_t r8_FCNpbuf_FCPpbuf;		// 64   - 8
		uint8_t r9_dis1a;				// 64   - 9
		uint8_t r10_dis1b_dis2a;		// 64   - 10
		uint8_t r11_dis2b;				// 64   - 11
		uint8_t r12_dis_charge;			// 64   - 12
		uint8_t r13_EnMask1;			// 64   - 13
		uint8_t r14_EnMask2;			// 64   - 14
	} ASICBias;
	struct {							// Addr - SubAddr
		uint8_t r0_BandGap;				// 65   - 0
		uint8_t r1_DAC1a;				// 65   - 1
		uint8_t r2_DAC1b_DAC2a;			// 65   - 2
		uint8_t r3_DAC2b_DACqa;			// 65   - 3
		uint8_t r4_DACqb;				// 65   - 4
		uint8_t r5_Ib_thrDac;			// 65   - 5
		uint8_t r6_Ib_thrDac;			// 65   - 6
		uint8_t r7_EnMask;				// 65   - 7
		uint8_t r8_delay;				// 65   - 8
		uint8_t r9_;					// 65   - 9
		uint8_t r10_;					// 65   - 10
		uint8_t r11_;					// 65   - 11
		uint8_t r12_;					// 65   - 12
		uint8_t r66_;					// 65   - 66
	} Common;
	struct {							// Addr - SubAddr
		uint8_t r0_BandGap;				// 65   - 0
		uint8_t r1_DAC1a;				// 65   - 1
		uint8_t r2_DAC1b_DAC2a;			// 65   - 2
		uint8_t r3_DAC2b_DACqa;			// 65   - 3
		uint8_t r4_DACqb;				// 65   - 4
		uint8_t r5_Ib_thrDac;			// 65   - 5
		uint8_t r6_Ib_thrDac;			// 65   - 6
		uint8_t r7_EnMask;				// 65   - 7
		uint8_t r8_delay;				// 65   - 8
		uint8_t r9_;					// 65   - 9
		uint8_t r10_;					// 65   - 10
		uint8_t r11_;					// 65   - 11
		uint8_t r12_;					// 65   - 12
		uint8_t r66_;					// 65   - 66
	} Outing;

} XR_Cfg_t;



// ############################################################################################
// PICOTDC REGISTERS
// ############################################################################################

// *****************************************************************
// picoTDC register Map
// *****************************************************************
#define a_pTDC_Control			0x0004				//!< R/W - Magic word + reset bits
#define a_pTDC_Enable 			0x0008				//!< R/W - Enable bits
#define a_pTDC_Header			0x000C				//!< R/W - Event data format (header)
#define a_pTDC_TrgWindow		0x0010				//!< R/W - Trg Window Latency + Width
#define a_pTDC_Trg0Del_ToT		0x0014				//!< R/W - Trg_Ch0 Delay + ToT
#define a_pTDC_BunchCount		0x0018				//!< R/W - Bunch Count Overflow + Offset
#define a_pTDC_EventID			0x001C				//!< R/W - EventID 
#define a_pTDC_Ch_Control(i)	(0x0020 + (i)*4)	//!< R/W - Channel offset + some Ctrl bits
#define a_pTDC_Buffers			0x0120				//!< R/W - Buffer settings
#define a_pTDC_Hit_RX_TX		0x0124				//!< R/W - Hit RX/TX
#define a_pTDC_DLL_TG			0x0128				//!< R/W - DLL/TG
#define a_pTDC_PLL1				0x0130				//!< R/W - PLL1
#define a_pTDC_PLL2				0x0134				//!< R/W - PLL2
#define a_pTDC_Clocks			0x0138				// R/W - 
#define a_pTDC_ClockShift		0x013C				// R/W - 
#define a_pTDC_Hit_RXen_T		0x0140				// R/W - 
#define a_pTDC_Hit_RXen_B		0x0144				// R/W - 
#define a_pTDC_PulseGen1		0x0148				// R/W - 
#define a_pTDC_PulseGen2		0x014C				// R/W - 
#define a_pTDC_PulseGen3		0x0150				// R/W - 
#define a_pTDC_ErrorFlagCtrl	0x0154				// R/W - 
#define a_pTDC_Ch_Status(i)		(0x0160 + (i)*4)	// R   - 
#define a_pTDC_Trg_Status(i)	(0x0260 + (i)*4)	// R   - 
#define a_pTDC_RO_Status(i)		(0x0270 + (i)*4)	// R   - 
#define a_pTDC_Cfg_Parity(i)	(0x0280 + (i)*4)	// R   - 
#define a_pTDC_PLL_Caps			0x28C				// R   - 
#define a_pTDC_DelayAdjust		0xFFFC				// W   - 

// *****************************************************************
// picoTDC Data Structures
// *****************************************************************

typedef struct {
	// ------------------------------------------------ Control
	union {
		struct {
			uint32_t magic_word : 8;
			uint32_t digital_reset : 1;
			uint32_t bunchcount_reset : 1;
			uint32_t eventid_reset : 1;
			uint32_t dropcnts_reset : 1;
			uint32_t errorcnts_reset : 1;
			uint32_t trigger_create : 1;
			uint32_t trigger_create_single : 1;
			uint32_t reserved1 : 17;
		} bits;
		uint32_t reg;
	} Control;
	// ------------------------------------------------ Enable
	union {
		struct {
			uint32_t reserved1 : 1;
			uint32_t falling_en : 1;
			uint32_t single_readout_en : 1;
			uint32_t reserved2 : 2;
			uint32_t highres_en : 1;
			uint32_t dig_rst_ext_en : 1;
			uint32_t bx_rst_ext_en : 1;
			uint32_t eid_rst_ext_en : 1;
			uint32_t reserved3 : 8;
			uint32_t crossing_en : 1;
			uint32_t rx_en_extref : 1;
			uint32_t rx_en_bxrst : 1;
			uint32_t rx_en_eidrst : 1;
			uint32_t rx_en_trigger : 1;
			uint32_t rx_en_reset : 1;
			uint32_t left_suppress : 1;
			uint32_t channel_split2 : 1;
			uint32_t channel_split4 : 1;
			uint32_t reserved4 : 6;
		} bits;
		uint32_t reg;
	} Enable;
	// ------------------------------------------------ Header
	union {
		struct {
			uint32_t untriggered : 1;
			uint32_t reserved1 : 1;
			uint32_t relative : 1;
			uint32_t second_header : 1;
			uint32_t full_events : 1;
			uint32_t trigger_ch0_en : 1;
			uint32_t reserved2 : 2;
			uint32_t header_fields0 : 3;
			uint32_t reserved3 : 1;
			uint32_t header_fields1 : 3;
			uint32_t reserved4 : 1;
			uint32_t header_fields2 : 3;
			uint32_t reserved5 : 1;
			uint32_t header_fields3 : 3;
			uint32_t reserved6 : 1;
			uint32_t max_eventsize : 8;
		} bits;
		uint32_t reg;
	} Header;
	// ------------------------------------------------ TrgWindow
	union {
		struct {
			uint32_t trigger_latency : 13;
			uint32_t reserved1 : 3;
			uint32_t trigger_window : 13;
			uint32_t reserved2 : 3;
		} bits;
		uint32_t reg;
	} TrgWindow;

	// ------------------------------------------------ Trg0Del_ToT
	union {
		struct {
			uint32_t trg_ch0_delay : 13;
			uint32_t reserved1 : 3;
			uint32_t tot : 1;
			uint32_t tot_8bit : 1;
			uint32_t tot_startbit : 5;
			uint32_t tot_saturate : 1;
			uint32_t tot_leadingstartbit : 4;
			uint32_t reserved2 : 4;
		} bits;
		uint32_t reg;
	} Trg0Del_ToT;
	// ------------------------------------------------ BunchCount
	union {
		struct {
			uint32_t bunchcount_overflow : 13;
			uint32_t reserved1 : 3;
			uint32_t bunchcount_offset : 13;
			uint32_t reserved2 : 3;
		} bits;
		uint32_t reg;
	} BunchCount;
	// ------------------------------------------------ EventID
	union {
		struct {
			uint32_t eventid_overflow : 13;
			uint32_t reserved1 : 3;
			uint32_t eventid_offset : 13;
			uint32_t reserved2 : 3;
		} bits;
		uint32_t reg;
	} EventID;
	// ------------------------------------------------ Ch_Control[64]
	union {
		struct {
			uint32_t channel_offset : 26;
			uint32_t falling_en_tm : 1;
			uint32_t highres_en_tm : 1;
			uint32_t scan_en_tm : 1;
			uint32_t scan_in_tm : 1;
			uint32_t reserved1 : 1;
			uint32_t channel_enable : 1;
		} bits;
		uint32_t reg;
	} Ch_Control[64];
	// ------------------------------------------------ Buffers
	union {
		struct {
			uint32_t channel_buffer_size : 4;
			uint32_t readout_buffer_size : 4;
			uint32_t trigger_buffer_size : 4;
			uint32_t disable_ro_reject : 1;
			uint32_t errorcnts_saturate : 1;
			uint32_t reserved1 : 2;
			uint32_t max_grouphits : 8;
			uint32_t reserved2 : 8;
		} bits;
		uint32_t reg;
	} Buffers;
	// ------------------------------------------------ Hit_RX_TX
	union {
		struct {
			uint32_t hrx_top_delay : 4;
			uint32_t hrx_top_bias : 4;
			uint32_t hrx_top_filter_trailing : 1;
			uint32_t hrx_top_filter_leading : 1;
			uint32_t hrx_top_en_r : 1;
			uint32_t hrx_top_en : 1;
			uint32_t hrx_bot_delay : 4;
			uint32_t hrx_bot_bias : 4;
			uint32_t hrx_bot_filter_trailing : 1;
			uint32_t hrx_bot_filter_leading : 1;
			uint32_t hrx_bot_en_r : 1;
			uint32_t hrx_bot_en : 1;
			uint32_t tx_strenght : 2;
			uint32_t reserved1 : 6;
		} bits;
		uint32_t reg;
	} Hit_RX_TX;
	// ------------------------------------------------ DLL_TG
	union {
		struct {
			uint32_t dll_bias_val : 7;
			uint32_t dll_bias_cal : 5;
			uint32_t dll_cp_comp : 3;
			uint32_t dll_ctrlval : 4;
			uint32_t dll_fixctrl : 1;
			uint32_t dll_extctrl : 1;
			uint32_t dll_cp_comp_dir : 1;
			uint32_t dll_en_bias_cal : 1;
			uint32_t tg_bot_nen_fine : 1;
			uint32_t tg_bot_nen_coarse : 1;
			uint32_t tg_top_nen_fine : 1;
			uint32_t tg_top_nen_coarse : 1;
			uint32_t tg_cal_nrst : 1;
			uint32_t tg_cal_parity_in : 1;
			uint32_t reserved1 : 3;
		} bits;
		uint32_t reg;
	} DLL_TG;
	// ------------------------------------------------ PLL1
	union {
		struct {
			uint32_t pll_cp_ce : 1;
			uint32_t pll_cp_dacset : 7;
			uint32_t pll_cp_irefcpset : 5;
			uint32_t pll_cp_mmcomp : 3;
			uint32_t pll_cp_mmdir : 1;
			uint32_t pll_pfd_enspfd : 1;
			uint32_t pll_resistor : 5;
			uint32_t pll_sw_ext : 1;
			uint32_t pll_vco_dacsel : 1;
			uint32_t pll_vco_dacset : 7;
		} bits;
		uint32_t reg;
	} PLL1;
	// ------------------------------------------------ PLL2
	union {
		struct {
			uint32_t pll_vco_dac_ce : 1;
			uint32_t pll_vco_igen_start : 1;
			uint32_t pll_railmode_vco : 1;
			uint32_t pll_abuffdacset : 5;
			uint32_t pll_buffce : 1;
			uint32_t pll_afcvcal : 4;
			uint32_t pll_afcstart : 1;
			uint32_t pll_afcrst : 1;
			uint32_t pll_afc_override : 1;
			uint32_t pll_bt0 : 1;
			uint32_t pll_afc_overrideval : 4;
			uint32_t pll_afc_overridesig : 1;
			uint32_t reserved1 : 10;
		} bits;
		uint32_t reg;
	} PLL2;

	// ------------------------------------------------ Clocks
	union {
		struct {
			uint32_t hit_phase : 2;
			uint32_t reserved1 : 2;
			uint32_t trig_phase : 3;
			uint32_t reserved2 : 1;
			uint32_t bx_rst_phase : 3;
			uint32_t reserved3 : 1;
			uint32_t eid_rst_phase : 3;
			uint32_t reserved4 : 1;
			uint32_t par_speed : 2;
			uint32_t reserved5 : 2;
			uint32_t par_phase : 3;
			uint32_t sync_clock : 1;
			uint32_t ext_clk_dll : 1;
			uint32_t reserved6 : 7;
		} bits;
		uint32_t reg;
	} Clocks;
	// ------------------------------------------------ ClockShift
	union {
		struct {
			uint32_t reserved1 : 4;
			uint32_t shift_clk1G28 : 4;
			uint32_t shift_clk640M : 4;
			uint32_t shift_clk320M : 4;
			uint32_t shift_clk320M_ref : 4;
			uint32_t shift_clk160M : 4;
			uint32_t shift_clk80M : 4;
			uint32_t shift_clk40M : 4;
		} bits;
		uint32_t reg;
	} ClockShift;
	// ------------------------------------------------ Hit_RXen_T 
	union {
		struct {
			uint32_t hrx_enable_t : 32;
		} bits;
		uint32_t reg;
	} Hit_RXen_T;
	// ------------------------------------------------ Hit_RXen_B
	union {
		struct {
			uint32_t hrx_enable_b : 32;
		} bits;
		uint32_t reg;
	} Hit_RXen_B;
	// ------------------------------------------------ PulseGen1
	union {
		struct {
			uint32_t pg_run : 1;
			uint32_t pg_rep : 1;
			uint32_t pg_direct : 1;
			uint32_t pg_ini : 1;
			uint32_t pg_en : 1;
			uint32_t pg_strength : 2;
			uint32_t reserved1 : 1;
			uint32_t pg_rising : 18;
			uint32_t reserved2 : 6;
		} bits;
		uint32_t reg;
	} PulseGen1;
	// ------------------------------------------------ PulseGen2
	union {
		struct {
			uint32_t pg_phase : 8;
			uint32_t pg_falling : 18;
			uint32_t reserved1 : 6;
		} bits;
		uint32_t reg;
	} PulseGen2;
	// ------------------------------------------------ PulseGen3
	union {
		struct {
			uint32_t pg_reload : 18;
			uint32_t reserved2 : 14;
		} bits;
		uint32_t reg;
	} PulseGen3;
	// ------------------------------------------------ ErrorFlagCtrl
	union {
		struct {
			uint32_t error_flags_1 : 10;
			uint32_t error_flags_2 : 10;
			uint32_t error_flags_3 : 10;
			uint32_t reserved1 : 2;
		} bits;
		uint32_t reg;
	} ErrorFlagCtrl;
	// ------------------------------------------------ DelayAdjust
	union {
		struct {
			uint32_t tg_delay_taps : 8;
			uint32_t reserved1 : 24;
		} bits;
		uint32_t reg;
	} DelayAdjust;

	// ************************************************
	// Status Regs (read only)
	// ************************************************
	// ------------------------------------------------ Ch_Status[64]
	union {
		struct {
			uint32_t ch_fillgrade : 8;
			uint32_t ch_dropped : 8;
			uint32_t ch_parity : 4;
			uint32_t scan_out : 1;
			uint32_t reserved1 : 2;
			uint32_t ch_stateerr : 1;
			uint32_t reserved2 : 8;
		} bits;
		uint32_t reg;
	} Ch_Status[64];
	// ------------------------------------------------ Trg_Status[4]
	union {
		struct {
			uint32_t tr_fillgrade : 8;
			uint32_t tr_dropped : 8;
			uint32_t tr_parity : 4;
			uint32_t reserved1 : 3;
			uint32_t tr_stateerr : 1;
			uint32_t reserved2 : 8;

		} bits;
		uint32_t reg;
	} Trg_Status[4];
	// ------------------------------------------------ RO_Status[4]
	union {
		struct {
			uint32_t ro_fillgrade : 8;
			uint32_t ro_dropped : 8;
			uint32_t ro_corrected : 4;
			uint32_t reserved1 : 3;
			uint32_t ro_multibiterr : 4;
			uint32_t reserved2 : 5;

		} bits;
		uint32_t reg;
	} RO_Status[4];
	// ------------------------------------------------ Cfg_Parity[3]
	union {
		struct {
			uint32_t parity : 32;
		} bits;
		uint32_t reg;
	} Cfg_Parity[3];
	// ------------------------------------------------ PLL_Caps
	union {
		struct {
			uint32_t pll_selectedcap : 24;
			uint32_t reserved1 : 8;
		} bits;
		uint32_t reg;
	} PLL_Caps;

} picoTDC_Cfg_t;


// *****************************************************************
// Address of I2C devices
// *****************************************************************
#define I2C_ADDR_TDC(i)					(0x63 - (i)) 
#define I2C_ADDR_PLL0					0x68
#define I2C_ADDR_PLL1					0x69
#define I2C_ADDR_PLL2					0x70
#define I2C_ADDR_EEPROM_MEM				0x57
#define I2C_ADDR_XR						0x78 // weeroc ASIC

#endif



