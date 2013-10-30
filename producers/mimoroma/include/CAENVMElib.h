/*!
	-----------------------------------------------------------------------------

	               --- CAEN SpA - Computing Systems Division --- 

	-----------------------------------------------------------------------------

	CAENVMElib.h

	-----------------------------------------------------------------------------

	Created: March 2004

  	-----------------------------------------------------------------------------
*/
#ifndef __V1718LIB_H
#define __V1718LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "CAENVMEoslib.h"
#include "CAENVMEtypes.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
	CAENVME_API CAENVME_SWRelease
	-----------------------------------------------------------------------------
	Parameters:
		[out] SwRel		: Returns the software release of the library.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		Permits to read the software release of the library.
*/	
CAENVME_API 
CAENVME_SWRelease(char *SwRel);

/*
	CAENVME_BoardFWRelease.
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] FWRel		: Returns the firmware release of the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		Permits to read the firmware release loaded into the device.
*/	
CAENVME_API 
CAENVME_BoardFWRelease(long Handle, char *FWRel);

/*
	CAENVME_Init
	-----------------------------------------------------------------------------
	Parameters:
		[in]  BdType	: The model of the bridge (V1718/V2718).
		[in]  Link		: The link number (don't care for V1718).
		[in]  BdNum		: The board number in the link.
		[out] Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function generates an opaque handle to identify a module 
		attached to the PC. In the case of V1718 bridge it must be 
		specified only the module index (BdNum) because the link is USB.
		In the case of V2718 it must be specified also the link because
		you can have some A2818 optical link inside the PC.
*/	
CAENVME_API 
CAENVME_Init(CVBoardTypes BdType, short Link, short BdNum, long *Handle);

/*
	CAENVME_End
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		Notifies the library the end of work and free the allocated 
		resources.
*/	
CAENVME_API 
CAENVME_End(long Handle);

/*
	CAENVME_ReadCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[out] Data		: The data read from the VME bus.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[in]  DW		: The data width.(see CVDataWidth enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a single VME read cycle.
*/	
CAENVME_API 
CAENVME_ReadCycle(long Handle, unsigned long Address, void *Data, 
				  CVAddressModifier AM, CVDataWidth DW);   

/*
	CAENVME_RMWCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]     Handle		: The handle that identifies the device.
		[in]     Address	: The VME bus address.
		[in/out] Data		: The data read and then written to the VME bus.
		[in]     AM			: The address modifier (see CVAddressModifier enum).
		[in]     DW			: The data width.(see CVDataWidth enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a Read-Modify-Write cycle. The Data parameter is 
		bidirectional: it is used to write the value to the VME bus and to 
		return the value read.
*/	
CAENVME_API 
CAENVME_RMWCycle(long Handle, unsigned long Address,  unsigned long *Data, 
			     CVAddressModifier AM, CVDataWidth DW);   

/*
	CAENVME_WriteCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[in]  Data		: The data written to the VME bus.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[in]  DW		: The data width.(see CVDataWidth enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a single VME write cycle.
*/	
CAENVME_API 
CAENVME_WriteCycle(long Handle, unsigned long Address, void *Data, 
			       CVAddressModifier AM, CVDataWidth DW);   

/*
	CAENVME_BLTReadCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[out] Buffer	: The data read from the VME bus.
		[in]  Size      : The size of the transfer in bytes.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[in]  DW		: The data width.(see CVDataWidth enum).
		[out] count		: The number of bytes transferred.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME block transfer read cycle. It can be used to
		perform MBLT transfers using 64 bit data width.
*/	
CAENVME_API 
CAENVME_BLTReadCycle(long Handle, unsigned long Address, unsigned char *Buffer, 
				     int Size, CVAddressModifier AM, CVDataWidth DW, int *count);

					 /*
	CAENVME_MBLTReadCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[out] Buffer	: The data read from the VME bus.
		[in]  Size      : The size of the transfer in bytes.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[out] count		: The number of bytes transferred.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME multiplexed block transfer read cycle.
*/	
CAENVME_API 
CAENVME_MBLTReadCycle(long Handle, unsigned long Address, unsigned char *Buffer, 
				      int Size, CVAddressModifier AM, int *count);

/*
	CAENVME_BLTWriteCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[in]  Buffer	: The data to be written to the VME bus.
		[in]  Size      : The size of the transfer in bytes.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[in]  DW		: The data width.(see CVDataWidth enum).
		[out] count		: The number of bytes tranferred.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME block transfer write cycle.
*/	
CAENVME_API 
CAENVME_BLTWriteCycle(long Handle, unsigned long Address, unsigned char *Buffer, 
				      int size, CVAddressModifier AM, CVDataWidth DW, int *count);

/*
	CAENVME_MBLTWriteCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[in]  Buffer	: The data to be written to the VME bus.
		[in]  Size      : The size of the transfer in bytes.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
		[out] count		: The number of bytes tranferred.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME multiplexed block transfer write cycle.
*/	
CAENVME_API 
CAENVME_MBLTWriteCycle(long Handle, unsigned long Address, unsigned char *Buffer, 
				       int size, CVAddressModifier AM, int *count);

/*
	CAENVME_ADOCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME address only cycle. It can be used to
		perform MBLT transfers using 64 bit data width.
*/	
CAENVME_API 
CAENVME_ADOCycle(long Handle, unsigned long Address, CVAddressModifier AM);

/*
	CAENVME_ADOHCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	: The VME bus address.
		[in]  AM		: The address modifier (see CVAddressModifier enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME address only with handshake cycle.
*/	
CAENVME_API 
CAENVME_ADOHCycle(long Handle, unsigned long Address, CVAddressModifier AM);

/*
	CAENVME_IACKCycle
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Level		: The IRQ level to aknowledge (see CVIRQLevels enum).
		[in]  DW		: The data width.(see CVDataWidth enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a VME interrupt acknowledge cycle.
*/	
CAENVME_API   
CAENVME_IACKCycle(long Handle, CVIRQLevels Level, void *Vector, CVDataWidth DW);

/*
	CAENVME_IRQCheck
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Mask		: A bit-mask indicating the active IRQ lines.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function returns a bit mask indicating the active IRQ lines.
*/	
CAENVME_API 
CAENVME_IRQCheck(long Handle, CAEN_BYTE *Mask);

/*
	CAENVME_SetPulserConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  PulSel	: The pulser to configure (see CVPulserSelect enum).
		[in]  Period	: The period of the pulse in time units.
		[in]  Width		: The width of the pulse in time units.
		[in]  Unit		: The time unit for the pulser configuration (see
						  CVTimeUnits enum).
		[in]  PulseNo	: The number of pulses to generate (0 = infite).
		[in]  Start		: The source signal to start the pulse burst (see 
						  CVIOSources enum).
		[in]  Reset		: The source signal to stop the pulse burst (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to configure the pulsers. All the timing parameters
		are expressed in the time units specified. The start signal source can be 
		one of: front panel button or software (cvManualSW), input signal 0 
		(cvInputSrc0),input signal 1 (cvInputSrc1) or input coincidence 
		(cvCoincidence). The reset signal source can be: front panel button or 
		software (cvManualSW) or, for pulser A the input signal 0 (cvInputSrc0), 
		for pulser B the input signal 1 (cvInputSrc1). 
*/	
CAENVME_API 
CAENVME_SetPulserConf(long Handle, CVPulserSelect PulSel, unsigned char Period, 
			          unsigned char Width, CVTimeUnits Unit, unsigned char PulseNo, 
			          CVIOSources Start, CVIOSources Reset);
 
/*
	CAENVME_SetScalerConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Limit		: The counter limit for the scaler.
		[in]  AutoReset	: Enable/disable the counter auto reset.
		[in]  Hit		: The source signal for the signal to count (see 
						  CVIOSources enum).
		[in]  Gate		: The source signal for the gate (see CVIOSources enum).
		[in]  Reset		: The source signal to stop the counter (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to configure the scaler. Limit range is 0 - 1024 
		(10 bit). The hit signal source can be: input signal 0 (cvInputSrc0)
		or input coincidence (cvCoincidence). The gate signal source can be: 
		front panel button or software (cvManualSW) or input signal 1 
		(cvInputSrc1). The reset signal source can be: front panel button or 
		software (cvManualSW) or input signal 1 (cvInputSrc1). 
*/	
CAENVME_API 
CAENVME_SetScalerConf(long Handle, short Limit, short AutoReset,
			          CVIOSources Hit, CVIOSources Gate, CVIOSources Reset);

/*
	CAENVME_SetOutputConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  OutSel	: The ouput line to configure (see CVOutputSelect enum).
		[in]  OutPol	: The output line polarity (see CVIOPolarity enum).
		[in]  LEDPol	: The output LED polarity (see CVLEDPolarity enum).
		[in]  Source	: The source signal to propagate to the output line (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to configure the output lines of the module. It can
		be specified the polarity for the line and for the LED. The output line
		source depends on the line as figured out by the following table:

                +----------------------------------------------------------+
                !           S O U R C E     S E L E C T I O N              !
                +--------------+--------------+---------------+------------+
                ! cvVMESignals ! cvCoicidence ! cvMiscSignals ! cvManualSW !
        +---+---+--------------+--------------+---------------+------------+
        !   ! 0 !      DS      ! Input Coinc. !   Pulser A    ! Manual/SW  !
        ! O +---+--------------+--------------+---------------+------------+
        ! U ! 1 !      AS      ! Input Coinc. !   Pulser A    ! Manual/SW  !
        ! T +---+--------------+--------------+---------------+------------+
        ! P ! 2 !    DTACK     ! Input Coinc. !   Pulser B    ! Manual/SW  !
        ! U +---+--------------+--------------+---------------+------------+
        ! T ! 3 !     BERR     ! Input Coinc. !   Pulser B    ! Manual/SW  !
        !   +---+--------------+--------------+---------------+------------+
        !   ! 4 !     LMON     ! Input Coinc. !  Scaler end   ! Manual/SW  !
        +---+---+--------------+--------------+---------------+------------+
*/	
CAENVME_API 
CAENVME_SetOutputConf(long Handle, CVOutputSelect OutSel, CVIOPolarity OutPol,
			          CVLEDPolarity LEDPol, CVIOSources Source);

/*
	CAENVME_SetInputConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  InSel		: The input line to configure (see CVInputSelect enum).
		[in]  InPol		: The input line polarity (see CVIOPolarity enum).
		[in]  LEDPol	: The output LED polarity (see CVLEDPolarity enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to configure the input lines of the module. It can
		be specified the polarity for the line and for the LED.
*/
CAENVME_API 
CAENVME_SetInputConf(long Handle, CVInputSelect InSel, CVIOPolarity InPol, 
			         CVLEDPolarity LEDPol);

/*
	CAENVME_GetPulserConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  PulSel	: The pulser to configure (see CVPulserSelect enum).
		[out] Period	: The period of the pulse in time units.
		[out] Width		: The width of the pulse in time units.
		[out] Unit		: The time unit for the pulser configuration (see
						  CVTimeUnits enum).
		[out] PulseNo	: The number of pulses to generate (0 = infite).
		[out] Start		: The source signal to start the pulse burst (see 
						  CVIOSources enum).
		[out] Reset		: The source signal to stop the pulse burst (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to read the configuration of the pulsers.
*/	
CAENVME_API 
CAENVME_GetPulserConf(long Handle, CVPulserSelect PulSel, unsigned char *Period, 
			          unsigned char *Width, CVTimeUnits *Unit, unsigned char *PulseNo, 
			          CVIOSources *Start, CVIOSources *Reset);
 
/*
	CAENVME_GetScalerConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Limit		: The counter limit for the scaler.
		[out] AutoReset	: The auto reset configuration.
		[out] Hit		: The source signal for the signal to count (see 
						  CVIOSources enum).
		[out] Gate		: The source signal for the gate (see CVIOSources enum).
		[out] Reset		: The source signal to stop the counter (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to read the configuration of the scaler.
*/	
CAENVME_API 
CAENVME_GetScalerConf(long Handle, short *Limit, short *AutoReset,
			          CVIOSources *Hit, CVIOSources *Gate, CVIOSources *Reset);


/*
	CAENVME_SetOutputConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  OutSel	: The ouput line to configure (see CVOutputSelect enum).
		[out] OutPol	: The output line polarity (see CVIOPolarity enum).
		[out] LEDPol	: The output LED polarity (see CVLEDPolarity enum).
		[out] Source	: The source signal to propagate to the output line (see 
						  CVIOSources enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to read the configuration of the output lines.
*/
CAENVME_API 
CAENVME_GetOutputConf(long Handle, CVOutputSelect OutSel, CVIOPolarity *OutPol,
			          CVLEDPolarity *LEDPol, CVIOSources *Source);

/*
	CAENVME_SetInputConf
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  InSel		: The input line to configure (see CVInputSelect enum).
		[out] InPol		: The input line polarity (see CVIOPolarity enum).
		[out] LEDPol	: The output LED polarity (see CVLEDPolarity enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to read the configuration of the input lines.
*/
CAENVME_API 
CAENVME_GetInputConf(long Handle, CVInputSelect InSel, CVIOPolarity *InPol, 
			         CVLEDPolarity *LEDPol);

/*
	CAENVME_ReadRegister
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Reg		: The internal register to read (see CVRegisters enum).
		[out] Data		: The data read from the module.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to read all internal registers.
*/
CAENVME_API  
CAENVME_ReadRegister(long Handle, CVRegisters Reg, unsigned short *Data);

/*
	CAENVME_WriteRegister
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Reg		: The internal register to read (see CVRegisters enum).
		[in]  Data		: The data to be written to the module.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function permits to write to all internal registers.
*/
CAENVME_API
CAENVME_WriteRegister(long Handle, CVRegisters Reg, unsigned short Data);
/*
	CAENVME_SetOutputRegister
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Mask		: The lines to be set.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the lines specified. Refer the CVOutputRegisterBits 
		enum to compose and decoding the bit mask.  
*/
CAENVME_API   
CAENVME_SetOutputRegister(long Handle, unsigned short Mask);

/*
	CAENVME_ClearOutputRegister
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Mask		: The lines to be cleared.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function clears the lines specified. Refer the CVOutputRegisterBits 
		enum to compose and decoding the bit mask.
*/
CAENVME_API   
CAENVME_ClearOutputRegister(long Handle, unsigned short Mask);

/*
	CAENVME_PulseOutputRegister
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Mask		: The lines to be pulsed.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function produces a pulse with the lines specified by setting and
		then clearing them. Refer the CVOutputRegisterBits enum to compose and 
		decoding the bit mask.
*/
CAENVME_API   
CAENVME_PulseOutputRegister(long Handle, unsigned short Mask);

/*
	CAENVME_ReadDisplay
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The values read from the module (see CVDisplay enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function reads the VME data display on the front panel of the module.
		Refer to the CVDisplay data type definition and comments to decode the 
		value returned.
*/
CAENVME_API   
CAENVME_ReadDisplay(long Handle, CVDisplay *Value);

/*
	CAENVME_SetArbiterType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: The type of VME bus arbitration to implement (see 
						  CVArbiterTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the behaviour of the VME bus arbiter on the module. 
*/
CAENVME_API  
CAENVME_SetArbiterType(long Handle, CVArbiterTypes Value);

/*
	CAENVME_SetRequesterType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: The type of VME bus requester to implement (see 
						  CVRequesterTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the behaviour of the VME bus requester on the module. 
*/
CAENVME_API   
CAENVME_SetRequesterType(long Handle, CVRequesterTypes Value);

/*
	CAENVME_SetReleaseType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: The type of VME bus release policy to implement (see 
						  CVReleaseTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the release policy of the VME bus on the module. 
*/
CAENVME_API   
CAENVME_SetReleaseType(long Handle, CVReleaseTypes Value);

/*
	CAENVME_SetBusReqLevel
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: The type of VME bus requester priority level to set 
						  (see CVBusReqLevels enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the specified VME bus requester priority level on the 
		module. 
*/
CAENVME_API   
CAENVME_SetBusReqLevel(long Handle, CVBusReqLevels Value);

/*
	CAENVME_SetTimeout
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: The value of VME bus timeout to set (see CVVMETimeouts
						  enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the specified VME bus timeout on the module. 
*/
CAENVME_API   
CAENVME_SetTimeout(long Handle, CVVMETimeouts Value);


/*
	CAENVME_SetLocationMonitor
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Address	:
		[in]  Write		:
		[in]  Lword		:
		[in]  Icak		:

	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the Location Monitor. 
*/
CAENVME_API 
CAENVME_SetLocationMonitor(long Handle, unsigned long Address, CVAddressModifier Am, 
						   short Write,short Lword, short Iack);
/*
	CAENVME_SetFIFOMode
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Value		: Enable/disable the FIFO mode.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function enables/disables the auto increment of the VME addresses
		during the block tranfer cycles. With the FIFO mode enabled the 
		addresses are not incremented.
*/
CAENVME_API   
CAENVME_SetFIFOMode(long Handle, short Value);

/*
	CAENVME_GetArbiterType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The type of VME bus arbitration implemented (see 
						  CVArbiterTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function get the type of VME bus arbiter implemented on the module. 
*/
CAENVME_API   
CAENVME_GetArbiterType(long Handle, CVArbiterTypes *Value);

/*
	CAENVME_GetRequesterType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The type of VME bus requester implemented (see 
						  CVRequesterTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function get the type of VME bus requester implemented on the module. 
*/
CAENVME_API   
CAENVME_GetRequesterType(long Handle, CVRequesterTypes *Value);

/*
	CAENVME_GetReleaseType
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The type of VME bus release policy implemented (see 
						  CVReleaseTypes enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function get the type of VME bus release implemented on the module. 
*/
CAENVME_API   
CAENVME_GetReleaseType(long Handle, CVReleaseTypes *Value);

/*
	CAENVME_GetBusReqLevel
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The type of VME bus requester priority level (see 
						  CVBusReqLevels enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function reads the specified VME bus requester priority level on the 
		module. 
*/
CAENVME_API   
CAENVME_GetBusReqLevel(long Handle, CVBusReqLevels *Value);

/*
	CAENVME_GetTimeout
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The value of VME bus timeout (see CVVMETimeouts enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function sets the specified VME bus timeout on the module. 
*/
CAENVME_API   
CAENVME_GetTimeout(long Handle, CVVMETimeouts *Value);

/*
	CAENVME_GetFIFOMode
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Value		: The FIFO mode read setting.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function read if the auto increment of the VME addresses during the 
		block tranfer cycles is enabled (0) or disabled (!=0). 
*/
CAENVME_API   
CAENVME_GetFIFOMode(long Handle, short *Value);

/*
	CAENVME_SystemReset
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function performs a system reset on the module. 
*/
CAENVME_API   
CAENVME_SystemReset(long Handle);

/*
	CAENVME_ResetScalerCount
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function resets the counter of the scaler.. 
*/
CAENVME_API
CAENVME_ResetScalerCount(long Handle);

/*
	CAENVME_EnableScalerGate
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function enables the gate of the scaler.
*/
CAENVME_API
CAENVME_EnableScalerGate(long Handle);

/*
	CAENVME_DisableScalerGate
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function disables the gate of the scaler.
*/
CAENVME_API
CAENVME_DisableScalerGate(long Handle);

/*
	CAENVME_StartPulser
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  PulSel	: The pulser to configure (see CVPulserSelect enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function starts the generation of pulse burst if the specified 
		pulser is configured for manual/software operation.
*/	
CAENVME_API  
CAENVME_StartPulser(long Handle, CVPulserSelect PulSel);

/*
	CAENVME_StopPulser
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  PulSel	: The pulser to configure (see CVPulserSelect enum).
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function stops the generation of pulse burst if the specified 
		pulser is configured for manual/software operation.
*/	
CAENVME_API  
CAENVME_StopPulser(long Handle, CVPulserSelect PulSel);

/*
	CAENVME_WriteFlashPage
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  Data		: The data to write.
		[in]  PageNum	: The flash page number to write.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function writes the data into the specified flash page.
*/	
CAENVME_API
CAENVME_WriteFlashPage(long Handle, unsigned char *Data, int PageNum);

/*
	CAENVME_ReadFlashPage
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[out] Data		: The data to write.
		[in]  PageNum	: The flash page number to write.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function reads the data from the specified flash page.
*/	
CAENVME_API
CAENVME_ReadFlashPage(long Handle, unsigned char *Data, int PageNum);

/*
	CAENVME_EraseFlashPage
	-----------------------------------------------------------------------------
	Parameters:
		[in]  Handle	: The handle that identifies the device.
		[in]  PageNum	: The flash page number to write.
	-----------------------------------------------------------------------------
	Returns:
		An error code about the execution of the function.
	-----------------------------------------------------------------------------
	Description:
		The function erases the specified flash page.
*/	
CAENVME_API
CAENVME_EraseFlashPage(long Handle, int Pagenum);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __V1718LIB_H
