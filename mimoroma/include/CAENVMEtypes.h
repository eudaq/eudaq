/*
	-----------------------------------------------------------------------------

	               --- CAEN SpA - Computing Systems Division --- 

	-----------------------------------------------------------------------------

	CAENVMEtypes.h

	-----------------------------------------------------------------------------

	Created: March 2004

  	-----------------------------------------------------------------------------
*/

#ifdef LINUX 
#define CAEN_BYTE       unsigned char
#define CAEN_BOOL       int
#else
#define CAEN_BYTE       byte
#define CAEN_BOOL       VARIANT_BOOL
#endif

/*
	CAEN VME bridges.
*/
typedef enum CVBoardTypes {
	cvV1718	=	0,				/* CAEN V1718 USB-VME bridge				*/
	cvV2718	=	1				/* V2718 PCI-VME bridge with optical link	*/
} CVBoardTypes;

/*
	VME cycles data width.
*/
typedef enum CVDataWidth {
	cvD8  = 1,					/*  8 bit									*/
	cvD16 = 2,					/* 16 bit									*/
	cvD32 = 4,					/* 32 bit									*/
	cvD64 = 8					/* 64 bit									*/
} CVDataWidth;

/*
	VME cycles address modifiers
*/
typedef enum CVAddressModifier {
	cvA16_S			=	0x2D,	/* A16 supervisory access			 	    */
	cvA16_U			=	0x29,	/* A16 non-privileged				 		*/
	cvA16_LCK		=	0x2C,	/* A16 lock command							*/

	cvA24_S_BLT		=	0x3F,	/* A24 supervisory block transfer			*/
	cvA24_S_PGM		=	0x3E,	/* A24 supervisory program access			*/
	cvA24_S_DATA	=	0x3D,	/* A24 supervisory data access				*/
	cvA24_S_MBLT	=	0x3C,	/* A24 supervisory 64-bit block trnsfer     */
	cvA24_U_BLT		=	0x3B,	/* A24 non-privileged block transfer		*/
	cvA24_U_PGM		=	0x3A,	/* A24 non-privileged program access		*/
	cvA24_U_DATA	=	0x39,	/* A24 non-privileged data access			*/
	cvA24_U_MBLT	=	0x38,	/* A24 non-privileged 64-bit block trnsfer  */
	cvA24_LCK		=	0x32,	/* A24 lock command							*/

	cvA32_S_BLT		=	0x0F,	/* A32 supervisory block transfer			*/
	cvA32_S_PGM		=	0x0E,	/* A32 supervisory program access			*/
	cvA32_S_DATA	=	0x0D,	/* A32 supervisory data access				*/
	cvA32_S_MBLT	=	0x0C,	/* A32 supervisory 64-bit block trnsfer     */
	cvA32_U_BLT		=	0x0B,	/* A32 non-privileged block transfer		*/
	cvA32_U_PGM		=	0x0A,	/* A32 non-privileged program access		*/
	cvA32_U_DATA	=	0x09,	/* A32 non-privileged data access			*/
	cvA32_U_MBLT	=	0x08,	/* A32 non-privileged 64-bit block trnsfer  */
	cvA32_LCK		=	0x05,	/* A32 lock command							*/

	cvCR_CSR		=	0x2F,	/* CR/CSR space                             */

/*  
	The following address modifiers are not yet implemented. 
*/

	cvA40_BLT		=	0x37,	/* A40 block transfer (MD32)                */
	cvA40_LCK		=	0x35,	/* A40 lock command                         */
	cvA40			=	0x34,	/* A40 access                               */

	cvA64			=	0x01,	/* A64 single trnsfer access                */ 
	cvA64_BLT		=	0x03,	/* A64 block transfer                       */
	cvA64_MBLT		=	0x00,	/* A64 64-bit block transfer                */
	cvA64_LCK		=	0x04,	/* A64 lock command                         */

	cvA3U_2eVME		=	0x21,	/* 2eVME for 3U modules                     */
	cvA6U_2eVME		=	0x20	/* 2eVME for 6U modules                     */
} CVAddressModifier;

/*
	Error codes returned by the exported functions.
*/
typedef enum CVErrorCodes {
	cvSuccess		=	 0,		/* Operation completed successfully         */
	cvBusError		=	-1,		/* VME bus error during the cycle           */
	cvCommError		=	-2,		/* Communication error                      */
	cvGenericError	=	-3,		/* Unspecified error                        */
	cvInvalidParam  =   -4		/* Invalid parameter						*/
} CVErrorCodes;

/*
	Pulser selection.
*/
typedef enum CVPulserSelect {
	cvPulserA		=	0,		/* Identifies the pulser 'A'                */
	cvPulserB		=	1		/* Identifies the pulser 'B'                */
} CVPulserSelect;

/*
	Output selection.
*/
typedef enum CVOutputSelect {
	cvOutput0		=	0,		/* Identifies the output line 0             */
	cvOutput1		=	1,		/* Identifies the output line 1             */
	cvOutput2		=	2,		/* Identifies the output line 2             */
	cvOutput3		=	3,		/* Identifies the output line 3             */
	cvOutput4		=	4		/* Identifies the output line 4             */
} CVOutputSelect;

/*
	Input selection.
*/
typedef enum CVInputSelect {
	cvInput0	=	0,			/* Identifies the input line 0              */
	cvInput1	=	1			/* Identifies the input line 1              */
} CVInputSelect;

/*
	Signal sources. .
*/
typedef enum CVIOSources {
	cvManualSW			=	0,	/* Manual (button) or software controlled   */
	cvInputSrc0			=	1,	/* Input line 0								*/
	cvInputSrc1			=	2,	/* Input line 1								*/
	cvCoincidence		=	3,	/* Inputs coincidence						*/
	cvVMESignals		=	4,	/* Signals from VME bus						*/
	cvMiscSignals		=	6	/* Various internal signals					*/
} CVIOSources;

/*
	Time base units to specify pulses period and width.
*/
typedef enum CVTimeUnits {
	cvUnit25ns		=	0,		/* Time unit is 25 nanoseconds				*/
	cvUnit1600ns	=	1,		/* Time unit is 1.6 microseconds			*/
	cvUnit410us		=	2,		/* Time unit is 410 microseconds			*/
	cvUnit104ms		=	3		/* Time unit is 104 milliseconds			*/
} CVTimeUnits;

/*
	Polarity for LED emitting.
*/
typedef enum CVLEDPolarity {
	cvActiveHigh	=	0,		/* LED emits on signal high level			*/ 
	cvActiveLow		=	1		/* LED emits on signal low level			*/
} CVLEDPolarity;

/*
	Input and Output signal polarity.
*/
typedef enum CVIOPolarity {
	cvDirect	=	0,			/* Normal polarity							*/
	cvInverted	=	1			/* Inverted polarity						*/
} CVIOPolarity;

/*
	Accessible registers.
*/
typedef enum CVRegisters {
	cvStatusReg			=	0x0,		/* Status register							*/
	cvVMEControlReg		=	0x1,		/* VME Control register						*/
	cvFwRelReg			=	0x2,		/* Firmware Release register				*/
	cvFwDldReg			=	0x3,		/* Firmware Download register				*/
	cvFlenaReg			=	0x4,		/* Flash Enable								*/
	cvInputReg			=	0x8,		/* Input register							*/
	cvOutRegSet			=	0xA,		/* Output register							*/
	cvInMuxRegSet		=	0xB,		/* Input Multiplexer						*/
	cvOutMuxRegSet		=	0xC,		/* Output Multiplexer						*/
	cvLedPolRegSet		=	0xD,		/* Led Polarity								*/
	cvOutRegClear		=	0x10,		/* Output register							*/
	cvInMuxRegClear		=	0x11,		/* Input Multiplexer						*/
	cvOutMuxRegClear	=	0x12,		/* Output Multiplexer						*/
	cvLedPolRegClear	=	0x13,		/* Led Polarity								*/
	cvPulserA0			=	0x16,		/* Period and width of Pulser A				*/
	cvPulserA1			=	0x17,		/* Num pulses and range of Pulser A			*/
	cvPulserB0			=	0x19,		/* Period and width of Pulser B				*/
	cvPulserB1			=	0x1A,		/* Num pulses and range of Pulser B			*/
	cvScaler0			=	0x1C,		/* Limit and Autores of Scaler A			*/
	cvScaler1			=	0x1D,		/* Counter value of Scaler A				*/
	cvDispADL			=	0x20,		/* Display AD[15:0]							*/
	cvDispADH			=	0x21,		/* Display AD[31:16]						*/
	cvDispDTL			=	0x22,		/* Display DT[15:0]							*/
	cvDispDTH			=	0x23,		/* Display DT[31:16]						*/
	cvDispC1			=	0x24,		/* Display Control left bar					*/
	cvDispC2			=	0x25,		/* Display Control left bar					*/
	cvLMADL				=	0x28,		/* Loc. Mon. AD[15:0]						*/
	cvLMADH				=	0x29,		/* Loc. Mon. AD[31:16]						*/
	cvLMC				=	0x2C		/* Loc. Mon. Controls						*/

} CVRegisters;

/*
	Bits for status register decoding.
*/
typedef enum CVStatusRegisterBits {
	cvSYSRES	=	0x0001,		/* VME is in system reset state				*/
	cvSYSCTRL	=	0x0002,		/* The bridge is the VME system controller  */     
	cvDTACK		=	0x0010,		/* Last access has generated a DTACK signal */
	cvBERR		=	0x0020,		/* Last access has generated a bus error    */
	cvDIP0		=	0x0100,		/* Dip Switch position 0 state				*/
	cvDIP1		=	0x0200,		/* Dip Switch position 1 state				*/
	cvDIP2		=	0x0400,		/* Dip Switch position 2 state				*/
	cvDIP3		=	0x0800,		/* Dip Switch position 3 state				*/
	cvDIP4		=	0x1000,		/* Dip Switch position 4 state				*/
	cvUSBTYPE	=	0x8000		/* USB Speed: 0 = Full; 1 = High			*/
} CVStatusRegisterBits;

/*
	Bits for input register decoding.
*/
typedef enum CVInputRegisterBits {
	cvIn0Bit		= 0x0001,	/* Input line 0 signal level.				*/
	cvIn1Bit 		= 0x0002,	/* Input line 1 signal level.				*/
	cvCoincBit		= 0x0004,	/* Coincidence of input signal level.		*/
	cvPulsAOutBit	= 0x0008,	/* Pulser A output signal level.			*/
	cvPulsBOutBit	= 0x0010,	/* Pulser B output signal level.			*/
	cvScalEndCntBit	= 0x0020,	/* Scaler end counter signal level.			*/
	cvLocMonBit		= 0x0040,	/* Location monitor signal level.			*/
} CVInputRegisterBits;

/*
	Bits for input register decoding.
*/
typedef enum CVOutputRegisterBits {
	cvPulsAStartBit	= 0x0001,	/* Pulser A start signal level. 			*/
	cvPulsAResetBit = 0x0002,	/* Pulser A reset signal level. 			*/
	cvPulsBStartBit	= 0x0004,	/* Pulser B start signal level. 			*/
	cvPulsBResetBit	= 0x0008,	/* Pulser B reset signal level. 			*/
	cvScalGateBit	= 0x0010,	/* Scaler gate signal level. 				*/
	cvScalResetBit  = 0x0020,	/* Scaler reset counter signal level. 		*/
	cvOut0Bit		= 0x0040,	/* Output line 0 signal level. 				*/
	cvOut1Bit		= 0x0080,	/* Output line 1 signal level. 				*/
	cvOut2Bit		= 0x0100,	/* Output line 2 signal level. 				*/
	cvOut3Bit		= 0x0200,	/* Output line 3 signal level. 				*/
	cvOut4Bit		= 0x0400,	/* Output line 4 signal level. 				*/
} CVOutputRegisterBits;

/*
	Types of VME Arbiter.
*/
typedef enum CVArbiterTypes {
	cvPriorized		= 0,		/* Priority Arbiter							*/
	cvRoundRobin	= 1			/* Round-Robin Arbiter						*/
} CVArbiterTypes;

/*
	Types of VME Bus Requester.
*/
typedef enum CVRequesterTypes {
	cvFair		= 0,			/* Fair bus requester						*/
	cvDemand	= 1				/* On demand bus requester					*/
} CVRequesterTypes;

/*
	Types of VME Bus release.
*/
typedef enum CVReleaseTypes {
	cvRWD	= 0,				/* Release When Done						*/
	cvROR	= 1					/* Release On Request						*/
} CVReleaseTypes;

/*
	VME bus request levels.
*/
typedef enum CVBusReqLevels {
	cvBR0	=	0,				/* Bus request level 0						*/
	cvBR1	=	1,				/* Bus request level 1						*/
	cvBR2	=	2,				/* Bus request level 2						*/
	cvBR3	=	3				/* Bus request level 3						*/
} CVBusReqLevels;

/*
	VME Interrupt levels.
*/
typedef enum CVIRQLevels {
	cvIRQ1	=	0x01,			/* Interrupt level 1						*/
	cvIRQ2	=	0x02,			/* Interrupt level 2						*/
	cvIRQ3	=	0x04,			/* Interrupt level 3						*/
	cvIRQ4	=	0x08,			/* Interrupt level 4						*/
	cvIRQ5	=	0x10,			/* Interrupt level 5						*/
	cvIRQ6	=	0x20,			/* Interrupt level 6						*/
	cvIRQ7	=	0x40			/* Interrupt level 7						*/
} CVIRQLevels;

/*
	VME bus timeouts.
*/
typedef enum CVVMETimeouts {
	cvTimeout50us	= 0,		/* Timeout is 50 microseconds				*/
	cvTimeout400us	= 1			/* Timeout is 400 microseconds				*/
} CVVMETimeouts;

/*
	Data type to store the front panel display last access data.
*/
typedef struct CVDisplay {
	long            cvAddress;	/* VME Address								*/
	long            cvData;		/* VME Data									*/
	CAEN_BYTE       cvAM;		/* Address modifier							*/
	CAEN_BYTE       cvIRQ;		/* IRQ levels								*/
	CAEN_BOOL       cvDS0;		/* Data Strobe 0 signal						*/
	CAEN_BOOL       cvDS1;		/* Data Strobe 1 signal						*/
	CAEN_BOOL       cvAS;		/* Address Strobe signal					*/
	CAEN_BOOL       cvIACK;		/* Interrupt Acknowledge signal				*/
	CAEN_BOOL       cvWRITE;	/* Write signal								*/
	CAEN_BOOL       cvLWORD;	/* Long Word signal							*/
	CAEN_BOOL       cvDTACK;	/* Data Acknowledge signal					*/
	CAEN_BOOL       cvBERR;		/* Bus Error signal							*/
	CAEN_BOOL       cvSYSRES;	/* System Reset signal						*/
	CAEN_BOOL       cvBR;		/* Bus Request signal						*/
	CAEN_BOOL       cvBG;		/* Bus Grant signal							*/
} CVDisplay;

