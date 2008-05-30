/*--------------------------------------------------------------------------------------
*Title          :       Eudet Readout Board (EUDRB) Library
*Date:          :       05-04-2007
*Programmer     :       Lorenzo Chiarelli
*Platform       :       Linux (ELinOS) PowerPC
*File           :       eudrblib.h
*Version        :       1.0
*               :
*                       This Header File is written referring to SpecificheInterfacciaVME2.x.doc
*                       written by Angelo Cotta Ramusino (cotta@fe.infn.it)
*                       INFN Ferrara published on www.fe.infn.it/~cotta
*--------------------------------------------------------------------------------------
*License        :       You are free to use this source files for your own development as long
*               :       as it stays in a public research context. You are not allowed to use it
*               :       for commercial purpose. You must put this header with
*               :       authors names in all development based on this library.
*--------------------------------------------------------------------------------------
*/



#ifndef _eudrblib
#define _eudrblib

#ifdef __cplusplus
extern "C" {
#endif

/*
*       FunctionalCtrl_Stat_Reg
*       EUDRB MAIN CONTROL/STATUS REGISTER (32 BIT)     (OFFSET = 0x0)
*/

typedef union EUDRB_register
{
        unsigned long int eudrb_csr_reg;
        struct bits
        {
          /*                                    NAME                                                                            bit                             Brief description*/

          unsigned int     TriggerProcessingBusy           :1;             /*              0               R               ‘1’ while EUDRB is extracting or transferring data in response to a trigger*/

          unsigned int     ReadoutDataAvailable            :1;             /*              1               R               ‘1’  there is more data to be read in the output FIFO (in ZS mode) or in the pixel data memory (in non ZS mode)*/

          unsigned int     OutputFIFOAlmostFull            :1;             /*              2               R               ‘1’ when the output FIFO used in ZS mode has only 1K locations free (out of 256K)*/

          unsigned int     NIOS_Reset                                      :1;             /*              3               R/W             ‘1’ puts the NIOS II in the reset state. ‘0’ clears the reset NIOS II reset condition*/

          unsigned int     Compact_nExtended                       :1;             /*              4               R/W             ‘1’ selects the “compact” format for output in ZS readout mode*/

          unsigned int     ZS_Enabled                                      :1;             /*              5               R/W             ‘1’ selects the “Zero Suppressed” readout mode*/

          unsigned int     TriggerProcessingReset          :1;             /*              6               R/W             ‘1’ resets the FPGA’s trigger processing module*/
                
          unsigned int     PixelSamplingUnitReset          :1;             /*              7               R/W             ‘1’ resets the FPGA’s module which scans the sensor and stores the samples of signal in the pixel data memories*/
                
          unsigned int     FakeTriggerEnable                       :1;             /*              8               R/W             ‘1’ enables the generation of simulated triggers*/
                
          unsigned int     FakeTriggerNo0                          :1;             /*              9               R/W*/
          unsigned int     FakeTriggerNo1                          :1;             /*              10              R/W*/
          unsigned int     FakeTriggerNo2                          :1;             /*              11              R/W*/
          unsigned int     FakeTriggerNo3                          :1;             /*              12              R/W*/
          unsigned int     FakeTriggerNo4                          :1;             /*              13              R/W*/
          unsigned int     FakeTriggerNo5                          :1;             /*              14              R/W*/
          unsigned int     FakeTriggerNo6                          :1;             /*              15              R/W*/
          unsigned int     FakeTriggerNo7                          :1;             /*              16              R/W*/
          unsigned int     FakeTriggerNo8                          :1;             /*              17              R/W*/
          unsigned int     FakeTriggerNo9                          :1;             /*              18              R/W*/
          unsigned int     FakeTriggerNo10                         :1;             /*              19              R/W*/
          unsigned int     FakeTriggerNo11                         :1;             /*              20              R/W*/
          unsigned int     FakeTriggerNo12                         :1;             /*              21              R/W*/
          unsigned int     FakeTriggerNo13                         :1;             /*              22              R/W*/
          unsigned int     FakeTriggerNo14                         :1;             /*              23              R/W*/
          unsigned int     FakeTriggerNo15                         :1;             /*              24              R/W*/
                
          unsigned int     NIOS_IntrptServEnable           :1;             /*              25              R/W             Set this bit to ‘1’ to enable the NIOS-II to serve the VME commands. */
          /*                                            Clear this bit to ‘0’ to let the NIOS-II return to serving the commands from the UART at the front panel*/

          unsigned int     VME_RWM_EN                                      :1;             /*              26              R/W             No Action: (used for debugging purposes) */
                
          unsigned int     FakeTriggerPulse                        :1;             /*              27              R/W             Writing a ‘1’ generates a simulated trigger pulse*/
                
          unsigned int     FromMCU_ErrCode0                        :1;             /*              28              R               Error code bits returned by the NIOS II in reply to a  CMD/DATA operation*/
          unsigned int     FromMCU_ErrCode1                        :1;             /*              29              R*/
                
          unsigned int     MCUDataReady                            :1;             /*              30              R               ‘1’ indicates that the NIOS II MCU has updated the “DataFromMCU” register with the result of a read command*/
                
          unsigned int     USB_nVME                                        :1;             /*              31              R               ‘1’ means that the EUDRB is working in standalone mode with I/O performed through the USB2.0 link*/
                                
        } Bit;
} EUDRB_CSR;

/*EUDRB_CSR Control_Status_Register = 0x0; (Default Value) */
/*Control_Status_Register.Bit.NAME = 0||1; to set a bit on the bit field*/

/*
*       CommandToMCU register (32 bit) : interface to the NIOS II MicroController Unit (MCU) Write only
*
*       A write to this location sets a flag which is polled by the MCU. When the MCU detects that the flag is set it decodes the command written to this register, executes it and sets the “MCUCommandExecuted” *     bit in the FunctionalCtrl_Stat register.
*       A write to this location also clears the MCUCommandExecuted flag set by a previous execution.
*/

#define CommandToMCU    0x00000010

/*
*       DataToMCU register (32 bit) : interface to the NIOS II MicroController Unit (MCU)  Write only
*
*       This register contains data to be passed to the MCU routine started by a write to the CommandToMCU register. DataToMCU must be set prior to issuing the MCU command if this requires a parameter.
*/

#define DataToMCU               0x00000014      

/*
*       DataFromMCU register (32 bit) : interface to the NIOS II MicroController Unit (MCU) Read only
*
*       This register contains data returned by the execution of an MCU routine. The data is valid only if the the “MCUCommandExecuted” bit in the FunctionalCtrl_Stat register is set. 
*/

#define DataFromMCU             0x00000018

/*
*       PedThrInitValues register (32 bit) : default values ( common to all pixels ) for pedestal and threshold used in Zero Suppressed processing
*
*       bit             name    Brief description 
*       5..0    R/W     ThrInitValues   Default threshold common value
*       11..6   R/W     PedInitValues   Default pedestal common value
*
*/

#define PedThrInitValues 0x00000100



/*
*       Physical dimension of SRAM for each channel
*/
#define         QSRAM_SIZE                      0x00040000 /* 06/04/07 ACR*/
#define         QSRAM_DATAOUT_Default   0x00000008  | (0x0 << 6) /*06/04/07 ACR*/
#define         QSRAM_A_Base            0x00000000 /*06/04/07 ACR*/
#define         QSRAM_B_Base            0x00040000 /*06/04/07 ACR*/
#define         QSRAM_C_Base            0x00080000 /*06/04/07 ACR*/
#define         QSRAM_D_Base            0x000C0000 /*06/04/07 ACR*/

/*
*       Description of  commands executed by the NIOS II MCU
*       
*                       COMMAND NAME                                    “CommandToMCU”                  “DataToMCU”                     “DataFromMCU”                           Brief description 
*                                                                                       register contents               register contents               register contents
*/

#define         Exit_InterruptServiceLoop               0xb0000000                              /*Not Required                  none                                            This NIOS II command must be issued to exit from the loop 
                                                                                                                                                                                                                                                in which the NIOS II is only serving VME and trigger interrupts. 
                                                                                                                                                                                                                                                The NIOS II returns to the execution of the main MENU loop,
                                                                                                                                                                                                                                                waiting for user’s input from the RS232 port*/  

#define         FakeTrig_Generate                               0xa0000000                              /*Not Required                  none                                            This NIOS II command must be issued to generate a diagnostic trigger to the board 
                                                                                                                                                                                                                                                in this MIMO*2 test configuration.
                                                                                                                                                                                                                                                No matter when this command is issued the trigger pulse will be generated when the
                                                                                                                                                                                                                                                detector is being scanned for the fourth time (FRAMENumber=3)  after a periodic 
                                                                                                                                                                                                                                                reset and the  Fake trigger event number is always 77.*/
                                                
#define         FakeTriggerEnable_SET                   0xF0000001                              /*Not required                  none                                             the internal fake trigger signals (FAKE_TLU_trigger and FAKE_TLU_clear ) 
                                                                                                                                                                                                                                                are now by default routed OUT of the EUDRB and back in through the TLU port to emulate
                                                                                                                                                                                                                                                the TLU. If this is not desired the fake trigger signals can be routed all internally 
                                                                                                                                                                                                                                                to the FPGA by setting the FakeTriggerEnable flag*/


#define         FakeTriggerEnable_CLR                   0xF0000000                              /*Not required                  none                                             clear the FakeTriggerEnable flag.*/


#define         ClearTrigProcUnits                              0xc0000000                              /*Not Required                  none                                            This NIOS II command must be issued after acquiring the data from a diagnostic 
                                                                                                                                                                                                                                                trigger generated by a  “FakeTrig_Generate” command and before a new diagnostic
                                                                                                                                                                                                                                                trigger can be issued.*/
#define         Chng_AB_SRAMs_Selected                  0x70000001                              
#define         Chng_CD_SRAMs_Selected                  0x70000000
                                                                                                                                        /*Not Required                  none                                            The MIMO*2 only has 2 submatrices of 66*128 pixels. When the NIOS II software 
                                                                                                                                                                                                                                                variable “AB_SRAMs_Selected” is 1, the pixel data transferred by the NIOS II 
                                                                                                                                                                                                                                                to the output FIFO on arrival of a test trigger from VME comes from SRAM banks A&B,
                                                                                                                                                                                                                                                otherwise from banks C&D.
                                                                                                                                                                                                                                                The data from analog channel A is infact unconditionally (for this test design) 
                                                                                                                                                                                                                                                also stored in memory C, as well as the data from analog channel B 
                                                                                                                                                                                                                                                is stored to SRAM D. 
                                                                                                                                                                                                                                                The hex digit s is:
                                                                                                                                                                                                                                                1 to select A&B (default)
                                                                                                                                                                                                                                                0 to select C&D*/

#define uCIsMasterOfSRAM_SET                            0x90000001                              /*Not Required                  none                                            The FPGA internal signal:“uCIsMasterOfSRAM” is set when this command is issued 
                                                                                                                                                                                                                                                to the NIOS II MCU.
                                                                                                                                                                                                                                                The pixel data memories   are put under complete control of the MCU, which can 
                                                                                                                                                                                                                                                then test them for integrity and/or write the “pedestal” and “Threshold” fields 
                                                                                                                                                                                                                                                used in the ZeroSuppressed readout mode.
                                                                                                                                                                                                                                                The memory contents are not cleared by this operation so any SRAM read command 
                                                                                                                                                                                                                                                issued to the MCU will return the values last written by the pixel data 
                                                                                                                                                                                                                                                acquisition  machine.*/

#define uCIsMasterOfSRAM_CLR                            0x90000000                              /*Not Required                  none                                            The FPGA internal signal: “uCIsMasterOfSRAM” is cleared. The pixel data acquisition
                                                                                                                                                                                                                                                machine is restarted and the pixel data memories   are updated with the new sample
                                                                                                                                                                                                                                                values.*/
                                                                                                                                                                                                                                                
#define SRAM_ACCESS                                             0x10000000
#define SRAM_WRITE                                                      0x08000000 
#define SRAM_READ                                                       0x00000000 
#define UPPER                                                           0x00100000 
#define LOWER                                                           0x00000000


/*
*       SRAM_Access                                                             0x1WUaaaaa                              Not required                    none
*
*       Examples:
*
*       Write lower bits to SRAM_B at location 0x1234: 0x18041234
*
*       Read upper bits from SRAM_D at location 0x1234:0x101C1234               
*
*       The command code for accessing  the pixel SRAM via the NIOS II MCU can be evaluated considering that:
*       - W is the hex digit 8 for a write operation and 0 for a read
*       - U is the hex digit 1 for accessing the upper 24 bits of a pixel data location and the hex digit 0 for accessing the lower 24 bits
*       - aaaaa is a 20 bit address where the 2 MSBs select the target bank of memory (0 for bank A, 1 for bank B etc.) and the remaining 18 bits select the location within the target memory
*
*       In case of a WRITE access, the desired SRAM data  must be written to the location “DataToMCU” (OFFSET = 0x14) BEFORE issuing the “SRAM_Access” command.
*
*       In case of a READ type of “SRAM_Access”, the returning data can be read at  the VME location “DataFromMCU” (OFFSET = 0x18)
*
*       USE A COMBINATION OF COSTANTS LIKE: SRAM_ACCESS | SRAM_WRITE | UPPER | AAAAAA
*/

/*
*       MIMO*2 COMMANDS TO NIOSII
*/

/*
*       ->>>> IF YOU WANTO TO SEND/RECEIVE TO/FROM MIMO*2 USE A COMBINATION OF THE FOLLOWING COSTANTS LIKE:     MimosCMD | SetMimoParam | TypeBIAS_DAC | 0 iidd <<<<<-
*
*/

/*
*       Description of  commands executed by the NIOS II MCU
*       
*                       COMMAND NAME                                    “CommandToMCU”                  “DataToMCU”                     “DataFromMCU”                           Brief description 
*                                                                                       register contents               register contents               register contents
*/

/*                      MIMOStarConfig                                  0x4WT0iidd*/

#define MimosCMD                                                        0x40000000                              /*Not required                  none                                            The command code for changing the MIMO*2 configuration parameters 
                                                                                                                                                                                                                                                and downloading them to the chip can be evaluated considering that:             */
                                                                                                                                        /*                                                                                                      
                                                                                                                                                                                                                                                - W is the hex digit:                                                                                                   */

#define SetMimoParam                                            0x08000000                              /*
                                                                                                                                                                                                                                                         8 for  setting a parameter (SET)       */
                                                                                                                                                                                                                                                         
                                                                                                                                                                                                                                                         
#define ReadMimoParamFromNIOSII                         0x04000000                              /*                                                                                                              4 for reading the parameter value from the NIOS II memory       */
                                                                                                                                                                                                                                        
#define RdBckMimoParamFromChip                          0x00000000                              /*                                                                                                              0 for reading the parameter value returned from the chip 
                                                                                                                                                                                                                                                        after downloading (RdBck)       */      


                                                                                                                                        /*                                                                                                      - T selects the type of MIMO*2 target register for this operation:              */
                                                                                                                                        
#define TypeBIAS_DAC                                            0x00100000                              /*                                                                                                                      T=1 for the BIAS_DAC reg.s      (14*8bits)      */




#define TypeRO_MODE                                                     0x00200000                              /*                                                                                                                      T=2 for the RO_MODE reg.    (6*1bits)   */
#define TypeBSR_PIN                                                     0x00300000                              /*                                                                                                                      T=3 for the BSR_PIN reg.        (10*1bits)      */
#define TypeDIS_COL                                                     0x00400000                              /*                                                                                                                      T=4 for the DIS_COL reg.        (128*1bits)     */

#define JTAG_HARD_Reset                                         0x00A00000                              /*                                                                                                                      T=A -> JTAG HARD reset  */
                                
                
#define JTAG_SOFT_Reset                                         0x00C00000                              /*                                                                                                                      T=C -> JTAG SOFT reset  */

#define JTAG_DOWNLOAD                                           0x00D00000                              /*                                                                                                                      T=D : it causes the download    
                                                                                                                                                                                                                                                                of the parameters to the chip via JTAG  */

/*
*       - ii is the index to the target register ( see MIMO*2 description for details)
*       - dd is the desired data for a SET operation
*       The execution of a RdBck access copies the data readback via JTAG from the chip into the location “DataFromMCU” (OFFSET = 0x18)*/
/*

MIMOStarConfig  0x4WT0iidd

Examples:

Write 1 to the “TestEnable” bit (bit0) in the RO_MODE set of 1bit registers:
0x48200001

Write 100 to the “Test1” 8bit register (index=11) in the set of BIAS_DAC registers:
0x48100B64

Copy the value of register “Test1” readback from the chip via JTAG to the 
“DataFromMCU” (OFFSET = 0x18) location in the VME address space

*/

void EUDRB_CSR_Default(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_InterruptServeEna(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_Reset(int fdOut, unsigned long int baseaddress);
void EUDRB_Fake_Trigger(int fdOut, unsigned long int baseaddress);
void EUDRB_TriggerProcessingUnit_Reset(int fdOut, unsigned long int baseaddress);
void EUDRB_ZS_On(int fdOut, unsigned long int baseaddress);
void EUDRB_ZS_Off(int fdOut, unsigned long int baseaddress);
unsigned long EventDataReady_size(int fdOut, unsigned long int baseaddress);
void EventDataReady_wait(int fdOut, unsigned long int baseaddress);
void EUDRB_TriggerProcessingUnit_Reset_Check(int fdOut, unsigned long int baseaddress);
void EUDRB_Reset(int fdOut, unsigned long int baseaddress);
void EUDRB_FakeTriggerEnable_Set(int fdOut, unsigned long int baseaddress);
void EUDRB_FakeTriggerEnable_Reset(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_FakeTriggerEnable_Set(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_FakeTriggerEnable_Reset(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_IsMasterOfSRAM_Set(int fdOut, unsigned long int baseaddress);
void EUDRB_NIOSII_IsMasterOfSRAM_Reset(int fdOut, unsigned long int baseaddress);
void PedThrInitialize(void);

#ifdef __cplusplus
}
#endif



#endif
