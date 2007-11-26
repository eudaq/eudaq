//------------------------------------------------------------------------------  
//title: Tundra Universe PCI-VME Kernel Driver
//version: Linux 3.1
//date: February 2004
//original designer: Michael Wyrick
//original programmer: Michael Wyrick
//  Extensively restructured by Tom Armistead in support of the 
//  Tempe/Universe II combo driver and additional functionality.
//platform: Linux 2.4.x
//language: GCC 2.95 and 3.0
//------------------------------------------------------------------------------  
//  Purpose: Provide a Kernel Driver to Linux for the Universe I and II 
//           Universe model number ca91c042
//  Docs:                                  
//------------------------------------------------------------------------------  
// RCS:
// $Id: ca91c042.c,v 1.5 2001/10/27 03:50:07 jhuggins Exp $
// $Log: ca91c042.c,v $
// Revision 1.5  2001/10/27 03:50:07  jhuggins
// These changes add support for the extra images offered by the Universe II.
// CVS : ----------------------------------------------------------------------
//
// Revision 1.6  2001/10/16 15:16:53  wyrick
// Minor Cleanup of Comments
//
//
//-----------------------------------------------------------------------------
#define MODULE

#include <linux/config.h>
#include <linux/version.h>

#ifdef CONFIG_MODVERSIONS
  #define MODVERSIONS
  #include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "vmedrv.h"
#include "ca91c042.h"

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
extern struct vmeSharedData *vmechip_interboard_data;
extern const int vmechip_revision;
extern const int vmechip_devid;
extern const int vmechip_irq;
extern int vmechipIrqOverHeadTicks;
extern char *vmechip_baseaddr;
extern const int vme_slotnum;
extern int vme_syscon;
extern unsigned int out_image_va[];
extern unsigned int vme_irqlog[8][0x100];

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

static int outCTL[] = {LSI0_CTL, LSI1_CTL, LSI2_CTL, LSI3_CTL,
                     LSI4_CTL, LSI5_CTL, LSI6_CTL, LSI7_CTL};
                     
static int outBS[]  = {LSI0_BS,  LSI1_BS,  LSI2_BS,  LSI3_BS,
                     LSI4_BS,  LSI5_BS,  LSI6_BS,  LSI7_BS}; 
                     
static int outBD[]  = {LSI0_BD,  LSI1_BD,  LSI2_BD,  LSI3_BD,
                     LSI4_BD,  LSI5_BD,  LSI6_BD,  LSI7_BD}; 

static int outTO[]  = {LSI0_TO,  LSI1_TO,  LSI2_TO,  LSI3_TO,
                     LSI4_TO,  LSI5_TO,  LSI6_TO,  LSI7_TO}; 


static int inCTL[] = {VSI0_CTL, VSI1_CTL, VSI2_CTL, VSI3_CTL,
                     VSI4_CTL, VSI5_CTL, VSI6_CTL, VSI7_CTL};
                     
static int inBS[]  = {VSI0_BS,  VSI1_BS,  VSI2_BS,  VSI3_BS,
                     VSI4_BS,  VSI5_BS,  VSI6_BS,  VSI7_BS}; 
                     
static int inBD[]  = {VSI0_BD,  VSI1_BD,  VSI2_BD,  VSI3_BD,
                     VSI4_BD,  VSI5_BD,  VSI6_BD,  VSI7_BD}; 

static int inTO[]  = {VSI0_TO,  VSI1_TO,  VSI2_TO,  VSI3_TO,
                     VSI4_TO,  VSI5_TO,  VSI6_TO,  VSI7_TO}; 
static int vmevec[7]          = {V1_STATID, V2_STATID, V3_STATID, V4_STATID,
                                 V5_STATID, V6_STATID, V7_STATID};


//----------------------------------------------------------------------------
// Vars and Defines
//----------------------------------------------------------------------------
extern wait_queue_head_t dma_queue[];
extern wait_queue_head_t lm_queue;
extern wait_queue_head_t mbox_queue;

extern	unsigned int vmeGetTime();
extern  void vmeSyncData();
extern  void vmeFlushLine();
extern	int	tb_speed;

unsigned int	uniIrqTime;
unsigned int	uniDmaIrqTime;
unsigned int	uniLmEvent;

//----------------------------------------------------------------------------
//  uni_procinfo()
//----------------------------------------------------------------------------
int uni_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
  char *p;
  unsigned int temp1,temp2,x;

  p = buf;

  for (x=0;x<8;x+=2) {
    temp1 = readl(vmechip_baseaddr+outCTL[x]);
    temp2 = readl(vmechip_baseaddr+outCTL[x+1]);
    p += sprintf(p,"  LSI%i_CTL = %08X    LSI%i_CTL = %08X\n",x,temp1,x+1,temp2);
    temp1 = readl(vmechip_baseaddr+outBS[x]);
    temp2 = readl(vmechip_baseaddr+outBS[x+1]);
    p += sprintf(p,"  LSI%i_BS  = %08X    LSI%i_BS  = %08X\n",x,temp1,x+1,temp2);
    temp1 = readl(vmechip_baseaddr+outBD[x]);
    temp2 = readl(vmechip_baseaddr+outBD[x+1]);
    p += sprintf(p,"  LSI%i_BD  = %08X    LSI%i_BD  = %08X\n",x,temp1,x+1,temp2);
    temp1 = readl(vmechip_baseaddr+outTO[x]);
    temp2 = readl(vmechip_baseaddr+outTO[x+1]);
    p += sprintf(p,"  LSI%i_TO  = %08X    LSI%i_TO  = %08X\n",x,temp1,x+1,temp2);
  }  

  p += sprintf(p,"\n");  

  *eof = 1;
  return p - buf;
}

//----------------------------------------------------------------------------
//  uniBusErrorChk()
//----------------------------------------------------------------------------
int
uniBusErrorChk(clrflag)
{
    int tmp;
    tmp = readl(vmechip_baseaddr+PCI_CSR);
    if (tmp & 0x08000000) {  // S_TA is Set
      if(clrflag)
        writel(tmp | 0x08000000,vmechip_baseaddr+PCI_CSR);
      return(1);
    }
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : DMA_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: Saves DMA completion timestamp and then wakes up DMA queue
//-----------------------------------------------------------------------------
void DMA_uni_irqhandler(void)
{
  uniDmaIrqTime = uniIrqTime;
  wake_up(&dma_queue[0]);
}

//-----------------------------------------------------------------------------
// Function   : LERR_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
void LERR_uni_irqhandler(void)
{
  int val;
  char buf[100];

  val = readl(vmechip_baseaddr+DGCS);

  if (!(val & 0x00000800)) {
    sprintf(buf,"<<ca91c042>> LERR_uni_irqhandler DMA Read Error DGCS=%08X\n",val);  
    printk(buf);

  }

}

//-----------------------------------------------------------------------------
// Function   : VERR_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
void VERR_uni_irqhandler(void)
{
  int val;
  char buf[100];

  val = readl(vmechip_baseaddr+DGCS);

  if (!(val & 0x00000800)) {
    sprintf(buf,"<<ca91c042>> VERR_uni_irqhandler DMA Read Error DGCS=%08X\n",val);  
    printk(buf);
  }

}
//-----------------------------------------------------------------------------
// Function   : MB_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
void MB_uni_irqhandler(mboxMask)
{
  if(vmechipIrqOverHeadTicks != 0){
     wake_up(&mbox_queue);
  }
}

//-----------------------------------------------------------------------------
// Function   : LM_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
void LM_uni_irqhandler(lmMask)
{
  uniLmEvent = lmMask;
  wake_up(&lm_queue);
}

//-----------------------------------------------------------------------------
// Function   : VIRQ_uni_irqhandler
// Inputs     : void
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
void VIRQ_uni_irqhandler(virqMask)
{
  int iackvec, i;

  for(i = 7; i > 0; i--){
    if(virqMask & (1 << i)){
       iackvec = readl(vmechip_baseaddr+vmevec[i-1]);
       vme_irqlog[i][iackvec]++;
    }
  }
}

//-----------------------------------------------------------------------------
// Function   : uni_irqhandler
// Inputs     : int irq, void *dev_id, struct pt_regs *regs
// Outputs    : void
// Description: 
//-----------------------------------------------------------------------------
static void uni_irqhandler(int irq, void *dev_id, struct pt_regs *regs)
{
  long stat, enable;

  uniIrqTime = vmeGetTime();

  enable = readl(vmechip_baseaddr+LINT_EN);
  stat = readl(vmechip_baseaddr+LINT_STAT);
  stat = stat & enable;
  writel(stat, vmechip_baseaddr+LINT_STAT);                // Clear all pending ints

  if (stat & 0x0100)
    DMA_uni_irqhandler();
  if (stat & 0x0200)
    LERR_uni_irqhandler();
  if (stat & 0x0400)
    VERR_uni_irqhandler();
  if (stat & 0xF0000)
    MB_uni_irqhandler((stat & 0xF0000) >> 16);
  if (stat & 0xF00000)
    LM_uni_irqhandler((stat & 0xF00000) >> 20);
  if (stat & 0x0000FE)
    VIRQ_uni_irqhandler(stat & 0x0000FE);

}

//-----------------------------------------------------------------------------
// Function   : uniGenerateIrq
// Description: 
//-----------------------------------------------------------------------------
int
uniGenerateIrq( virqInfo_t *vmeIrq)
{
  int timeout;
  int looptimeout;

  timeout = vmeIrq->waitTime;
  if(timeout == 0){
     timeout++;		// Wait at least 1 tick...
  }
  looptimeout = HZ/20;	// try for 1/20 second

  vmeIrq->timeOutFlag = 0;

  // Validate & setup vector register.
  if(vmeIrq->vector & 1){	// Universe can only generate even vectors
    return(-EINVAL);
  }
  writel(vmeIrq->vector << 24, vmechip_baseaddr+STATID);  

  // Assert VMEbus IRQ
  writel(1 << (vmeIrq->level + 24), vmechip_baseaddr+VINT_EN);  

  // Wait for syscon to do iack
  while( readl(vmechip_baseaddr+VINT_STAT) & (1 << (vmeIrq->level + 24))  ){
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(looptimeout);
    timeout = timeout - looptimeout;
    if(timeout <= 0){
       vmeIrq->timeOutFlag = 1;
       break;
    }
  }

  // Clear VMEbus IRQ bit
  writel(0, vmechip_baseaddr+VINT_EN);  

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetArbiter
// Description: 
//-----------------------------------------------------------------------------
int
uniSetArbiter( vmeArbiterCfg_t *vmeArb)
{
  int tempCtl = 0;
  int vbto = 0;

  tempCtl = readl(vmechip_baseaddr+MISC_CTL);
  tempCtl &= 0x00FFFFFF;

  if(vmeArb->globalTimeoutTimer == 0xFFFFFFFF){
     vbto = 7;
  } else if (vmeArb->globalTimeoutTimer > 1024) {
     return(-EINVAL);
  } else if (vmeArb->globalTimeoutTimer == 0) {
     vbto = 0;
  } else {
     vbto = 1;
     while((16 * (1 << (vbto-1))) < vmeArb->globalTimeoutTimer){
        vbto += 1;
     }
  } 
  tempCtl |= (vbto << 28);

  if(vmeArb->arbiterMode == VME_PRIORITY_MODE){
    tempCtl |= 1 << 26;
  }
  
  if(vmeArb->arbiterTimeoutFlag){
    tempCtl |= 2 << 24;
  }

  writel(tempCtl, vmechip_baseaddr+ MISC_CTL);
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniGetArbiter
// Description: 
//-----------------------------------------------------------------------------
int
uniGetArbiter( vmeArbiterCfg_t *vmeArb)
{
  int tempCtl = 0;
  int vbto = 0;

  tempCtl = readl(vmechip_baseaddr+MISC_CTL);

  vbto = (tempCtl >> 28) & 0xF;
  if(vbto != 0){
    vmeArb->globalTimeoutTimer = (16 * (1 << (vbto-1)));
  }

  if(tempCtl & (1 << 26)){
     vmeArb->arbiterMode = VME_PRIORITY_MODE;
  } else {
     vmeArb->arbiterMode = VME_R_ROBIN_MODE;
  }
  
  if(tempCtl & (3 << 24)){
    vmeArb->arbiterTimeoutFlag = 1;
  }
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetRequestor
// Description: 
//-----------------------------------------------------------------------------
int
uniSetRequestor( vmeRequesterCfg_t *vmeReq)
{
  int tempCtl = 0;

  tempCtl = readl(vmechip_baseaddr+MAST_CTL);
  tempCtl &= 0xFF0FFFFF;

  if(vmeReq->releaseMode == 1){
    tempCtl |=  (1 << 20);
  }

  if(vmeReq->fairMode == 1){
    tempCtl |=  (1 << 21);
  }

  tempCtl |= (vmeReq->requestLevel << 22);

  writel(tempCtl, vmechip_baseaddr+ MAST_CTL);
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniGetRequestor
// Description: 
//-----------------------------------------------------------------------------
int
uniGetRequestor( vmeRequesterCfg_t *vmeReq)
{
  int tempCtl = 0;

  tempCtl = readl(vmechip_baseaddr+MAST_CTL);

  if(tempCtl & (1 << 20)){
    vmeReq->releaseMode = 1;
  }

  if(tempCtl & (1 << 21)){
    vmeReq->fairMode = 1;
  }

  vmeReq->requestLevel = (tempCtl & 0xC00000) >> 22;
  
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetInBound
// Description: 
//-----------------------------------------------------------------------------
int
uniSetInBound( vmeInWindowCfg_t *vmeIn)
{
  int tempCtl = 0;

  // Verify input data
  if(vmeIn->windowNbr > 7){
    return(-EINVAL);
  }
  if((vmeIn->vmeAddrU) || (vmeIn->windowSizeU) || (vmeIn->pciAddrU)){
    return(-EINVAL);
  }
  if((vmeIn->vmeAddrL & 0xFFF) ||
     (vmeIn->windowSizeL & 0xFFF) ||
     (vmeIn->pciAddrL & 0xFFF)){
    return(-EINVAL);
  }

  if(vmeIn->bcastRespond2esst){
    return(-EINVAL);
  }
  switch(vmeIn->addrSpace){
    case VME_A64:
    case VME_CRCSR:
    case VME_USER3:
    case VME_USER4:
      return(-EINVAL);
    case VME_A16:
      tempCtl |= 0x00000;
      break;
    case VME_A24:
      tempCtl |= 0x10000;
      break;
    case VME_A32:
      tempCtl |= 0x20000;
      break;
    case VME_USER1:
      tempCtl |= 0x60000;
      break;
    case VME_USER2:
      tempCtl |= 0x70000;
      break;
  }

  // Disable while we are mucking around
  writel(0x00000000,vmechip_baseaddr+inCTL[vmeIn->windowNbr]);     
  writel(vmeIn->vmeAddrL, vmechip_baseaddr+inBS[vmeIn->windowNbr]);     
  writel(vmeIn->vmeAddrL + vmeIn->windowSizeL, 
           vmechip_baseaddr+inBD[vmeIn->windowNbr]);
  writel(vmeIn->pciAddrL - vmeIn->vmeAddrL, 
           vmechip_baseaddr+inTO[vmeIn->windowNbr]);     

  // Setup CTL register.
  if(vmeIn->wrPostEnable)
    tempCtl |= 0x40000000;
  if(vmeIn->prefetchEnable)
    tempCtl |= 0x20000000;
  if(vmeIn->rmwLock)
    tempCtl |= 0x00000040;
  if(vmeIn->data64BitCapable)
    tempCtl |= 0x00000080;
  if(vmeIn->userAccessType & VME_USER)
    tempCtl |= 0x00100000;
  if(vmeIn->userAccessType & VME_SUPER)
    tempCtl |= 0x00200000;
  if(vmeIn->dataAccessType & VME_DATA)
    tempCtl |= 0x00400000;
  if(vmeIn->dataAccessType & VME_PROG)
    tempCtl |= 0x00800000;

  // Write ctl reg without enable
  writel(tempCtl,vmechip_baseaddr+inCTL[vmeIn->windowNbr]);     

  if(vmeIn->windowEnable)
    tempCtl |= 0x80000000;
 
  writel(tempCtl,vmechip_baseaddr+inCTL[vmeIn->windowNbr]);     
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniGetInBound
// Description: 
//-----------------------------------------------------------------------------
int
uniGetInBound( vmeInWindowCfg_t *vmeIn)
{
  int tempCtl = 0;

  // Verify input data
  if(vmeIn->windowNbr > 7){
    return(-EINVAL);
  }

  // Get Window mappings.
  vmeIn->vmeAddrL = readl(vmechip_baseaddr+inBS[vmeIn->windowNbr]);     
  vmeIn->pciAddrL = vmeIn->vmeAddrL + 
                       readl(vmechip_baseaddr+inTO[vmeIn->windowNbr]);     
  vmeIn->windowSizeL = readl(vmechip_baseaddr+inBD[vmeIn->windowNbr]) - 
                       vmeIn->vmeAddrL;

  tempCtl = readl(vmechip_baseaddr+inCTL[vmeIn->windowNbr]);     

  // Get Control & BUS attributes
  if(tempCtl & 0x40000000)
    vmeIn->wrPostEnable = 1;
  if(tempCtl & 0x20000000)
    vmeIn->prefetchEnable = 1;
  if(tempCtl & 0x00000040)
    vmeIn->rmwLock = 1;
  if(tempCtl & 0x00000080)
    vmeIn->data64BitCapable = 1;
  if(tempCtl & 0x00100000)
    vmeIn->userAccessType |= VME_USER;
  if(tempCtl & 0x00200000)
    vmeIn->userAccessType |= VME_SUPER;
  if(tempCtl & 0x00400000)
    vmeIn->dataAccessType |= VME_DATA;
  if(tempCtl & 0x00800000)
    vmeIn->dataAccessType |= VME_PROG;
  if(tempCtl & 0x80000000)
     vmeIn->windowEnable = 1;

  switch((tempCtl & 0x70000) >> 16) {
    case 0x0:
      vmeIn->addrSpace = VME_A16;
      break;
    case 0x1:
      vmeIn->addrSpace = VME_A24;
      break;
    case 0x2:
      vmeIn->addrSpace = VME_A32;
      break;
    case 0x6:
      vmeIn->addrSpace = VME_USER1;
      break;
    case 0x7:
      vmeIn->addrSpace = VME_USER2;
      break;
  }

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetOutBound
// Description: 
//-----------------------------------------------------------------------------
int
uniSetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  int tempCtl = 0;

  // Verify input data
  if(vmeOut->windowNbr > 7){
    return(-EINVAL);
  }
  if((vmeOut->xlatedAddrU) || (vmeOut->windowSizeU) || (vmeOut->pciBusAddrU)){
    return(-EINVAL);
  }
  if((vmeOut->xlatedAddrL & 0xFFF) ||
     (vmeOut->windowSizeL & 0xFFF) ||
     (vmeOut->pciBusAddrL & 0xFFF)){
    return(-EINVAL);
  }
  if(vmeOut->bcastSelect2esst){
    return(-EINVAL);
  }
  switch(vmeOut->addrSpace){
    case VME_A64:
    case VME_USER3:
    case VME_USER4:
      return(-EINVAL);
    case VME_A16:
      tempCtl |= 0x00000;
      break;
    case VME_A24:
      tempCtl |= 0x10000;
      break;
    case VME_A32:
      tempCtl |= 0x20000;
      break;
    case VME_CRCSR:
      tempCtl |= 0x50000;
      break;
    case VME_USER1:
      tempCtl |= 0x60000;
      break;
    case VME_USER2:
      tempCtl |= 0x70000;
      break;
  }

  // Disable while we are mucking around
  writel(0x00000000,vmechip_baseaddr+outCTL[vmeOut->windowNbr]);     
  writel(vmeOut->pciBusAddrL, vmechip_baseaddr+outBS[vmeOut->windowNbr]);     
  writel(vmeOut->pciBusAddrL + vmeOut->windowSizeL, 
           vmechip_baseaddr+outBD[vmeOut->windowNbr]);
  writel(vmeOut->xlatedAddrL - vmeOut->pciBusAddrL, 
           vmechip_baseaddr+outTO[vmeOut->windowNbr]);     

  // Sanity check.
  if(vmeOut->pciBusAddrL != 
     readl(vmechip_baseaddr+outBS[vmeOut->windowNbr])){
     printk("   ca91c042: out window: %x, failed to configure\n", 
             vmeOut->windowNbr);
     return(-EINVAL);
  }

  if(vmeOut->pciBusAddrL + vmeOut->windowSizeL != 
     readl( vmechip_baseaddr+outBD[vmeOut->windowNbr])){
     printk("   ca91c042: out window: %x, failed to configure\n", 
             vmeOut->windowNbr);
     return(-EINVAL);
  }

  if(vmeOut->xlatedAddrL - vmeOut->pciBusAddrL != 
     readl( vmechip_baseaddr+outTO[vmeOut->windowNbr])){   
     printk("   ca91c042: out window: %x, failed to configure\n", 
             vmeOut->windowNbr);
     return(-EINVAL);
  }

  // Setup CTL register.
  if(vmeOut->wrPostEnable)
    tempCtl |= 0x40000000;
  if(vmeOut->userAccessType & VME_SUPER)
    tempCtl |= 0x001000;
  if(vmeOut->dataAccessType & VME_PROG)
    tempCtl |= 0x004000;
  if(vmeOut->maxDataWidth == VME_D16)
    tempCtl |= 0x00400000;
  if(vmeOut->maxDataWidth == VME_D32)
    tempCtl |= 0x00800000;
  if(vmeOut->maxDataWidth == VME_D64)
    tempCtl |= 0x00C00000;
  if(vmeOut->xferProtocol & (VME_BLT | VME_MBLT))
    tempCtl |= 0x00000100;

  // Write ctl reg without enable
  writel(tempCtl,vmechip_baseaddr+outCTL[vmeOut->windowNbr]);     

  if(vmeOut->windowEnable)
    tempCtl |= 0x80000000;
 
  writel(tempCtl,vmechip_baseaddr+outCTL[vmeOut->windowNbr]);     
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniGetOutBound
// Description: 
//-----------------------------------------------------------------------------
int
uniGetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  int tempCtl = 0;

  // Verify input data
  if(vmeOut->windowNbr > 7){
    return(-EINVAL);
  }

  // Get Window mappings.
  vmeOut->pciBusAddrL = readl(vmechip_baseaddr+outBS[vmeOut->windowNbr]);     
  vmeOut->xlatedAddrL = vmeOut->pciBusAddrL + 
                       readl(vmechip_baseaddr+outTO[vmeOut->windowNbr]);     
  vmeOut->windowSizeL = readl(vmechip_baseaddr+outBD[vmeOut->windowNbr]) - 
                       vmeOut->pciBusAddrL;

  tempCtl = readl(vmechip_baseaddr+outCTL[vmeOut->windowNbr]);     

  // Get Control & BUS attributes
  if(tempCtl & 0x40000000)
    vmeOut->wrPostEnable = 1;
  if(tempCtl & 0x001000)
    vmeOut->userAccessType = VME_SUPER;
  else
    vmeOut->userAccessType = VME_USER;
  if(tempCtl & 0x004000)
    vmeOut->dataAccessType = VME_PROG;
  else 
    vmeOut->dataAccessType = VME_DATA;
  if(tempCtl & 0x80000000)
     vmeOut->windowEnable = 1;

  switch((tempCtl & 0x00C00000) >> 22){
    case 0:
      vmeOut->maxDataWidth = VME_D8;
      break;
    case 1:
      vmeOut->maxDataWidth = VME_D16;
      break;
    case 2:
      vmeOut->maxDataWidth = VME_D32;
      break;
    case 3:
      vmeOut->maxDataWidth = VME_D64;
      break;
  }
  if(tempCtl & 0x00000100)
     vmeOut->xferProtocol = VME_BLT;
  else
     vmeOut->xferProtocol = VME_SCT;

  switch((tempCtl & 0x70000) >> 16) {
    case 0x0:
      vmeOut->addrSpace = VME_A16;
      break;
    case 0x1:
      vmeOut->addrSpace = VME_A24;
      break;
    case 0x2:
      vmeOut->addrSpace = VME_A32;
      break;
    case 0x5:
      vmeOut->addrSpace = VME_CRCSR;
      break;
    case 0x6:
      vmeOut->addrSpace = VME_USER1;
      break;
    case 0x7:
      vmeOut->addrSpace = VME_USER2;
      break;
  }

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetupLm
// Description: 
//-----------------------------------------------------------------------------
int
uniSetupLm( vmeLmCfg_t *vmeLm)
{
  int tempCtl = 0;

  if(vmeLm->addrU ) {
    return(-EINVAL);
  }
  switch(vmeLm->addrSpace){
    case VME_A64:
    case VME_USER3:
    case VME_USER4:
      return(-EINVAL);
    case VME_A16:
      tempCtl |= 0x00000;
      break;
    case VME_A24:
      tempCtl |= 0x10000;
      break;
    case VME_A32:
      tempCtl |= 0x20000;
      break;
    case VME_CRCSR:
      tempCtl |= 0x50000;
      break;
    case VME_USER1:
      tempCtl |= 0x60000;
      break;
    case VME_USER2:
      tempCtl |= 0x70000;
      break;
  }

  // Disable while we are mucking around
  writel(0x00000000,vmechip_baseaddr+LM_CTL);
  writel(vmeLm->addr, vmechip_baseaddr+LM_BS);

  // Setup CTL register.
  if(vmeLm->userAccessType & VME_SUPER)
    tempCtl |= 0x00200000;
  if(vmeLm->userAccessType & VME_USER)
    tempCtl |= 0x00100000;
  if(vmeLm->dataAccessType & VME_PROG)
    tempCtl |= 0x00800000;
  if(vmeLm->dataAccessType & VME_DATA)
    tempCtl |= 0x00400000;

  uniLmEvent = 0;
  // Write ctl reg and enable
  writel(0x80000000 | tempCtl,vmechip_baseaddr+LM_CTL);

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniWaitLm
// Description: 
//-----------------------------------------------------------------------------
int
uniWaitLm( vmeLmCfg_t *vmeLm)
{
  if(uniLmEvent == 0){
    interruptible_sleep_on_timeout(&lm_queue, vmeLm->lmWait);
  }
  vmeLm->lmEvents = uniLmEvent;
  writel(0x00000000,vmechip_baseaddr+LM_CTL);

  return(0);
}

#define	SWIZZLE(X) ( ((X & 0xFF000000) >> 24) | ((X & 0x00FF0000) >>  8) | ((X & 0x0000FF00) <<  8) | ((X & 0x000000FF) << 24))

//-----------------------------------------------------------------------------
// Function   : uniDoRmw
// Description: 
//-----------------------------------------------------------------------------
int
uniDoRmw( vmeRmwCfg_t *vmeRmw)
{
  int tempCtl = 0;
  int tempBS = 0;
  int tempBD = 0;
  int tempTO = 0;
  int vmeBS = 0;
  int vmeBD = 0;
  int *rmwPciDataPtr = NULL;
  int *vaDataPtr = NULL;
  int i;
  vmeOutWindowCfg_t vmeOut;
  if(vmeRmw->maxAttempts < 1){
    return(-EINVAL);
  }
  if(vmeRmw->targetAddrU ) {
    return(-EINVAL);
  }

  // Find the PCI address that maps to the desired VME address 
  for(i = 0; i < 8; i++){
    tempCtl = readl(vmechip_baseaddr+outCTL[i]);
    if((tempCtl & 0x80000000) == 0){
      continue;
    }
    memset(&vmeOut, 0, sizeof(vmeOut));
    vmeOut.windowNbr = i;
    uniGetOutBound(&vmeOut);
    if(vmeOut.addrSpace != vmeRmw->addrSpace) {
       continue;
    }
    tempBS = readl(vmechip_baseaddr+outBS[i]);
    tempBD = readl(vmechip_baseaddr+outBD[i]);
    tempTO = readl(vmechip_baseaddr+outTO[i]);
    vmeBS = tempBS+tempTO;
    vmeBD = tempBD+tempTO;
    if((vmeRmw->targetAddr >= vmeBS) &&
       (vmeRmw->targetAddr < vmeBD)){
         rmwPciDataPtr = (int *)(tempBS + (vmeRmw->targetAddr - vmeBS));
         vaDataPtr = (int *)(out_image_va[i] + (vmeRmw->targetAddr - vmeBS));
         break;
    }
  }
     
  // If no window - fail.
  if(rmwPciDataPtr == NULL){
    return(-EINVAL);
  }

  // Setup the RMW registers.
  writel(0, vmechip_baseaddr+SCYC_CTL);
  writel(SWIZZLE(vmeRmw->enableMask), vmechip_baseaddr+SCYC_EN);
  writel(SWIZZLE(vmeRmw->compareData), vmechip_baseaddr+SCYC_CMP);
  writel(SWIZZLE(vmeRmw->swapData), vmechip_baseaddr+SCYC_SWP);
  writel((int)rmwPciDataPtr, vmechip_baseaddr+SCYC_ADDR);
  writel(1, vmechip_baseaddr+SCYC_CTL);

  // Run the RMW cycle until either success or max attempts.
  vmeRmw->numAttempts = 1;
  while(vmeRmw->numAttempts <= vmeRmw->maxAttempts){

    if((readl(vaDataPtr) & vmeRmw->enableMask) == 
       (vmeRmw->swapData & vmeRmw->enableMask)){

          writel(0, vmechip_baseaddr+SCYC_CTL);
          break;
 
    }
    vmeRmw->numAttempts++;
  }

  // If no success, set num Attempts to be greater than max attempts
  if(vmeRmw->numAttempts > vmeRmw->maxAttempts){
    vmeRmw->numAttempts = vmeRmw->maxAttempts +1;
  }

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniSetupDctlReg
// Description: 
//-----------------------------------------------------------------------------
int
uniSetupDctlReg(
vmeDmaPacket_t *vmeDma,
int *dctlregreturn
)
{
  unsigned int dctlreg = 0x80;
  struct vmeAttr *vmeAttr;

  if(vmeDma->srcBus == VME_DMA_VME) {
    dctlreg=0;
    vmeAttr = &vmeDma->srcVmeAttr;
  } else {
    dctlreg = 0x80000000;
    vmeAttr = &vmeDma->dstVmeAttr;
  }
  
  switch(vmeAttr->maxDataWidth){
    case VME_D8:
      break;
    case VME_D16:
      dctlreg |= 0x00400000;
      break;
    case VME_D32:
      dctlreg |= 0x00800000;
      break;
    case VME_D64:
      dctlreg |= 0x00C00000;
      break;
  }

  switch(vmeAttr->addrSpace){
    case VME_A16:
      break;
    case VME_A24:
      dctlreg |= 0x00010000;
      break;
    case VME_A32:
      dctlreg |= 0x00020000;
      break;
    case VME_USER1:
      dctlreg |= 0x00060000;
      break;
    case VME_USER2:
      dctlreg |= 0x00070000;
      break;

    case VME_A64:		// not supported in Universe DMA
    case VME_CRCSR:
    case VME_USER3:
    case VME_USER4:
      return(-EINVAL);
      break;
  }
  if(vmeAttr->userAccessType == VME_PROG){
      dctlreg |= 0x00004000;
  }
  if(vmeAttr->dataAccessType == VME_SUPER){
      dctlreg |= 0x00001000;
  }
  if(vmeAttr->xferProtocol != VME_SCT){
      dctlreg |= 0x00000100;
  }
  *dctlregreturn = dctlreg;
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniStartDma
// Description: 
//-----------------------------------------------------------------------------
unsigned int
uniStartDma(
int channel,
unsigned int dgcsreg,
TDMA_Cmd_Packet *vmeLL
)
{
  unsigned int val;
    

  // Setup registers as needed for direct or chained.
  if(dgcsreg & 0x8000000){
     writel(0, vmechip_baseaddr+DTBC);
     writel((unsigned int)vmeLL, vmechip_baseaddr+DCPP);
  } else {
#if	0
printk("Starting: DGCS = %08x\n", dgcsreg);
printk("Starting: DVA  = %08x\n", readl(&vmeLL->dva));
printk("Starting: DLV  = %08x\n", readl(&vmeLL->dlv));
printk("Starting: DTBC = %08x\n", readl(&vmeLL->dtbc));
printk("Starting: DCTL = %08x\n", readl(&vmeLL->dctl));
#endif
     // Write registers 
     writel(readl(&vmeLL->dva), vmechip_baseaddr+DVA);
     writel(readl(&vmeLL->dlv), vmechip_baseaddr+DLA);
     writel(readl(&vmeLL->dtbc), vmechip_baseaddr+DTBC);
     writel(readl(&vmeLL->dctl), vmechip_baseaddr+DCTL);
     writel(0, vmechip_baseaddr+DCPP);
  }
  vmeSyncData();

  // Start the operation
  writel(dgcsreg, vmechip_baseaddr+DGCS);
  vmeSyncData();
  val = vmeGetTime();
  writel(dgcsreg | 0x8000000F, vmechip_baseaddr+DGCS);
  vmeSyncData();
  return(val);
}

//-----------------------------------------------------------------------------
// Function   : uniSetupDma
// Description: 
//-----------------------------------------------------------------------------
TDMA_Cmd_Packet *
uniSetupDma(vmeDmaPacket_t *vmeDma)
{
  vmeDmaPacket_t *vmeCur;
  int	maxPerPage;
  int	currentLLcount;
  TDMA_Cmd_Packet *startLL;
  TDMA_Cmd_Packet *currentLL;
  TDMA_Cmd_Packet *nextLL;
  unsigned int dctlreg=0;

  maxPerPage = PAGESIZE/sizeof(TDMA_Cmd_Packet)-1;
  startLL = (TDMA_Cmd_Packet *)__get_free_pages(GFP_KERNEL, 0);
  if(startLL == 0){ return(startLL); }

  // First allocate pages for descriptors and create linked list
  vmeCur = vmeDma;
  currentLL = startLL;
  currentLLcount = 0;
  while(vmeCur != 0){
     if(vmeCur->pNextPacket != 0){
        currentLL->dcpp = (unsigned int)(currentLL + 1);
        currentLLcount++;
        if(currentLLcount >= maxPerPage){
              currentLL->dcpp = __get_free_pages(GFP_KERNEL,0);
              currentLLcount = 0;
        } 
        currentLL = (TDMA_Cmd_Packet *)currentLL->dcpp;
     } else {
        currentLL->dcpp = (unsigned int)0;
     }
     vmeCur = vmeCur->pNextPacket;
  }
  
  // Next fill in information for each descriptor
  vmeCur = vmeDma;
  currentLL = startLL;
  while(vmeCur != 0){
     if(vmeCur->srcBus == VME_DMA_VME){
       writel(vmeCur->srcAddr, &currentLL->dva);
       writel(vmeCur->dstAddr, &currentLL->dlv);
     } else {
       writel(vmeCur->srcAddr, &currentLL->dlv);
       writel(vmeCur->dstAddr, &currentLL->dva);
     }
     uniSetupDctlReg(vmeCur,&dctlreg);
     writel(dctlreg, &currentLL->dctl);
     writel(vmeCur->byteCount, &currentLL->dtbc);

     currentLL = (TDMA_Cmd_Packet *)currentLL->dcpp;
     vmeCur = vmeCur->pNextPacket;
  }
  
  // Convert Links to PCI addresses.
  currentLL = startLL;
  while(currentLL != 0){
     nextLL = (TDMA_Cmd_Packet *)currentLL->dcpp;
     if(nextLL == 0) {
        writel(1, &currentLL->dcpp);
     } else {
        writel((unsigned int)virt_to_bus(nextLL), &currentLL->dcpp);
     }
     vmeFlushLine(currentLL);
     currentLL = nextLL;
  }
  
  // Return pointer to descriptors list 
  return(startLL);
}

//-----------------------------------------------------------------------------
// Function   : uniFreeDma
// Description: 
//-----------------------------------------------------------------------------
int
uniFreeDma(TDMA_Cmd_Packet *startLL)
{
  TDMA_Cmd_Packet *currentLL;
  TDMA_Cmd_Packet *prevLL;
  TDMA_Cmd_Packet *nextLL;
  unsigned int dcppreg;
  
  // Convert Links to virtual addresses.
  currentLL = startLL;
  while(currentLL != 0){
     dcppreg = readl(&currentLL->dcpp);
     dcppreg &= ~6;
     if(dcppreg & 1) {
        currentLL->dcpp = 0;
     } else {
        currentLL->dcpp = (unsigned int)bus_to_virt(dcppreg);
     }
     currentLL = (TDMA_Cmd_Packet *)currentLL->dcpp;
  }

  // Free all pages associated with the descriptors.
  currentLL = startLL;
  prevLL = currentLL;
  while(currentLL != 0){
     nextLL = (TDMA_Cmd_Packet *)currentLL->dcpp;
     if(currentLL+1 != nextLL) {
         free_pages((int)prevLL, 0);
         prevLL = nextLL;
     }
     currentLL = nextLL;
  }

  // Return pointer to descriptors list 
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uniDoDma
// Description: 
//-----------------------------------------------------------------------------
int
uniDoDma(vmeDmaPacket_t *vmeDma)
{
  unsigned int dgcsreg=0;
  unsigned int dctlreg=0;
  int val;
  char buf[100];
  int channel, x;
  vmeDmaPacket_t *curDma;
  TDMA_Cmd_Packet *dmaLL;
    
  // Sanity check the VME chain.
  channel = vmeDma->channel_number;
  if(channel > 0) { return(-EINVAL); }
  curDma = vmeDma;
  while(curDma != 0){
    if(curDma->byteCount == 0){ return(-EINVAL); }
    if(curDma->byteCount >= 0x1000000){ return(-EINVAL); }
    if((curDma->srcAddr & 7) != (curDma->dstAddr & 7)) { return(-EINVAL); }
    switch(curDma->srcBus){
      case VME_DMA_PCI:
        if(curDma->dstBus != VME_DMA_VME) { return(-EINVAL); }
      break;
      case VME_DMA_VME:
        if(curDma->dstBus != VME_DMA_PCI) { return(-EINVAL); }
      break;
      default:
        return(-EINVAL);
      break;
    }
    if(uniSetupDctlReg(curDma, &dctlreg) < 0) { 
        return(-EINVAL); 
    }

    curDma = curDma->pNextPacket;
    if(curDma == vmeDma) {          // Endless Loop!
       return(-EINVAL);
    }
  }

  // calculate control register
  if(vmeDma->pNextPacket != 0){
    dgcsreg = 0x8000000;
  } else {
    dgcsreg = 0;
  }

  for(x = 0; x < 8; x++){	// vme block size
    if((256 << x) >= vmeDma->maxVmeBlockSize) { break; }
  }
  if (x == 8) x = 7;
  dgcsreg |= (x << 20);

  if(vmeDma->vmeBackOffTimer){
    for(x = 1; x < 8; x++){	// vme timer
      if((16 << (x-1)) >= vmeDma->vmeBackOffTimer) { break; }
    }
    if (x == 8) x = 7;
    dgcsreg |= (x << 16);
  }

  // Setup the dma chain
  dmaLL = uniSetupDma(vmeDma);

  // Start the DMA
  if(dgcsreg & 0x8000000){
      vmeDma->vmeDmaStartTick = 
                      uniStartDma( channel, dgcsreg,
                                 (TDMA_Cmd_Packet *)virt_to_phys(dmaLL));
  } else {
      vmeDma->vmeDmaStartTick = 
                      uniStartDma( channel, dgcsreg, dmaLL);
  }

  wait_event_interruptible(dma_queue[0], 
                           readl(vmechip_baseaddr+DGCS) & 0x800);

  val = readl(vmechip_baseaddr+DGCS);
  writel(val | 0xF00, vmechip_baseaddr+DGCS);

  vmeDma->vmeDmaStatus = 0;
  vmeDma->vmeDmaStopTick = uniDmaIrqTime;
  if(vmeDma->vmeDmaStopTick < vmeDma->vmeDmaStartTick){
     vmeDma->vmeDmaElapsedTime = 
        (0xFFFFFFFF - vmeDma->vmeDmaStartTick) + vmeDma->vmeDmaStopTick;
  } else {
     vmeDma->vmeDmaElapsedTime = 
        vmeDma->vmeDmaStopTick - vmeDma->vmeDmaStartTick;
  }
  vmeDma->vmeDmaElapsedTime -= vmechipIrqOverHeadTicks;
  vmeDma->vmeDmaElapsedTime /= (tb_speed/1000000);

  if (!(val & 0x00000800)) {
    vmeDma->vmeDmaStatus = val & 0x700;
    sprintf(buf,"<<ca91c042>> DMA Error in DMA_uni_irqhandler DGCS=%08X\n",val);  
    printk(buf);
    val = readl(vmechip_baseaddr+DCPP);
    sprintf(buf,"<<ca91c042>> DCPP=%08X\n",val);  
    printk(buf);
    val = readl(vmechip_baseaddr+DCTL);
    sprintf(buf,"<<ca91c042>> DCTL=%08X\n",val);  
    printk(buf);
    val = readl(vmechip_baseaddr+DTBC);
    sprintf(buf,"<<ca91c042>> DTBC=%08X\n",val);  
    printk(buf);
    val = readl(vmechip_baseaddr+DLA);
    sprintf(buf,"<<ca91c042>> DLA=%08X\n",val);  
    printk(buf);
    val = readl(vmechip_baseaddr+DVA);
    sprintf(buf,"<<ca91c042>> DVA=%08X\n",val);  
    printk(buf);

  }

  // Free the dma chain
  uniFreeDma(dmaLL);
  
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : uni_shutdown
// Description: Put VME bridge in quiescent state.
//-----------------------------------------------------------------------------
void
uni_shutdown()
{
    writel(0,vmechip_baseaddr+LINT_EN);                   // Turn off Ints
  
    // Turn off the windows
    writel(0x00800000,vmechip_baseaddr+LSI0_CTL);     
    writel(0x00800000,vmechip_baseaddr+LSI1_CTL);     
    writel(0x00800000,vmechip_baseaddr+LSI2_CTL);     
    writel(0x00800000,vmechip_baseaddr+LSI3_CTL);     
    writel(0x00F00000,vmechip_baseaddr+VSI0_CTL);     
    writel(0x00F00000,vmechip_baseaddr+VSI1_CTL);     
    writel(0x00F00000,vmechip_baseaddr+VSI2_CTL);     
    writel(0x00F00000,vmechip_baseaddr+VSI3_CTL);     
    if(vmechip_revision >= 2){
      writel(0x00800000,vmechip_baseaddr+LSI4_CTL);     
      writel(0x00800000,vmechip_baseaddr+LSI5_CTL);     
      writel(0x00800000,vmechip_baseaddr+LSI6_CTL);     
      writel(0x00800000,vmechip_baseaddr+LSI7_CTL);     
      writel(0x00F00000,vmechip_baseaddr+VSI4_CTL);     
      writel(0x00F00000,vmechip_baseaddr+VSI5_CTL);     
      writel(0x00F00000,vmechip_baseaddr+VSI6_CTL);     
      writel(0x00F00000,vmechip_baseaddr+VSI7_CTL);     
    }
}

//-----------------------------------------------------------------------------
// Function   : uni_init()
// Description: 
//-----------------------------------------------------------------------------
int
uni_init()
{
    int result;
    unsigned int tmp;
    unsigned int crcsr_addr;
    unsigned int irqOverHeadStart;
    int overHeadTicks;

    uni_shutdown();

    // Write to Misc Register
    // Set VME Bus Time-out
    //   Arbitration Mode
    //   DTACK Enable
    tmp = readl(vmechip_baseaddr+MISC_CTL) & 0x0832BFFF;     
    tmp |= 0x76040000;
    writel(tmp,vmechip_baseaddr+MISC_CTL);     
    if(tmp & 0x20000){
        vme_syscon = 1;
    } else {
        vme_syscon = 0;
    }

    // Clear DMA status log
    writel(0x00000F00,vmechip_baseaddr+DGCS);     
    // Clear and enable error log
    writel(0x00800000,vmechip_baseaddr+L_CMDERR);     
    // Turn off location monitor
    writel(0x00000000,vmechip_baseaddr+LM_CTL);     

    // Initialize crcsr map
    if(vme_slotnum != -1){
      writel(vme_slotnum << 27, vmechip_baseaddr+VCSR_BS);     
    }
    crcsr_addr = readl(vmechip_baseaddr+VCSR_BS) >> 8 ;     
    writel(virt_to_bus(vmechip_interboard_data) - crcsr_addr,
      vmechip_baseaddr+VCSR_TO);     
    if(vme_slotnum != -1){
      writel(0x80000000, vmechip_baseaddr+VCSR_CTL);     
    }

    // Turn off interrupts
    writel(0x00000000,vmechip_baseaddr+LINT_EN);   // Disable interrupts in the Universe first
    writel(0x00FFFFFF,vmechip_baseaddr+LINT_STAT); // Clear Any Pending Interrupts
    writel(0x00000000,vmechip_baseaddr+VINT_EN);   // Disable interrupts in the Universe first

    result = request_irq(vmechip_irq, uni_irqhandler, SA_INTERRUPT, "VMEBus (ca91c042)", NULL);
    if (result) {
      printk("  ca91c042: can't get assigned pci irq vector %02X\n", vmechip_irq);
      return(0);
    } else {
      writel(0x0000, vmechip_baseaddr+LINT_MAP0);    // Map all ints to 0
      writel(0x0000, vmechip_baseaddr+LINT_MAP1);    // Map all ints to 0
    }

    // Enable DMA, mailbox, VIRQ & LM Interrupts
    if(vme_syscon)
      tmp = 0x00FF07FE;
    else
      tmp = 0x00FF0700;
    writel(tmp, vmechip_baseaddr+LINT_EN);

    // Do a quick sanity test of the bridge
    if(readl(vmechip_baseaddr+LINT_EN) != tmp){
      return(0);
    }
    if(readl(vmechip_baseaddr+PCI_CLASS) != 0x06800002){
      return(0);
    }
    for(tmp = 1; tmp < 0x80000000; tmp = tmp << 1){
      writel(tmp, vmechip_baseaddr+SCYC_EN);
      writel(~tmp, vmechip_baseaddr+SCYC_CMP);
      if(readl(vmechip_baseaddr+SCYC_EN) != tmp){
        return(0);
      }
      if(readl(vmechip_baseaddr+SCYC_CMP) != ~tmp){
        return(0);
      }
    }

    // do a mail box interrupt to calibrate the interrupt overhead.

    irqOverHeadStart = vmeGetTime();
    writel(0, vmechip_baseaddr+MBOX1);
    for(tmp = 0; tmp < 10; tmp++) { vmeSyncData(); }

    irqOverHeadStart = vmeGetTime();
    writel(0, vmechip_baseaddr+MBOX1);
    for(tmp = 0; tmp < 10; tmp++) { vmeSyncData(); }

    overHeadTicks = uniIrqTime - irqOverHeadStart;
    if(overHeadTicks > 0){
       vmechipIrqOverHeadTicks = overHeadTicks;
    } else {
       vmechipIrqOverHeadTicks = 1;
    }
    return(1);
}
