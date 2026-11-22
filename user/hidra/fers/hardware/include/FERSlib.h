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
******************************************************************************//*!
*
*	\file		FERSlib.h
*	\brief		CAEN FERS Library
*	\author		Daniele Ninci, Carlo Tintori
* 
*	@version 1.2.0
*	@date 06/05/2025
********************************************************************************/

/*!
 * @file FERSlib.h
 * @brief CAEN FERS Library
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation. This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
 * software, documentation and results solely at his own risk.
 *
 * 
 *
 */


#ifndef _FERSLIB_H
#define _FERSLIB_H				// Protect against multiple inclusion

#include <stdint.h>
#include <stdlib.h>  // for min/max definition
#include <stdio.h>
#include <math.h>
#include "ExportTypes.h"

#include "FERS_Registers_520X.h"
#include "FERS_Registers_5215.h"

//#include "FERS_MultiPlatform.h"

#ifndef WIN32
#include <stdbool.h>
#endif


#define FERSLIB_STR_HELPER_(S)			#S
#define FERSLIB_STR(S)					FERSLIB_STR_HELPER_(S)


#define f_sprintf	sprintf_s
#define f_fopen		fopen_s


/*!
 * @defgroup structs FERS Library Structures
 * @brief Structures of FERS Library
 */

/*!
 * @ingroup structs
 * @defgroup FERSlib_DStructs Data
 * @brief Data structures for the FERS library.
 */

/*!
 * @ingroup structs
 * @defgroup FERSlib_BStructs Board Info
 * @brief Board Info structures for the FERS library.
 */

/*!
 * @ingroup structs
 * @defgroup FERSlib_CStructs Configuration parameters
 * @brief FERS configuration parameters structure (lib + board)
 */

/*!
 * @defgroup FERS_Registers FERS 520X Board registers
 * @brief FERS configuration registers
 */

/*! 
 * @defgroup Macros Macros
 * @brief Common macros
 */

/*!
* @defgroup FERSlib_Enum Enumerations
* @brief Application enumerations
*/

 /*!
 * @ingroup Macros
 * @defgroup FERSLIB_VERSION Library version
 * @brief Macros to define the library version.
 * @{
 */
#define FERSLIB_VERSION_MAJOR			1
#define FERSLIB_VERSION_MINOR			2
#define FERSLIB_VERSION_PATCH			0
#define FERSLIB_RELEASE_NUM				(FERSLIB_VERSION_MAJOR * 10000) + (FERSLIB_VERSION_MINOR * 100) + (FERSLIB_VERSION_PATCH)	/*!< Library release version (int) */
#define FERSLIB_RELEASE_STRING			FERSLIB_STR(FERSLIB_VERSION_MAJOR) "." FERSLIB_STR(FERSLIB_VERSION_MINOR) "." FERSLIB_STR(FERSLIB_VERSION_PATCH) /*!<Library release version (string) */
#define FERSLIB_RELEASE_DATE			"06/06/2025"
/*! @} */

#define THROUGHPUT_METER			0		///< Must be 0 in normal operation (can be used to test the data throughput in different points of the readout process)

#define INVALID_TEMP				999		///< Invalid temperature 

/*!
* @ingroup Macros
* @defgroup BRD_Constant Board constants
* @brief FERS board family constants
* @{
*/
#define	FERSLIB_MAX_NCNC		  4		/*!< Max number of concentrators */
#define	FERSLIB_MAX_NBRD		  128	/*!< Max number of connected boards */
#define	FERSLIB_MAX_NCH			  128	/*!< Max number of channels per board (for all boards) */
#define	FERSLIB_MAX_NCH_5202	  64	/*!< Max number of channels per board */
#define	FERSLIB_MAX_NCH_5203	  128	/*!< Max number of channels per board */
#define	FERSLIB_MAX_NCH_5204	  64	/*!< Max number of channels per board */
#define	PICOTDC_NCH  			  64	/*!< number of picoTDC channels */
#define	FERSLIB_MAX_NTDL		  8		/*!< Max number of TDlinks (chains) in a concentrator */
#define	FERSLIB_MAX_NNODES		  16	/*!< Max number of nodes in a TDL chain */
/*! @} */


/*!
* @ingroup Macros
* @defgroup DBLOG Debug Log bitmask
* @brief Debug Log bitmask values
* @{
*/
#define	DBLOG_FERSLIB_MSG		 0x0001	/*!< Enable FERSlib to print log messages to console */
#define	DBLOG_RAW_DATA_OUTFILE	 0x0002	/*!< Enable read data function to dump event data (32 bit words) into a text file */
#define	DBLOG_LL_DATADUMP		 0x0004	/*!< Enable low level lib to dump raw data (from usb, eth and tdl) into a text file */
#define	DBLOG_LL_MSGDUMP		 0x0008	/*!< Enable low level lib to log messages (from usb, eth and tdl) into a text file */
#define	DBLOG_QUEUES			 0x0010	/*!< Enable messages from queues (push and pop) used in event sorting */
#define	DBLOG_RAW_DECODE		 0x0020	/*!< Enable messages from raw data decoding */
#define	DBLOG_LL_READDUMP		 0x0040	/*!< Enable low level read data to dump raw data (from usb, eth and tdl) into a text file */
#define	DBLOG_PARAMS			 0x0080	/*!< Enable dump of Board Parameters */
#define	DBLOG_CONFIG			 0x0100	/*!< Enable dump of Board Configuration */
#define	ENABLE_FERSLIB_LOGMSG	 1		/*!< Force the dump of FERSlib log message*/
/*! @} */

/*!
* @ingroup FERSlib_Enum
* @brief Status of the data RX thread
*/
typedef enum {
	RXSTATUS_OFF = 0,
	RXSTATUS_IDLE = 1,
	RXSTATUS_RUNNING = 2,
	RXSTATUS_EMPTYING = 3
} FERSLIB_RxThreadSatatus;

/*!
* @ingroup FERSlib_Enum
* @brief Readout status
*/
typedef enum {
	ROSTATUS_IDLE = 0,	/*!< idle (acquisition not running) */
	ROSTATUS_RUNNING = 1,	/*!< running (data readout) */
	ROSTATUS_EMPTYING = 2,	/*!< boards stopped, reading last data in the pipes */
	ROSTATUS_FLUSHING = 3,	/*!< flushing old data (clear) */
	RAWDATA_REPROCESS_FINISHED = 4	/*!< end of offline raw data reprocessing */
} FERSLIB_ReadoutStatus;

/*!
* @ingroup FERSlib_Enum
* @brief Error codes
*/
typedef enum {
	FERSLIB_SUCCESS = 0,					/*!< Operation completed succesfully */
	FERSLIB_ERR_GENERIC = -1,				/*!< Generic error */
	FERSLIB_ERR_COMMUNICATION = -2,			/*!< Communication error */
	FERSLIB_ERR_MAX_NBOARD_REACHED = -3,	/*!< Maximum number of devices supported exceeded */
	FERSLIB_ERR_DEVICE_ALREADY_OPENED = -4, /*! <Device already opened*/
	FERSLIB_ERR_INVALID_PATH = -5,			/*!< Invalid file/address path provided */
	FERSLIB_ERR_INVALID_HANDLE = -6,		/*!< Invalid handle used */
	FERSLIB_ERR_READOUT_ERROR = -7,			/*!< Readout error */
	FERSLIB_ERR_MALLOC_BUFFERS = -8,		/*!< Failed to allocate memory for buffers */
	FERSLIB_ERR_INVALID_BIC = -9,			/*!< Invalid BIC (Board Interface Controller) detected */
	FERSLIB_ERR_READOUT_NOT_INIT = -10,		/*!< Readout buffers not initialized */
	FERSLIB_ERR_CALIB_NOT_FOUND = -11,		/*!< Calibration data for A5256 not found */
	FERSLIB_ERR_PEDCALIB_NOT_FOUND = -11,	/*!< Pedestal calibration data in flash not found */
	FERSLIB_ERR_INVALID_FWFILE = -12,		/*!< Invalid firmware file provided */
	FERSLIB_ERR_UPGRADE_ERROR = -13,		/*!< Firmware upgrade failed */
	FERSLIB_ERR_INVALID_PARAM = -14,		/*!< Invalid parameter name provided */
	FERSLIB_ERR_NOT_APPLICABLE = -15,		/*!< Operation not applicable on the current FERS module family */
	FERSLIB_ERR_TDL_ERROR = -16,			/*!< Error generated during enum, sync or reset through TDL */
	FERSLIB_ERR_QUEUE_OVERRUN = -17,		/*!< Readout event-bulding queue overrun occurred during operation */
	FERSLIB_ERR_START_STOP_ERROR = -18,		/*!< Start or stop operation failed */
	FERSLIB_ERR_OPER_NOT_ALLOWED = -19,		/*!< Operation not allowed in the current state */
	FERSLIB_ERR_INVALID_CLK_SETTING = -20,	/*!< Invalid High resolution clock settings provided */
	FERSLIB_ERR_TDL_CHAIN_BROKEN = -21,		/*!< TDL chain broken detected */
	FERSLIB_ERR_TDL_CHAIN_DOWN = -22,		/*!< TDL chain down detected */
	FERSLIB_ERR_I2C_NACK = -23,				/*!< I2C NACK received */
	FERSLIB_ERR_CALIB_FAIL = -24,			/*!< A5256 threshold calibration process failed */
	FERSLIB_ERR_INVALID_FW = -25,			/*!< Invalid firmware detected */
	FERSLIB_ERR_INVALID_PARAM_VALUE = -26,	/*!< Invalid parameter value provided */
	FERSLIB_ERR_INVALID_PARAM_UNIT = -27	/*!< Invalid parameter unit specified */
} FERSLIB_ErrorCodes;

/*!
* @ingroup Macros
* @defgroup ACQMode Acquisition modes
* @brief Acquisition modes
* @{
*/
#define	ACQMODE_SPECT			  0x01	/*!< Spectroscopy Mode (Energy) */
#define	ACQMODE_TSPECT			  0x03	/*!< Spectroscopy + Timing Mode (Energy + Tstamp) */
#define	ACQMODE_TIMING_CSTART	  0x02	/*!< Timing Mode - Common Start (List) XROC */
#define	ACQMODE_TIMING_GATED      0x02	/*!< Timing Mode - Gated XROC */
#define	ACQMODE_TIMING_CSTOP	  0x12	/*!< Timing Mode - Common Stop (List) XROC */
#define	ACQMODE_TIMING_STREAMING  0x22  /*!< Timing Mode - Streaming (List) XROC */
#define	ACQMODE_COMMON_START	  0x02	/*!< Timing Mode - Common Start (List) 5203 */
#define	ACQMODE_COMMON_STOP		  0x12	/*!< Timing Mode - Common Stop (List) 5203 */
#define	ACQMODE_STREAMING		  0x22	/*!< Timing Mode - Streaming (List) 5203 */
#define	ACQMODE_TRG_MATCHING	  0x32	/*!< Timing Mode - Trigger Matching (List) 5203 */
#define	ACQMODE_TEST_MODE		  0x01	/*!< Test Mode - (Common start, List) 5203 */
#define	ACQMODE_COUNT			  0x04	/*!< Counting Mode (MCS) */
#define	ACQMODE_WAVE			  0x08	/*!< Waveform Mode */
/*! @} */

/*!
* @ingroup Macros
* @defgroup DTQ Data qualifier
* @brief Data Qualifier
* @{
*/
#define	DTQ_SPECT	0x01  /*!< Spectroscopy Mode (Energy) */
#define	DTQ_TIMING  0x02  /*!< Timing Mode */
#define	DTQ_COUNT	0x04  /*!< Counting Mode (MCS) */
#define	DTQ_WAVE	0x08  /*!< Waveform Mode */
#define	DTQ_TSPECT	0x03  /*!< Spectroscopy + Timing Mode (Energy + Tstamp) */
#define	DTQ_SERVICE 0x2F  /*!< Service event */
#define	DTQ_TEST	0xFF  /*!< Test Mode */
#define	DTQ_START	0x0F  /*!< Start Event */
/*! @} */

/*!
* @ingroup FERSlib_Enum
* @brief Event Building modes
*/
typedef enum {
	ROMODE_DISABLE_SORTING = 0x0000,	/*!< Disable sorting */
	ROMODE_TRGTIME_SORTING = 0x0001,	/*!< Enable event sorting by Trigger Tstamp */
	ROMODE_TRGID_SORTING = 0x0002	/*!< Enable event sorting by Trigger ID */
} FERSLIB_EvtBuildingModes;

/*!
* @ingroup FERSlib_Enum
* @brief Service event Modes
*/
typedef enum {
	SE_HV = 1,  /*!< Only Hv data */
	SE_COUNT = 2,  /*!< Only counter data */
	SE_ALL = 3  /*!< Hv + Counter data */
} FERSLIB_ServEventsMode;


/*!
* @ingroup Macros
* @brief Timeout values
*/
#define NODATA_TIMEOUT				100		/*!< time to wait after there is no data from the board to consider it empty */
#define STOP_TIMEOUT				500		/*!< timeout for forcing the RX thread to go in idle state after the stop of the run in the boards */
#define FERS_CONNECT_TIMEOUT		3		/*!< timeout connection in seconds */

/*!
* @ingroup Macros
* @defgroup CLK_DEF Clock definition
* @brief Clock and TDL delay definitions
* @{
*/
#define TDL_COMMAND_DELAY			1000000	/*!< Delay for the command execution in TDlink (the delay must be greater than the maximum delivery time of the command acress the TDL chains). 1 LSB = 10 ns */
#define CLK_PERIOD_5202				8		/*!< FPGA clock period in ns (5202)	*/
#define CLK_PERIOD_5203				12.8	/*!< FPGA clock period in ns (5203)	*/
#define CLK_PERIOD_5204				12.8	/*!< FPGA clock period in ns (5204) */
/*! @} */

/*!
* @ingroup Macros
* @defgroup TDC_CONST TDC constants
* @brief Configuration contants for TDC operations (5203 only)
* @{
*/
#define TDC_CLK_PERIOD				(CLK_PERIOD_5203*2)				/*!< TDC clock period in ns */
#define TDC_PULSER_CLK_PERIOD		(TDC_CLK_PERIOD / 32)			/*!< TDC Pulser clock */
#define TDC_LSB_ps					(TDC_CLK_PERIOD / 8 / 1.024)	/*!< TDC LSB in ps (clock * 8 + 1024 taps) */
#define CLK2LSB						4096
/*! @} */
// ---------------------

/*!
* @ingroup Macros
* @brief Readout constants
*/
#define MAX_WAVEFORM_LENGTH			2048	/*!< Maximum waveform length */
#define MAX_LIST_SIZE				2048	/*!< Maximum hits list size */
#define MAX_TEST_NWORDS				4		/*!< Maximum number of words in test event (5203) */
#define MAX_SERV_NWORDS				6		/*!< Max number of words in service event */


/*!
* @ingroup Macros
* @defgroup MEASMode Measurement modes
* @brief Measurement modes for 5203
* @{
*/
#define MEASMODE_LEAD_ONLY			0x01	/*!< Leading edge only mode */
#define MEASMODE_LEAD_TRAIL			0x03	/*!< Leading and trailing edges mode */
#define MEASMODE_LEAD_TOT8			0x05	/*!< Leading edge + ToT 8 bits mode */
#define MEASMODE_LEAD_TOT11			0x09	/*!< Leading edge + ToT 11 bits mode */
#define MEASMODE_OWLT(m)			(((m) == MEASMODE_LEAD_TOT8) || ((m) == MEASMODE_LEAD_TOT11))  /*! True if TDC MeasMode includes ToT */
/*! @} */

#define EDGE_LEAD					1
#define EDGE_TRAIL					0

/*!
* @ingroup FERSlib_Enum
* @brief Start/Stop modes
* 
*/
typedef enum {
	STARTRUN_ASYNC = 0,	/*!< Run start sent to each board individually */
	STARTRUN_CHAIN_T0 = 1,	/*!< Run start sent to board 0 and propagate in daisy chain through T0-out/T0-in */
	STARTRUN_CHAIN_T1 = 2,	/*!< Run start sent to board 0 and propagate in daisy chain through T1-out-T1-in */
	STARTRUN_TDL = 3,	/*!< Run start sent thorugh concentrator to all board in sync */
} FERSLIB_StartMode;

#define	STOPRUN_MANUAL			0	/*!< Janus parameter */
#define	STOPRUN_PRESET_TIME		1	/*!< Janus parameter */
#define	STOPRUN_PRESET_COUNTS	2	/*!< Janus parameter */

#define EVBLD_DISABLED				0	// Event Building disabled */
#define EVBLD_TRGTIME_SORTING		1	// Sorting events by timestamp of the trigger */
#define EVBLD_TRGID_SORTING			2	// Sorting events by the trigger ID */

// Citiroc Configuration
#define CITIROC_CFG_FROM_REGS		0
#define CITIROC_CFG_FROM_FILE		1


/*! 
* @ingroup Macros
* @brief Data width for Energy, ToA and ToT (5202)
*/
#define ENERGY_NBIT					14	/*!< Max nbits for PHA (5202) */
#define TOA_NBIT					16	/*!< Max nbits for ToA (5202) */
#define TOA_LSB_ns					0.5 /*!< LSB value of ToA in ns (5202) */
#define TOT_NBIT					9   /*!< Max #bits for ToT (5202) */


// Parameter Options
/*!
* @ingroup FERSlib_Enum
* @brief Glitch filter modes
* 
*/
typedef enum {
	GLITCHFILTERMODE_DISABLED	= 0,
	GLITCHFILTERMODE_TRAILING	= 1,
	GLITCHFILTERMODE_LEADING	= 2,
	GLITCHFILTERMODE_BOTH		= 3
} FERSLIB_GlitchFilterModes;

/*!
* @ingroup FERSlib_Enum
* @brief High resolution clock modes
*
*/
typedef enum {
	HRCLK_DISABLED				= 0,
	HRCLK_DAISY_CHAIN			= 1,
	HRCLK_FAN_OUT				= 2
} FERSLIB_HRClockModes;

/*!
* @ingroup Macros
* @defgroup EEPROMM EEPROM A5256 constants
* @brief EEPROM A5256 constants
* @{
*/
#define EEPROM_BIC_SIZE				44		/*!< EEPROM BIC size for A5256 */
#define EEPROM_BIC_PAGE				0		/*!< EEPROM page with Adapter IDcard */
#define EEPROM_CAL_PAGE				256		/*!< EEPROM page with calibrations data */
#define EEPROM_CAL_SIZE				256		/*!< EEPROM calibration section size */
/*! @} */

/*!
* @ingroup Macros
* @defgroup FlashConstants FERS Flash constants
* @brief FERS Flash constants
* @{
*/
#define FLASH_PAGE_SIZE				528	/*!< flash page size for AT54DB321 */
#define FLASH_BIC_PAGE				0	/*!< flash page with Board IDcard */
										 
#define FLASH_PEDCALIB_PAGE			4	/*!< flash page with Pedestal calibration XROC */
#define FLASH_PEDCALIB_BCK_PAGE		5	/*!< flash page with Pedestal calibration (backup) XROC */
										 
#define FLASH_THR_OFFS_PAGE			4	/*!< flash page with Threshold Offset calibration 5256 */
#define FLASH_THR_OFFS_BCK_PAGE		5	/*!< flash page with Threshold Offset calibration (backup) 5256 */
/*! @} */


/*!
* @ingroup Macros
* @{
*/
#define A5203_FLASH					0		/*!< Save calibration to FLASH */
#define A5256_EEPROM				1		/*!< Save calibration to EEPROM */
/*! @} */

// Handles and indexing
/*!
* @ingroup Macros
* @defgroup HI Handles and indexing
* @brief Handles and indexing 
* @{
*/
#define FERS_INDEX(handle)				((handle) & 0xFF)		/*!< Board Index */
#define FERS_CONNECTIONTYPE(handle)		((handle) & 0xF0000)	/*!< Connection Type */
#define FERS_CONNECTIONTYPE_ETH			0x00000	/*!< Handle bit for eth connection */
#define FERS_CONNECTIONTYPE_USB			0x10000	/*!< Handle bit for usb connection */
#define FERS_CONNECTIONTYPE_TDL			0x20000	/*!< Handle bit for tdl connection */
#define FERS_CONNECTIONTYPE_CNC			0x80000	/*!< Handle bit for connection to concentrator only  */
#define FERS_NODE(handle)				((handle >> 20) & 0xF)	/*!< Tdl node index */
#define FERS_CHAIN(handle)				((handle >> 24) & 0xF)	/*!< Tdl chain index */
#define FERS_CNCINDEX(handle)			((handle >> 30) & 0xF)	/*!< Concentrator index */
#define FERS_CNC_HANDLE(handle)			(0x80000 | ((handle >> 30) & 0xF)) /*!< returns the CNC handle to which a FERS unit is connected */
/*! @} */

// Fimrware upgrade via TDL
/*!
* @ingroup Macros
* @brief Fimrware upgrade via tdl constants
* 
*/
#define FUP_BA							0xFF000000
#define FUP_CONTROL_REG					1023
#define FUP_PARAM_REG					1022
#define FUP_RESULT_REG					1021
#define FUP_CMD_READ_VERSION			0xFF
#define POLY							0x82f63b78


// Definition of limits and sizes
#define FERSLIB_MAX_GW					20	/*!< max. number of generic write commads */ 

#define OUTFILE_RAW_LL					0x0001  // Can be a Janus parameter

/*!
* @ingroup Macros
* @brief Configuration Mode
*/
#define CFG_HARD	0	/*!< reset + configure (acq must be restarted) */
#define CFG_SOFT	1	/*!< runtime reconfigure params (no restart required) */

// Other macros
/*!
* @ingroup Macros
* @defgroup mM Min/Max
* @brief Re-define of Max and min
* @{
*/
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min 
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
/*! @} */

//******************************************************************
// TDL-chain Info Struct
//******************************************************************
/*!
 * @brief TDL chain information structure
 * @ingroup FERSlib_BStructs
 */
typedef struct {
	uint16_t Status;		/*!< Chain Status (0=not enumerated, 1=enumerated, ...) */
	uint16_t BoardCount;	/*!< Num of boards in the chain */
	float rrt;				/*!< */
	uint64_t EventCount;	/*!< total number of transferred events */
	uint64_t ByteCount;		/*!< total number of transferred bytes */
	float EventRate;		/*!< Current event rate (cps) */
	float Mbps;				/*!< Current transfer rate (Megabit per second) */
} FERS_TDL_ChainInfo_t;

//******************************************************************
// Concentrator Info Struct 
//******************************************************************
/*!
 * @brief Concentrator board information structure
 * @ingroup FERSlib_BStructs
 */
typedef struct {
	uint32_t pid;			//!< Board PID (5 decimal digits)
	char PCBrevision[16];	//!< PCB revision 
	char ModelCode[16];		//!< Model code, e.g. WDT5215XAAAA
	char ModelName[16];		//!< Model name, e.g. DT5215
	char FPGA_FWrev[20];	//!< FPGA FW revision 
	char SW_rev[20];		//!< Software Revision (embedded ARM)
	char MACaddr_10GbE[20];	//!< MAC address of 10 GbE
	uint16_t NumLink;		//!< Number of links
	FERS_TDL_ChainInfo_t ChainInfo[8];	//!< TDL Chain info
} FERS_CncInfo_t;

//******************************************************************
// FERS Board Info Struct 
//******************************************************************
/*!
 * @ingroup FERSlib_BStructs
 * @brief FERS board information structure
 */
typedef struct {
	uint32_t pid;			//!< Board PID (5 decimal digits)
	uint16_t FERSCode;		//!< FERS code, e.g. 5202
	uint8_t PCBrevision;	//!< PCB revision 
	char ModelCode[16];		//!< Model code, e.g. WA5202XAAAAA
	char ModelName[16];		//!< Model name, e.g. A5202
	uint8_t FormFactor;		//!< Form factor (0=naked version (A52XX), 1=boxed version (DT52XX)
	uint16_t NumCh;			//!< Number of channels
	uint32_t FPGA_FWrev;	//!< FPGA FW revision 
	uint32_t uC_FWrev;		//!< Microcontroller (uC) FW revision
} FERS_BoardInfo_t;

//******************************************************************
// A5256 Adapter Info Struct 
//******************************************************************
/*!
 * @brief A5256 adapter information structure
 * @ingroup FERSlib_BStructs
*/
typedef struct {
	uint32_t pid;			//!< Adapter PID (5 decimal digits)
	uint16_t AdapterCode;		//!< Adapter code, e.g. 5256
	uint8_t PCBrevision;	//!< PCB revision 
	char ModelCode[16];		//!< Model code, e.g. WA5256XAAAAA
	char ModelName[16];		//!< Model name, e.g. A5256
	uint16_t NumCh;			//!< Number of channels
} FERS_A5256_Info_t;

//******************************************************************
// Event Data Structures
//******************************************************************
// Spectroscopy Event (with or without timing)
/*!
 * @brief Spectroscopy Event data structure (with or without timing)
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;		//!< Timestamp in us
	double rel_tstamp_us;	//!< Relative timestamp in us, reset by Tref
	uint64_t trigger_id;	//!< Trigger ID
	uint64_t chmask;		//!< Channel Mask
	uint64_t qdmask;		//!< QD Mask
	uint16_t energyHG[64];	//!< High-gain energy values per channel
	uint16_t energyLG[64];	//!< Low-gain energy values per channel
	uint32_t tstamp[64];	//!< ToAs (used in TSPEC mode only) per channel 
	uint16_t ToT[64];		//!< ToT values (used in TSPEC mode only) per channel
} SpectEvent_t;

// Counting Event
/*!
 * @brief Counting Event data structure
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;		//!< Timestamp in us
	double rel_tstamp_us;	//!< Relative timestamp in us, reset by Tref
	uint64_t trigger_id;	//!< Trigger ID
	uint64_t chmask;		//!< Channel Mask from zero suppression
	uint32_t counts[64];	//!< Counts per channel
	uint32_t t_or_counts;	//!< T-OR counts
	uint32_t q_or_counts;	//!< Q-OR counts
} CountingEvent_t;

// Waveform Event
/*!
 * @brief Waveform Event data structure
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;		//!< Timestamp in us
	uint64_t trigger_id;	//!< Trigger ID
	uint16_t ns;			//!< 
	uint16_t *wave_hg;		//!< High-gain waveform samples
	uint16_t *wave_lg;		//!< Low-gain waveform samples
	uint8_t *dig_probes;	//!< Digital probes samples
} WaveEvent_t;

/*!
 * @brief List Event data structure for timing mode
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;		//!< Timestamp in microseconds
	uint64_t Tref_tstamp;	//!< Timestamp of reference time signal (5202 only)
	uint64_t tstamp_clk;	//!< Timestamp in LSB (5203 only)
	uint64_t trigger_id;	//!< Trigger ID
	uint16_t nhits;			//!< Number of hits
	uint32_t header1[8];	//!< PicoTDC header 1
	uint32_t header2[8];	//!< PicoTDC header 2
	uint32_t ow_trailer;	//!< PicoTDC one-word trailer
	uint32_t trailer[8];	//!< PicoTDC trailer
	uint8_t  channel[MAX_LIST_SIZE];	//!< List of channels
	uint8_t  edge[MAX_LIST_SIZE];		//!< List of associated edges
	uint32_t tstamp[MAX_LIST_SIZE];		//!< List of ToA in LSB (5202 only)
	uint32_t ToA[MAX_LIST_SIZE];		//!< List of ToA in LSB (other board)
	uint16_t ToT[MAX_LIST_SIZE];		//!< List of ToT in LSB
} ListEvent_t; // 5202 + 5203

/*!
 * @brief Service Event data structure, common to all FERS module
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;			//!< Time stamp of service event
	uint64_t update_time;		//!< Update time (epoch, ms)
	uint16_t pkt_size;			//!< Event size
	uint8_t version;			//!< Service event version
	uint8_t format;				//!< Event Format
	uint32_t ch_trg_cnt[FERSLIB_MAX_NCH_5202];  // Channel trigger counts 
	uint32_t q_or_cnt;			//!< Q-OR counts value
	uint32_t t_or_cnt;			//!< T-OR counts value
	float tempFPGA;				//!< FPGA core temperature 
	float tempBoard;			//!< Board temperature (near uC PIC)
	float tempTDC[2];			//!< Temperature of TDC0 and TDC1
	float tempHV;				//!< High-voltage module temperature
	float tempDetector;			//!< Detector temperature (referred to ?)
	float hv_Vmon;				//!< HV voltage monitor
	float hv_Imon;				//!< HV current monitor
	uint8_t hv_status_on;		//!< HV status ON/OFF
	uint8_t hv_status_ramp;		//!< HV ramp status
	uint8_t hv_status_ovv;		//!< HV over-voltage status
	uint8_t hv_status_ovc;		//!< HV over-current status
	uint16_t Status;			//!< Status Register
	uint16_t TDCROStatus;		//!< TDC Readout Status Register
	uint64_t ChAlmFullFlags[2];	//!< Channel Almost Full flag (from picoTDC)
	uint32_t ReadoutFlags;		//!< Readout Flags from picoTDC and FPGA
	uint32_t TotTrg_cnt;		//!< Total triggers counter
	uint32_t RejTrg_cnt;		//!< Rejected triggers counter
	uint32_t SupprTrg_cnt;		//!< Zero suppressed triggers counter
} ServEvent_t; // 5202 + 5203


// Test Mode Event (fixed data patterns)
/*!
 * @brief Test Mode Event data structure
 * @ingroup FERSlib_DStructs
 */
typedef struct {
	double tstamp_us;		//!< Timestamp in us
	uint64_t trigger_id;	//!< Trigger ID
	uint16_t nwords;		//!< Number of words
	uint32_t test_data[MAX_TEST_NWORDS];	//!< Test data words
} TestEvent_t;


/*!
* @ingroup Macros
* @defgroup A5256 A5256 macros
* @brief A5256 macros
* @{
*/
#define A5256_CH0_POSITIVE				0  //!< CH0 positive polarity
#define A5256_CH0_NEGATIVE				1  //!< CH0 negative polarity
#define A5256_CH0_DUAL					2  //!< CH0 dual polarity


#define ADAPTER_NONE					0x0000	//!< No adapter
#define ADAPTER_A5255					0x0000	//!< A5255 is just a mechanical adapter (same as NONE)
#define ADAPTER_A5256					0x0002	//!< 16+1 ch discriminator with programmable thresholds
#define ADAPTER_A5256_REV0_POS			0x0100	//!< Proto0 of A5256 used with positive signals (discontinued)
#define ADAPTER_A5256_REV0_NEG			0x0101	//!< Proto0 of A5256 used with negative signals (discontinued)


#define NC -1	// DNIN=?

#define A5256_DAC_LSB				((float)2500/4095) 									//!< LSB of the threshold in mV
#define A5256_mV_to_DAC(mV)			(uint32_t)round((1250 - (mV)) / A5256_DAC_LSB)		//!< Convert Threshold from mV into DAC LSB
#define A5256_DAC_to_mV(lsb)		(1250 - (float)((lsb) * A5256_DAC_LSB))				//!< Convert Threshold from DAC LSB to mV
/*! @} */

#ifdef __cplusplus
extern "C" {
#endif
	// *****************************************************************
	// External Variables (defined in FERlib.c)
	// *****************************************************************
	/*!
	 * @cond EXCLUDE
	 */
	extern uint16_t PedestalLG[FERSLIB_MAX_NBRD][FERSLIB_MAX_NCH_5202];	//!< Low-Gain Pedestals (calibrate PHA Mux Readout)    
	extern uint16_t PedestalHG[FERSLIB_MAX_NBRD][FERSLIB_MAX_NCH_5202];	//!< High-Gain Pedestals (calibrate PHA Mux Readout)
	extern uint16_t CommonPedestal;										//!< Common pedestal added to all channels
	extern int EnablePedCal;											//!< 0 = disable calibration, 1 = enable calibration
	extern uint32_t TDL_NumNodes[FERSLIB_MAX_NCNC][FERSLIB_MAX_NTDL];	//!< Number of nodes in the chain

	//extern uint8_t EnableRawData;								//!< Flag to enable Low Level data saving
	extern uint8_t ProcessRawData;								//!< Flag to enable the reading of rawData file saved
	//extern uint8_t EnableMaxSizeFile;							//!< Flag to enable the maximum size eforcement for Raw Data file saving
	extern uint8_t EnableSubRun;								//!< Flag to enable sub run incrementing
	//extern float MaxSizeRawDataFile;							//!< Maximum size for saving low-level data
	extern char RawDataFilename[FERSLIB_MAX_NBRD][500];			//!< Rawdata Filename (eth, usb connection)
	extern char RawDataFilenameTdl[FERSLIB_MAX_NBRD][500];		//!< Rawdata Filename (tdl connection)
	extern int FERS_Offline;									//!< Flag to indicate offline connection mode for raw data reading

	extern int FERS_RunningCnt;									//!< Number of boards currently running
	extern int FERS_ReadoutStatus;								//!< Status of the readout processes (idle, running, flushing, etc...)
	extern int FERS_TotalAllocatedMem;							//!< Total allocated memory for library in byte 
	extern int DebugLogs;										//!< Level of debug logs to be generated

	//extern mutex_t FERS_mutex;									//!< Mutex for access to shared resources (FERScfg, InitBuffers ...)
	/*! 
	 * @endcond
	 */
	
	// *****************************************************************
	// Funtion Prototypes
	// *****************************************************************
	/*!
	* @defgroup Functions API
	* @brief FERSlib Application programmin interface
	*/
	
	CAEN_FERS_DLLAPI int ConfigureProbe(int handle);

	// -----------------------------------------------------------------
	// Get FERSlib info
	// -----------------------------------------------------------------
	/*!
	* @ingroup Functions
	* @defgroup release Library info
	*/
	/*!
	 * @brief   Get the release number of the library
	 * @return					FERSlib release number
	 * @ingroup release
	*/
	CAEN_FERS_DLLAPI char* FERS_GetLibReleaseNum();

	/*!
	 * @brief   Get the release date of the library
	 * @return					FERSlib release date
	 * @ingroup release
	*/
	CAEN_FERS_DLLAPI char* FERS_GetLibReleaseDate();

	// -----------------------------------------------------------------
	// Messaging and errors
	// -----------------------------------------------------------------

	/*!
	* @ingroup Functions
	* @defgroup MsgErr Messaging and errors
	*/

	/*!
	 * @brief	Write on FERSlib log file
	 * @param[in] fmt				Message to dump on log
	 * @param[in] ...				Arguments specifying data to print
	 * @return						0
	 * @ingroup MsgErr
	*/
	CAEN_FERS_DLLAPI int FERS_LibMsg(char* fmt, ...);

	/*!
	 * @brief   Get the last message error of library
	 * @param[out]	description		Last error message written by library [max size 1024]
	 * @return						0
	 * @ingroup MsgErr
	*/
	CAEN_FERS_DLLAPI int FERS_GetLastError(char description[1024]);

	// Not exported
	void _setLastLocalError(const char* description, ...);

	// -----------------------------------------------------------------
	// Raw Data Save and Load
	// -----------------------------------------------------------------
	// DNIN: Is it better to have an API to enable both RawData and LimitFileSize
	//		 and then do the open directly inside the StartAcquisition?
	///*!
	//* @ingroup Functions
	//* @defgroup RawData RawData Saving/Loading
	//*/
	///*!
	// * @brief   Enable the rawdata file saving and set default file name
	// * @param[in] RawDataEnable		bool to activate rawdata file saving
	// * @param[in] DataOutputPath	path where rawdata will be saved
	// * @param[in] RunNumber			number of the current run
	// * @return						0
	// * @ingroup RawData
	//*/
	//CAEN_FERS_DLLAPI int FERS_EnableRawdataWriteFile(uint8_t RawDataEnable, char* DataOutputPath, int RunNumber);

	///*!
	// * @brief   Enable and set the size limit for rawdata file
	// * @param[in] LimitSizeEnable		Flag to enable the size limit
	// * @param[in] MaxSizeRawOutputFile	Maximum size of output file
	// * @return	 0
	// * @ingroup RawData
	//*/
	//CAEN_FERS_DLLAPI int FERS_EnableLimitRawdataFileSize(uint8_t LimitSizeEnable, float MaxSizeRawOutputFile);

	//CAEN_FERS_DLLAPI int FERS_SetRawdataReadFile(char DataRawFilePath[500], int brd);

	/*!
	 * @brief   Open raw data file, to be done before starting the run
	 * @param[in] *handle		pointer to the handles array
	 * @param[in] RunNum		number of the starting run
	 * @return					0 
	 * @ingroup RawData
	*/
	CAEN_FERS_DLLAPI int FERS_OpenRawDataFile(int* handle, int RunNum);

	/*!
     * @brief   Close raw data file after run stops
     * @param[in] *handle		pointer to the board handles array
     * @return					0
     * @ingroup RawData
    */
	CAEN_FERS_DLLAPI int FERS_CloseRawDataFile(int* handle);
	
	/*!
	* @brief	Get the clock of the FERS board
	* @parma[in] handle		board handle
	* return				Clock period in ns
	* @ingroup	OC
	*/
	CAEN_FERS_DLLAPI float FERS_GetClockPeriod(int handle);
	//CAEN_FERS_DLLAPI int FERS_SetPedestalOffline(uint16_t PedLG[FERSLIB_MAX_NCH_5202], uint16_t PedHG[FERSLIB_MAX_NCH_5202], int handle);
	
	// -----------------------------------------------------------------
	// Open/Close
	// -----------------------------------------------------------------
	/*!
	* @ingroup Functions
	* @defgroup OC Open/Close
	*/

	/*!
	 * @brief   Open a device (either FERS board, concentrator or offline)
	 * @param[in] path				device path (e.g. eth:192.168.50.125:tdl:0:0)
	 * @param[in] *handle			device handle
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup OC
	*/
	CAEN_FERS_DLLAPI int FERS_OpenDevice(char* path, int* handle);

	/*!
     * @brief   Check if a device is already opened
     * @param[in] *path				path to be checked
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_IsOpen(char* path);

	/*!
     * @brief   Cloase a device (either FERS board or concentrator)
     * @param[in] handle			handle of the device to be closed
     * @return						0
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_CloseDevice(int handle);

	/*!
     * @brief   Return the total size of the allocated buffers for the readout of all boards
     * @return						memory allocated in Byte
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_TotalAllocatedMemory();

	/*!
     * @brief   Restore the factory IP address (192.168.50.3) of the device. The board must be connected through the USB port
     * @param[in] handle			handle of the board to be reset
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_Reset_IPaddress(int handle);

	/*!
     * @brief   Find the path of the concentrator to which a device is connected
	 * @param[out] cnc_path			concentrator path
	 * @param[in] dev_path			device address
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_Get_CncPath(char* dev_path, char* cnc_path);

	/*!
     * @brief   Send a sync broadcast command via TDL 
     * @param[in] handle			concentrator handel
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI int FERS_InitTDLchains(int handle, float DelayAdjust[FERSLIB_MAX_NTDL][FERSLIB_MAX_NNODES]);

	/*!
     * @brief   Check if TDL chains are initialized
     * @param[in] handle			concentator handle
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup OC
    */
	CAEN_FERS_DLLAPI bool FERS_TDLchainsInitialized(int handle);

	//CAEN_FERS_DLLAPI int FERS_SetOffline(int offline);
	//CAEN_FERS_DLLAPI int FERS_SetBoardInfo(FERS_BoardInfo_t* BrdInfo, int b);
	//CAEN_FERS_DLLAPI int FERS_SetConcentratorInfo(int handle, FERS_CncInfo_t* cinfo);

	/*!
	 * @brief			Get the number of boards connected
	 * 
	 * @return			number of board connected
	 * @ingroup OC
	*/
	CAEN_FERS_DLLAPI uint16_t FERS_GetNumBrdConnected();


	// -----------------------------------------------------------------
	// Register Read/Write
	// -----------------------------------------------------------------
	/*!
	* @ingroup Functions
	* @defgroup RW Read/Write registers
	*/

	/*!
	 * @brief						Read a register of a FERS board
	 * 
	 * @param[out] data				register data
	 * @param[in] handle			device handle
	 * @param[in] address			register address
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_ReadRegister(int handle, uint32_t address, uint32_t* data);

	/*!
     * @brief						Write a register of a FERS board
	 * 
     * @param[in] handle			device handle
	 * @param[in] address			register address
	 * @param[in] data				register data
     * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
     * @ingroup RW
    */
	CAEN_FERS_DLLAPI int FERS_WriteRegister(int handle, uint32_t address, uint32_t data);

	/*!
	 * @brief						Write a slice of a register of a FERS board
	 * 
	 * @param[in] handle			device handle
	 * @param[in] address			register address
	 * @param[in] start_bit			first bit of the slice (included)
	 * @param[in] stop_bit			last bit of the slice (included)
	 * @param[in] data				slice data
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_WriteRegisterSlice(int handle, uint32_t address, uint32_t start_bit, uint32_t stop_bit, uint32_t data);

	/*!
	 * @brief						
	 Send a command to the board
	 * @param[in] handle			device handle
	 * @param[in] cmd				command opcode
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_SendCommand(int handle, uint32_t cmd);

	/*!
	 * @brief						Send a broadcast command to multiple boards connected to a concentrator
	 * 
	 * @param[in] handle			device handles of all the board that should receive the command
	 * @param[in] cmd				command opcode
	 * @param[in] delay				execution delay (0 for automatic)
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_SendCommandBroadcast(int* handle, uint32_t cmd, uint32_t delay);

	/*!
	 * @brief						Read a register of an I2C register (picoTDC, PLL, ...)
	 * 
	 * @param[out] reg_data			register data
	 * @param[in] handle			device handle
	 * @param[in] i2c_dev_addr		I2C device address (7 bit)
	 * @param[in] reg_addr			register address (in the device)
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_I2C_ReadRegister(int handle, uint32_t i2c_dev_addr, uint32_t reg_addr, uint32_t* reg_data);

	/*!
	 * @brief						Write a register of an I2C register (picoTDC, PLL, ...)
	 * 
	 * @param[in] handle			device handle
	 * @param[in] i2c_dev_addr		I2C device address (7 bit)
	 * @param[in] reg_addr			register address (in the device)
	 * @param[in] reg_data			register data
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_I2C_WriteRegister(int handle, uint32_t i2c_dev_addr, uint32_t reg_addr, uint32_t reg_data);

	/*!
	 * @brief						Write a slice of a register of an I2C device 
	 * 
	 * @param[in] handle			device handle
	 * @param[in] i2c_dev_addr		I2C device address (7 bit)
	 * @param[in] address			register address (in the device)
	 * @param[in] start_bit			first bit of the slice (included)
	 * @param[in] stop_bit			last bit of the slice (included)
	 * @param[in] data				slice data
	 * @return						0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_I2C_WriteRegisterSlice(int handle, uint32_t i2c_dev_addr, uint32_t address, uint32_t start_bit, uint32_t stop_bit, uint32_t data);
	//CAEN_FERS_DLLAPI int FERS_ReadBoardInfo(int handle, FERS_BoardInfo_t* binfo);

	// -----------------------------------------------------------------
	// Flash Read/Write
	// -----------------------------------------------------------------
	/*!
	 * @ingroup RW
	 * @brief				Read a page from the flash memory
	 *
	 * @param[out] data		Buffer to store the read data
	 * @param[in] handle	Handle to the FERS device
	 * @param[in] pagenum	Page number to read
	 * @param[in] size		Number of bytes to read
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	*/
	CAEN_FERS_DLLAPI int FERS_ReadFlashPage(int handle, int pagenum, int size, uint8_t* data);

	/*!
	 * @ingroup RW
	 * @brief				Write a page of the flash memory
	 * @warning				The flash memory contains vital parameters for the board. Overwriting certain pages can damage the hardware!!! Do no use this function withou contacting CAEN first
	 *
	 * @param[in] handle	Handle to the FERS device
	 * @param[in] pagenum	Page number to write
	 * @param[in] size		Number of bytes to write
	 * @param[in] data		Buffer containing the data to write
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	*/
	CAEN_FERS_DLLAPI int FERS_WriteFlashPage(int handle, int pagenum, int size, uint8_t* data);


	// -----------------------------------------------------------------
	// XROC Register Read/Write
	// -----------------------------------------------------------------
	/*!
	 * @ingroup RW
	 * @brief					Write a register of the XROC ASIC chip via I2C
	 *
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] page_addr		Page address of the register, indentifies the register group
	 * @param[in] sub_addr		Sub-address of the register, identifies the register in the page
	 * @param[in] data			Data to write
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_XROC_WriteRegister(int handle, int page_addr, int sub_addr, uint8_t data);

	/*!
	 * @ingroup RW
	 * @brief					Read a register of the XROC ASIC chip via I2C
	 *
	 * @param[out]	data		Pointer to store the read value
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] page_addr		Page address of the register, identifies the register group
	 * @param[in] sub_addr		Sub-address of the register, identifies the register in the page
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_XROC_ReadRegister(int handle, int page_addr, int sub_addr, uint8_t* data);

	/*!
	 * @ingroup RW
	 * @brief					Write a register slice of the XROC ASIC chip via I2C
	 *
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] page_addr		Page address, identifies the register group
	 * @param[in] sub_addr		Sub-address, identifies the register in the page
	 * @param[in] start_bit		First bit of the slice (included)
	 * @param[in] stop_bit		Last bit of the slice (included)
	 * @param[in] data			Data to write
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_XROC_WriteRegisterSilce(int handle, int page_addr, int sub_addr, uint32_t start_bit, uint32_t stop_bit, uint8_t data);


	/*!
	 * @brief					Read concentrator info from the device
	 * 
	 * @param[out] cinfo		concentrator info structure
	 * @param[in] handle		concentrator handle
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_ReadConcentratorInfo(int handle, FERS_CncInfo_t* cinfo);

	/*!
	 * @brief					Write Board info into the relevant flash memory page 
	 * @warning					The flash memory contains vital parameters for the board. Overwriting certain pages can damage the hardware!!! Do not use this function without contacting CAEN first 
	 * 
	 * @param[in] handle		device handle
	 * @param[in] binfo			board info structure
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup RW
	*/
	CAEN_FERS_DLLAPI int FERS_WriteBoardInfo(int handle, FERS_BoardInfo_t binfo);

	/*!
	* @ingroup Functions
	* @defgroup Ped Pedestal
	*/

	/*!
	 * @brief					Write Pedestal calibration
	 * @warning					The flash memory contains vital parameters for the board. Overwriting certain pages can damage the hardware!!! Do not use this function without contacting CAEN first 
	 * 
	 * @param[in] handle		device handle
	 * @param[in] PedLG			Low Gain pedestal array[64]
	 * @param[in] PedHG			High Gain pedestal array[64]
	 * @param[in] dco			DCoffset (use NULL to keep old values)
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup Ped
	*/
	CAEN_FERS_DLLAPI int FERS_WritePedestals(int handle, uint16_t* PedLG, uint16_t* PedHG, uint16_t* dco);

	/*!
	 * @brief					Read pedestal calibration and DC offset  
	 * @warning					The flash memory contains vital parameters for the board. Overwriting certain pages can damage the hardware!!! Do not use this function without contacting CAEN first 
	 * 
	 * @param[in] handle		device handle
	 * @param[out] date			date of calibration saved (DD/MM/YYYY)
	 * @param[out] PedLG		Log Gain pedestal array[64]
	 * @param[out] PedHG		High Gain pedestal array[64]
	 * @param[out] dco			DCoffset (DAC). 4 values. Use NULL pointer if not requested
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup Ped
	*/
	CAEN_FERS_DLLAPI int FERS_ReadPedestalsFromFlash(int handle, char* date, uint16_t* PedLG, uint16_t* PedHG, uint16_t* dco);

	/*!
	 * @brief					Switch to pedestal backup page
	 * 
	 * @param[in] handle		device handle
	 * @param[in] EnBckPage		EnBckPage: 0=normal page, 1=packup page
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup Ped
	*/
	CAEN_FERS_DLLAPI int FERS_PedestalBackupPage(int handle, int EnBckPage);

	/*!
	* @brief					Set a common pedestal (applied to all channels after pedestal calibration) 
	* @warning					This function enables pedestal calibration
	* 
	* @param[in] handle			device handle
	* @param[in] Pedestal		common pedestal
	* @return					0
	* @ingroup Ped
	*/
	CAEN_FERS_DLLAPI int FERS_SetCommonPedestal(int handle, uint16_t Pedestal);

	/*!
	* @brief					Enable / Disabled pedestal calibration
	* 
	* @param[in] handle			Device handle
	* @param[in] enable			disabled, enabled
	* @return					0
	* @ingroup Ped
	*/
	CAEN_FERS_DLLAPI int FERS_EnablePedestalCalibration(int handle, int enable);

	int FERS_GetChannelPedestalBeforeCalib(int handle, int ch, uint16_t* PedLG, uint16_t* PedHG);

	CAEN_FERS_DLLAPI int FERS_SetEnergyBitsRange(uint16_t EnergyRange);
	//CAEN_FERS_DLLAPI int FERS_Model(int handle);				// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->Model : 0)


	/*!
	* @ingroup Functions
	* @defgroup FERS_BInfo Board info
	*/

	/*!
	 * @brief   Get board info saved into the library structure
	 * @param[out] BrdInfo			board information saved on library
	 * @param[in] handle			handle of the board
	 * @return						0
	 * @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI int FERS_GetBoardInfo(int handle, FERS_BoardInfo_t* BrdInfo);

	/*!
	 * @brief   Get concentrator info saved into the library structure
	 * @param[out] BrdInfo			concentrator information saved in library
	 * @param[in] handle			concentrator handle
	 * @return						0
	 * @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI int FERS_GetCncInfo(int handle, FERS_CncInfo_t* BrdInfo);

	/*!
	* @brief				Get the board PID
	* 
	* @param[in] handle		Handle to the FERS device.
	* @return				PID of the board 
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint32_t FERS_pid(int handle);				// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->pid : 0)
	
	/*!
	* @brief				Get the code of the FERS board
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				FERS code 
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint16_t FERS_Code(int handle);			// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode : 0)

	/*!
	* @brief				Get the model name of the FERS board
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				FERS model name
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI char* FERS_ModelName(int handle);			// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->ModelName : "")

	/*!
	* @brief				Get the FPGA firmware revision of the FERS board
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				FPGA firmware revision
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint32_t FERS_FPGA_FWrev(int handle);		// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev : 0)

	/*!
	* @brief				Get the major revision of the FPGA firmware
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				FPGA firmware major revision
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint32_t FERS_FPGA_FW_MajorRev(int handle);// ((FERS_INDEX(handle) >= 0) ? ((FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev) >> 8) & 0xFF : 0)

	/*!
	* @brief				Get the minor revision of the FPGA firmware
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				FPGA firmware minor revision
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint32_t FERS_FPGA_FW_MinorRev(int handle);// ((FERS_INDEX(handle) >= 0) ? (FERS_BoardInfo[FERS_INDEX(handle)]->FPGA_FWrev) & 0xFF : 0)

	/*!
	* @brief				Get them microcontroller firmware revision
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				Microcontroller firmware revision
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint32_t FERS_uC_FWrev(int handle);		// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->uC_FWrev : 0)

	/*!
	* @brief				Get the number of channels of the FERS board
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				Number of channels of FERS board
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint16_t FERS_NumChannels(int handle);		// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->NumCh : 0)

	/*!
	* @brief				Get the PCB revision of the FERS board
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				PCB revision of FERS board
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI uint8_t FERS_PCB_Rev(int handle);			// ((FERS_INDEX(handle) >= 0) ? FERS_BoardInfo[FERS_INDEX(handle)]->PCBrevision : 0)

	/*!
	* @brief				Check if the board mounts an XROC ASIC chip
	* 
	* @param[in] handle		Handle to the FERS device
	* @return				true is the board mounts a XROC ASIC chip, false otherwise
	* @ingroup FERS_BInfo
	*/
	CAEN_FERS_DLLAPI bool FERS_IsXROC(int handle);

	// DNIN: This 2 should be intern function. The user should only set the DebugLogMask
	/*!
	* @ingroup Functions
	* @brief					Dump the board register information to a file
	* 
	* @param[in] handle			Handle to the FERS device
	* @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	*/
	CAEN_FERS_DLLAPI int FERS_DumpBoardRegister(int handle);

	/*!
	 * @ingroup Functions
	 * @brief				Dump the saved configuration
	 * 
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_DumpCfgSaved(int handle);


	/*!
	* @ingroup Functions
	* @defgroup Temp Read sensors temperature
	*/

	/*!
	 * @ingroup Temp
	 * @brief					Get the temperature from picoTDC0 (on the board)
	 * 
	 * @param[out]	temp		TDC temperature in Celsiuse
	 * @param[in]	handle		Handle to the FERS device
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_Get_TDC0_Temp(int handle, float* temp);

	/*!
	 * @ingroup Temp
	 * @brief				Get the temperature from picoTDC1 (on the mezzanine)
	 * 
	 * @param[out] temp		TDC temperature in Celsius
	 * @param[in]  handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	*/
	CAEN_FERS_DLLAPI int FERS_Get_TDC1_Temp(int handle, float* temp);

	/*!
	 * @brief				Get FPGA die temperature
	 *
	 * @param[out] temp		FPGA temperature in Celsius
	 * @param[in] handle		device handle
	 * @return				0
	 * @ingroup Temp
	 */
	CAEN_FERS_DLLAPI int FERS_Get_FPGA_Temp(int handle, float* temp);


	/*!
	* @brief					Get Board temperature, between PIC and FPGA
	*
	* @param[out] temp			PIC board temperature in Celsius
	* @param[in] handle			device handle
	* @return					0
	* @ingroup Temp
	*/
	CAEN_FERS_DLLAPI int FERS_Get_Board_Temp(int handle, float* temp);




	// -----------------------------------------------------------------
	// High Voltage Control
	// -----------------------------------------------------------------

	/*!
	* @ingroup Functions
	* @defgroup HV High Voltage control
	*/

	/*!
	 * @ingroup HV
	 * @brief				Initialize the HV communication bus (I2C)
	 * 
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Init(int handle);

	/*!
	 * @ingroup HV
	 * @brief					Write a value of the HV
	 * 
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] reg_addr		Address of the register
	 * @param[in] dtype			Data type (0=signed int, 1=fixed point (Val*10000), 2=unsigned int, 3=float)
	 * @param[in] reg_data		Data to write
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_WriteReg(int handle, uint32_t reg_addr, uint32_t dtype, uint32_t reg_data);

	/*!
	 * @ingroup HV
	 * @brief					Read a register of HV
	 * 
	 * @param[out] reg_data		Pointer to the read value
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] reg_addr		Address of the register
	 * @param[in] dtype			Data type (0=signed int, 1=fixed point (Val*10000), 2=unsigned int, 3=float)
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_ReadReg(int handle, uint32_t reg_addr, uint32_t dtype, uint32_t* reg_data);

	/*!
	 * @ingroup HV
	 * @brief					Turn the high voltage module ON or OFF
	 * 
	 * @param[in] handle		Handle to the FERS device
	 * @param[in] OnOff			1 to turn on, 0 to turn off
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Set_OnOff(int handle, int OnOff);

	/*!
	 * @ingroup HV
	 * @brief					Get the status of the high voltage module
	 * 
	 * @param[out] OnOff		Pointer to the on/off status (1 = on, 0 = off)
	 * @param[out] Ramping		Pointer to the ramping status (1 = ramping, 0 = not ramping)
	 * @param[out] OvC			Pointer to the overcurrent status (1 = fault, 0 = normal)
	 * @param[out] OvV			Pointer to the overvoltage status (1 = fault, 0 = normal)
	 * @param[in] handle		Handle to the FERS device
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_Status(int handle, int* OnOff, int* Ramping, int* OvC, int* OvV);

	/*!
	 * @ingroup HV
	 * @brief				Get the serial number of the high voltage module
	 * 
	 * @param[out] sernum	Pointer to the Getd serial number
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_SerNum(int handle, int* sernum);

	/*!
	 * @ingroup HV
	 * @brief				Set HV output voltage (bias)
	 * 
	 * @param[in] handle	Handle to the FERS device
	 * @param[in] vbias		Output bias voltage in Volts
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Set_Vbias(int handle, float vbias);

	/*!
	 * @ingroup HV
	 * @brief				Get the output bias voltage set
	 * 
	 * @param[out] vbias	Pointer to the output voltage setting in Volts (read from register)
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_Vbias(int handle, float* vbias);

	/*!
	 * @ingroup HV
	 * @brief				Get HV output voltage (HV read back with internal ADC)
	 * 
	 * @param[out] vmon		Pointer to the monitored voltage in Volts
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_Vmon(int handle, float* vmon);

	/*!
	 * @ingroup HV
	 * @brief				Set the maximum output current from the high voltage module
	 * 
	 * @param[in] handle	Handle to the FERS device
	 * @param[in] imax		Maximum output current in mA
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Set_Imax(int handle, float imax);

	/*!
	 * @ingroup HV
	 * @brief				Get the maximum allowed current of the high voltage module
	 * 
	 * @param[out]	imax	Pointer to the maximum current
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_Imax(int handle, float* imax);

	/*!
	 * @ingroup HV
	 * @brief				Get the output monitored current flowing into the detector
	 * 
	 * @param[out] imon		Pointer to the output current in mA
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_Imon(int handle, float* imon);

	/*!
	 * @ingroup HV
	 * @brief				Get the internal temperature of the HV module
	 * 
	 * @param[out] temp		Pointer to the internal temperature in Celsius
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_IntTemp(int handle, float* temp);

	/*!
	 * @ingroup HV
	 * @brief				Get external temperature (of the detector). Temp sensor must be connected to dedicated lines
	 * 
	 * @param[out] temp		Pointer to the detector temperature in Celsius
	 * @param[in] handle	Handle to the FERS device
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Get_DetectorTemp(int handle, float* temp);

	/*!
	 * @ingroup HV
	 * @brief						Set coefficients for external temperature sensor. T = V*V*c2 + V*c1 + c0
	 * 
	 * @param [in] handle			Handle to the FERS device
	 * @param [in] Tsens_coeff		Coefficients array (c0=offset, c1=linear, c2=quadratic)
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Set_Tsens_Coeff(int handle, float Tsens_coeff[3]);

	/*!
	 * @ingroup HV
	 * @brief						Set coefficient for Vbias temperature feedback 
	 * 
	 * @param [in] handle			Handle to the FERS device
	 * @param [in] enable			1 to enable, 0 to disable
	 * @param [in] Tsens_coeff		Coefficients for temperature feedback (mV/C)
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_HV_Set_TempFeedback(int handle, int enable, float Tsens_coeff);


	// -----------------------------------------------------------------
	// TDC Config and readout (for test)
	// -----------------------------------------------------------------
	CAEN_FERS_DLLAPI int FERS_TDC_WriteReg(int handle, int tdc_id, uint32_t addr, uint32_t data);
	CAEN_FERS_DLLAPI int FERS_TDC_ReadReg(int handle, int tdc_id, uint32_t addr, uint32_t* data);
	CAEN_FERS_DLLAPI int FERS_TDC_Config(int handle, int tdc_id, int StartSrc, int StopSrc);
	CAEN_FERS_DLLAPI int FERS_TDC_DoStartStopMeasurement(int handle, int tdc_id, double* tof_ns);


	// -----------------------------------------------------------------
	// Set/Get param, parse file and configure FERS
	// -----------------------------------------------------------------
	
	/*!
	* @ingroup Functions
	* @defgroup cfg FERS configuration
	*/

	/*!
	 * @ingroup cfg
	 * @brief				Configures a FERS board
	 *
	 * @param[in] handle	Board handle
	 * @param[in] mode		Configuration mode
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_configure(int handle, int mode);

	/*!
	 * @ingroup cfg
	 * @brief Loads a configuration file.
	 *
	 * @param[in] filepath  Path to the configuration file
	 * @return 0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_LoadConfigFile(char* filepath);

	/*!
	 * @ingroup cfg
	 * @brief					Set a parameter by name. The function assigns the value to the relevant parameter in the 
	 *							FERScfg struct, but it doesn't actually write it into the board (this is done by the "FERS_configure" function)
	 * 
	 * @param[in] handle		Board handle
	 * @param[in] param_name	Name of the parameter to set
	 * @param[in] value			Value to set for the parameter
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_SetParam(int handle, const char* param_name, const char* value_original);

	/*!
	 * @ingroup cfg
	 * @brief					Get the value of a parameter. The function reads the value from the relevant parameter in the
	 *							FERScfg struct, but it doesn't actually read it from the board 
	 * 
	 * @param[out] value		Pointer to the value of the parameter
	 * @param[in]  handle		Board handle
	 * @param[in]  param_name	Name of the parameter to get
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_GetParam(int handle, char* param_name, char* value);


	// -----------------------------------------------------------------
	// Data Readout
	// -----------------------------------------------------------------
	
	/*!
	* @ingroup Functions
	* @defgroup RO Data readout
	*/

	/*!
	 * @ingroup RO
	 * @brief						Init readout for one board (allocate buffers and initialize variables)
	 *
	 * @param[out] AllocatedSize	Pointer to the tot. num. of bytes allocated for data buffers and descriptors
	 * @param[in]  handle			Board handle
	 * @param[in]  ROmode			EventBuilding readout mode
	 * @return 0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_InitReadout(int handle, int ROmode, int* AllocatedSize);

	/*!
	 * @ingroup RO
	 * @brief				De-init readoout (free buffers)
	 *
	 * @param[in] handle	Board handle
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_CloseReadout(int handle);

	/*!
	 * @ingroup RO
	 * @brief				Flush the data buffer (Read and discard data until the RX buffer is empty)
	 *
	 * @param[in] handle	Board handle
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_FlushData(int handle);

	/*!
	 * @ingroup RO
	 * @brief						Get the number of CRC errors detected by the concentrator since the last start of run
	 *
	 * @param[out] errcnt			Pointer to the CRC error counter
	 * @param[in]  cnc_handle		Handle of the concentratore
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_GetCrcErrorCnt(int cnc_handle, uint32_t* errcnt);

	/*!
	 * @ingroup RO
	 * @brief					Start data acquisition. Set ReadoutState = RUNNING, wait until all threads are running, then send the run command 
	 *							to the boards, according to the given start mode
	 *
	 * @param[in] handle		Array of board handles
	 * @param[in] NumBrd		Number of boards to start
	 * @param[in] StartMode		Acquisition start mode (Async, T0/T1 chain, TDlink)
	 * @param[in] RunNum		Run number
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_StartAcquisition(int* handle, int NumBrd, int StartMode, int RunNum);

	/*!
	 * @ingroup RO
	 * @brief					Send the stop command to the boards (according to the same mode used to start them)
	 * @note					This function stops the run in the hardware (thus stopping the data flow) 
	 *							but it doesn't stop the RX threads because they need to empty the buffers and the pipes.
	 *							The threads will stop automatically when there is no data or when a flush command is sent.
	 * 
	 * @param[in] handle		Array of board handles
	 * @param[in] NumBrd		Number of boards to stop
	 * @param[in] StartMode		Acquisition stop mode
	 * @param[in] RunNum		Run number
	 * @return 0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_StopAcquisition(int* handle, int NumBrd, int StartMode, int RunNum);

	/*!
	 * @ingroup RO
	 * @brief						Read and decode one event from the readout buffers. There are two readout modes: sorted or unsorted. If sorting is requested, the readout init function will allocate queues for sorting
	 *
	 * @param[out] bindex			Pointer to the board index from which the event comes
	 * @param[out] DataQualifier	Pointer to the data qualifier (type of data, used to determine the struct for event data)
	 * @param[out] tstamp_us		Pointer to the event timestamp in microseconds (the information is also reported in the event data struct)
	 * @param[out] Event			Pointer to the read event data structure
	 * @param[out] nb				Pointer to the size of the read event (in bytes)
	 * @param[in]  handle			Array of boards handles
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_GetEvent(int* handle, int* bindex, int* DataQualifier, double* tstamp_us, void** Event, int* nb);

	/*!
	 * @ingroup RO
	 * @brief						Read and decode one event from a specific board
	 *
	 * @param[out] DataQualifier	Pointer to the data qualifier (type of data, used to determine the struct for event data)
	 * @param[out] tstamp_us		Pointer to the event timestamp in microseconds (the information is also reported in the event data struct)
	 * @param[out] Event			Pointer to the read event data
	 * @param[out] nb				Pointer to the size of the read event (in bytes)
	 * @param[in]  handle			Board handle
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_GetEventFromBoard(int handle, int* DataQualifier, double* tstamp_us, void** Event, int* nb);


	// -----------------------------------------------------------------
	// Firmware Upgrade
	// -----------------------------------------------------------------

	/*!
	* @ingroup Functions
	* @defgroup FWupg Firmware upgrade
	*/

	/*!
	 * @ingroup FWupg
	 * @brief				Upgrade the firmware of the board.
	 *
	 * @param[in] handle	Board handle
	 * @param[in] filen		Firmware file name
	 * @param[in] ptr		Callback function to report progress (message and progress percentage)
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_FirmwareUpgrade(int handle, char filen[200], void(*ptr)(char* msg, int progress));

	/*!
	 * @ingroup FWupg
	 * @brief				Reboot the firmware application via Ethernet/USB.
	 *
	 * @param[in] handle	Board handle
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_FirmwareBootApplication_ethusb(int handle);

	/*!
	 * @ingroup FWupg
	 * @brief					Reboot the firmware application via TDL
	 *
	 * @param[in] handle		Array of board handles
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_FirmwareBootApplication_tdl(int* handle);

	/*!
	 * @ingroup FWupg
	 * @brief						Check if the FPGA is in "bootloader mode" and read the bootloader version
	 *
	 * @param[out] isInBootloader	Pointer to the bootloader state (1 if in bootloader, 0 otherwise)
	 * @param[out] version			Pointer to the bootloader version
	 * @param[out] release			Pointer to the bootloader release number
	 * @param[in]  handle			Board handle
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_CheckBootloaderVersion(int handle, int* isInBootloader, uint32_t* version, uint32_t* release);

	// -----------------------------------------------------------------
	// EEPROM 5256
	// -----------------------------------------------------------------

	/*!
	* @ingroup Functions
	* @defgroup A5256F A5256 adapter
	*/

	/*!
	* @ingroup A5256F
	* @defgroup EEPROM EEPROM
	*/

	/*!
	 * @ingroup EEPROM
	 * @brief				Read Adapter A5256 info into the relevant EEPROM memory page
	 *
	 * @param[out] binfo	Pointer to the adapter information
	 * @param[in]  handle	Board handle
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_ReadA5256EEPROMInfo(int handle, FERS_A5256_Info_t* binfo);

	/*!
	 * @ingroup EEPROM
	 * @brief				Write Adapter A5256 info into the relevant EEPROM memory page
	 *
	 * @param[in] handle	Board handle
	 * @param[in] binfo		Adapter information to write
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_WriteA5256EEPROMInfo(int handle, FERS_A5256_Info_t binfo);

	/*!
	 * @ingroup EEPROM
	 * @brief						Reads a block of data from the EEPROM
	 *
	 * @param[out] data				Pointer to store the data read from EEPROM
	 * @param[in]  handle			Board handle
	 * @param[in]  start_address	Starting address in EEPROM to read from
	 * @param[in]  size				Number of bytes to read
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_ReadEEPROMBlock(int handle, int start_address, int size, uint8_t* data);

	/*!
	 * @ingroup EEPROM
	 * @brief						Writes a block of data to the EEPROM.
	 *
	 * @param[in] handle			Board handle
	 * @param[in] start_address		Starting address in EEPROM to write to
	 * @param[in] size				Number of bytes to write
	 * @param[in] data				Pointer to the data to write
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_WriteEEPROMBlock(int handle, int start_address, int size, uint8_t* data);

	/*!
	 * @ingroup EEPROM
	 * @brief						Check A5256 presence
	 *
	 * @param[in] handle			Board handle,
	 * @param[in] tinfo				A5256 info structure (it will be removedfrom v2.0.0)
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_checkA5256presence(int handle, FERS_A5256_Info_t* tinfo);

	/*!
	* @ingroup A5256F
	* @defgroup chmap Channel remapping
	*/



	/*!
	 * @ingroup chmap
	 * @brief					Remap Adapter channel to TDC channel
	 *
	 * @param[out] TDC_ch		Pointer to the remapped TDC channel
	 * @param[in]  Adapter_ch	Adapter channel
	 * @param[in]  brd			Board identifier
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_ChIndex_ada2tdc(int Adapter_ch, int* TDC_ch, int brd);

	/*!
	 * @ingroup chmap
	 * @brief					Remap TDC channel to Adapter channel
	 *
	 * @param[out] Adapter_ch	Pointer to the remapped adapter channel
	 * @param[in]  TDC_ch		The TDC channel
	 * @param[in]  brd			Board identifier
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_ChIndex_tdc2ada(int TDC_ch, int* Adapter_ch, int brd);


	/*!
	* @ingroup A5256F
	* @defgroup calib Calibration
	*/

	/*!
	 * @ingroup calib
	 * @brief						Find the source where calibration is stored (if present EEPROM is default)
	 * 
	 * @param[in]					Board handle
	 * @return						Source code (0 = flash,  1 = EEPROM)
	 */
	CAEN_FERS_DLLAPI int FERS_FindMemThrDest(int handle);

	/*!
	 * @ingroup calib
	 * @brief						Read threshold calibration offsets from flash memory
	 *
	 * @param[out] date				Pointer to the calibration date (DD/MM/YYYY)
	 * @param[out] ThrOffset		Pointer to the threshold offsets values
	 * @param[in]  MemThrDest		Flag indicating the source (0 = flash, 1 = EEPROM) (see @ref FERS_FindMemThrDest function)
	 * @param[in]  handle			Board handle
	 * @param[in]  npts				Number of points to read
	 * @return						0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	CAEN_FERS_DLLAPI int FERS_ReadThrCalib(int handle, int npts, int MemThrDest, char* date, float* ThrOffset);

	/*!
	 * @ingroup chmap
	 * @brief			Get the number of channels for the adapter
	 *
	 * @param[in] brd	Board identifier
	 * @return			The number of channels on success, (0 = no adapter)
	 */
	CAEN_FERS_DLLAPI int FERS_AdapterNch(int brd);

	/*!
	 * @ingroup Functions
	 * @brief				Disable threshold calibration
	 *
	 * @param[in] handle	Board handle
	 * @return				0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_DisableThrCalib(int handle);

	/*!
	 * @ingroup calib
	 * @brief					Set the discriminator threshold
	 *
	 * @param[in] handle		Board handle
	 * @param[in] Adapter_ch	Adapter channel identifier
	 * @param[in] thr_mv		Threshold in millivolts
	 * @param[in] brd			Board identifier
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_Set_DiscrThreshold(int handle, int Adapter_ch, float thr_mv, int brd);

	/*!
	 * @ingroup chmap
	 * @brief					Remap bits of adapter channel mask to the picoTDC channel mask
	 *
	 * @param[out] ChMask0      Pointer to the resulting TDC channel mask (0..31)
	 * @param[out] ChMask1      Pointer to the resulting TDC channel mask (32..63)
	 * @param[in]  AdapterMask  Mask of adapter channels
	 * @param[in]  brd          Board identifier
	 * @return					0 on success, or a negative error code as defined in #FERSLIB_ErrorCodes
	 */
	int FERS_ChMask_ada2tdc(uint32_t AdapterMask, uint32_t* ChMask0, uint32_t* ChMask1, int brd);

	/*!
	 * @brief Calibate the threshold offset of A5256 adapter
	 * @param[out] ThrOffset	Threshold offsets
	 * @param[out] done			Calibration completed, per channel
	 * @param[out] RMSnoise		Standard deviation of threshold offset
	 * @param[out] ptr			Callback function to report progress
	 * @param[in] handle		handle of the board to calibrate
	 * @param[in] min_thr		minimum threshold fo the scan
	 * @param[in] max_thr		maximum threshold for the scan (not true)
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup calib
	*/
	CAEN_FERS_DLLAPI int FERS_CalibThresholdOffset(int handle, float min_thr, float max_thr, int* done, float* ThrOffset, float* RMSnoise, void(*ptr)(char* msg, int progress));

	/*!
	 * @brief   Write Threshold offset calibration of A5256 adapter to flash
	 * @warning The flash memory contains vital parameters for the board. Overwriting certain pages can damage the hardware!!! Do not use this function without contacting CAEN first
	 *
	 * @param[in] handle		handle of the board to calibrate
	 * @param[in] npts			number of values to write
	 * @param[in] MemThrCalib	Flag indicating the source (0 = flash, 1 = EEPROM) (see @ref FERS_FindMemThrDest function)
	 * @param[in] ThrOffset		Threshold offsets (use NULL pointer to keep old values)
	 * @return					0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	 * @ingroup calib
	*/
	CAEN_FERS_DLLAPI int FERS_WriteThrCalib(int handle, int npts, int MemThrDest, float* ThrOffset);

	///*!
	// * @ingroup calib
	// * @brief				Read BIC from the EEPROM of the Adapter (if present)
	// * @param[in]  handle	Board handle 
	// * @param[out] binfo	Board info struct
	// * @return				0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	// */ 
	//CAEN_FERS_DLLAPI int FERS_ReadAdapterBIC(int handle, FERS_A5256_Info_t* binfo);

	///*!
	// * @ingroup calib
	// * @brief				Write BIC into the EEPROM of the Adapter (if present)
	// * @param[in]  handle	Board handle
	// * @param[out] binfo	Board info struct
	// * @return				0 in case of success, or a negative error code specified in #FERSLIB_ErrorCodes
	// */
	//CAEN_FERS_DLLAPI int FERS_WriteAdapterBIC(int handle, FERS_A5256_Info_t binfo);


#ifdef __cplusplus
 }
#endif

#endif

