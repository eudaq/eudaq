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

#ifndef _WDCONFIG__H
#define _WDCONFIG__H

#include "WaveDump.h"
#include "flash.h"

/* ###########################################################################
*  Functions
*  ########################################################################### */

/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) 
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   f_ini        Pointer to the config file
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg);

/*! \fn      int WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask)
*   \brief   writes 'data' on register at 'address' using 'mask' as bitmask
*
*   \param   handle :   Digitizer handle
*   \param   address:   Address of the Register to write
*   \param   data   :   Data to Write on the Register
*   \param   mask   :   Bitmask to use for data masking
*   \return  0 = Success; negative numbers are error codes
*/
int WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask);

/*! \fn      int DoProgramDigitizer(int handle, WaveDumpConfig_t WDcfg)
*   \brief   configure the digitizer according to the parameters read from
*            the cofiguration file and saved in the WDcfg data structure
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int DoProgramDigitizer(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

/*! \fn      int ProgramDigitizerWithRelativeThreshold(int handle, WaveDumpConfig_t WDcfg)
*   \brief   configure the digitizer according to the parameters read from the cofiguration
*            file and saved in the WDcfg data structure, performing a calibration of the
*            DCOffset to set the required BASELINE_LEVEL.
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ProgramDigitizerWithRelativeThreshold(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

/*! \fn      int ProgramDigitizerWithRelativeThreshold(int handle, WaveDumpConfig_t WDcfg)
*   \brief   configure the digitizer according to the parameters read from the cofiguration
*            file and saved in the WDcfg data structure, performing a calibration of the
*            DCOffset to set the required BASELINE_LEVEL (if provided).
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

/*! \fn      void CheckKeyboardCommands(WaveDumpRun_t *WDrun) 
*   \brief   check if there is a key pressed and execute the relevant command
*            
*   \param   WDrun:   Pointer to the WaveDumpRun data structure
*/
void CheckKeyboardCommands(int handle, WaveDumpRun_t *WDrun, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

/*! \fn      void Load_DAC_Calibration_From_Flash(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo)
*   \brief   look for DAC calibration in flash and load it
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   Pointer to WaveDumpConfig data structure
*	\param   BoardInfo 
*/
void Load_DAC_Calibration_From_Flash(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

/*! \fn      void Save_DAC_Calibration_To_Flash(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo)
*   \brief   save DAC calibration to flash 
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*	\param   BoardInfo
*/
void Save_DAC_Calibration_To_Flash(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

#endif // _WDCONFIG__H
