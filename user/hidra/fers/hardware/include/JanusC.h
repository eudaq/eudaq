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

#ifndef _JanusC_H
#define _JanusC_H                    // Protect against multiple inclusion

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "FERSlib.h"
#include "MultiPlatform.h"

#ifdef _WIN32
	#define popen _popen
	#define DEFAULT_GNUPLOT_PATH "..\\gnuplot\\gnuplot.exe"
	#define GNUPLOT_TERMINAL_TYPE "wxt"
	#define PATH_SEPARATOR "\\"
	#define CONFIG_FILE_PATH ""
	#define DATA_FILE_PATH "."
	#define WORKING_DIR ""
	#define EXE_NAME "JanusC.exe"
#else
    #include <unistd.h>
    #include <stdint.h>   /* C99 compliant compilers: uint64_t */
    #include <ctype.h>    /* toupper() */
    #include <sys/time.h>
	#define DEFAULT_GNUPLOT_PATH	"gnuplot"
	#define GNUPLOT_TERMINAL_TYPE "x11"
	#define PATH_SEPARATOR "/"
	#define EXE_NAME "JanusC"
	#ifndef Sleep
		#define Sleep(x) usleep((x)*1000)
	#endif
	#ifdef _ROOT_
		#define CONFIG_FILE_PATH _ROOT_"/Janus/config/"
		#define DATA_FILE_PATH _ROOT_"/Janus/"
		#define WORKING_DIR _ROOT_"/Janus/"
	#else
		#define CONFIG_FILE_PATH ""
		#define DATA_FILE_PATH "./DataFiles"
		#define WORKING_DIR ""
	#endif
#endif

#define SW_RELEASE_NUM			"4.1.2"
#define SW_RELEASE_DATE			"15/04/2025"
#define FILE_LIST_VER			"3.3"

#ifdef _WIN32
#define CONFIG_FILENAME			"Janus_Config.txt"
#define RUNVARS_FILENAME		"RunVars.txt"
#define PIXMAP_FILENAME			"pixel_map.txt"
#else	// with eclipse the Debug run from Janus folder, and the executable in Janus/Debug
#define CONFIG_FILENAME			"Janus_Config.txt"
#define RUNVARS_FILENAME		"RunVars.txt"
#define PIXMAP_FILENAME			"pixel_map.txt"
#endif

//****************************************************************************
// Definition of limits and sizes
//****************************************************************************
//#define MAX_NBRD						128	// max. number of boards supported in console
#define MAX_NBRD						16	// max. number of boards supported in console
#define MAX_NBRD_GUI					16	// max. number of boards in the GUI	
#define MAX_NCNC						8	// max. number of concentrators
#define MAX_NCH							64	// max. number of channels 
#define MAX_GW							20	// max. number of generic write commads
#define MAX_NTRACES						8	// Max num traces in the plot

// *****************************************************************
// Parameter options
// *****************************************************************
#define OUTFILE_RAW_LL					0x0001
#define OUTFILE_LIST_BIN				0x0002
#define OUTFILE_LIST_ASCII				0x0004
#define OUTFILE_LIST_CSV				0x0008
#define OUTFILE_SPECT_HISTO				0x0010
#define OUTFILE_ToA_HISTO				0x0020
#define OUTFILE_TOT_HISTO				0x0040
#define OUTFILE_STAIRCASE				0x0080
#define OUTFILE_RUN_INFO				0x0100
#define OUTFILE_MCS_HISTO				0x0200
#define OUTFILE_SYNC					0x0400
#define OUTFILE_SERVICE_INFO			0x0800

#define OF_UNIT_LSB						0
#define OF_UNIT_NS						1

#define PLOT_E_SPEC_LG					0
#define PLOT_E_SPEC_HG					1
#define PLOT_TOA_SPEC					2
#define PLOT_TOT_SPEC					3
#define PLOT_CHTRG_RATE					4
#define PLOT_WAVE						5
#define PLOT_2D_CNT_RATE				6
#define PLOT_2D_CHARGE_LG				7
#define PLOT_2D_CHARGE_HG				8
#define PLOT_SCAN_THR					9
#define PLOT_SCAN_HOLD_DELAY			10
#define PLOT_MCS_TIME					11	// DNIN those value have to be sorted later

#define DATA_ANALYSIS_CNT				0x0001
#define DATA_ANALYSIS_MEAS				0x0002
#define DATA_ANALYSIS_HISTO				0x0004
#define DATA_MONITOR					0x0008

#define SMON_CHTRG_RATE					0
#define SMON_CHTRG_CNT					1
#define SMON_HIT_RATE					2
#define SMON_HIT_CNT					3
#define SMON_PHA_RATE					4
#define SMON_PHA_CNT					5

#define STOPRUN_MANUAL					0
#define STOPRUN_PRESET_TIME				1
#define STOPRUN_PRESET_COUNTS			2

#define EVBLD_DISABLED					0
#define EVBLD_TRGTIME_SORTING			1
#define EVBLD_TRGID_SORTING				2

#define TEST_PULSE_DEST_ALL 			-1
#define TEST_PULSE_DEST_EVEN			-2
#define TEST_PULSE_DEST_ODD				-3
#define TEST_PULSE_DEST_NONE			-4

#define CITIROC_CFG_FROM_REGS			0
#define CITIROC_CFG_FROM_FILE			1

#define SCPARAM_BRD						0
#define SCPARAM_MIN						1
#define SCPARAM_MAX						2
#define SCPARAM_STEP					3
#define SCPARAM_DWELL					4
										
#define HDSPARAM_BRD					0
#define HDSPARAM_MIN					1
#define HDSPARAM_MAX					2
#define HDSPARAM_STEP					3
#define HDSPARAM_NMEAN					4


// Temperatures
#define TEMP_BOARD						0
#define TEMP_FPGA						1
#define TEMP_DETECTOR					2
#define TEMP_HV							3


// HV monitor
#define HV_VMON							0
#define HV_IMON							1


// Acquisition Status Bits
#define ACQSTATUS_SOCK_CONNECTED		1	// GUI connected through socket
#define ACQSTATUS_HW_CONNECTED			2	// Hardware connected
#define ACQSTATUS_READY					3	// ready to start (HW connected, memory allocated and initialized)
#define ACQSTATUS_RUNNING				4	// acquisition running (data taking)
#define ACQSTATUS_RESTARTING			5	// Restarting acquisition
#define ACQSTATUS_EMPTYING				6	// Acquiring data still in the boards buffers after the software stop
#define ACQSTATUS_STAIRCASE				10	// Running Staircase
#define ACQSTATUS_RAMPING_HV			11	// Switching HV ON or OFF
#define ACQSTATUS_UPGRADING_FW			12	// Upgrading the FW
#define ACQSTATUS_HOLD_SCAN				13	// Running Scan Hold
#define ACQSTATUS_ERROR					-1	// Error


//****************************************************************************
// struct that contains the configuration parameters (HW and SW)
//****************************************************************************
typedef struct Config_t {

	// System info 
	char ConnPath[MAX_NBRD][200];	// IP address of the board
	int NumBrd;                     // Tot number of connected boards
	int NumCh;						// Number of channels 

	int EnLiveParamChange;			// Enable param change while running (when disabled, Janus will stops and restarts the acq. when a param changes)
	int AskHVShutDownOnExit;		// Ask if the HV must be shut down before quitting
	int OutFileEnableMask;			// Enable/Disable output files 
	char DataFilePath[500];			// Output file data path
	uint8_t EnableMaxFileSize;		// Enable the Limited size for list output files. Value set in MaxOutFileSize parameter
	float MaxOutFileSize;			// Max size of list output files in bytes. Minimum size allowed 1 MB
	uint8_t EnableRawDataRead;		// Enable the readout from RawData file
	uint8_t OutFileUnit;			// Unit for time measurement in output files (0 = LSB, 1 = ns)
	int EnableJobs;					// Enable Jobs
	int JobFirstRun;				// First Run Number of the job
	int JobLastRun;					// Last Run Number of the job
	float RunSleep;					// Wait time between runs of one job
	int StartRunMode;				// Start Mode (this is a HW setting that defines how to start/stop the acquisition in the boards)
	int StopRunMode;				// Stop Mode (unlike the start mode, this is a SW setting that deicdes the stop criteria)
	int RunNumber_AutoIncr;			// auto increment run number after stop
	float PresetTime;				// Preset Time (Real Time in s) for auto stop
	int PresetCounts;				// Preset Number of counts (triggers)
	int EventBuildingMode;			// Event Building Mode
	int TstampCoincWindow;			// Time window (ns) in event buiding based on trigger Tstamp
	int DataAnalysis;				// Data Analysis Enable/disable mask

	float FiberDelayAdjust[MAX_NCNC][8][16];		// Fiber length (in meters) for individual tuning of the propagation delay along the TDL daisy chains

	int EHistoNbin;					// Number of channels (bins) in the Energy Histogram
	int ToAHistoNbin;				// Number of channels (bins) in the ToA Histogram
	int8_t ToARebin;				// Rebin factor for ToA histogram. New bin = 0.5*Rebin ns
	float ToAHistoMin;				// Minimum time value for ToA Histogram. Maximum is Min+0.5*Rebin*Nbin
	int ToTHistoNbin;				// Number of channels (bins) in the ToT Histogram
	int MCSHistoNbin;				// Number of channels (bins) in the MCS Histogram

	int CitirocCfgMode;				// 0=from regs, 1=from file
	//uint16_t Pedestal;				// Common pedestal added to all channels

	//                                                                       
	// Acquisition Setup (HW settings)
	//                                                                       
	// Board Settings
	uint32_t AcquisitionMode;						// ACQMODE_COUNT, ACQMODE_SPECT, ACQMODE_TIMING, ACQMODE_WAVE
	uint32_t EnableToT;								// Enable readout of ToT (time over threshold)

	uint32_t TriggerMask;	// Variable needed in plot.c. There no handle is passed
	//uint32_t WaveformLength;
	float PtrgPeriod;
	float HV_Vbias[MAX_NBRD];

	int EnableServiceEvent;								// Enable service events
 
} Janus_Config_t;


typedef struct RunVars_t {
	int ActiveBrd;				// Active board
	int ActiveCh;				// Active channel
	int PlotType;				// Plot Type
	int SMonType;				// Statistics Monitor Type
	int Xcalib;					// X-axis calibration
	int RunNumber;				// Run Number (used in output file name; -1 => no run number)
	char PlotTraces[MAX_NTRACES][100];	// Plot Traces (format: "0 3 X" => Board 0, Channel 3, From X[B board - F offline - S from file)
	int StaircaseCfg[5];		// Staircase Params: Board MinThr Maxthr Step Dwell
	int HoldDelayScanCfg[5];	// Hold Delay Scan Params: Board MinDelay MaxDelay Step Nmean
} RunVars_t;



//****************************************************************************
// Global Variables
//****************************************************************************
extern Janus_Config_t J_cfg;				// struct containing all acquisition parameters
extern RunVars_t RunVars;			// struct containing run time variables
extern int handle[MAX_NBRD];		// board handles
extern int cnc_handle[FERSLIB_MAX_NCNC];	// concentrator handles
//extern int ActiveCh, ActiveBrd;		// Board and Channel active in the plot
extern int AcqRun;					// Acquisition running
extern int AcqStatus;				// Acquisition Status
extern int SockConsole;				// 0: use stdio console, 1: use socket console
extern char ErrorMsg[250];			// Error Message
extern int MAX_NBRD_JANUS;			// Max number of boards supported by Janus, 16 in GUI mode, 128 in console mode

#endif
