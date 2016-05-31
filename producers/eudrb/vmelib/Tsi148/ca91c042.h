//------------------------------------------------------------------------------  
//title: Tundra Universe PCI-VME Kernel Driver
//version: Linux 1.1
//date: March 1999                                                                
//designer: Michael Wyrick                                                      
//programmer: Michael Wyrick                                                    
//platform: Linux 2.4.x
//language: GCC 2.95 and 3.0
//module: ca91c042
//------------------------------------------------------------------------------  
//  Purpose: Provide a Kernel Driver to Linux for the Universe I and II 
//           Universe model number ca91c042
//  Docs:                                  
//    This driver supports both the Universe and Universe II chips                                     
//------------------------------------------------------------------------------  
// RCS:
// $Id: ca91c042.h,v 1.4 2001/10/27 03:50:07 jhuggins Exp $
// $Log: ca91c042.h,v $
// Revision 1.4  2001/10/27 03:50:07  jhuggins
// These changes add support for the extra images offered by the Universe II.
// CVS : ----------------------------------------------------------------------
//
// Revision 1.4  2001/10/16 15:16:53  wyrick
// Minor Cleanup of Comments
//
//
//-----------------------------------------------------------------------------
#ifndef _ca91c042_H
#define _ca91c042_H

//-----------------------------------------------------------------------------
// Public Functions
//-----------------------------------------------------------------------------
// This is the typedef for a VmeIrqHandler
typedef void (*TirqHandler)(int vmeirq, int vector, void *dev_id, struct pt_regs *regs);
// This is the typedef for a DMA Transfer Callback function
typedef void (*TDMAcallback)(int status);

//  Returns the PCI baseaddress of the Universe chip
char* Universe_BaseAddr(void);
//  Returns the PCI IRQ That the universe is using
int Universe_IRQ(void);

char* mapvme(unsigned int pci, unsigned int vme, unsigned int size, 
             int image,int ctl);
void unmapvme(char *ptr, int image);

// Interrupt Stuff
void enable_vmeirq(unsigned int irq);
void disable_vmeirq(unsigned int irq);
int request_vmeirq(unsigned int irq, TirqHandler);
void free_vmeirq(unsigned int irq);                                     

// DMA Stuff
void VME_DMA(void* pci, void* vme, unsigned int count, int ctl, TDMAcallback cback);
void VME_DMA_LinkedList(void* CmdPacketList,TDMAcallback cback);

// Misc
int VME_Bus_Error(void);

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#define IRQ_VOWN    0x0001
#define IRQ_VIRQ1   0x0002
#define IRQ_VIRQ2   0x0004
#define IRQ_VIRQ3   0x0008
#define IRQ_VIRQ4   0x0010
#define IRQ_VIRQ5   0x0020
#define IRQ_VIRQ6   0x0040
#define IRQ_VIRQ7   0x0080
#define IRQ_DMA     0x0100
#define IRQ_LERR    0x0200
#define IRQ_VERR    0x0400
#define IRQ_res     0x0800
#define IRQ_IACK    0x1000
#define IRQ_SWINT   0x2000
#define IRQ_SYSFAIL 0x4000
#define IRQ_ACFAIL  0x8000

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
// See Page 2-77 in the Universe User Manual
typedef struct
{
   unsigned int dctl;   // DMA Control
   unsigned int dtbc;   // Transfer Byte Count
   unsigned int dlv;    // PCI Address
   unsigned int res1;   // Reserved
   unsigned int dva;    // Vme Address
   unsigned int res2;   // Reserved
   unsigned int dcpp;   // Pointer to Numed Cmd Packet with rPN
   unsigned int res3;   // Reserved
} TDMA_Cmd_Packet;

//-----------------------------------------------------------------------------
// Below here is normaly not used by a user module
//-----------------------------------------------------------------------------
#define  DMATIMEOUT 2*HZ;     

// Define for the Universe
#define SEEK_SET 0
#define SEEK_CUR 1

#define CONFIG_REG_SPACE        0xA0000000

#define PCI_SIZE_8	    0x0001
#define PCI_SIZE_16	    0x0002
#define PCI_SIZE_32	    0x0003

#define IOCTL_SET_CTL 	0xF001
#define IOCTL_SET_BS	  0xF002
#define IOCTL_SET_BD	  0xF003
#define IOCTL_SET_TO	  0xF004
#define IOCTL_PCI_SIZE  0xF005
#define IOCTL_SET_MODE 	0xF006
#define IOCTL_SET_WINT  0xF007    // Wait for interrupt before read

#define PCI_ID          0x0000
#define PCI_CSR        	0x0004
#define PCI_CLASS	      0x0008
#define PCI_MISC0  	    0x000C
#define PCI_BS		      0x0010
#define PCI_MISC1       0x003C

#define LSI0_CTL	      0x0100
#define LSI0_BS		      0x0104
#define LSI0_BD         0x0108
#define LSI0_TO		      0x010C

#define LSI1_CTL	      0x0114
#define LSI1_BS		      0x0118
#define LSI1_BD		      0x011C
#define LSI1_TO		      0x0120

#define LSI2_CTL	      0x0128
#define LSI2_BS		      0x012C
#define LSI2_BD		      0x0130
#define LSI2_TO		      0x0134

#define LSI3_CTL	      0x013C
#define LSI3_BS		      0x0140
#define LSI3_BD		      0x0144
#define LSI3_TO		      0x0148

#define LSI4_CTL	      0x01A0
#define LSI4_BS		      0x01A4
#define LSI4_BD		      0x01A8
#define LSI4_TO		      0x01AC

#define LSI5_CTL	      0x01B4
#define LSI5_BS		      0x01B8
#define LSI5_BD		      0x01BC
#define LSI5_TO		      0x01C0

#define LSI6_CTL	      0x01C8
#define LSI6_BS		      0x01CC
#define LSI6_BD		      0x01D0
#define LSI6_TO		      0x01D4

#define LSI7_CTL	      0x01DC
#define LSI7_BS		      0x01E0
#define LSI7_BD		      0x01E4
#define LSI7_TO		      0x01E8

#define SCYC_CTL	      0x0170
#define SCYC_ADDR    	  0x0174
#define SCYC_EN		      0x0178
#define SCYC_CMP	      0x017C
#define SCYC_SWP	      0x0180
#define LMISC		        0x0184
#define SLSI		        0x0188
#define L_CMDERR	      0x018C
#define LAERR		        0x0190

#define DCTL		        0x0200
#define DTBC		        0x0204
#define DLA		          0x0208
#define DVA		          0x0210
#define DCPP		        0x0218
#define DGCS		        0x0220
#define D_LLUE		      0x0224

#define LINT_EN		      0x0300
#define LINT_STAT	      0x0304
#define LINT_MAP0	      0x0308
#define LINT_MAP1	      0x030C
#define VINT_EN		      0x0310
#define VINT_STAT	      0x0314
#define VINT_MAP0	      0x0318
#define VINT_MAP1	      0x031C
#define STATID		      0x0320
#define V1_STATID	      0x0324
#define V2_STATID	      0x0328
#define V3_STATID	      0x032C
#define V4_STATID	      0x0330
#define V5_STATID	      0x0334
#define V6_STATID	      0x0338
#define V7_STATID	      0x033C

#define MBOX0	              0x0348
#define MBOX1	              0x034C
#define MBOX2		      0x0350
#define MBOX3		      0x0354
#define SEMA0		      0x0358
#define SEMA1		      0x035C

#define MAST_CTL	      0x0400
#define MISC_CTL	      0x0404
#define MISC_STAT	      0x0408
#define USER_AM		      0x040C

#define VSI0_CTL	      0x0F00
#define VSI0_BS		      0x0F04
#define VSI0_BD		      0x0F08
#define VSI0_TO		      0x0F0C

#define VSI1_CTL	      0x0F14
#define VSI1_BS		      0x0F18
#define VSI1_BD		      0x0F1C
#define VSI1_TO		      0x0F20

#define VSI2_CTL	      0x0F28
#define VSI2_BS		      0x0F2C
#define VSI2_BD		      0x0F30
#define VSI2_TO		      0x0F34

#define VSI3_CTL	      0x0F3C
#define VSI3_BS		      0x0F40
#define VSI3_BD		      0x0F44
#define VSI3_TO		      0x0F48

#define LM_CTL		      0x0F64
#define LM_BS		      0x0F68

#define VRAI_CTL	      0x0F70
#define VRAI_BS		      0x0F74
#define VCSR_CTL	      0x0F80
#define VCSR_TO		      0x0F84
#define V_AMERR		      0x0F88
#define VAERR		        0x0F8C

#define VSI4_CTL	      0x0F90
#define VSI4_BS		      0x0F94
#define VSI4_BD		      0x0F98
#define VSI4_TO		      0x0F9C

#define VSI5_CTL	      0x0FA4
#define VSI5_BS		      0x0FA8
#define VSI5_BD		      0x0FAC
#define VSI5_TO		      0x0FB0
                              
#define VSI6_CTL	      0x0FB8
#define VSI6_BS		      0x0FBC
#define VSI6_BD		      0x0FC0
#define VSI6_TO		      0x0FC4

#define VSI7_CTL	      0x0FCC
#define VSI7_BS		      0x0FD0
#define VSI7_BD		      0x0FD4
#define VSI7_TO		      0x0FD8
                              
#define VCSR_CLR	      0x0FF4
#define VCSR_SET	      0x0FF8
#define VCSR_BS		      0x0FFC


// DMA General Control/Status Register DGCS (0x220)
// 32-24 ||  GO   | STOPR | HALTR |   0   || CHAIN |   0   |   0   |   0   ||
// 23-16 ||              VON              ||             VOFF              ||
// 15-08 ||  ACT  | STOP  | HALT  |   0   || DONE  | LERR  | VERR  | P_ERR ||
// 07-00 ||   0   | INT_S | INT_H |   0   || I_DNE | I_LER | I_VER | I_PER ||

// VON - Length Per DMA VMEBus Transfer
//  0000 = None
//  0001 = 256 Bytes
//  0010 = 512 
//  0011 = 1024
//  0100 = 2048
//  0101 = 4096
//  0110 = 8192
//  0111 = 16384

// VOFF - wait between DMA tenures
//  0000 = 0    us
//  0001 = 16   
//  0010 = 32   
//  0011 = 64   
//  0100 = 128  
//  0101 = 256  
//  0110 = 512  
//  0111 = 1024 

#endif
