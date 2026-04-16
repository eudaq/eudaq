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


#include <ctype.h>
#include <string.h>
#include "FERS_MultiPlatform.h"
#include "FERS_paramparser.h"
#include "FERSlib.h"
#include "FERS_config.h"


#define SETBIT(r, m, b)  (((r) & ~(m)) | ((m) * (b)))


int ValidParameterName = 0;
int ValidParameterValue = 0;
int ValidUnits = 0;

FILE* fcfg[FERSLIB_MAX_NBRD];
char* scfg[FERSLIB_MAX_NBRD][2048];
const uint8_t plog = 0;
static int FERS_parseConfigFile(FILE* f_ini);


// #################################################################################
// String operations
// #################################################################################
// ---------------------------------------------------------------------------------
// Description: compare two strings
// Return:		1=equal, 0=not equal
// ---------------------------------------------------------------------------------
int streq(char *str1, char *str2)
{
	if (strcmp(str1, str2) == 0) {
		ValidParameterName = 1;
		return 1;
	} else {
		return 0;
	}
}

// ---------------------------------------------------------------------------------
// Description: Trim a string left and right
// Inputs:		string to be trimmed
// Outputs:		string trimmed
// Return:		string trimmed
// ---------------------------------------------------------------------------------
char* ltrim(char* s) {
	while (isspace(*s)) s++;
	return s;
}

char* rtrim(char* s) {
	char* back = s + strlen(s) - 1;
	while (isspace(*back)) --back;
	*(back + 1) = '\0';
	return s;
}

char* trim(char* s) {
	return rtrim(ltrim(s));
}


// #################################################################################
// Get values of different type and format from a string
// #################################################################################
// ---------------------------------------------------------------------------------
// Get a 32 bit decimal 
// ---------------------------------------------------------------------------------
static uint32_t GetInt(char *val)
{
	uint32_t ret=0;
	int num = sscanf(val, "%d", &ret);
	if (ret < 0 || num < 1) ValidParameterValue = 0;
	else ValidParameterValue = 1;
	return ret;
}

// ---------------------------------------------------------------------------------
// Get a 32 bit hex
// ---------------------------------------------------------------------------------
static uint32_t GetHex32(char* str) {
	uint32_t ret;
	ValidParameterValue = 1;
	if ((str[1] == 'x') || (str[1] == 'X')) {
		sscanf(str + 2, "%x", &ret);
		if (str[0] != '0') ValidParameterValue = 0;	// Rise a warning for wrong HEX format 0x
		for (uint8_t i = 2; i < strlen(str); ++i) {
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	} else {
		sscanf(str, "%x", &ret);
		for (uint8_t i = 0; i < strlen(str); ++i) {	// Rise a warning for wrong HEX format
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	}
	return ret;
}


// ---------------------------------------------------------------------------------
// Get a string from the given token postion (multiple strings separated by spaces)
// ---------------------------------------------------------------------------------
static char* Get_pos(char* val, int tok) {
	int i;
	const char* del = " ";
	char* token = strtok(val, "MASK");
	token = strtok(val, del); // param name
	for (i = 0; i < tok; i++) {
		token = strtok(NULL, del);
		if (token == NULL) break;
	}

	return token;
}


// ---------------------------------------------------------------------------------
// Get a 32 bit hexdecimal from the given token postion (multiple strings separated by spaces)
// ---------------------------------------------------------------------------------
static uint32_t GetHex_pos(char* val, int tok) {
	char* tkn = Get_pos(val, tok);
	uint32_t ret = GetHex32(tkn);

	return ret;
}

// ---------------------------------------------------------------------------------
// Get a 32 bit decimal from the given token postion (multiple strings separated by spaces)
// ---------------------------------------------------------------------------------
static uint32_t GetInt_pos(char* val, int tok) {
	char* tkn = Get_pos(val, tok);
	uint32_t ret = GetInt(tkn);

	return ret;
}


// ---------------------------------------------------------------------------------
// Get a 64 bit hex
// ---------------------------------------------------------------------------------
static uint64_t GetHex64(char* str) {
	uint64_t ret;
	ValidParameterValue = 1;
	if ((str[1] == 'x') || (str[1] == 'X')) {
		sscanf(str + 2, "%" SCNx64, &ret);
		if (str[0] != '0') ValidParameterValue = 0;	// Rise a warning for wrong HEX format 0x
		for (uint8_t i = 2; i < strlen(str); ++i) {
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	} else {
		sscanf(str, "%" SCNx64, &ret);
		for (uint8_t i = 0; i < strlen(str); ++i) {	// Rise a warning for wrong HEX format
			if (!isxdigit(str[i])) {
				ValidParameterValue = 0;
				break;
			}
		}
	}
	return ret;
}

// ---------------------------------------------------------------------------------
// Get a float
// ---------------------------------------------------------------------------------
static float GetFloat(char* str) {
	float ret;
	int i;
	ValidParameterValue = 1;
	// replaces ',' with '.' (decimal separator)
	for (i = 0; i < (int)strlen(str); i++)
		if (str[i] == ',') str[i] = '.';
	sscanf(str, "%f", &ret);
	return ret;
}

// ---------------------------------------------------------------------------------
// Get the number of bin (power of 2)
// ---------------------------------------------------------------------------------
static int GetNbin(char* str)
{
	int i;
	for (i = 0; i < (int)strlen(str); ++i)
		str[i] = (char)toupper(str[i]);
	if ((streq(str, "DISABLED")) || (streq(str, "0")))	return 0;
	else if (streq(str, "256"))							return 256;
	else if (streq(str, "512"))							return 512;
	else if ((streq(str, "1024") || streq(str, "1K")))	return 1024;
	else if ((streq(str, "2048") || streq(str, "2K")))	return 2048;
	else if ((streq(str, "4096") || streq(str, "4K")))	return 4096;
	else if ((streq(str, "8192") || streq(str, "8K")))	return 8192;
	else if ((streq(str, "16384") || streq(str, "16K")))return 16384;
	else {
		ValidParameterValue = 0;
		return 1024;  // assign a default value on error
	}
}


// ---------------------------------------------------------------------------------
// Get a time (can be followed by the unit)
// ---------------------------------------------------------------------------------
static float GetTime(char* val, char* tu)
{
	double timev = -1;
	double ns;
	char str[100];

	int element = sscanf(val, "%lf %s", &timev, str);

	ValidUnits = 1;
	if (streq(str, "ps"))		ns = timev * 1e-3;
	else if (streq(str, "ns"))	ns = timev;
	else if (streq(str, "us"))	ns = timev * 1e3;
	else if (streq(str, "ms"))	ns = timev * 1e6;
	else if (streq(str, "s"))	ns = timev * 1e9;
	else if (streq(str, "m"))	ns = timev * 60e9;
	else if (streq(str, "h"))	ns = timev * 3600e9;
	else if (streq(str, "d"))	ns = timev * 24 * (3600e9);
	else if (element == 1 || streq(str, "#")) return (float)timev;  // No units
	else {	// No valid units
		ValidUnits = 0;
		return (float)timev;  // no time unit specified in the config file; assuming equal to the requested one (ns of default)
	}

	if (streq(tu, "ps"))		return (float)(ns * 1e3);
	else if (streq(tu, "ns"))	return (float)(ns);
	else if (streq(tu, "us"))	return (float)(ns / 1e3);
	else if (streq(tu, "ms"))	return (float)(ns / 1e6);
	else if (streq(tu, "s"))	return (float)(ns / 1e9);
	else if (streq(tu, "m"))	return (float)(ns / 60e9);
	else if (streq(tu, "h"))	return (float)(ns / 3600e9);
	else if (streq(tu, "d"))	return (float)(ns / (24 * 3600e9));
	return (float)timev;
}

// ---------------------------------------------------------------------------------
// Get a voltage (can be followed by the unit)
// ---------------------------------------------------------------------------------
static float GetVoltage(char* val)
{
	float var;
	char str[100];

	int element = sscanf(val, "%f %s", &var, str);
	// try to read the unit from the config file (string)
	int val1 = element - 1;  // fscanf(f_ini, "%s", str);  // read string "str"
	if (val1 >= 0) ValidParameterValue = 1;
	else ValidParameterValue = 0;

	ValidUnits = 1;
	if (streq(str, "uV"))		return var / 1000000;
	else if (streq(str, "mV"))	return var / 1000;
	else if (streq(str, "V"))	return var;
	else if (val1 != 1 || streq(str, "#")) {	// no units, assumed Voltage
		return var;
	} else {	// wrong units, raise warning
		ValidUnits = 0;
		return var;  // no voltage unit specified in the config file; assuming volts
	}
}

// ---------------------------------------------------------------------------------
// Get a current (can be followed by the unit)
// ---------------------------------------------------------------------------------
static float GetCurrent(char* val)
{
	char van[50];
	float var = 0;
	//char val[500] = "";
	char stra[100] = "";
	int element0 = 0;
	int element1 = 0;

	element0 = sscanf(val, "%s %s", van, stra);
	element1 = sscanf(van, "%f", &var);
	if (element1 > 0) ValidParameterValue = 1;
	else ValidParameterValue = 0;

	// try to read the unit from the config file (string)
	//fp = ftell(f_ini);  // save current pointer before "str"
	//fscanf(f_ini, "%s", stra);  // read string "str"
	ValidUnits = 1;
	if (streq(stra, "uA"))		return var / 1000;
	else if (streq(stra, "mA"))	return var;
	else if (streq(stra, "A"))	return var * 1000;
	else if (element1 * element0 == 1 || streq(stra, "#")) {	// No units, no warning raised
		return var;
	} else {	// Wrong units entered
		ValidUnits = 0;
		return var;  // wrong unit specified in the config file; assuming mA
	}
}

// ---------------------------------------------------------------------------------
// Get size in bytes (can be followed by the unit)
// ---------------------------------------------------------------------------------
static float GetBytes(char* val)
{
	char van[50];
	float var;
	//long fp;
	char str[100];
	float minSize = 1e3; // 1 kB

	int element0 = sscanf(val, "%s %s", van, str);
	int element1 = sscanf(van, "%f", &var);
	if (element1 > 0) ValidParameterValue = 1;
	else ValidParameterValue = 0;

	ValidUnits = 1;
	if (streq(str, "TB"))		return (var * 1e12 > minSize) ? var * (float)1e12 : minSize;
	else if (streq(str, "GB"))	return (var * 1e9  > minSize) ? var * (float)1e9 : minSize;
	else if (streq(str, "MB"))	return (var * 1e6  > minSize) ? var * (float)1e6 : minSize;
	else if (streq(str, "kB"))	return (var * 1e3  > minSize) ? var * (float)1e3 : minSize;
	else if (streq(str, "B"))	return (var > minSize) ? var : minSize;
	else if (element1 * element0 == 1 || streq(str, "#")) {	// no units, assumed Byte
		return (var > 1e6) ? var : minSize;
	} else {	// wrong units, raise warning
		ValidUnits = 0;
		return (var > minSize) ? var : minSize;  // no units specified in the config file; assuming bytes
	}
}


// ---------------------------------------------------------------------------------
// Description: check if the directory exists, if not it is created
// Inputs:		J_cfg, Cfg file
// Outputs:		-
// Return:		void
// ---------------------------------------------------------------------------------
int f_mkdir(const char* path) {	// taken from CAENMultiplatform.c (https://gitlab.caen.it/software/utility/-/blob/develop/src/CAENMultiplatform.c#L216)
	int32_t ret = 0;
#ifdef _WIN32
	DWORD r = (CreateDirectoryA(path, NULL) != 0) ? 0 : GetLastError();
	switch (r) {
	case 0:
	case ERROR_ALREADY_EXISTS:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
#else
	int r = mkdir(path, ACCESSPERMS) == 0 ? 0 : errno;
	switch (r) {
	case 0:
	case EEXIST:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
#endif
	return ret;
}


// ---------------------------------------------------------------------------------
// Get RawData file saving path
// ---------------------------------------------------------------------------------
void GetDatapath(char* value, Config_t* F_cfg) {

	if (access(value, 0) == 0) { // taken from https://stackoverflow.com/questions/6218325/
		struct stat status;
		stat(value, &status);
		int myb = status.st_mode & S_IFDIR;
		if (myb == 0) {
			_setLastLocalError("WARNING: DataFilePath: %s is not a valid directory. Default .DataFiles folder is used\n", value);
			strcpy(F_cfg->OF_RawDataPath, "DataFiles");
			ValidParameterValue = 0;
		} else
			strcpy(F_cfg->OF_RawDataPath, value);
	} else {
		int ret = f_mkdir(value);
		if (ret == 0)
			strcpy(F_cfg->OF_RawDataPath, value);
		else {
			_setLastLocalError("WARNING: DataFilePath: %s cannot be created, default .DataFiles folder is used\n", value);
			strcpy(F_cfg->OF_RawDataPath, "DataFiles");
			ValidParameterValue = 0;
		}
	}
}


// #################################################################################
// Set a channel parameter (-1 = all channels)
// #################################################################################
static void SetChannelParam32(int handle, uint32_t *param, uint32_t val, int ch)
{
	int i;
	if (ch == -1) {
		for(i=0; i < FERS_NumChannels(handle); i++)
			param[i] = val;
	} else {
        param[ch] = val;
	}
}

static void SetChannelParam16(int handle, uint16_t *param, uint16_t val, int ch) {
	int i;
	if (ch == -1) {
		for (i = 0; i < FERS_NumChannels(handle); i++)
			param[i] = val;
	} else {
		param[ch] = val;
	}
}

static void SetChannelParamFloat(int handle, float *param, float val, int ch) {
	int i;
	if (ch == -1) {
		for (i = 0; i < FERS_NumChannels(handle); i++)
			param[i] = val;
	} else {
		param[ch] = val;
	}
}



// #################################################################################
// Set/Get Parameter
// #################################################################################

// ---------------------------------------------------------------------------------
// Description: Inizialize Configuration for each opened board
// Inputs:		brd: board index
// Outputs:		-
// Return:		0=OK, -1=error
// ---------------------------------------------------------------------------------
static int _SetDefaultConfig(int brd) {

	// reset all parameters, then set the default value of those ones that are not zero
	memset(FERScfg[brd], 0, sizeof(Config_t));

	//strcpy(FERScfg[brd]->ConnPath, path);

	FERScfg[brd]->OF_RawData = 0;
	FERScfg[brd]->OF_LimitedSize = 0;
	FERScfg[brd]->MaxSizeDataOutputFile = 1e9;
	FERScfg[brd]->OF_RawDataPath[0] = '\0';

	FERScfg[brd]->StartRunMode = STARTRUN_ASYNC;			// Start Mode
	FERScfg[brd]->StopRunMode = STOPRUN_MANUAL;				// Stop Mode

	FERScfg[brd]->CitirocCfgMode = CITIROC_CFG_FROM_REGS;	// 0=from regs, 1=from file
	FERScfg[brd]->Pedestal = 100;							// Common pedestal added to all channels

	if (FERS_BoardInfo[brd]->FERSCode == 5203) {
		FERScfg[brd]->AcquisitionMode = ACQMODE_COMMON_START;
		FERScfg[brd]->MeasMode = MEASMODE_LEAD_ONLY;
	} else {
		FERScfg[brd]->AcquisitionMode = ACQMODE_SPECT;		
	}
	
	FERScfg[brd]->TDC_ChBufferSize = 4;						// Channel buffer size in the picoTDC (set a limit to the max number of hits acquired by the channel)
	FERScfg[brd]->EnableCntZeroSuppr = 1;					// Enable zero suppression in Counting Mode (1 by default)
	FERScfg[brd]->En_Head_Trail = 1;						// Enable Header and Trailer: 0=Keep all (group header+trail), 1=One word, 2=Header and trailer suppressed
	FERScfg[brd]->En_Empty_Ev_Suppr = 1;					// Enable event suppression (only in Custom header mode)
	FERScfg[brd]->EnableServiceEvents = 3;					// Enable service events. 0=disabled, 1=enabled HV, 2=enabled counters, 3=enabled all
	// If 5203 a 128 canali = 1, senn\F2 0
	//FERScfg[brd]->En_128_ch = 0;							// Enable 128 channels
	FERScfg[brd]->TrgWindowWidth = 1000;					// Trigger window width in ns (will be rounded to steps of 25 ns)
	FERScfg[brd]->TrgWindowOffset = -500;					// Trigger window offset in ns; can be negative (will be rounded to steps of 25 ns)
	FERScfg[brd]->GateWidth = -500;
	FERScfg[brd]->TriggerBufferSize = 16;
	FERScfg[brd]->Counting_Mode = 0;						// Counting Mode (Singles, Paired_AND)
	FERScfg[brd]->TrefWindow = 2;							// Tref Windows in ns (Common start/stop)
	FERScfg[brd]->TrefDelay = -1;							// Tref delay in ns (can be negative)
	FERScfg[brd]->ChTrg_Width = 5;							// Self Trg Width in ns => Coinc window for the paired counting
	FERScfg[brd]->MajorityLevel = 2;						// Majority Level
	FERScfg[brd]->PtrgPeriod = 1e6;							// period in ns of the internal periodic trigger (dwell time)
	FERScfg[brd]->TestPulseSource = -1;						// EXT, INT_T0, INT_T1, INT_PTRG, INT_SW
	FERScfg[brd]->TestPulseDestination = 0;					// -1=ALL, -2=EVEN, -3=ODD or channel number (0 to 63) for single channel pulsing
	FERScfg[brd]->TestPulsePreamp = TEST_PULSE_PREAMP_BOTH;	// 1=LG, 2=HG, 3=BOTH
	FERScfg[brd]->WaveformLength = 800;						// Num of samples in the waveform
	FERScfg[brd]->HeaderField0 = 4;
	FERScfg[brd]->HeaderField1 = 5;
	FERScfg[brd]->LeadTrail_LSB = 1;
	FERScfg[brd]->ToT_LSB = 1;
	FERScfg[brd]->TrgWindowWidth = 1000;
	FERScfg[brd]->Enable_2nd_tstamp = 0;
	FERScfg[brd]->Tlogic_Width = 0;

	FERScfg[brd]->ChEnableMask   = 0xFFFFFFFFFFFFFFFF;		// Channel enable mask 
	FERScfg[brd]->ChEnableMask_e = 0xFFFFFFFFFFFFFFFF;		// Channel enable mask ch 64..123 (TDC1)

	FERScfg[brd]->QD_Mask = 0xFFFFFFFFFFFFFFFF;				// Charge Discriminator mask
	FERScfg[brd]->Tlogic_Mask = 0xFFFFFFFFFFFFFFFF;			// Trigger Logic mask 

	FERScfg[brd]->QD_CoarseThreshold = 250;					// Coarse Threshold for Citiroc charge discriminator
	FERScfg[brd]->TD_CoarseThreshold = 0;					// Coarse Threshold for Citiroc time discriminator
	FERScfg[brd]->HG_ShapingTime = 25;						// Shaping Time of the High Gain preamp
	FERScfg[brd]->LG_ShapingTime = 25;						// Shaping Time of the Low Gain preamp
	FERScfg[brd]->Enable_HV_Adjust = 0;						// Enable input DAC for HV fine adjust
	FERScfg[brd]->HV_Adjust_Range = 1;						// HV adj DAC range (reference): 0 = 2.5V, 1 = 4.5V
	FERScfg[brd]->MuxClkPeriod = 300;						// Period of the Mux Clock
	FERScfg[brd]->MuxNSmean = 0;							// Num of samples for the Mux mean: 0: 4 samples, 1: 16 samples
	FERScfg[brd]->HoldDelay = 100;							// Time between Trigger and Hold
	FERScfg[brd]->GainSelect = GAIN_SEL_AUTO;				// Select gain between High/Low/Auto
	FERScfg[brd]->EnableQdiscrLatch = 1;					// Q-dicr mode: 1 = Latched, 0 = Direct
	FERScfg[brd]->EnableChannelTrgout = 1;					// 0 = Channel Trgout Disabled, 1 = Enabled
	FERScfg[brd]->FastShaperInput = 0;						// Fast Shaper (Tdiscr) connection: 0 = High Gain PA, 1 = Low Gain PA
	FERScfg[brd]->CncBufferSize = 0;
	FERScfg[brd]->CncProbe_A = (uint32_t)-1;
	FERScfg[brd]->CncProbe_B = (uint32_t)-1;

	FERScfg[brd]->HV_Vbias = 55;							// Voltage setting for HV
	FERScfg[brd]->HV_Imax = 1.0;							// Imax for HV
	FERScfg[brd]->TempSensCoeff[0] = 0;						// Temperature Sensor Coefficients (2=quad, 1=lin, 0=offset)
	FERScfg[brd]->TempSensCoeff[1] = 50;
	FERScfg[brd]->TempSensCoeff[2] = 0;
	FERScfg[brd]->TempFeedbackCoeff = 35;					// Temperature Feedback Coeff: Vout = Vset - k * (T-25)
	FERScfg[brd]->EnableTempFeedback = 0;					// Enable Temp Feedback

	// Channel Settings 
	for (int i = 0; i < 64; ++i) {
		FERScfg[brd]->HG_Gain[i] = 50;						// Gain fo the High Gain Preamp
		FERScfg[brd]->LG_Gain[i] = 50;						// Gain fo the Low Gain Preamp
		FERScfg[brd]->HV_IndivAdj[i] = 255;					// HV individual bias adjust (Citiroc 8bit input DAC)
	}

	// Settings for the input adapters
	FERScfg[brd]->AdapterType = ADAPTER_NONE;				// Adapter Type
	for (int i = 0; i < FERSLIB_MAX_NCH_5203; ++i) {
		FERScfg[brd]->DiscrThreshold[i] = 100;				// Discriminator Threshold
		FERScfg[brd]->DiscrThreshold2[i] = 100;				// Discriminator 2nd Threshold (double thershold mode only)
	}
	FERScfg[brd]->A5256_Ch0Polarity = A5256_CH0_DUAL;		// Polarity of Ch0 in A5256 (POS, NEG)

	// Generic write accesses 
	FERScfg[brd]->GWn = 0;
	return 0;
}



// ---------------------------------------------------------------------------------
// Description: Set a parameter by name. The function assigns the value to the relevant parameter in the 
//				FERScfg struct, but it doesn't actually write it into the board (this is done by the "FERS_configure" function)
// Inputs:		handle: board handle
//				param_name: parameter name
//				param_name: parameter value
// Outputs:		-
// Return:		0=OK, -1=error
// ---------------------------------------------------------------------------------
int FERS_SetParam(int handle, const char *param_name, const char *value_original) {
	int brd = -1, ch = -1;
	static int SetDefault[FERSLIB_MAX_NBRD] = { 0 };
	char* str_to_split = j_strdup(param_name);
	const char* delim = "[]";
	char* before_str = strtok(str_to_split, delim); // Param Name
	char* token = strtok(NULL, delim);	// ch Idx

	brd = FERS_INDEX(handle);

	if (brd < 0 || brd > FERSLIB_MAX_NBRD) {
		_setLastLocalError("Set Param: brd index %d out of range", brd);
		free(str_to_split);
		return FERSLIB_ERR_INVALID_HANDLE;
	}
	if (!BoardConnected[brd]) {
		_setLastLocalError("Set Param: brd %d not connected", brd);
		free(str_to_split);
		return FERSLIB_ERR_OPER_NOT_ALLOWED;
	}

	if (!SetDefault[brd]) {
		_SetDefaultConfig(brd);
		SetDefault[brd] = 1;
	}

	if (token != NULL) {
		sscanf(token, "%d", &ch);
	}

	ValidParameterName = 0;  // init to 0. It will be set by the streq funtion (valid param name)
	ValidParameterValue = 1;  // init to 1. It will be reset if no valid param value is found
	ValidUnits = 1; // init to 1. It will be reset if no valid unit is found

	// Some name replacement for back compatibility
	char* str = NULL;
	if (streq(before_str, "TriggerSource"))
		str = j_strdup("BunchtrgSource");
	else if (streq(before_str, "DwellTime"))
		str = j_strdup("PtrgPeriod");
	else if (streq(before_str, "Hit_HoldOff"))
		str = j_strdup("TrgHoldOff");
	else if (streq(before_str, "Trg_HoldOff"))
		str = j_strdup("TrgHoldOff");
	else if (streq(before_str, "Q_DiscrMask0"))
		str = j_strdup("QD_Mask0");
	else if (streq(before_str, "Q_DiscrMask1"))
		str = j_strdup("QD_Mask1");
	else if (streq(before_str, "Q_DiscrMask"))
		str = j_strdup("QD_Mask");
	else
		str = j_strdup(before_str);

	lock(FERScfg[brd]->bmutex);

	char* value = j_strdup(value_original);
	// Write down configuration on file
	if (plog) fprintf(fcfg[brd], "%s = %s\n", str, value);

	// -------------------------------------------------------------
	// Raw Data Saving
	// -------------------------------------------------------------
	if (streq(str, "OF_RawData") && !FERS_Offline)		FERScfg[brd]->OF_RawData			= GetInt(value);
	if (streq(str, "OF_LimitedSize"))		FERScfg[brd]->OF_LimitedSize		= GetInt(value);
	if (streq(str, "MaxSizeOutputFile"))	FERScfg[brd]->MaxSizeDataOutputFile = GetFloat(value);
	if (streq(str, "OF_RawDataPath"))		GetDatapath(value, FERScfg[brd]);

	// -------------------------------------------------------------
	// Generic Register Read/Write
	// -------------------------------------------------------------
	if (streq(str, "WriteRegister")) {
		if (FERScfg[brd]->GWn < FERSLIB_MAX_GW) {
			uint32_t ad, dt, mk;
			int ns = sscanf(value, "%x %x %x", &ad, &dt, &mk);
			if (ns == 3) {
				FERScfg[brd]->GWaddr[FERScfg[brd]->GWn] = ad;
				FERScfg[brd]->GWdata[FERScfg[brd]->GWn] = dt;
				FERScfg[brd]->GWmask[FERScfg[brd]->GWn] = mk;
				FERScfg[brd]->GWn++;
			} else ValidParameterValue = 0;
		}
	}

	if (streq(str, "WriteRegisterBits")) {
		if (FERScfg[brd]->GWn < FERSLIB_MAX_GW) {
			uint32_t ad, start, stop, dt;
			int ns = sscanf(value, "%x %d %d %x", &ad, &start, &stop, &dt);
			if (ns == 4) {
				FERScfg[brd]->GWaddr[FERScfg[brd]->GWn] = ad;
				FERScfg[brd]->GWmask[FERScfg[brd]->GWn] = ((1 << (stop - start + 1)) - 1) << start;
				FERScfg[brd]->GWdata[FERScfg[brd]->GWn] = (dt << start) & FERScfg[brd]->GWmask[FERScfg[brd]->GWn];
				FERScfg[brd]->GWn++;
			} else ValidParameterValue = 0;
		}
	}

	// -------------------------------------------------------------
	// Settings for the acquisition modes and I/O masks
	// -------------------------------------------------------------
	if (streq(str, "AcquisitionMode")) {
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "COUNTING"))					FERScfg[brd]->AcquisitionMode = ACQMODE_COUNT;
			else if (streq(value, "SPECTROSCOPY"))			FERScfg[brd]->AcquisitionMode = ACQMODE_SPECT;
			else if (streq(value, "SPECT_TIMING"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TSPECT;
			else if (streq(value, "TIMING"))				FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_CSTART;
			else if (streq(value, "TIMING_CSTART"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_CSTART;
			else if (streq(value, "TIMING_CSTOP"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_CSTOP;
			else if (streq(value, "TIMING_STREAMING"))		FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_STREAMING;
			else if (streq(value, "WAVEFORM"))				FERScfg[brd]->AcquisitionMode = ACQMODE_WAVE;
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "TRG_MATCHING"))				FERScfg[brd]->AcquisitionMode = ACQMODE_TRG_MATCHING;
			else if (streq(value, "COMMON_START"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_CSTART;
			else if (streq(value, "COMMON_STOP"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_CSTOP;
			else if (streq(value, "STREAMING"))				FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_STREAMING;
			else if (strstr(value, "TEST_MODE") != NULL) {
				int tn = 1;
				if (strlen(value) > 10) sscanf(value + 10, "%d", &tn);
				FERScfg[brd]->AcquisitionMode = ACQMODE_COMMON_START;
				FERScfg[brd]->TestMode = tn;
			} else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			if (streq(value, "COUNTING"))					FERScfg[brd]->AcquisitionMode = ACQMODE_COUNT;
			else if (streq(value, "SPECTROSCOPY"))			FERScfg[brd]->AcquisitionMode = ACQMODE_SPECT;
			else if (streq(value, "SPECT_TIMING"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TSPECT;
			else if (streq(value, "TIMING_GATED"))			FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_GATED;
			else if (streq(value, "TIMING_STREAMING"))		FERScfg[brd]->AcquisitionMode = ACQMODE_TIMING_STREAMING;
			else if (streq(value, "WAVEFORM"))				FERScfg[brd]->AcquisitionMode = ACQMODE_WAVE;
			else ValidParameterValue = 0;
		} else ValidParameterValue = 0;
	}

	if (streq(str, "StartRunMode")) {
		if (streq(value, "MANUAL"))				FERScfg[brd]->StartRunMode = STARTRUN_ASYNC;  // keep "MANUAL" option for backward compatibility
		else if (streq(value, "ASYNC"))			FERScfg[brd]->StartRunMode = STARTRUN_ASYNC;
		else if (streq(value, "CHAIN_T0"))		FERScfg[brd]->StartRunMode = STARTRUN_CHAIN_T0;
		else if (streq(value, "CHAIN_T1"))		FERScfg[brd]->StartRunMode = STARTRUN_CHAIN_T1;
		else if (streq(value, "TDL"))			FERScfg[brd]->StartRunMode = STARTRUN_TDL;
		else ValidParameterValue = 0;
	}

	if (streq(str, "StopRunMode")) {  
		if (streq(value, "MANUAL"))				FERScfg[brd]->StopRunMode = STOPRUN_MANUAL;  
		else if (streq(value, "PRESET_TIME"))	FERScfg[brd]->StopRunMode = STOPRUN_PRESET_TIME;
		else if (streq(value, "PRESET_COUNTS"))	FERScfg[brd]->StopRunMode = STOPRUN_PRESET_COUNTS;
		else ValidParameterValue = 0;
	}

	if (streq(str, "BunchTrgSource") || streq(str, "TriggerSource")) {
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "SW_ONLY"))			FERScfg[brd]->TriggerMask = 0x1;
			else if (streq(value, "T1-IN"))			FERScfg[brd]->TriggerMask = 0x3;
			else if (streq(value, "Q-OR"))			FERScfg[brd]->TriggerMask = 0x5;
			else if (streq(value, "T-OR"))			FERScfg[brd]->TriggerMask = 0x9;
			else if (streq(value, "T0-IN"))			FERScfg[brd]->TriggerMask = 0x11;
			else if (streq(value, "PTRG"))			FERScfg[brd]->TriggerMask = 0x21;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->TriggerMask = 0x41;
			else if (strstr(value, "MASK"))			FERScfg[brd]->TriggerMask = GetHex32(trim(strtok(value, "MASK")));
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "SW_ONLY"))			FERScfg[brd]->TriggerMask = 0x1;
			else if (streq(value, "T1-IN"))			FERScfg[brd]->TriggerMask = 0x3;
			else if (streq(value, "T0-IN"))			FERScfg[brd]->TriggerMask = 0x11;
			else if (streq(value, "PTRG"))			FERScfg[brd]->TriggerMask = 0x21;
			else if (streq(value, "EDGE_CONN"))		FERScfg[brd]->TriggerMask = 0x41;
			else if (streq(value, "EDGE_CONN_MB"))	FERScfg[brd]->TriggerMask = 0x41;
			else if (streq(value, "EDGE_CONN_PB"))	FERScfg[brd]->TriggerMask = 0x81;
			else if (strstr(value, "MASK"))			FERScfg[brd]->TriggerMask = GetHex32(trim(strtok(value, "MASK")));
		} else if (FERS_Code(handle) == 5204) {
			if (streq(value, "SW_ONLY"))			FERScfg[brd]->TriggerMask = 0x1;
			else if (streq(value, "T1-IN"))			FERScfg[brd]->TriggerMask = 0x3;
			else if (streq(value, "OR-T1"))			FERScfg[brd]->TriggerMask = 0x5;
			else if (streq(value, "OR-T2"))			FERScfg[brd]->TriggerMask = 0x9;
			else if (streq(value, "T0-IN"))			FERScfg[brd]->TriggerMask = 0x11;
			else if (streq(value, "PTRG"))			FERScfg[brd]->TriggerMask = 0x21;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->TriggerMask = 0x41;
			else if (streq(value, "OR-TQ"))			FERScfg[brd]->TriggerMask = 0x81;
			else if (strstr(value, "MASK"))			FERScfg[brd]->TriggerMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else ValidParameterValue = 0;
	}

	if (streq(str, "T0_Out")) {
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "T0-IN"))				FERScfg[brd]->T0_outMask = 0x001;
			else if (streq(value, "BUNCHTRG"))		FERScfg[brd]->T0_outMask = 0x002;
			else if (streq(value, "T-OR"))			FERScfg[brd]->T0_outMask = 0x004;
			else if (streq(value, "RUN"))			FERScfg[brd]->T0_outMask = 0x008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T0_outMask = 0x010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T0_outMask = 0x020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T0_outMask = 0x040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T0_outMask = 0x080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T0_outMask = 0x100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T0_outMask = 0x200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T0_outMask = 0x400;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T0_outMask = 0x000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T0_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "T0-IN"))				FERScfg[brd]->T0_outMask = 0x001;
			else if (streq(value, "TRIGGER"))		FERScfg[brd]->T0_outMask = 0x002;
			else if (streq(value, "RUN"))			FERScfg[brd]->T0_outMask = 0x008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T0_outMask = 0x010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T0_outMask = 0x020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T0_outMask = 0x040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T0_outMask = 0x080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T0_outMask = 0x100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T0_outMask = 0x200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T0_outMask = 0x400;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T0_outMask = 0x000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T0_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			if (streq(value, "T0-IN"))			FERScfg[brd]->T0_outMask = 0x0001;
			else if (streq(value, "BUNCHTRG"))		FERScfg[brd]->T0_outMask = 0x0002;
			else if (streq(value, "OR-TQ"))			FERScfg[brd]->T0_outMask = 0x0004;
			else if (streq(value, "RUN"))			FERScfg[brd]->T0_outMask = 0x0008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T0_outMask = 0x0010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T0_outMask = 0x0020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T0_outMask = 0x0040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T0_outMask = 0x0080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T0_outMask = 0x0100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T0_outMask = 0x0200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T0_outMask = 0x0400;
			else if (streq(value, "OR-T1"))			FERScfg[brd]->T0_outMask = 0x0800;
			else if (streq(value, "OR-T2"))			FERScfg[brd]->T0_outMask = 0x1000;
			else if (streq(value, "XROC_TRG_ASYN"))	FERScfg[brd]->T0_outMask = 0x2000;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T0_outMask = 0x0000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T0_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else ValidParameterValue = 0;
	}

	if (streq(str, "T1_Out")) {
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "T1-IN"))			FERScfg[brd]->T1_outMask = 0x001;
			else if (streq(value, "BUNCHTRG"))		FERScfg[brd]->T1_outMask = 0x002;
			else if (streq(value, "Q-OR"))			FERScfg[brd]->T1_outMask = 0x004;
			else if (streq(value, "RUN"))			FERScfg[brd]->T1_outMask = 0x008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T1_outMask = 0x010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T1_outMask = 0x020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T1_outMask = 0x040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T1_outMask = 0x080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T1_outMask = 0x100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T1_outMask = 0x200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T1_outMask = 0x400;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T1_outMask = 0x000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T1_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "T1-IN"))				FERScfg[brd]->T1_outMask = 0x001;
			else if (streq(value, "TRIGGER"))		FERScfg[brd]->T1_outMask = 0x002;
			else if (streq(value, "RUN"))			FERScfg[brd]->T1_outMask = 0x008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T1_outMask = 0x010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T1_outMask = 0x020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T1_outMask = 0x040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T1_outMask = 0x080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T1_outMask = 0x100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T1_outMask = 0x200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T1_outMask = 0x400;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T1_outMask = 0x000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T1_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			if (streq(value, "T1-IN"))			FERScfg[brd]->T1_outMask = 0x0001;
			else if (streq(value, "BUNCHTRG"))		FERScfg[brd]->T1_outMask = 0x0002;
			else if (streq(value, "OR-TQ"))			FERScfg[brd]->T1_outMask = 0x0004;
			else if (streq(value, "RUN"))			FERScfg[brd]->T1_outMask = 0x0008;
			else if (streq(value, "PTRG"))			FERScfg[brd]->T1_outMask = 0x0010;
			else if (streq(value, "BUSY"))			FERScfg[brd]->T1_outMask = 0x0020;
			else if (streq(value, "DPROBE"))		FERScfg[brd]->T1_outMask = 0x0040;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->T1_outMask = 0x0080;
			else if (streq(value, "SQ_WAVE"))		FERScfg[brd]->T1_outMask = 0x0100;
			else if (streq(value, "TDL_SYNC"))		FERScfg[brd]->T1_outMask = 0x0200;
			else if (streq(value, "RUN_SYNC"))		FERScfg[brd]->T1_outMask = 0x0400;
			else if (streq(value, "OR-T1"))			FERScfg[brd]->T1_outMask = 0x0800;
			else if (streq(value, "OR-T2"))			FERScfg[brd]->T1_outMask = 0x1000;
			else if (streq(value, "XROC_TRG_ASYN"))	FERScfg[brd]->T1_outMask = 0x2000;
			else if (streq(value, "ZERO"))			FERScfg[brd]->T1_outMask = 0x0000;
			else if (strstr(value, "MASK"))			FERScfg[brd]->T1_outMask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		}
	}

	if (streq(str, "TrefSource")) {
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "T0-IN"))				FERScfg[brd]->Tref_Mask = 0x1;
			else if (streq(value, "T1-IN"))			FERScfg[brd]->Tref_Mask = 0x2;
			else if (streq(value, "Q-OR"))			FERScfg[brd]->Tref_Mask = 0x4;
			else if (streq(value, "T-OR"))			FERScfg[brd]->Tref_Mask = 0x8;
			else if (streq(value, "PTRG"))			FERScfg[brd]->Tref_Mask = 0x10;
			else if (streq(value, "TLOGIC"))		FERScfg[brd]->Tref_Mask = 0x40;
			else if (strstr(value, "MASK"))			FERScfg[brd]->Tref_Mask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "ZERO") || streq(value, "0") || streq(value, "CH0"))	
				FERScfg[brd]->Tref_Mask = 0x0;
			else if (streq(value, "T0-IN"))				FERScfg[brd]->Tref_Mask = 0x1;
			else if (streq(value, "T1-IN"))			FERScfg[brd]->Tref_Mask = 0x2;
			else if (streq(value, "PTRG"))			FERScfg[brd]->Tref_Mask = 0x10;
			else if (strstr(value, "MASK"))			FERScfg[brd]->Tref_Mask = GetHex32(trim(strtok(value, "MASK")));
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			// HACK CTIN Need this for A5204???
		} else ValidParameterValue = 0;
	}

	if (streq(str, "ValidationSource")) {
		if (streq(value, "SW_CMD"))		FERScfg[brd]->Validation_Mask = 0x1;
		else if (streq(value, "T0-IN"))			FERScfg[brd]->Validation_Mask = 0x2;
		else if (streq(value, "T1-IN"))			FERScfg[brd]->Validation_Mask = 0x4;
		else if (strstr(value, "MASK"))			FERScfg[brd]->Validation_Mask = GetHex32(trim(strtok(value, "MASK")));
		else ValidParameterValue = 0;
	}

	if (streq(str, "ValidationMode")) {
		if (streq(value, "DISABLED"))			FERScfg[brd]->Validation_Mode = 0;
		else if (streq(value, "ACCEPT"))		FERScfg[brd]->Validation_Mode = 1;
		else if (streq(value, "REJECT"))		FERScfg[brd]->Validation_Mode = 2;
		else ValidParameterValue = 0;
	}

	if (streq(str, "CountingMode")) {
		if (streq(value, "SINGLES"))			FERScfg[brd]->Counting_Mode = 0;
		else if (streq(value, "PAIRED_AND"))	FERScfg[brd]->Counting_Mode = 1;
		else ValidParameterValue = 0;
	}

	if (streq(str, "VetoSource") && FERS_BoardInfo[brd]->FERSCode == 5202) {
		if (streq(value, "DISABLED"))		FERScfg[brd]->Veto_Mask = 0x0;
		else if (streq(value, "SW_CMD"))	FERScfg[brd]->Veto_Mask = 0x1;
		else if (streq(value, "T0-IN"))		FERScfg[brd]->Veto_Mask = 0x2;
		else if (streq(value, "T1-IN"))		FERScfg[brd]->Veto_Mask = 0x4;
		else if (strstr(value, "MASK"))     FERScfg[brd]->Veto_Mask = GetHex32(trim(strtok(value, "MASK")));
		else ValidParameterValue = 0;
	}

	if (streq(str, "TrgIdMode")) {
		if (streq(value, "TRIGGER_CNT"))			FERScfg[brd]->TrgIdMode = 0;
		else if (streq(value, "VALIDATION_CNT"))	FERScfg[brd]->TrgIdMode = 1;
		else ValidParameterValue = 0;
	}

	if (streq(str, "En_service_event") || streq(str, "EnableServiceEvents")) {
		if (streq(value, "DISABLED") || streq(value, "0"))		FERScfg[brd]->EnableServiceEvents = 0;
		else if (streq(value, "ENABLED")) {
			if (FERS_IsXROC(handle)) FERScfg[brd]->EnableServiceEvents = 3;
			else FERScfg[brd]->EnableServiceEvents = 1;
		} else FERScfg[brd]->EnableServiceEvents = GetInt(value);
	}

	if (streq(str, "En_Empty_Ev_Suppr")) {
		if (streq(value, "DISABLED") || streq(value, "0"))			FERScfg[brd]->En_Empty_Ev_Suppr = 0;
		else if (streq(value, "ENABLED") || streq(value, "1"))		FERScfg[brd]->En_Empty_Ev_Suppr = 1;
		else ValidParameterValue = 0;
	}

	if (streq(str, "PtrgPeriod"))				FERScfg[brd]->PtrgPeriod = GetTime(value, "ns");
	if (streq(str, "TrgHoldOff"))				FERScfg[brd]->TrgHoldOff = GetTime(value, "ns");
	if (streq(str, "EnableChannelTrgout"))		FERScfg[brd]->EnableChannelTrgout = GetInt(value);
	if (streq(str, "Enable_2nd_tstamp"))		FERScfg[brd]->Enable_2nd_tstamp = GetInt(value);

	if (streq(str, "EnableToT"))				
		FERScfg[brd]->EnableToT = GetInt(value);
	if (streq(str, "GateWidth"))				FERScfg[brd]->GateWidth = GetTime(value, "ns");
	if (streq(str, "TrefWindow"))				FERScfg[brd]->TrefWindow = GetTime(value, "ns");
	if (streq(str, "TrefDelay"))				FERScfg[brd]->TrefDelay = GetTime(value, "ns");
	if (streq(str, "WaveformLength"))			FERScfg[brd]->WaveformLength = GetInt(value);
	if (streq(str, "Range_14bit")) {
		FERScfg[brd]->Range_14bit = GetInt(value);
		FERS_SetEnergyBitsRange((uint16_t)FERScfg[brd]->Range_14bit);
	}
	if (streq(str, "EnableCntZeroSuppr"))		FERScfg[brd]->EnableCntZeroSuppr = GetInt(value);
	if (streq(str, "ZS_Threshold_LG"))			SetChannelParam16(handle, FERScfg[brd]->ZS_Threshold_LG, (uint16_t)GetInt(value), ch);
	if (streq(str, "ZS_Threshold_HG"))			SetChannelParam16(handle, FERScfg[brd]->ZS_Threshold_HG, (uint16_t)GetInt(value), ch);

	// -------------------------------------------------------------
	// Settings for Channel enabling
	// -------------------------------------------------------------
	if (streq(str, "ChEnable")) {
		int en = 0;
		uint64_t en_bit = 0;
		sscanf(value, "%d", &en);
		if ((ch >= 0) && (ch < 64)) {
			if (en) en_bit = (uint64_t)1 << ch;
			FERScfg[brd]->ChEnableMask = (FERScfg[brd]->ChEnableMask & ~((uint64_t)1 << ch)) | en_bit;
		} else {
			if (en) en_bit = (uint64_t)1 << (ch - 64);
			FERScfg[brd]->ChEnableMask_e = (FERScfg[brd]->ChEnableMask_e & ~((uint64_t)1 << (ch - 64))) | en_bit;
		}
	}
	if (streq(str, "ChEnableMask0"))			FERScfg[brd]->ChEnableMask = FERScfg[brd]->ChEnableMask & 0xFFFFFFFF00000000 | GetHex32(value);
	if (streq(str, "ChEnableMask1"))			FERScfg[brd]->ChEnableMask = FERScfg[brd]->ChEnableMask & 0x00000000FFFFFFFF | ((uint64_t)GetHex32(value) << 32);
	if (streq(str, "ChEnableMask"))				FERScfg[brd]->ChEnableMask = GetHex64(value);
	if (streq(str, "ChEnableMask2"))			FERScfg[brd]->ChEnableMask_e = FERScfg[brd]->ChEnableMask_e & 0xFFFFFFFF00000000 | GetHex32(value);
	if (streq(str, "ChEnableMask3"))			FERScfg[brd]->ChEnableMask_e = FERScfg[brd]->ChEnableMask_e & 0x00000000FFFFFFFF | ((uint64_t)GetHex32(value) << 32);
	if (streq(str, "ChEnableMask_e"))			FERScfg[brd]->ChEnableMask_e = GetHex64(value);


	// -------------------------------------------------------------
	// Settings for analog and digital probes
	// -------------------------------------------------------------
	if (streq(str, "DigitalProbe") || streq(str, "DigitalProbe0") || streq(str, "DigitalProbe1")) {
		//int val_b;
		int val;
		int dp = 0;
		if (streq(str, "DigitalProbe1")) dp = 1;
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "OFF"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_OFF;
			else if (streq(value, "PEAK_LG"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_PEAK_LG;
			else if (streq(value, "PEAK_HG"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_PEAK_HG;
			else if (streq(value, "HOLD"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_HOLD;
			else if (streq(value, "START_CONV"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_START_CONV;
			else if (streq(value, "DATA_COMMIT"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_COMMIT;
			else if (streq(value, "DATA_VALID"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_VALID;
			else if (streq(value, "CLK_1024"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_CLK_1024;
			else if (streq(value, "VAL_WINDOW"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_VAL_WINDOW;
			else if (streq(value, "T_OR"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_T_OR;
			else if (streq(value, "Q_OR"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_Q_OR;
			else if (strstr(value, "ACQCTRL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else if (strstr(value, "CRIF") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80010000 | val;
			} else if (strstr(value, "DTBLD") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80020000 | val;
			} else if (strstr(value, "TSTMP") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80030000 | val;
			} else if (strstr(value, "TDL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80040000 | val;
			} else if (strstr(value, "PMP") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80050000 | val;
			} else if ((value[0] == '0') && (tolower(value[1]) == 'x')) {
				sscanf(value + 2, "%x", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5203) {
			if (streq(value, "OFF"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_OFF;
			else if (streq(value, "DATA_COMMIT"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_COMMIT;
			else if (streq(value, "DATA_VALID"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_VALID;
			else if (streq(value, "CLK_1024"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_CLK_1024;
			else if (streq(value, "VAL_WINDOW"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_VAL_WINDOW;
			else if (strstr(value, "ACQCTRL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else if (strstr(value, "DTBLD") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80020000 | val;
			} else if (strstr(value, "TDL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80040000 | val;
			} else if (strstr(value, "PMP") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80050000 | val;
			} else if ((value[0] == '0') && (tolower(value[1]) == 'x')) {
				sscanf(value + 2, "%x", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {  // HACK CTIN: set dprobe for 5203
			if (streq(value, "OFF"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_OFF;
			else if (streq(value, "HOLD"))			FERScfg[brd]->DigitalProbe[dp] = DPROBE_HOLD;
			else if (streq(value, "START_CONV"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_START_CONV;
			else if (streq(value, "DATA_COMMIT"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_COMMIT;
			else if (streq(value, "DATA_VALID"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_DATA_VALID;
			else if (streq(value, "CLK_1024"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_CLK_1024;
			else if (streq(value, "VAL_WINDOW"))	FERScfg[brd]->DigitalProbe[dp] = DPROBE_VAL_WINDOW;
			else if (streq(value, "XROC_TRG"))		FERScfg[brd]->DigitalProbe[dp] = DPROBE_XROC_TRG;
			else if (strstr(value, "ACQCTRL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else if (strstr(value, "XRIF") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80010000 | val;
			} else if (strstr(value, "DTBLD") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80020000 | val;
			} else if (strstr(value, "TDL") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80040000 | val;
			} else if (strstr(value, "PMP") != NULL) {
				char* c = strchr(value, '_');
				sscanf(c + 1, "%d", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80050000 | val;
			} else if ((value[0] == '0') && (tolower(value[1]) == 'x')) {
				sscanf(value + 2, "%x", &val);
				FERScfg[brd]->DigitalProbe[dp] = 0x80000000 | val;
			} else ValidParameterValue = 0;
		} else ValidParameterValue = 0;

		if (streq(str, "DigitalProbe")) FERScfg[brd]->DigitalProbe[1] = FERScfg[brd]->DigitalProbe[0];
	}

	if (streq(str, "ProbeChannel0"))			FERScfg[brd]->ProbeChannel[0] = GetInt(value);
	if (streq(str, "ProbeChannel1"))			FERScfg[brd]->ProbeChannel[1] = GetInt(value);
	if (streq(str, "ProbeChannel")) {
		int pch = GetInt(value);
		FERScfg[brd]->ProbeChannel[0] = pch;
		FERScfg[brd]->ProbeChannel[1] = pch;
	}

	// -------------------------------------------------------------
	// Settings for DT5215 (Concentrator)
	// -------------------------------------------------------------
	if (streq(str, "FiberDelayAdjust")) {
		int np, cnc, chain, node;
		float length; // length expressed in m
		np = sscanf(value, "%d %d %d %f", &cnc, &chain, &node, &length);
		if ((np == 4) && (cnc >= 0) && (cnc < FERSLIB_MAX_NCNC) && (chain >= 0) && (chain < 8) && (node >= 0) && (node < 16))
			TDL_FiberDelayAdjust[cnc][chain][node] = length;
	}
	if (streq(str, "TdlClkPhase"))				FERScfg[brd]->TdlClkPhase = GetInt(value);
	if (streq(str, "CncBufferSize"))			FERScfg[brd]->CncBufferSize = GetInt(value);
	if (streq(str, "CncProbe_A"))				FERScfg[brd]->CncProbe_A = GetInt(value);
	if (streq(str, "CncProbe_B"))				FERScfg[brd]->CncProbe_B = GetInt(value);
	if (streq(str, "MaxPck_Block"))				FERScfg[brd]->MaxPck_Block = GetInt(value);
	if (streq(str, "MaxPck_Train"))				FERScfg[brd]->MaxPck_Train = GetInt(value);	


	// -------------------------------------------------------------
	// Settings for FPGA trigger logic
	// -------------------------------------------------------------
	if (streq(str, "TriggerLogic")) {
		if (streq(value, "OR64"))				FERScfg[brd]->TriggerLogic = 0;
		else if (streq(value, "AND2_OR32"))		FERScfg[brd]->TriggerLogic = 1;
		else if (streq(value, "OR32_AND2"))		FERScfg[brd]->TriggerLogic = 2;
		else if (streq(value, "MAJ64"))			FERScfg[brd]->TriggerLogic = 4;
		else if (streq(value, "MAJ32_AND2"))	FERScfg[brd]->TriggerLogic = 5;
		else ValidParameterValue = 0;
	}

	if (streq(str, "Tlogic_Mask0"))				FERScfg[brd]->Tlogic_Mask = FERScfg[brd]->Tlogic_Mask & 0xFFFFFFFF00000000 | (uint64_t)GetHex32(value);
	if (streq(str, "Tlogic_Mask1"))				FERScfg[brd]->Tlogic_Mask = FERScfg[brd]->Tlogic_Mask & 0x00000000FFFFFFFF | ((uint64_t)GetHex32(value) << 32);
	if (streq(str, "Tlogic_Mask"))				FERScfg[brd]->Tlogic_Mask = GetHex64(value);
	if (streq(str, "ChTrg_Width"))				FERScfg[brd]->ChTrg_Width = GetTime(value, "ns");
	if (streq(str, "Tlogic_Width"))				FERScfg[brd]->Tlogic_Width = GetTime(value, "ns");
	if (streq(str, "MajorityLevel"))			FERScfg[brd]->MajorityLevel = GetInt(value);

	// -------------------------------------------------------------
	// Settings for XROC ASICs
	// -------------------------------------------------------------
	if (streq(str, "TestPulseDestination")) {
		if (streq(value, "ALL"))				FERScfg[brd]->TestPulseDestination = TEST_PULSE_DEST_ALL;
		else if (streq(value, "EVEN"))			FERScfg[brd]->TestPulseDestination = TEST_PULSE_DEST_EVEN;
		else if (streq(value, "ODD"))			FERScfg[brd]->TestPulseDestination = TEST_PULSE_DEST_ODD;
		else if (streq(value, "NONE"))			FERScfg[brd]->TestPulseDestination = TEST_PULSE_DEST_NONE;
		else if (strstr(value, "CH") != NULL)	FERScfg[brd]->TestPulseDestination = GetInt(trim(strtok(value, "CH")));
		else ValidParameterValue = 0;
	}

	if (streq(str, "TestPulsePreamp")) {
		if (streq(value, "HG"))			FERScfg[brd]->TestPulsePreamp = TEST_PULSE_PREAMP_HG;
		else if (streq(value, "LG"))	FERScfg[brd]->TestPulsePreamp = TEST_PULSE_PREAMP_LG;
		else if (streq(value, "BOTH"))	FERScfg[brd]->TestPulsePreamp = TEST_PULSE_PREAMP_BOTH;
		else ValidParameterValue = 0;
	}

	if (streq(str, "LG_ShapingTime")) {
		if (FERS_Code(handle) == 5202) {
			float st = GetTime(value, "ns");
			if ((st == 0) || (st == 87.5))			FERScfg[brd]->LG_ShapingTime = 0;
			else if ((st == 1) || (st == 75))		FERScfg[brd]->LG_ShapingTime = 1;
			else if ((st == 2) || (st == 62.5))		FERScfg[brd]->LG_ShapingTime = 2;
			else if ((st == 3) || (st == 50))		FERScfg[brd]->LG_ShapingTime = 3;
			else if ((st == 4) || (st == 37.5))		FERScfg[brd]->LG_ShapingTime = 4;
			else if ((st == 5) || (st == 25))		FERScfg[brd]->LG_ShapingTime = 5;
			else if ((st == 6) || (st == 12.5))		FERScfg[brd]->LG_ShapingTime = 6;
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			float st_ns = GetTime(value, "ns");
			uint16_t st_set = (uint16_t)(st_ns / 20);
			if ((st_set < 0) || (st_set > 0xF) || ((float)(st_set * 20) != st_ns))
				ValidParameterValue = 0;
			else
				SetChannelParam16(handle, FERScfg[brd]->LG_ShapingTime_ind, st_set, ch);
		} else ValidParameterValue = 0;
	}

	if (streq(str, "HG_ShapingTime")) {
		if (FERS_Code(handle) == 5202) {
			float st = GetTime(value, "ns");
			if ((st == 0) || (st == 87.5))			FERScfg[brd]->HG_ShapingTime = 0;
			else if ((st == 1) || (st == 75))		FERScfg[brd]->HG_ShapingTime = 1;
			else if ((st == 2) || (st == 62.5))		FERScfg[brd]->HG_ShapingTime = 2;
			else if ((st == 3) || (st == 50))		FERScfg[brd]->HG_ShapingTime = 3;
			else if ((st == 4) || (st == 37.5))		FERScfg[brd]->HG_ShapingTime = 4;
			else if ((st == 5) || (st == 25))		FERScfg[brd]->HG_ShapingTime = 5;
			else if ((st == 6) || (st == 12.5))		FERScfg[brd]->HG_ShapingTime = 6;
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			float st_ns = GetTime(value, "ns");
			uint16_t st_set = (uint16_t)(st_ns / 20);
			if ((st_set < 0) || (st_set > 0xF) || ((float)(st_set * 20) != st_ns))
				ValidParameterValue = 0;
			else
				SetChannelParam16(handle, FERScfg[brd]->HG_ShapingTime_ind, st_set, ch);
		} else ValidParameterValue = 0;
	}

	if (streq(str, "AnalogProbe") || streq(str, "AnalogProbe0") || streq(str, "AnalogProbe1")) {
		int ap = 0;
		if (streq(str, "AnalogProbe1")) ap = 1;
		if (FERS_Code(handle) == 5202) {
			if (streq(value, "OFF"))					FERScfg[brd]->AnalogProbe[ap] = 0;
			else if (streq(value, "FAST"))				FERScfg[brd]->AnalogProbe[ap] = 1;
			else if (streq(value, "SLOW_LG"))			FERScfg[brd]->AnalogProbe[ap] = 2;
			else if (streq(value, "SLOW_HG"))			FERScfg[brd]->AnalogProbe[ap] = 3;
			else if (streq(value, "PREAMP_LG"))			FERScfg[brd]->AnalogProbe[ap] = 4;
			else if (streq(value, "PREAMP_HG"))			FERScfg[brd]->AnalogProbe[ap] = 5;
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5204) {
			if (streq(value, "OFF"))					FERScfg[brd]->AnalogProbe[ap] = 0;
			else if (streq(value, "PREAMP_LG"))			FERScfg[brd]->AnalogProbe[ap] = 1;
			else if (streq(value, "PREAMP_HG"))			FERScfg[brd]->AnalogProbe[ap] = 2;
			else if (streq(value, "SHAPER_LG"))			FERScfg[brd]->AnalogProbe[ap] = 3;
			else if (streq(value, "SHAPER_HG"))			FERScfg[brd]->AnalogProbe[ap] = 4;
			else if (streq(value, "CHARGE_TRIGGER"))	FERScfg[brd]->AnalogProbe[ap] = 5;
			else ValidParameterValue = 0;
		} else if (FERS_Code(handle) == 5205) {
			if (streq(value, "OFF"))					FERScfg[brd]->AnalogProbe[ap] = 0;
			else if (streq(value, "PREAMP"))			FERScfg[brd]->AnalogProbe[ap] = 1;
			else if (streq(value, "FAST"))				FERScfg[brd]->AnalogProbe[ap] = 2;
			else if (streq(value, "SHAPER_LG"))			FERScfg[brd]->AnalogProbe[ap] = 3;
			else if (streq(value, "SHAPER_HG"))			FERScfg[brd]->AnalogProbe[ap] = 4;
			else if (streq(value, "CHARGE_TRIGGER"))	FERScfg[brd]->AnalogProbe[ap] = 5;
			else ValidParameterValue = 0;
		}
		if (streq(str, "AnalogProbe"))			FERScfg[brd]->AnalogProbe[1] = FERScfg[brd]->AnalogProbe[0];
	}

	if (streq(str, "CitirocCfgMode")) {
		if (streq(value, "FROM_FILE"))			FERScfg[brd]->CitirocCfgMode = CITIROC_CFG_FROM_FILE;
		else if (streq(value, "FROM_REGS"))		FERScfg[brd]->CitirocCfgMode = CITIROC_CFG_FROM_REGS;
		else ValidParameterValue = 0;
	}

	if (streq(str, "PeakDetectorMode")) {
		if (streq(value, "PEAK_STRETCH"))		FERScfg[brd]->PeakDetectorMode = 0;
		else if (streq(value, "TRACK&HOLD"))	FERScfg[brd]->PeakDetectorMode = 1;
		else ValidParameterValue = 0;
	}

	if (streq(str, "FastShaperInput")) {
		if ((streq(value, "HG-PA") || streq(value, "HG")))		FERScfg[brd]->FastShaperInput = FAST_SHAPER_INPUT_HGPA;
		else if ((streq(value, "LG-PA") || streq(value, "LG")))	FERScfg[brd]->FastShaperInput = FAST_SHAPER_INPUT_LGPA;
		else ValidParameterValue = 0;
	}

	if (streq(str, "HV_Adjust_Range")) {
		if ((streq(value, "2.5") || streq(value, "0")))			FERScfg[brd]->HV_Adjust_Range = 0;
		else if ((streq(value, "4.5") || streq(value, "1")))	FERScfg[brd]->HV_Adjust_Range = 1;
		else if (streq(value, "DISABLED"))						FERScfg[brd]->HV_Adjust_Range = (uint16_t)-1;
		else ValidParameterValue = 0;
	}

	if (streq(str, "MuxNSmean")) {
		if ((streq(value, "1") || streq(value, "0")))			FERScfg[brd]->MuxNSmean = 0;
		else if ((streq(value, "4") || streq(value, "1")))		FERScfg[brd]->MuxNSmean = 1;
		else if ((streq(value, "16") || streq(value, "2")))		FERScfg[brd]->MuxNSmean = 2;
		else ValidParameterValue = 0;
	}

	if (streq(str, "GainSelect")) {
		if ((streq(value, "HIGH") || streq(value, "HG")))		FERScfg[brd]->GainSelect = GAIN_SEL_HIGH;
		else if ((streq(value, "LOW") || streq(value, "LG")))	FERScfg[brd]->GainSelect = GAIN_SEL_LOW;
		else if (streq(value, "AUTO"))							FERScfg[brd]->GainSelect = GAIN_SEL_AUTO;
		else if (streq(value, "BOTH"))							FERScfg[brd]->GainSelect = GAIN_SEL_BOTH;
		else ValidParameterValue = 0;
	}

	if (streq(str, "TD1_FineThreshold"))		SetChannelParam16(handle, FERScfg[brd]->TD1_FineThreshold, (uint16_t)GetInt(value), ch);
	if (streq(str, "TD2_FineThreshold"))		SetChannelParam16(handle, FERScfg[brd]->TD2_FineThreshold, (uint16_t)GetInt(value), ch);
	if (streq(str, "TOTD_FineThreshold"))		SetChannelParam16(handle, FERScfg[brd]->TOTD_FineThreshold, (uint16_t)GetInt(value), ch);
	if (streq(str, "QD_FineThreshold"))			SetChannelParam16(handle, FERScfg[brd]->QD_FineThreshold, (uint16_t)GetInt(value), ch);
	if (streq(str, "TD_FineThreshold"))			SetChannelParam16(handle, FERScfg[brd]->TD_FineThreshold, (uint16_t)GetInt(value), ch);
	if (streq(str, "HG_Gain"))					SetChannelParam16(handle, FERScfg[brd]->HG_Gain, (uint16_t)GetInt(value), ch);
	if (streq(str, "LG_Gain"))					SetChannelParam16(handle, FERScfg[brd]->LG_Gain, (uint16_t)GetInt(value), ch);
	if (streq(str, "T_Gain"))					SetChannelParam16(handle, FERScfg[brd]->T_Gain, (uint16_t)GetInt(value), ch);
	if (streq(str, "HV_IndivAdj"))				SetChannelParam16(handle, FERScfg[brd]->HV_IndivAdj, (uint16_t)GetInt(value), ch);
	if (streq(str, "QD_CoarseThreshold"))		FERScfg[brd]->QD_CoarseThreshold = (uint16_t)GetInt(value);
	if (streq(str, "TD1_CoarseThreshold"))		FERScfg[brd]->TD1_CoarseThreshold = (uint16_t)GetInt(value);
	if (streq(str, "TD2_CoarseThreshold"))		FERScfg[brd]->TD2_CoarseThreshold = (uint16_t)GetInt(value);
	if (streq(str, "TOTD_CoarseThreshold"))		FERScfg[brd]->TOTD_CoarseThreshold = (uint16_t)GetInt(value);
	if (streq(str, "TD_CoarseThreshold"))		FERScfg[brd]->TD_CoarseThreshold = (uint16_t)GetInt(value);
	if (streq(str, "QD_CoarseThreshold"))		FERScfg[brd]->QD_CoarseThreshold = (uint16_t)GetInt(value);

	if (streq(str, "Enable_HV_Adjust"))			FERScfg[brd]->Enable_HV_Adjust = GetInt(value);
	if (streq(str, "HoldDelay"))				FERScfg[brd]->HoldDelay = GetTime(value, "ns");
	if (streq(str, "EnableQdiscrLatch"))		FERScfg[brd]->EnableQdiscrLatch = GetInt(value);
	if (streq(str, "MuxClkPeriod"))				FERScfg[brd]->MuxClkPeriod = GetTime(value, "ns");
	if (streq(str, "Pedestal"))					FERScfg[brd]->Pedestal = (uint16_t)GetInt(value);
	if (streq(str, "QD_Mask0"))					FERScfg[brd]->QD_Mask = FERScfg[brd]->QD_Mask & 0xFFFFFFFF00000000 | (uint64_t)GetHex32(value);
	if (streq(str, "QD_Mask1"))					FERScfg[brd]->QD_Mask = FERScfg[brd]->QD_Mask & 0x00000000FFFFFFFF | ((uint64_t)GetHex32(value) << 32);
	if (streq(str, "QD_Mask"))					FERScfg[brd]->QD_Mask = GetHex64(value);
	if (streq(str, "TD1_Mask"))					FERScfg[brd]->TD1_Mask = GetHex64(value);
	if (streq(str, "TD2_Mask"))					FERScfg[brd]->TD2_Mask = GetHex64(value);
	if (streq(str, "TD_Mask"))					FERScfg[brd]->TD_Mask = GetHex64(value);
	if (streq(str, "TOTD_Mask"))				FERScfg[brd]->TOTD_Mask = GetHex64(value);


	// -------------------------------------------------------------
	// Settings for Analog Test Pulser
	// -------------------------------------------------------------
	if (streq(str, "TestPulseSource")) {
		if (streq(value, "OFF"))			FERScfg[brd]->TestPulseSource = -1;
		else if (streq(value, "EXT"))		FERScfg[brd]->TestPulseSource = TEST_PULSE_SOURCE_EXT;
		else if (streq(value, "T0-IN"))		FERScfg[brd]->TestPulseSource = TEST_PULSE_SOURCE_T0_IN;
		else if (streq(value, "T1-IN"))		FERScfg[brd]->TestPulseSource = TEST_PULSE_SOURCE_T1_IN;
		else if (streq(value, "PTRG"))		FERScfg[brd]->TestPulseSource = TEST_PULSE_SOURCE_PTRG;
		else if (streq(value, "SW-CMD"))	FERScfg[brd]->TestPulseSource = TEST_PULSE_SOURCE_SW_CMD;
		else ValidParameterValue = 0;
	}
	if (streq(str, "TestPulseAmplitude"))		FERScfg[brd]->TestPulseAmplitude = GetInt(value);


	// -------------------------------------------------------------
	// Settings for HV
	// -------------------------------------------------------------
	if (streq(str, "TempSensType")) {
		if (streq(value, "TMP37")) {
			FERScfg[brd]->TempSensCoeff[0] = 0;
			FERScfg[brd]->TempSensCoeff[1] = (float)50;
			FERScfg[brd]->TempSensCoeff[2] = 0;
		} else if (streq(value, "LM94021_G11")) {
			FERScfg[brd]->TempSensCoeff[0] = (float)194.25;
			FERScfg[brd]->TempSensCoeff[1] = (float)-73.63;
			FERScfg[brd]->TempSensCoeff[2] = 0;
		} else if (streq(value, "LM94021_G00")) {
			FERScfg[brd]->TempSensCoeff[0] = (float)188.12;
			FERScfg[brd]->TempSensCoeff[1] = (float)-181.62;
			FERScfg[brd]->TempSensCoeff[2] = 0;
		} else {
			float v0, v1, v2;
			if (sscanf(value, "%f", &v0) == 1) {   // DNIN: To BeChanged
				sscanf(value, "%f %f %f", &v0, &v1, &v2);
				FERScfg[brd]->TempSensCoeff[0] = v0;
				FERScfg[brd]->TempSensCoeff[1] = v1;
				FERScfg[brd]->TempSensCoeff[2] = v2;
			} else ValidParameterValue = 0;
		}
	}
	if (streq(str, "EnableTempFeedback"))		FERScfg[brd]->EnableTempFeedback = GetInt(value);
	if (streq(str, "TempFeedbackCoeff"))		FERScfg[brd]->TempFeedbackCoeff = GetFloat(value);
	if (streq(str, "HV_Vbias"))					FERScfg[brd]->HV_Vbias = GetVoltage(value);
	if (streq(str, "HV_Imax"))					FERScfg[brd]->HV_Imax = GetCurrent(value);


	// -------------------------------------------------------------
	// Settings for picoTDC
	// -------------------------------------------------------------
	if (streq(str, "MeasMode")) {
		if (streq(value, "LEAD_ONLY"))			FERScfg[brd]->MeasMode = MEASMODE_LEAD_ONLY;
		else if (streq(value, "LEAD_TRAIL"))	FERScfg[brd]->MeasMode = MEASMODE_LEAD_TRAIL;
		else if (streq(value, "LEAD_TOT8"))		FERScfg[brd]->MeasMode = MEASMODE_LEAD_TOT8;
		else if (streq(value, "LEAD_TOT11"))	FERScfg[brd]->MeasMode = MEASMODE_LEAD_TOT11;
		else ValidParameterValue = 0;
	}

	if (streq(str, "En_Head_Trail")) {
		if (streq(value, "KEEP_ALL") || streq(value, "0"))			FERScfg[brd]->En_Head_Trail = 0;
		else if (streq(value, "ONE_WORD") || streq(value, "1"))		FERScfg[brd]->En_Head_Trail = 1;
		else if (streq(value, "SUPPRESSED") || streq(value, "2"))	FERScfg[brd]->En_Head_Trail = 2;
		else ValidParameterValue = 0;
	}

	if (streq(str, "GlitchFilterMode")) {
		if (streq(value, "DISABLED"))			FERScfg[brd]->GlitchFilterMode = GLITCHFILTERMODE_DISABLED;
		else if (streq(value, "TRAILING"))		FERScfg[brd]->GlitchFilterMode = GLITCHFILTERMODE_TRAILING;
		else if (streq(value, "LEADING"))		FERScfg[brd]->GlitchFilterMode = GLITCHFILTERMODE_LEADING;
		else if (streq(value, "BOTH"))			FERScfg[brd]->GlitchFilterMode = GLITCHFILTERMODE_BOTH;
		else ValidParameterValue = 0;
	}

	if (streq(str, "TDC_ChannelBufferSize")) {
		if (streq(value, "4"))			FERScfg[brd]->TDC_ChBufferSize = 0x0;
		else if (streq(value, "8"))		FERScfg[brd]->TDC_ChBufferSize = 0x1;
		else if (streq(value, "16"))	FERScfg[brd]->TDC_ChBufferSize = 0x2;
		else if (streq(value, "32"))	FERScfg[brd]->TDC_ChBufferSize = 0x3;
		else if (streq(value, "64"))	FERScfg[brd]->TDC_ChBufferSize = 0x4;
		else if (streq(value, "128"))	FERScfg[brd]->TDC_ChBufferSize = 0x5;
		else if (streq(value, "256"))	FERScfg[brd]->TDC_ChBufferSize = 0x6;
		else if (streq(value, "512"))	FERScfg[brd]->TDC_ChBufferSize = 0x7;
		else ValidParameterValue = 0;
	}

	if (streq(str, "HighResClock")) {
		if (streq(value, "DISABLED"))			FERScfg[brd]->HighResClock = HRCLK_DISABLED;
		else if (streq(value, "DAISY_CHAIN"))	FERScfg[brd]->HighResClock = HRCLK_DAISY_CHAIN;
		else if (streq(value, "FAN_OUT"))		FERScfg[brd]->HighResClock = HRCLK_FAN_OUT;
		else ValidParameterValue = 0;
	}

	if (streq(str, "GlitchFilterDelay"))		FERScfg[brd]->GlitchFilterMode = GetInt(value);
	if (streq(str, "TriggerBufferSize"))		FERScfg[brd]->TriggerBufferSize = GetInt(value);
	if (streq(str, "HeaderField0"))				FERScfg[brd]->HeaderField0 = GetInt(value);
	if (streq(str, "HeaderField1"))				FERScfg[brd]->HeaderField1 = GetInt(value);
	if (streq(str, "LeadTrail_LSB"))			FERScfg[brd]->LeadTrail_LSB = GetInt(value);
	if (streq(str, "ToT_LSB"))					FERScfg[brd]->ToT_LSB = GetInt(value);
	if (streq(str, "ToT_reject_low_thr"))		FERScfg[brd]->ToT_reject_low_thr = (uint16_t)GetInt(value);
	if (streq(str, "ToT_reject_high_thr"))		FERScfg[brd]->ToT_reject_high_thr = (uint16_t)GetInt(value);
	if (streq(str, "TrgWindowWidth"))			FERScfg[brd]->TrgWindowWidth = GetTime(value, "ns");
	if (streq(str, "TrgWindowOffset"))			FERScfg[brd]->TrgWindowOffset = GetTime(value, "ns");
	if (streq(str, "TDCpulser_Width"))			FERScfg[brd]->TDCpulser_Width = GetTime(value, "ns");
	if (streq(str, "TDCpulser_Period"))			FERScfg[brd]->TDCpulser_Period = GetTime(value, "ns");
	if (streq(str, "Ch_Offset"))				SetChannelParam32(handle, FERScfg[brd]->Ch_Offset, (uint16_t)GetTime(value, "ns"), ch);

	//FERS_LibMsg("Set parname: %s, value=%s\n", str, value);

	// -------------------------------------------------------------
	// Settings for Adapters (A5256)
	// -------------------------------------------------------------
	if (streq(str, "AdapterType")) {
		if (streq(value, "NONE"))					FERScfg[brd]->AdapterType = ADAPTER_NONE;
		else if (streq(value, "A5256_REV0_POS"))	FERScfg[brd]->AdapterType = ADAPTER_A5256_REV0_POS;
		else if (streq(value, "A5256_REV0_NEG"))	FERScfg[brd]->AdapterType = ADAPTER_A5256_REV0_NEG;
		else if (streq(value, "A5256"))				FERScfg[brd]->AdapterType = ADAPTER_A5256;
		else ValidParameterValue = 0;
	}

	if (streq(str, "A5256_Ch0Polarity")) { 
		if (streq(value, "POSITIVE"))			FERScfg[brd]->A5256_Ch0Polarity = A5256_CH0_POSITIVE;
		else if (streq(value, "NEGATIVE"))		FERScfg[brd]->A5256_Ch0Polarity = A5256_CH0_NEGATIVE;
		else ValidParameterValue = 0;
	}

	if (streq(str, "DiscrThreshold"))			SetChannelParamFloat(handle, FERScfg[brd]->DiscrThreshold, GetFloat(value), ch);
	if (streq(str, "DiscrThreshold2"))			SetChannelParamFloat(handle, FERScfg[brd]->DiscrThreshold2, GetFloat(value), ch);
	if (streq(str, "DisableThresholdCalib"))	FERScfg[brd]->DisableThresholdCalib = GetInt(value);


	// -------------------------------------------------------------
	// Generic Lib Settings  
	// -------------------------------------------------------------
	//if (streq(str, "EnableDumpLibCfg"))			DebugLogs |= (DBLOG_PARAMS * GetInt(value));  // keep EnableDumpLibCfg for backward compatibility. Better to use DebugLogMask 
	if (streq(str, "DebugLogMask"))				DebugLogs = GetHex32(value);



	// -------------------------------------------------------------
	// Error checking
	// -------------------------------------------------------------
	if (!ValidParameterName) {
		_setLastLocalError("WARNING: %s: unkown parameter\n", str);
		unlock(FERScfg[brd]->bmutex);
		free(str_to_split);
		free(str);
		free(value);
		return FERSLIB_ERR_INVALID_PARAM;
	}
	if (!ValidParameterValue) {
		_setLastLocalError("WARNING: %s: invalid setting %s\n", str, value);
		unlock(FERScfg[brd]->bmutex);
		free(str_to_split);
		free(str);
		free(value);
		return FERSLIB_ERR_INVALID_PARAM_VALUE;
	}
	if (!ValidUnits) {
		_setLastLocalError("WARNING: %s: unkown units. Defualt units: V,mA,ns\n", str);
		unlock(FERScfg[brd]->bmutex);
		free(str_to_split);
		free(str);
		free(value);
		return FERSLIB_ERR_INVALID_PARAM_UNIT;  // Send something out conserning the Wrong configuration
	}

	unlock(FERScfg[brd]->bmutex);
	free(str_to_split);
	free(str);
	free(value);
	return 0;
}


// ---------------------------------------------------------------------------------
// Description: Get the value of a parameter. The function reads the value from the relevant parameter in the 
//				FERScfg struct, but it doesn't actually read it from the board 
// Inputs:		handle: board handle
//				param_name: parameter name
//				value: parameter value
// Outputs:		-
// Return:		0=OK, -1=error
// ---------------------------------------------------------------------------------
int FERS_GetParam(int handle, char *param_name, char *value) {
	int8_t brd = -1, ch = -1, node = -1;
	char* str_to_split = j_strdup(param_name);
	const char* delim = "[]";
	char* str = strtok(str_to_split, delim); // Param Name
	char* token = strtok(NULL, delim);	// channel Idx		

	brd = FERS_INDEX(handle);
	if (brd < 0 || brd > FERSLIB_MAX_NBRD) {
		_setLastLocalError("Get Param: brd index %d out of range", brd);
		free(str_to_split);
		return FERSLIB_ERR_INVALID_HANDLE;
	}
	if (!BoardConnected[brd]) {
		_setLastLocalError("Get Param: brd %d not connected", brd);
		free(str_to_split);
		return FERSLIB_ERR_OPER_NOT_ALLOWED;
	}

	lock(FERScfg[brd]->bmutex);

	if (token != NULL) {
		sscanf(token, "%" SCNd8, &ch);
		token = strtok(NULL, delim);
		if (token != NULL)
			sscanf(token, "%" SCNd8, &node);
	}

	strcpy(value, ""); // if value remains empty, the param name is not valid

	if (streq(str, "OF_RawData"))				sprintf(value, "%" PRIu8, FERScfg[brd]->OF_RawData);
	if (streq(str, "OF_LimitedSize"))			sprintf(value, "%" PRIu8, FERScfg[brd]->OF_LimitedSize);
	if (streq(str, "MaxSizeDataOutputFile"))	sprintf(value, "%f", FERScfg[brd]->MaxSizeDataOutputFile);
	if (streq(str, "OF_RawDataPath"))			sprintf(value, "%s", FERScfg[brd]->OF_RawDataPath);
	if (streq(str, "ChEnableMask"))				sprintf(value, "%" PRIu64, FERScfg[brd]->ChEnableMask);
	if (streq(str, "ChEnableMask0"))			sprintf(value, "%" PRIu64, FERScfg[brd]->ChEnableMask&0x00000000FFFFFFFF);
	if (streq(str, "ChEnableMask1"))			sprintf(value, "%" PRIu64, (FERScfg[brd]->ChEnableMask & 0xFFFFFFFF00000000) >> 32);
	if (streq(str, "ChEnableMask_e"))			sprintf(value, "%" PRIu64, FERScfg[brd]->ChEnableMask_e);
	if (streq(str, "ChEnableMask2"))			sprintf(value, "%" PRIu64, FERScfg[brd]->ChEnableMask_e & 0x00000000FFFFFFFF);
	if (streq(str, "ChEnableMask3"))			sprintf(value, "%" PRIu64, (FERScfg[brd]->ChEnableMask_e & 0xFFFFFFFF00000000) >> 32);
	if (streq(str, "Tlogic_Mask"))				sprintf(value, "%" PRIu64, FERScfg[brd]->Tlogic_Mask);
	if (streq(str, "QD_Mask"))					sprintf(value, "%" PRIu64, FERScfg[brd]->QD_Mask);
	if (streq(str, "TD1_Mask"))					sprintf(value, "%" PRIu64, FERScfg[brd]->TD1_Mask);
	if (streq(str, "TD2_Mask"))					sprintf(value, "%" PRIu64, FERScfg[brd]->TD2_Mask);
	if (streq(str, "DebugLogMask"))				sprintf(value, "%x", DebugLogs);
	if (streq(str, "TriggerMask")	|| streq(str, "TriggerSource"))		sprintf(value, "%x", FERScfg[brd]->TriggerMask);
	if (streq(str, "T0_outMask")	|| streq(str, "T0_Out"))			sprintf(value, "%x", FERScfg[brd]->T0_outMask);
	if (streq(str, "T1_outMask")	|| streq(str, "T1_Out"))			sprintf(value, "%x", FERScfg[brd]->T1_outMask);
	if (streq(str, "Tref_Mask")		|| streq(str, "TrefSource"))		sprintf(value, "%x", FERScfg[brd]->Tref_Mask);
	if (streq(str, "Veto_Mask")		|| streq(str, "VetoSource"))		sprintf(value, "%x", FERScfg[brd]->Veto_Mask);
	if (streq(str, "Validation_Mask"))			sprintf(value, "%x", FERScfg[brd]->Validation_Mask);

	if (streq(str, "FiberDelayAdjust"))			sprintf(value, "%f", TDL_FiberDelayAdjust[brd][ch][node]);  // CTIN: are link and node correct? Maybe better to make a string with all values (8x16)

	if (streq(str, "StartRunMode"))				sprintf(value, "%d", (int)FERScfg[brd]->StartRunMode);
	if (streq(str, "StopRunMode"))				sprintf(value, "%d", (int)FERScfg[brd]->StopRunMode);
	if (streq(str, "AcquisitionMode"))			sprintf(value, "%d", (int)FERScfg[brd]->AcquisitionMode);
	if (streq(str, "TdlClkPhase"))				sprintf(value, "%d", (int)FERScfg[brd]->TdlClkPhase);
	if (streq(str, "CncBufferSize"))			sprintf(value, "%d", (int)FERScfg[brd]->CncBufferSize);
	if (streq(str, "CncProbe_A"))				sprintf(value, "%d", (int)FERScfg[brd]->CncProbe_A);
	if (streq(str, "CncProbe_B"))				sprintf(value, "%d", (int)FERScfg[brd]->CncProbe_B);
	if (streq(str, "MaxPck_Block"))				sprintf(value, "%d", (int)FERScfg[brd]->MaxPck_Block);
	if (streq(str, "TriggerLogic"))				sprintf(value, "%d", (int)FERScfg[brd]->TriggerLogic);
	if (streq(str, "PtrgPeriod"))				sprintf(value, "%f", (float)FERScfg[brd]->PtrgPeriod);
	if (streq(str, "DigitalProbe0"))			sprintf(value, "%d", (int)FERScfg[brd]->DigitalProbe[0]);
	if (streq(str, "DigitalProbe1"))			sprintf(value, "%d", (int)FERScfg[brd]->DigitalProbe[1]);
	if (streq(str, "DigitalProbe"))				sprintf(value, "%d", (int)FERScfg[brd]->DigitalProbe[ch]);
	if (streq(str, "AnalogProbe0"))				sprintf(value, "%d", (int)FERScfg[brd]->AnalogProbe[0]);
	if (streq(str, "AnalogProbe1"))				sprintf(value, "%d", (int)FERScfg[brd]->AnalogProbe[1]);
	if (streq(str, "AnalogProbe"))				sprintf(value, "%d", (int)FERScfg[brd]->AnalogProbe[ch]);
	if (streq(str, "ProbeChannel0"))			sprintf(value, "%d", (int)FERScfg[brd]->ProbeChannel[0]);
	if (streq(str, "ProbeChannel1"))			sprintf(value, "%d", (int)FERScfg[brd]->ProbeChannel[1]);
	if (streq(str, "ProbeChannel"))				sprintf(value, "%d", (int)FERScfg[brd]->ProbeChannel[ch]);
	if (streq(str, "TrgIdMode"))				sprintf(value, "%d", (int)FERScfg[brd]->TrgIdMode);
	if (streq(str, "TrgHoldOff"))				sprintf(value, "%d", (int)FERScfg[brd]->TrgHoldOff);
	if (streq(str, "CitirocCfgMode"))			sprintf(value, "%d", (int)FERScfg[brd]->CitirocCfgMode);
	if (streq(str, "Enable_2nd_tstamp"))		sprintf(value, "%d", (int)FERScfg[brd]->Enable_2nd_tstamp);
	if (streq(str, "Pedestal"))					sprintf(value, "%d", (int)FERScfg[brd]->Pedestal);
	if (streq(str, "EnableToT"))				sprintf(value, "%d", (int)FERScfg[brd]->EnableToT);
	if (streq(str, "EnableServiceEvents"))		sprintf(value, "%d", (int)FERScfg[brd]->EnableServiceEvents);
	if (streq(str, "EnableCntZeroSuppr"))		sprintf(value, "%d", (int)FERScfg[brd]->EnableCntZeroSuppr);
	if (streq(str, "Validation_Mode"))			sprintf(value, "%d", (int)FERScfg[brd]->Validation_Mode);
	if (streq(str, "Counting_Mode"))			sprintf(value, "%d", (int)FERScfg[brd]->Counting_Mode);
	if (streq(str, "TestPulseSource"))			sprintf(value, "%d", (int)FERScfg[brd]->TestPulseSource);
	if (streq(str, "TestPulseDestination"))		sprintf(value, "%d", (int)FERScfg[brd]->TestPulseDestination);
	if (streq(str, "TestPulsePreamp"))			sprintf(value, "%d", (int)FERScfg[brd]->TestPulsePreamp);
	if (streq(str, "TestPulseAmplitude"))		sprintf(value, "%d", (int)FERScfg[brd]->TestPulseAmplitude);
	if (streq(str, "WaveformLength"))			sprintf(value, "%d", (int)FERScfg[brd]->WaveformLength);
	if (streq(str, "WaveformSource"))			sprintf(value, "%d", (int)FERScfg[brd]->WaveformSource);
	if (streq(str, "Range_14bit"))				sprintf(value, "%d", (int)FERScfg[brd]->Range_14bit);
	if (streq(str, "QD_CoarseThreshold"))		sprintf(value, "%d", (int)FERScfg[brd]->QD_CoarseThreshold);
	if (streq(str, "TD_CoarseThreshold"))		sprintf(value, "%d", (int)FERScfg[brd]->TD_CoarseThreshold);
	if (streq(str, "TD1_CoarseThreshold"))		sprintf(value, "%d", (int)FERScfg[brd]->TD1_CoarseThreshold);
	if (streq(str, "TD2_CoarseThreshold"))		sprintf(value, "%d", (int)FERScfg[brd]->TD2_CoarseThreshold);
	if (streq(str, "HG_ShapingTime"))			sprintf(value, "%d", (int)FERScfg[brd]->HG_ShapingTime);
	if (streq(str, "LG_ShapingTime"))			sprintf(value, "%d", (int)FERScfg[brd]->LG_ShapingTime);
	if (streq(str, "Enable_HV_Adjust"))			sprintf(value, "%d", (int)FERScfg[brd]->Enable_HV_Adjust);
	if (streq(str, "MajorityLevel"))			sprintf(value, "%d", (int)FERScfg[brd]->MajorityLevel);
	if (streq(str, "HV_Adjust_Range"))			sprintf(value, "%d", (int)FERScfg[brd]->HV_Adjust_Range);
	if (streq(str, "MuxClkPeriod")) 			sprintf(value, "%d", (int)FERScfg[brd]->MuxClkPeriod);
	if (streq(str, "MuxNSmean")) 				sprintf(value, "%d", (int)FERScfg[brd]->MuxNSmean);
	if (streq(str, "HoldDelay")) 				sprintf(value, "%d", (int)FERScfg[brd]->HoldDelay);
	if (streq(str, "GainSelect")) 				sprintf(value, "%d", (int)FERScfg[brd]->GainSelect);
	if (streq(str, "PeakDetectorMode")) 		sprintf(value, "%d", (int)FERScfg[brd]->PeakDetectorMode);
	if (streq(str, "EnableQdiscrLatch")) 		sprintf(value, "%d", (int)FERScfg[brd]->EnableQdiscrLatch);
	if (streq(str, "EnableChannelTrgout")) 		sprintf(value, "%d", (int)FERScfg[brd]->EnableChannelTrgout);
	if (streq(str, "FastShaperInput")) 			sprintf(value, "%d", (int)FERScfg[brd]->FastShaperInput);
	if (streq(str, "EnableTempFeedback")) 		sprintf(value, "%d", (int)FERScfg[brd]->EnableTempFeedback);
	if (streq(str, "ZS_Threshold_LG")) 			sprintf(value, "%d", (int)FERScfg[brd]->ZS_Threshold_LG[ch]);
	if (streq(str, "ZS_Threshold_HG")) 			sprintf(value, "%d", (int)FERScfg[brd]->ZS_Threshold_HG[ch]);
	if (streq(str, "QD_FineThreshold")) 		sprintf(value, "%d", (int)FERScfg[brd]->QD_FineThreshold[ch]);
	if (streq(str, "TD_FineThreshold")) 		sprintf(value, "%d", (int)FERScfg[brd]->TD_FineThreshold[ch]);
	if (streq(str, "HG_Gain")) 					sprintf(value, "%d", (int)FERScfg[brd]->HG_Gain[ch]);
	if (streq(str, "LG_Gain")) 					sprintf(value, "%d", (int)FERScfg[brd]->LG_Gain[ch]);
	if (streq(str, "HV_IndivAdj")) 				sprintf(value, "%d", (int)FERScfg[brd]->HV_IndivAdj[ch]);
	if (streq(str, "TD1_FineThreshold")) 		sprintf(value, "%d", (int)FERScfg[brd]->TD1_FineThreshold[ch]);
	if (streq(str, "TD2_FineThreshold")) 		sprintf(value, "%d", (int)FERScfg[brd]->TD2_FineThreshold[ch]);
	if (streq(str, "HG_ShapingTime_ind"))		sprintf(value, "%d", (int)FERScfg[brd]->HG_ShapingTime_ind[ch]);
	if (streq(str, "LG_ShapingTime_ind"))		sprintf(value, "%d", (int)FERScfg[brd]->LG_ShapingTime_ind[ch]);
	if (streq(str, "T_Gain"))					sprintf(value, "%d", (int)FERScfg[brd]->T_Gain[ch]);
	if (streq(str, "MeasMode")) 				sprintf(value, "%d", (int)FERScfg[brd]->MeasMode);
	if (streq(str, "HighResClock")) 			sprintf(value, "%d", (int)FERScfg[brd]->HighResClock);
	if (streq(str, "MaxPck_Train")) 			sprintf(value, "%d", (int)FERScfg[brd]->MaxPck_Train);
	if (streq(str, "GlitchFilterDelay")) 		sprintf(value, "%d", (int)FERScfg[brd]->GlitchFilterDelay);
	if (streq(str, "GlitchFilterMode")) 		sprintf(value, "%d", (int)FERScfg[brd]->GlitchFilterMode);
	if (streq(str, "TDC_ChBufferSize")) 		sprintf(value, "%d", (int)FERScfg[brd]->TDC_ChBufferSize);
	if (streq(str, "TriggerBufferSize")) 		sprintf(value, "%d", (int)FERScfg[brd]->TriggerBufferSize);
	if (streq(str, "HeaderField0")) 			sprintf(value, "%d", (int)FERScfg[brd]->HeaderField0);
	if (streq(str, "HeaderField1")) 			sprintf(value, "%d", (int)FERScfg[brd]->HeaderField1);
	if (streq(str, "TestMode")) 				sprintf(value, "%d", (int)FERScfg[brd]->TestMode);
	if (streq(str, "En_Head_Trail")) 			sprintf(value, "%d", (int)FERScfg[brd]->En_Head_Trail);
	if (streq(str, "En_Empty_Ev_Suppr")) 		sprintf(value, "%d", (int)FERScfg[brd]->En_Empty_Ev_Suppr);
	if (streq(str, "Ch_Offset")) 				sprintf(value, "%d", (int)FERScfg[brd]->Ch_Offset[ch]);
	if (streq(str, "LeadTrail_LSB")) 			sprintf(value, "%d", (int)FERScfg[brd]->LeadTrail_LSB);
	if (streq(str, "ToT_LSB")) 					sprintf(value, "%d", (int)FERScfg[brd]->ToT_LSB);
	if (streq(str, "ToT_reject_low_thr")) 		sprintf(value, "%d", (int)FERScfg[brd]->ToT_reject_low_thr);
	if (streq(str, "ToT_reject_high_thr")) 		sprintf(value, "%d", (int)FERScfg[brd]->ToT_reject_high_thr);
	if (streq(str, "AdapterType")) 				sprintf(value, "%d", (int)FERScfg[brd]->AdapterType);
	if (streq(str, "DiscrThreshold")) 			sprintf(value, "%d", (int)FERScfg[brd]->DiscrThreshold[ch]);
	if (streq(str, "DiscrThreshold2")) 			sprintf(value, "%d", (int)FERScfg[brd]->DiscrThreshold2[ch]);
	if (streq(str, "DisableThresholdCalib")) 	sprintf(value, "%d", (int)FERScfg[brd]->DisableThresholdCalib);
	if (streq(str, "A5256_Ch0Polarity")) 		sprintf(value, "%d", (int)FERScfg[brd]->A5256_Ch0Polarity);

	if (streq(str, "ChTrg_Width"))				sprintf(value, "%f", FERScfg[brd]->ChTrg_Width);
	if (streq(str, "Tlogic_Width"))				sprintf(value, "%f", FERScfg[brd]->Tlogic_Width);
	if (streq(str, "TrefWindow"))				sprintf(value, "%f", FERScfg[brd]->TrefWindow);
	if (streq(str, "TrefDelay"))				sprintf(value, "%f", FERScfg[brd]->TrefDelay);
	if (streq(str, "GateWidth")) 				sprintf(value, "%f", FERScfg[brd]->GateWidth);
	if (streq(str, "TrgWindowWidth")) 			sprintf(value, "%f", FERScfg[brd]->TrgWindowWidth);
	if (streq(str, "TrgWindowOffset")) 			sprintf(value, "%f", FERScfg[brd]->TrgWindowOffset);
	if (streq(str, "TDCpulser_Width")) 			sprintf(value, "%f", FERScfg[brd]->TDCpulser_Width);
	if (streq(str, "TDCpulser_Period")) 		sprintf(value, "%f", FERScfg[brd]->TDCpulser_Period);
	if (streq(str, "HV_Vbias")) 				sprintf(value, "%f", FERScfg[brd]->HV_Vbias);
	if (streq(str, "HV_Imax")) 					sprintf(value, "%f", FERScfg[brd]->HV_Imax);
	if (streq(str, "TempSensCoeff")) 			sprintf(value, "%f %f %f", FERScfg[brd]->TempSensCoeff[0], FERScfg[brd]->TempSensCoeff[1], FERScfg[brd]->TempSensCoeff[2]);
	if (streq(str, "TempFeedbackCoeff")) 		sprintf(value, "%f", FERScfg[brd]->TempFeedbackCoeff);

	//FERS_LibMsg("Get param: %s, value=%s\n", str, value);

	unlock(FERScfg[brd]->bmutex);

	free(str_to_split);
	// Report incorrect parmeter name
	if (strlen(value) == 0) {
		_setLastLocalError("Unkown parameter name %s for FERS %d\n", str, FERS_BoardInfo[brd]->FERSCode);
		return FERSLIB_ERR_INVALID_PARAM;
	}
	return 0;

}


// #################################################################################
// Parsing of Configuration files (read/write parameters from/to text files)
// The file parser is currently not used in the library. The application (e.g. Janus) must parse
// the config file and call the FERS_SetParam function in the library.
// #################################################################################

// ---------------------------------------------------------------------------------
// Description: Parse FERScfg with parameter of a new configuration file
// ---------------------------------------------------------------------------------
static void fLoadExtCfgFile(FILE* f_ini) {	// DNIN: The first initialization should not be done, it must be inside and if block [NEED TO BE CHECKED]
	char nfile[500];
	int mf = 0;
	mf = fscanf(f_ini, "%s", nfile);
	if (mf == 0) ValidParameterValue = 0;	// DNIN: filename missing
	else {
		FILE* n_cfg;
		n_cfg = fopen(nfile, "r");
		if (n_cfg != NULL) {
			//Con_printf("LCSm", "Overwriting parameters from %s\n", nfile);
			FERS_parseConfigFile(n_cfg);
			fclose(n_cfg);
			ValidParameterValue = 1;
			ValidParameterName = 1;
		} else {
			_setLastLocalError("WARNING: Loading Macro: Macro file\"%s\" not found\n", nfile);
			ValidParameterValue = 0;
		}
	}
}



// ---------------------------------------------------------------------------------
// Description: Read a config file, parse the parameters and set the relevant fields in the FERScfg structure
// Inputs:		f_ini: config file pinter
// Outputs:		FERScfg: struct with all parameters
// Return:		0=OK, -1=error
// ---------------------------------------------------------------------------------
static int FERS_parseConfigFile(FILE* f_ini)
{
	int8_t ch = -1, brd = -1;
	int ret = 0;
	char tstr[1000], *parval, *tparval, str1[1000];


	// read config file and assign parameters 
	while(!feof(f_ini)) {
		brd = -1;
		ch = -1;

		// Read a line from the file
		char* gret = fgets(tstr, sizeof(tstr), f_ini);

		// skip comments
		if (tstr[0] == '#' || strlen(tstr) <= 2) {
			//fprintf(parlog, "\n");
			continue;
		}

		// Strip comment from each line
		if (strstr(tstr, "#") != NULL)
			tparval = strtok(tstr, "#");
		else
			tparval = tstr;

		// Get param name (str) and values (parval)
		sscanf(tparval, "%s", str1);
		tparval += strlen(str1);
		parval = trim(tparval);

		ValidParameterName = 1;
		ValidParameterValue = 1;
		ValidUnits = 1;

		// Search for boards and channels
		char *str = strtok(trim(str1), "[]"); // Param name with [brd][ch]
		char* token = strtok(NULL, "[]");
		if (token != NULL) {
			sscanf(token, "%" SCNd8, &brd);
			if ((token = strtok(NULL, "[]")) != NULL)
				sscanf(token, "%" SCNd8, &ch);
		}

		int tb, mc;	// Assign value for each board with a single for loop 
		if (brd != -1) {
			tb = brd;
			mc = brd+1;
		} else {
			tb = 0;
			mc = NumBoardConnected;
		}

		// Append the '[ch]' if ch != 0
		if (ch >= 0) {
			char sch[10];
			sprintf(sch, "[%" SCNd8 "]", ch);
			strcat(str, sch);
		}

		for (int j = tb; j < mc; ++j) {
			if (BoardConnected[j]) {
				ret = FERS_SetParam(j, str, parval);
				if (ret != 0) {
					char des[1024];
					FERS_GetLastError(des);
					if (ENABLE_FERSLIB_LOGMSG) FERS_LibMsg("[WARNING][PARSER] %s", des);
				}
			}
		}
			
		if (streq(str, "Load"))				
			fLoadExtCfgFile(f_ini);

		// Performed in SetParam function
		//if (!ValidParameterName || !ValidParameterValue || !ValidUnits) {
		//	if (!ValidUnits && ValidParameterValue)
		//		_setLastLocalError("WARNING: %s: unknown units %s. Using default unit (V, mA, ns)\n", str, parval);
		//	else
		//		_setLastLocalError("WARNING: %s: unknown parameter\n", str);
		//}
	}

	if (DebugLogs & DBLOG_PARAMS) {
		for (int nb = 0; nb < FERS_GetNumBrdConnected(); ++nb)
			FERS_DumpCfgSaved(nb);
	}
	return 0;
}

int FERS_LoadConfigFile(char* filepath) {
	FILE* cfg;
	cfg = fopen(filepath, "r");
	if (cfg == NULL) {
		return FERSLIB_ERR_INVALID_PATH;
	} else {
		// Set default Lib parameters
		if (plog) {
			for (int bc = 0; bc < NumBoardConnected; ++bc) {
				char fname[100];
				sprintf(fname, "ferslib_cfg_brd%02d.txt", bc);
				fcfg[bc] = fopen(fname, "w");
			}
		}

		int ret;
		ret = FERS_parseConfigFile(cfg);
		fclose(cfg);
		return ret;
	}
}


int FERS_DumpCfgSaved5202(int handle) {
	char parname[100][2][50] = {
		{"StartRunMode",			"0"},
		{"StopRunMode",				"0"},
		{"ChEnableMask0",			"0"},
		{"ChEnableMask1",			"0"},
		{"AcquisitionMode",			"0"},
		{"EnableServiceEvents",		"0"},
		{"EnableCntZeroSuppr",		"0"},
		{"SupprZeroCntListFile",	"0"},
		{"TriggerMask",				"0"},
		{"TriggerLogic",			"0"},
		{"Tlogic_Width",			"0"},
		{"Tref_Mask",				"0"},
		{"Validation_Mask",			"0"},
		{"Validation_Mode",			"0"},
		{"Counting_Mode",			"0"},
		{"TrefWindowW",				"0"},
		{"TrefDelay",				"0"},
		{"PtrgPeriod",				"0"},
		{"DigitalProbe0",			"0"},
		{"DigitalProbe1",			"0"},
		{"T0_outMask",				"0"},
		{"T1_outMask",				"0"},
		{"ChTrg_Width",				"0"},
		{"MajorityLevel",			"0"},
		{"TestPulseSource",			"0"},
		{"TestPulseDestination",	"0"},
		{"TestPulsePreamp",			"0"},
		{"TestPulseAmplitude",		"0"},
		{"WaveformLength",			"0"},
		{"WaveformSource",			"0"},
		{"TrgIdMode",				"0"},
		{"Range_14bit",				"0"},
		{"Enable_2nd_tstamp",		"0"},
		{"Tlogic_Mask0",			"0"},
		{"Tlogic_Mask1",			"0"},
		{"TD_CoarseThreshold",		"0"},
		{"HG_ShapingTime",			"0"},
		{"LG_ShapingTime",			"0"},
		{"Enable_HV_Adjust",		"0"},
		{"HV_Adjust_Range",			"0"},
		{"MuxClkPeriod",			"0"},
		{"MuxNSmean",				"0"},
		{"HoldDelay",				"0"},
		{"GainSelect",				"0"},
		{"PeakDetectorMode",		"0"},
		{"EnableQdisctLatch",		"0"},
		{"EnableChannelTrgout",		"0"},
		{"FastShaperInput",			"0"},
		{"Trg_HoldOff",				"0"},
		{"HV_Vbias",				"0"},
		{"HV_Imax",					"0"},
		{"TempFeedbackCoeff",		"0"},
		{"EnableTempFeedback",		"0"},
		{"ZS_Threshold_LG",			"64"},
		{"ZS_Threshold_HG",			"64"},
		{"QD_FineThreshold",		"64"},
		{"TD_FineThreshold",		"64"},
		{"HG_Gain",					"64"},
		{"LG_Gain",					"64"},
		{"HV_IndivAdj",				"64"}
	};

	FILE* fParName = NULL;
	char ff[100];
	int brdfile = FERS_INDEX(handle);
	sprintf(ff, "FERSLIB_ParCfg_brd%02d_PID%d.txt", brdfile, FERS_BoardInfo[brdfile]->pid);
	fParName = fopen(ff, "w");
	if (fParName == NULL) {
		return -1;
	}
	for (int pp = 0; pp < 50; ++pp) {
		if (strstr(parname[pp][1], "0") != NULL) {
			char parval[50];
			FERS_GetParam(handle, parname[pp][0], parval);
			fprintf(fParName, "%s\t%s\n", parname[pp][0], parval);
		} else {
			int larray = 0;
			sscanf(parname[pp][1], "%d", &larray);
			for (int j = 0; j < larray; ++j) {
				char chPar[128];
				char parval[50];
				sprintf(chPar, "%.50s[%d]", parname[pp][0], j);
				FERS_GetParam(handle, chPar, parval);
				fprintf(fParName, "%s\t%s\n", chPar, parval);
			}
		}
	}
	fclose(fParName);
	return 0;
}


// Dump on file the parameters configuration of FERSlib, here as an example for 5202
int FERS_DumpCfgSaved5203(int handle) {
	char parname[46][2][50] = {
		{"StartRunMode",			"0"},
		{"ChEnableMask0",			"0"},
		{"ChEnableMask1",			"0"},
		{"ChEnableMask2",			"0"},
		{"ChEnableMask3",			"0"},
		{"AcquisitionMode",			"0"},
		{"MeasMode",				"0"},
		{"FiberDelayAdjust",		"0"},
		{"TdlClkPhase",				"0"},
		{"MaxPck_Block",			"0"},
		{"MaxPck_Train",			"0"},
		{"HeaderField0",			"0"},
		{"HeaderField1",			"0"},
		{"En_service_event",		"0"},
		{"En_Head_Trail",			"0"},
		{"En_Empty_Ev_Suppr",		"0"},
		{"TriggerMask",				"0"},
		{"Tref_Mask",				"0"},
		{"Veto_Mask",				"0"},
		{"GateWidth",				"0"},
		{"TrgWindowWidth",			"0"},
		{"TrgWindowOffset",			"0"},
		{"PtrgPeriod",				"0"},
		{"DigitalProbe0",			"0"},
		{"DigitalProbe1",			"0"},
		{"T0_outMask",				"0"},
		{"T1_outMask",				"0"},
		{"GlitchFilterMode",		"0"},
		{"GlitchFilterDelay",		"0"},
		{"ToT_reject_low_thr",		"0"},
		{"ToT_reject_high_thr",		"0"},
		{"TDC_ChannelBufferSize",	"0"},
		{"TriggerBufferSize",		"0"},
		{"TDCpulser_Width",			"0"},
		{"TDCpulser_Period",		"0"},
		{"HighResClock",			"0"},
		{"LeadTrail_LSB",			"0"},
		{"ToT_LSB",					"0"},
		{"AdapterType",				"0"},
		{"DiscrThreshold",			"128"},
		{"DiscrThreshold2",			"128"},
		{"DisableThresholdCalib",	"128"},
		{"A5256_Ch0Polarity",		"0"}
	};

	FILE* fParName = NULL;
	char ff[100];
	int brdfile = FERS_INDEX(handle);
	sprintf(ff, "FERSLIB_ParCfg_brd%02d_PID%d.txt", brdfile, FERS_BoardInfo[brdfile]->pid);
	fParName = fopen(ff, "w");
	if (fParName == NULL) {
		return -1;
	}
	for (int pp = 0; pp < 46; ++pp) {
		if (strstr(parname[pp][1], "0") != NULL) {
			char parval[50];
			FERS_GetParam(handle, parname[pp][0], parval);
			fprintf(fParName, "%s\t%s\n", parname[pp][0], parval);
		} else {
			int larray = 0;
			sscanf(parname[pp][1], "%d", &larray);
			for (int j = 0; j < larray; ++j) {
				char chPar[50];
				char parval[128];
				sprintf(chPar, "%.50s[%d]", parname[pp][0], j);
				FERS_GetParam(handle, chPar, parval);
				fprintf(fParName, "%s\t%s\n", chPar, parval);
			}
		}
	}
	fclose(fParName);
	return 0;
}


int FERS_DumpCfgSaved(int handle) {
	if (DebugLogs & DBLOG_PARAMS) {
		if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5202)
			FERS_DumpCfgSaved5202(handle);
		if (FERS_BoardInfo[FERS_INDEX(handle)]->FERSCode == 5203)
			FERS_DumpCfgSaved5203(handle);
	}
	return 0;
}


