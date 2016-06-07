//------------------------------------------------------------------------------  
//title: Tundra TSI148 (Tempe) PCI-VME Kernel Driver
//version: Linux 3.1
//date: February 2004
//designer: Tom Armistead
//programmer: Tom Armistead
//platform: Linux 2.4.x
//------------------------------------------------------------------------------  
//  Purpose: Provide interface to the Tempe chip.
//  Docs:                                  
//------------------------------------------------------------------------------  
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
#include "tsi148.h"

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

//----------------------------------------------------------------------------
// Vars and Defines
//----------------------------------------------------------------------------
extern wait_queue_head_t dma_queue[];
extern wait_queue_head_t lm_queue;
extern wait_queue_head_t mbox_queue;

extern	unsigned int vmeGetTime();
extern	void	vmeSyncData();
extern	void	vmeFlushLine();
extern	int	tb_speed;

unsigned int	tempeIrqTime;
unsigned int	tempeDmaIrqTime[2];
unsigned int	tempeLmEvent;



//----------------------------------------------------------------------------
//  add64hi - calculate upper 32 bits of 64 bit addition operation.
//----------------------------------------------------------------------------
unsigned int
add64hi(
unsigned int lo0,
unsigned int hi0,
unsigned int lo1,
unsigned int hi1
)
{
   if((lo1 + lo0) < lo1){
      return(hi0 + hi1 + 1);
   }
   return(hi0 + hi1);
}

//----------------------------------------------------------------------------
//  sub64hi - calculate upper 32 bits of 64 bit subtraction operation
//----------------------------------------------------------------------------
int
sub64hi(
unsigned int lo0,
unsigned int hi0,
unsigned int lo1,
unsigned int hi1
)
{
   if(lo0 < lo1){
      return(hi0 - hi1 - 1);
   }
   return(hi0 - hi1);
}


//----------------------------------------------------------------------------
//  tempe_procinfo()
//----------------------------------------------------------------------------
int tempe_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  char *p;
  unsigned int x;

  p = buf;

  // Display outbound decoders
  for (x=0;x<8;x+=1) {
    p += sprintf(p,"O%d: %08X %08X:%08X %08X:%08X: %08X:%08X\n",
                 x,
                 pTempe->lcsr.outboundTranslation[x].otat,
                 pTempe->lcsr.outboundTranslation[x].otsau,
                 pTempe->lcsr.outboundTranslation[x].otsal,
                 pTempe->lcsr.outboundTranslation[x].oteau,
                 pTempe->lcsr.outboundTranslation[x].oteal,
                 pTempe->lcsr.outboundTranslation[x].otofu,
                 pTempe->lcsr.outboundTranslation[x].otofl
                 );
  }  

  p += sprintf(p,"\n");  

  *eof = 1;
  return p - buf;
}

//-----------------------------------------------------------------------------
// Function   : tempeSetupAttribute
// Description: helper function for calculating attribute register contents.
//-----------------------------------------------------------------------------
static
int
tempeSetupAttribute(
addressMode_t addrSpace, 
int userAccessType, 
int dataAccessType,
dataWidth_t maxDataWidth,
int xferProtocol,
vme2esstRate_t xferRate2esst
)
{
  int tempCtl = 0;

  // Validate & initialize address space field.
  switch(addrSpace){
    case VME_A16:
      tempCtl |= 0x0;
      break;
    case VME_A24:
      tempCtl |= 0x1;
      break;
    case VME_A32:
      tempCtl |= 0x2;
      break;
    case VME_A64:
      tempCtl |= 0x4;
      break;
    case VME_CRCSR:
      tempCtl |= 0x5;
      break;
    case VME_USER1:
      tempCtl |= 0x8;
      break;
    case VME_USER2:
      tempCtl |= 0x9;
      break;
    case VME_USER3:
      tempCtl |= 0xA;
      break;
    case VME_USER4:
      tempCtl |= 0xB;
      break;
    default:
      return(-EINVAL);
  }

  // Setup CTL register.
  if(userAccessType & VME_SUPER)
    tempCtl |= 0x0020;
  if(dataAccessType & VME_PROG)
    tempCtl |= 0x0010;
  if(maxDataWidth == VME_D16)
    tempCtl |= 0x0000;
  if(maxDataWidth == VME_D32)
    tempCtl |= 0x0040;
  switch(xferProtocol){
    case VME_SCT:
      tempCtl |= 0x000;
      break;
    case VME_BLT:
      tempCtl |= 0x100;
      break;
    case VME_MBLT:
      tempCtl |= 0x200;
      break;
    case VME_2eVME:
      tempCtl |= 0x300;
      break;
    case VME_2eSST:
      tempCtl |= 0x400;
      break;
    case VME_2eSSTB:
      tempCtl |= 0x500;
      break;
  }
  switch(xferRate2esst){
    case VME_SSTNONE:
    case VME_SST160:
      tempCtl |= 0x0000;
      break;
    case VME_SST267:
      tempCtl |= 0x800;
      break;
    case VME_SST320:
      tempCtl |= 0x1000;
      break;
  }

  return(tempCtl);
}

//----------------------------------------------------------------------------
//  tempeBusErrorChk()
//  Return zero if no VME bus error has occured, 1 otherwise.
//  Optionally, clear bus error status.
//----------------------------------------------------------------------------
int
tempeBusErrorChk(clrflag)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
    int tmp;
    tmp = pTempe->lcsr.veat;
    if (tmp & 0x80000000) {  // VES is Set
      if(clrflag)
        pTempe->lcsr.veat = 0x20000000;
      return(1);
    }
  return(0);
}


//-----------------------------------------------------------------------------
// Function   : DMA_tempe_irqhandler
// Description: Saves DMA completion timestamp and then wakes up DMA queue
//-----------------------------------------------------------------------------
void DMA_tempe_irqhandler(int channelmask)
{

  if(channelmask & 1){
      tempeDmaIrqTime[0] = tempeIrqTime;
      wake_up(&dma_queue[0]);
  }
  if(channelmask & 2){
      tempeDmaIrqTime[1] = tempeIrqTime;
      wake_up(&dma_queue[1]);
  }
}

//-----------------------------------------------------------------------------
// Function   : LERR_tempe_irqhandler
// Description: Display error & status message when LERR (PCI) exception
//    interrupt occurs.
//-----------------------------------------------------------------------------
void LERR_tempe_irqhandler(void)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  printk(" VME PCI Exception at address: 0x%08x:%08x, attributes: %08x\n",
     pTempe->lcsr.edpau, pTempe->lcsr.edpal, pTempe->lcsr.edpat);
  printk(" PCI-X attribute reg: %08x, PCI-X split completion reg: %08x\n",
     pTempe->lcsr.edpxa, pTempe->lcsr.edpxs);
  pTempe->lcsr.edpat = 0x20000000;
  vmeSyncData();
}

//-----------------------------------------------------------------------------
// Function   : VERR_tempe_irqhandler
// Description:  Display error & status when VME error interrupt occurs.
// Not normally enabled or used.  tempeBusErrorChk() is used instead.
//-----------------------------------------------------------------------------
void VERR_tempe_irqhandler(void)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  printk(" VME Exception at address: 0x%08x:%08x, attributes: %08x\n",
     pTempe->lcsr.veau, pTempe->lcsr.veal, pTempe->lcsr.veat);
  pTempe->lcsr.veat = 0x20000000;
  vmeSyncData();
}

//-----------------------------------------------------------------------------
// Function   : MB_tempe_irqhandler
// Description: Wake up mail box queue.
//-----------------------------------------------------------------------------
void MB_tempe_irqhandler(mboxMask)
{
  if(vmechipIrqOverHeadTicks != 0){
     wake_up(&mbox_queue);
  }
}

//-----------------------------------------------------------------------------
// Function   : LM_tempe_irqhandler
// Description: Wake up location monitor queue
//-----------------------------------------------------------------------------
void LM_tempe_irqhandler(lmMask)
{
  tempeLmEvent = lmMask;
  wake_up(&lm_queue);
}

//-----------------------------------------------------------------------------
// Function   : VIRQ_tempe_irqhandler
// Description: Record the level & vector from the VME bus interrupt.
//-----------------------------------------------------------------------------
void VIRQ_tempe_irqhandler(virqMask)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int iackvec, i;

  for(i = 7; i > 0; i--){
    if(virqMask & (1 << i)){
       iackvec = pTempe->lcsr.viack[(i*4)+3];
       vme_irqlog[i][iackvec]++;
    }
  }
}

//-----------------------------------------------------------------------------
// Function   : tempe_irqhandler
// Description: Top level interrupt handler.  Clears appropriate interrupt
// status bits and then calls appropriate sub handler(s).
//-----------------------------------------------------------------------------
static void tempe_irqhandler(int irq, void *dev_id, struct pt_regs *regs)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  long stat, enable;

  // Save time that this IRQ occurred at.
  tempeIrqTime = vmeGetTime();

  // Determine which interrupts are unmasked and active.  
  enable = pTempe->lcsr.inteo;
  stat = pTempe->lcsr.ints;
  stat = stat & enable;

  // Clear them.
  pTempe->lcsr.intc = stat;
  vmeSyncData();

  // Call subhandlers as appropriate
  if (stat & 0x03000000)	// DMA irqs
    DMA_tempe_irqhandler((stat & 0x03000000) >> 24);
  if (stat & 0x02000)		// PCI bus error.
    LERR_tempe_irqhandler();
  if (stat & 0x01000)		// VME bus error.
    VERR_tempe_irqhandler();
  if (stat & 0xF0000)		// Mail box irqs
    MB_tempe_irqhandler((stat & 0xF0000) >> 16);
  if (stat & 0xF00000)		// Location monitor irqs.
    LM_tempe_irqhandler((stat & 0xF00000) >> 20);
  if (stat & 0x0000FE)		// VME bus irqs.
    VIRQ_tempe_irqhandler(stat & 0x0000FE);
}

//-----------------------------------------------------------------------------
// Function   : tempeGenerateIrq
// Description: Generate a VME bus interrupt at the requested level & vector.
// Wait for system controller to ack the interrupt.
//-----------------------------------------------------------------------------
int
tempeGenerateIrq( virqInfo_t *vmeIrq)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int timeout;
  int looptimeout;
  unsigned int tmp;

  timeout = vmeIrq->waitTime;
  if(timeout == 0){
     timeout++;		// Wait at least 1 tick...
  }
  looptimeout = HZ/20;	// try for 1/20 second

  vmeIrq->timeOutFlag = 0;

  // Validate & setup vector register.
  tmp = pTempe->lcsr.vicr;
  tmp &= ~0xFF;
  pTempe->lcsr.vicr = tmp | vmeIrq->vector;
  vmeSyncData();

  // Assert VMEbus IRQ
  pTempe->lcsr.vicr = tmp | (vmeIrq->level << 8) |vmeIrq->vector;
  vmeSyncData();

  // Wait for syscon to do iack
  while( pTempe->lcsr.vicr & 0x800 ){
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(looptimeout);
    timeout = timeout - looptimeout;
    if(timeout <= 0){
       vmeIrq->timeOutFlag = 1;
       break;
    }
  }

  return(0);
}


//-----------------------------------------------------------------------------
// Function   : tempeSetArbiter
// Description: Set the VME bus arbiter with the requested attributes
//-----------------------------------------------------------------------------
int
tempeSetArbiter( vmeArbiterCfg_t *vmeArb)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  int gto = 0;

  tempCtl = pTempe->lcsr.vctrl;
  tempCtl &= 0xFFEFFF00;

  if(vmeArb->globalTimeoutTimer == 0xFFFFFFFF){
     gto = 8;
  } else if (vmeArb->globalTimeoutTimer > 2048) {
     return(-EINVAL);
  } else if (vmeArb->globalTimeoutTimer == 0) {
     gto = 0;
  } else {
     gto = 1;
     while((16 * (1 << (gto-1))) < vmeArb->globalTimeoutTimer){
        gto += 1;
     }
  } 
  tempCtl |= gto;

  if(vmeArb->arbiterMode != VME_PRIORITY_MODE){
    tempCtl |= 1 << 6;
  }
  
  if(vmeArb->arbiterTimeoutFlag){
    tempCtl |= 1 << 7;
  }

  if(vmeArb->noEarlyReleaseFlag){
    tempCtl |= 1 << 20;
  }
  pTempe->lcsr.vctrl = tempCtl;
  vmeSyncData();

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeGetArbiter
// Description: Return the attributes of the VME bus arbiter.
//-----------------------------------------------------------------------------
int
tempeGetArbiter( vmeArbiterCfg_t *vmeArb)
{
    tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  int gto = 0;

  tempCtl = pTempe->lcsr.vctrl;

  gto = tempCtl & 0xF;
  if(gto != 0){
    vmeArb->globalTimeoutTimer = (16 * (1 << (gto-1)));
  }

  if(tempCtl & (1 << 6)){
     vmeArb->arbiterMode = VME_R_ROBIN_MODE;
  } else {
     vmeArb->arbiterMode = VME_PRIORITY_MODE;
  }
  
  if(tempCtl & (1 << 7)){
    vmeArb->arbiterTimeoutFlag = 1;
  }

  if(tempCtl & (1 << 20)){
    vmeArb->noEarlyReleaseFlag = 1;
  }

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeSetRequestor
// Description: Set the VME bus requestor with the requested attributes
//-----------------------------------------------------------------------------
int
tempeSetRequestor( vmeRequesterCfg_t *vmeReq)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;

  tempCtl = pTempe->lcsr.vmctrl;
  tempCtl &= 0xFFFF0000;

  if(vmeReq->releaseMode == 1){
    tempCtl |=  (1 << 3);
  }

  if(vmeReq->fairMode == 1){
    tempCtl |=  (1 << 2);
  } 

  tempCtl |= (vmeReq->timeonTimeoutTimer & 7) << 8;
  tempCtl |= (vmeReq->timeoffTimeoutTimer & 7) << 12;
  tempCtl |= vmeReq->requestLevel;

  pTempe->lcsr.vmctrl = tempCtl;
  vmeSyncData();
  return(0);
}


//-----------------------------------------------------------------------------
// Function   : tempeGetRequestor
// Description: Return the attributes of the VME bus requestor
//-----------------------------------------------------------------------------
int
tempeGetRequestor( vmeRequesterCfg_t *vmeReq)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;

  tempCtl = pTempe->lcsr.vmctrl;

  if(tempCtl & 0x18){
    vmeReq->releaseMode = 1;
  }

  if(tempCtl & (1 << 2)){
    vmeReq->fairMode = 1;
  }

  vmeReq->requestLevel = tempCtl & 3;
  vmeReq->timeonTimeoutTimer  = (tempCtl >> 8) & 7;
  vmeReq->timeoffTimeoutTimer  = (tempCtl >> 12) & 7;

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeSetInBound
// Description: Initialize an inbound window with the requested attributes.
//-----------------------------------------------------------------------------
int
tempeSetInBound( vmeInWindowCfg_t *vmeIn)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  unsigned int i,x, granularity = 0x10000;

  // Verify input data
  if(vmeIn->windowNbr > 7){
    return(-EINVAL);
  }
  i = vmeIn->windowNbr;

  switch(vmeIn->addrSpace){
    case VME_CRCSR:
    case VME_USER1:
    case VME_USER2:
    case VME_USER3:
    case VME_USER4:
      return(-EINVAL);
    case VME_A16:
      granularity = 0x10;
      tempCtl |= 0x00;
      break;
    case VME_A24:
      granularity = 0x1000;
      tempCtl |= 0x10;
      break;
    case VME_A32:
      granularity = 0x10000;
      tempCtl |= 0x20;
      break;
    case VME_A64:
      granularity = 0x10000;
      tempCtl |= 0x40;
      break;
  }

  if((vmeIn->vmeAddrL & (granularity - 1)) ||
     (vmeIn->windowSizeL & (granularity - 1)) ||
     (vmeIn->pciAddrL & (granularity - 1))){
    return(-EINVAL);
  }

  // Disable while we are mucking around
  pTempe->lcsr.inboundTranslation[i].itat = 0;
  vmeSyncData();

  pTempe->lcsr.inboundTranslation[i].itsal = vmeIn->vmeAddrL;
  pTempe->lcsr.inboundTranslation[i].itsau = vmeIn->vmeAddrU;
  pTempe->lcsr.inboundTranslation[i].iteal = 
            vmeIn->vmeAddrL + vmeIn->windowSizeL - granularity;

  pTempe->lcsr.inboundTranslation[i].iteau = add64hi(
            vmeIn->vmeAddrL, vmeIn->vmeAddrU, 
            vmeIn->windowSizeL-granularity, vmeIn->windowSizeU); 
  pTempe->lcsr.inboundTranslation[i].itofl = 
                           vmeIn->pciAddrL - vmeIn->vmeAddrL;
  pTempe->lcsr.inboundTranslation[i].itofu = sub64hi(
            vmeIn->pciAddrL, vmeIn->pciAddrU, vmeIn->vmeAddrL, vmeIn->vmeAddrU);

  // Setup CTL register.
  tempCtl |= (vmeIn->xferProtocol & 0x3E) << 6;
  for(x = 0; x < 4; x++){
    if((64 << x) >= vmeIn->prefetchSize){
       break;
    }
  }
  if(x == 4) x--;
  tempCtl |= (x << 16);

  if(vmeIn->prefetchThreshold)

  if(vmeIn->prefetchThreshold)
    tempCtl |= 0x40000;

  switch(vmeIn->xferRate2esst){
    case VME_SSTNONE:
      break;
    case VME_SST160:
      tempCtl |= 0x0400;
      break;
    case VME_SST267:
      tempCtl |= 0x1400;
      break;
    case VME_SST320:
      tempCtl |= 0x2400;
      break;
  }

  if(vmeIn->userAccessType & VME_USER)
    tempCtl |= 0x4;
  if(vmeIn->userAccessType & VME_SUPER)
    tempCtl |= 0x8;
  if(vmeIn->dataAccessType & VME_DATA)
    tempCtl |= 0x1;
  if(vmeIn->dataAccessType & VME_PROG)
    tempCtl |= 0x2;

  // Write ctl reg without enable
  pTempe->lcsr.inboundTranslation[i].itat = tempCtl;
  vmeSyncData();

  if(vmeIn->windowEnable)
    tempCtl |= 0x80000000;
 
  pTempe->lcsr.inboundTranslation[i].itat = tempCtl;
  vmeSyncData();

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeGetInBound
// Description: Return the attributes of an inbound window.
//-----------------------------------------------------------------------------
int
tempeGetInBound( vmeInWindowCfg_t *vmeIn)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  unsigned int i, vmeEndU, vmeEndL;

  // Verify input data
  if(vmeIn->windowNbr > 7){
    return(-EINVAL);
  }
  i = vmeIn->windowNbr;

  tempCtl = pTempe->lcsr.inboundTranslation[i].itat;

  // Get Control & BUS attributes
  if(tempCtl & 0x80000000)
     vmeIn->windowEnable = 1;
  vmeIn->xferProtocol = ((tempCtl & 0xF8) >> 6) | VME_SCT;
  vmeIn->prefetchSize = 64 << ((tempCtl >> 16) & 3);
  vmeIn->wrPostEnable = 1;
  vmeIn->prefetchEnable = 1;
  if(tempCtl & 0x40000)
     vmeIn->prefetchThreshold = 1;
  if(tempCtl & 0x4)
    vmeIn->userAccessType |= VME_USER;
  if(tempCtl & 0x8)
    vmeIn->userAccessType |= VME_SUPER;
  if(tempCtl & 0x1)
    vmeIn->dataAccessType |= VME_DATA;
  if(tempCtl & 0x2)
    vmeIn->dataAccessType |= VME_PROG;

  switch((tempCtl & 0x70) >> 4) {
    case 0x0:
      vmeIn->addrSpace = VME_A16;
      break;
    case 0x1:
      vmeIn->addrSpace = VME_A24;
      break;
    case 0x2:
      vmeIn->addrSpace = VME_A32;
      break;
    case 0x4:
      vmeIn->addrSpace = VME_A64;
      break;
  }

  switch((tempCtl & 0x7000) >> 12) {
    case 0x0:
      vmeIn->xferRate2esst = VME_SST160;
      break;
    case 0x1:
      vmeIn->xferRate2esst = VME_SST267;
      break;
    case 0x2:
      vmeIn->xferRate2esst = VME_SST320;
      break;
  }

  // Get VME inbound start & end addresses
  vmeIn->vmeAddrL =  pTempe->lcsr.inboundTranslation[i].itsal;
  vmeIn->vmeAddrU =  pTempe->lcsr.inboundTranslation[i].itsau;
  vmeEndL =  pTempe->lcsr.inboundTranslation[i].iteal;
  vmeEndU =  pTempe->lcsr.inboundTranslation[i].iteau;

  // Adjust addresses for window granularity
  switch(vmeIn->addrSpace){
      case VME_A16:
          vmeIn->vmeAddrU = 0;
          vmeIn->vmeAddrL &= 0x0000FFF0;
          vmeEndU =  0;
          vmeEndL &=  0x0000FFF0;
          vmeEndL |= 0x10;
      break;
      case VME_A24:
          vmeIn->vmeAddrU = 0;
          vmeIn->vmeAddrL &= 0x00FFF000;
          vmeEndU =  0;
          vmeEndL &=  0x00FFF000;
          vmeEndL |= 0x1000;
      break;
      case VME_A32:
          vmeIn->vmeAddrU = 0;
          vmeIn->vmeAddrL &= 0xFFFF0000;
          vmeEndU =  0;
          vmeEndL &=  0xFFFF0000;
          vmeEndL |= 0x1000;
      default:
          vmeIn->vmeAddrL &= 0xFFFF0000;
          vmeEndL &=  0xFFFF0000;
          vmeEndL |= 0x10000;
      break;
  }

  // Calculate size of window.
  vmeIn->windowSizeL = vmeEndL - vmeIn->vmeAddrL;
  vmeIn->windowSizeU = sub64hi(
                         vmeEndL, vmeEndU,
                         vmeIn->vmeAddrL, vmeIn->vmeAddrU
                       );

  // Calculate coorelating PCI bus address
  vmeIn->pciAddrL = vmeIn->vmeAddrL + pTempe->lcsr.inboundTranslation[i].itofl;
  vmeIn->pciAddrU = add64hi(
                         vmeIn->vmeAddrL, vmeIn->vmeAddrU,
                         pTempe->lcsr.inboundTranslation[i].itofl,
                         pTempe->lcsr.inboundTranslation[i].itofu
                       );

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeSetOutBound
// Description: Set the attributes of an outbound window.
//-----------------------------------------------------------------------------
int
tempeSetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  unsigned int i,x;

  // Verify input data
  if(vmeOut->windowNbr > 7){
    return(-EINVAL);
  }
  i = vmeOut->windowNbr;

  if((vmeOut->xlatedAddrL & 0xFFFF) ||
     (vmeOut->windowSizeL & 0xFFFF) ||
     (vmeOut->pciBusAddrL & 0xFFFF)){
    return(-EINVAL);
  }

  tempCtl = tempeSetupAttribute(
                         vmeOut->addrSpace, 
                         vmeOut->userAccessType, 
                         vmeOut->dataAccessType,
                         vmeOut->maxDataWidth,
                         vmeOut->xferProtocol,
                         vmeOut->xferRate2esst
             );

  if(vmeOut->prefetchEnable){
    tempCtl |= 0x40000;
    for(x = 0; x < 4; x++){
      if((2 << x) >= vmeOut->prefetchSize) break;
    }
    if(x == 4) x=3;
    tempCtl |= (x << 16);
  }

  // Disable while we are mucking around
  pTempe->lcsr.outboundTranslation[i].otat = 0;
  vmeSyncData();

  pTempe->lcsr.outboundTranslation[i].otbs = vmeOut->bcastSelect2esst;
  pTempe->lcsr.outboundTranslation[i].otsal = vmeOut->pciBusAddrL;
  pTempe->lcsr.outboundTranslation[i].otsau = vmeOut->pciBusAddrU;
  pTempe->lcsr.outboundTranslation[i].oteal = 
    vmeOut->pciBusAddrL + (vmeOut->windowSizeL-0x10000);
  pTempe->lcsr.outboundTranslation[i].oteau = add64hi(
    vmeOut->pciBusAddrL, vmeOut->pciBusAddrU, 
    vmeOut->windowSizeL-0x10000, vmeOut->windowSizeU);
  pTempe->lcsr.outboundTranslation[i].otofl = 
       vmeOut->xlatedAddrL - vmeOut->pciBusAddrL;
  if(vmeOut->addrSpace == VME_A64) {
    pTempe->lcsr.outboundTranslation[i].otofu = sub64hi(
      vmeOut->xlatedAddrL, vmeOut->xlatedAddrU,
      vmeOut->pciBusAddrL, vmeOut->pciBusAddrU);
  } else {
    pTempe->lcsr.outboundTranslation[i].otofu = 0;
  }
#if	0
  printk("Configuring window #%d: ctl = %08x\n", i, tempCtl);
  printk("otbs %08x\n", pTempe->lcsr.outboundTranslation[i].otbs);
  printk("otsal %08x\n", pTempe->lcsr.outboundTranslation[i].otsal);
  printk("otsau %08x\n", pTempe->lcsr.outboundTranslation[i].otsau);
  printk("oteal %08x\n", pTempe->lcsr.outboundTranslation[i].oteal);
  printk("oteau %08x\n", pTempe->lcsr.outboundTranslation[i].oteau);
  printk("otofl %08x\n", pTempe->lcsr.outboundTranslation[i].otofl);
  printk("otofu %08x\n", pTempe->lcsr.outboundTranslation[i].otofu);
#endif



  // Write ctl reg without enable
  pTempe->lcsr.outboundTranslation[i].otat = tempCtl;
  vmeSyncData();

  if(vmeOut->windowEnable)
    tempCtl |= 0x80000000;
 
  pTempe->lcsr.outboundTranslation[i].otat = tempCtl;
  vmeSyncData();

  return(0);
}


//-----------------------------------------------------------------------------
// Function   : tempeGetOutBound
// Description: Return the attributes of an outbound window.
//-----------------------------------------------------------------------------
int
tempeGetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  unsigned int i;

  // Verify input data
  if(vmeOut->windowNbr > 7){
    return(-EINVAL);
  }
  i = vmeOut->windowNbr;

  // Get Control & BUS attributes
  tempCtl = pTempe->lcsr.outboundTranslation[i].otat;
  vmeOut->wrPostEnable = 1;
  if(tempCtl & 0x0020)
    vmeOut->userAccessType = VME_SUPER;
  else
    vmeOut->userAccessType = VME_USER;
  if(tempCtl & 0x0010)
    vmeOut->dataAccessType = VME_PROG;
  else 
    vmeOut->dataAccessType = VME_DATA;
  if(tempCtl & 0x80000000)
     vmeOut->windowEnable = 1;

  switch((tempCtl & 0xC0) >> 6){
    case 0:
      vmeOut->maxDataWidth = VME_D16;
      break;
    case 1:
      vmeOut->maxDataWidth = VME_D32;
      break;
  }
  vmeOut->xferProtocol = 1 << ((tempCtl >> 8) & 7);

  switch(tempCtl & 0xF){
    case 0x0:
      vmeOut->addrSpace = VME_A16;
      break;
    case 0x1:
      vmeOut->addrSpace = VME_A24;
      break;
    case 0x2:
      vmeOut->addrSpace = VME_A32;
      break;
    case 0x4:
      vmeOut->addrSpace = VME_A64;
      break;
    case 0x5:
      vmeOut->addrSpace = VME_CRCSR;
      break;
    case 0x8:
      vmeOut->addrSpace = VME_USER1;
      break;
    case 0x9:
      vmeOut->addrSpace = VME_USER2;
      break;
    case 0xA:
      vmeOut->addrSpace = VME_USER3;
      break;
    case 0xB:
      vmeOut->addrSpace = VME_USER4;
      break;
  }

  vmeOut->xferRate2esst = VME_SSTNONE;
  if(vmeOut->xferProtocol == VME_2eSST){

    switch(tempCtl & 0x1800){
      case 0x000:
        vmeOut->xferRate2esst = VME_SST160;
        break;
      case 0x800:
        vmeOut->xferRate2esst = VME_SST267;
        break;
      case 0x1000:
        vmeOut->xferRate2esst = VME_SST320;
        break;
    }

  }

  // Get Window mappings.

  vmeOut->bcastSelect2esst = pTempe->lcsr.outboundTranslation[i].otbs;
  vmeOut->pciBusAddrL = pTempe->lcsr.outboundTranslation[i].otsal;
  vmeOut->pciBusAddrU = pTempe->lcsr.outboundTranslation[i].otsau;

  vmeOut->windowSizeL =
      (pTempe->lcsr.outboundTranslation[i].oteal + 0x10000) - 
                             vmeOut->pciBusAddrL;
  vmeOut->windowSizeU = sub64hi(
       pTempe->lcsr.outboundTranslation[i].oteal + 0x10000,
       pTempe->lcsr.outboundTranslation[i].oteau,
       vmeOut->pciBusAddrL, 
       vmeOut->pciBusAddrU); 
  vmeOut->xlatedAddrL = pTempe->lcsr.outboundTranslation[i].otofl + 
       vmeOut->pciBusAddrL;
  if(vmeOut->addrSpace == VME_A64) {
    vmeOut->xlatedAddrU = add64hi(
         pTempe->lcsr.outboundTranslation[i].otofl,
         pTempe->lcsr.outboundTranslation[i].otofu,
         vmeOut->pciBusAddrL, 
         vmeOut->pciBusAddrU); 
  } else {
    vmeOut->xlatedAddrU = 0;
  }

  return(0);
}


//-----------------------------------------------------------------------------
// Function   : tempeSetupLm
// Description: Set the attributes of the location monitor
//-----------------------------------------------------------------------------
int
tempeSetupLm( vmeLmCfg_t *vmeLm)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;

  // Setup CTL register.
  switch(vmeLm->addrSpace){
    case VME_A16:
      tempCtl |= 0x00;
      break;
    case VME_A24:
      tempCtl |= 0x10;
      break;
    case VME_A32:
      tempCtl |= 0x20;
      break;
    case VME_A64:
      tempCtl |= 0x40;
      break;
    default:
      return(-EINVAL);
  }
  if(vmeLm->userAccessType & VME_USER)
    tempCtl |= 0x4;
  if(vmeLm->userAccessType & VME_SUPER)
    tempCtl |= 0x8;
  if(vmeLm->dataAccessType & VME_DATA)
    tempCtl |= 0x1;
  if(vmeLm->dataAccessType & VME_PROG)
    tempCtl |= 0x2;

  // Disable while we are mucking around
  pTempe->lcsr.lmat = 0;
  vmeSyncData();

  pTempe->lcsr.lmbal = vmeLm->addr;
  pTempe->lcsr.lmbau = vmeLm->addrU;


  tempeLmEvent = 0;
  // Write ctl reg and enable
  pTempe->lcsr.lmat = tempCtl | 0x80;
  vmeSyncData();

  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeWaitLm
// Description: Wait for location monitor to be triggered.
//-----------------------------------------------------------------------------
int
tempeWaitLm( vmeLmCfg_t *vmeLm)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  if(tempeLmEvent == 0){
    interruptible_sleep_on_timeout(&lm_queue, vmeLm->lmWait);
  }
  pTempe->lcsr.lmat = 0;
  vmeSyncData();
  vmeLm->lmEvents = tempeLmEvent;
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempeDoRmw
// Description: Perform an RMW cycle on the VME bus.
//    A VME outbound window must already be setup which maps to the desired 
//    RMW address.
//-----------------------------------------------------------------------------
int
tempeDoRmw( vmeRmwCfg_t *vmeRmw)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  int tempCtl = 0;
  unsigned int vmeEndU, vmeEndL;
  int *rmwPciDataPtr = NULL;
  int *vaDataPtr = NULL;
  int i;
  vmeOutWindowCfg_t vmeOut;

  if(vmeRmw->maxAttempts < 1){
    return(-EINVAL);
  }

  // Find the PCI address that maps to the desired VME address 
  for(i = 0; i < 7; i++){
    tempCtl = pTempe->lcsr.outboundTranslation[i].otat;
    if((tempCtl & 0x80000000) == 0){
      continue;
    }
    memset(&vmeOut, 0, sizeof(vmeOut));
    vmeOut.windowNbr = i;
    tempeGetOutBound(&vmeOut);
    if(vmeOut.addrSpace != vmeRmw->addrSpace) {
       continue;
    }
    if(vmeOut.xlatedAddrU > vmeRmw->targetAddrU) {
      continue;
    }
    if(vmeOut.xlatedAddrU == vmeRmw->targetAddrU) {
      if(vmeOut.xlatedAddrL > vmeRmw->targetAddr) {
        continue;
      }
    }
    vmeEndL = vmeOut.xlatedAddrL + vmeOut.windowSizeL;
    vmeEndU = add64hi(vmeOut.xlatedAddrL, vmeOut.xlatedAddrU,
                      vmeOut.windowSizeL, vmeOut.windowSizeU);
    if(sub64hi(vmeEndL, vmeEndU, 
               vmeRmw->targetAddr, vmeRmw->targetAddrU) >= 0){
         rmwPciDataPtr = (int *)(vmeOut.pciBusAddrL+ (vmeRmw->targetAddr - vmeOut.xlatedAddrL));
         vaDataPtr = (int *)(out_image_va[i] + (vmeRmw->targetAddr - vmeOut.xlatedAddrL));
         break;
    }
  }
     
  // If no window - fail.
  if(rmwPciDataPtr == NULL){
    return(-EINVAL);
  }

  // Setup the RMW registers.
  pTempe->lcsr.vmctrl &= ~0x100000;
  vmeSyncData();
  pTempe->lcsr.rmwen = vmeRmw->enableMask;
  pTempe->lcsr.rmwc = vmeRmw->compareData;
  pTempe->lcsr.rmws = vmeRmw->swapData;
  pTempe->lcsr.rmwau = 0;
  pTempe->lcsr.rmwal = (int)rmwPciDataPtr;
  pTempe->lcsr.vmctrl |= 0x100000;
  vmeSyncData();

  // Run the RMW cycle until either success or max attempts.
  vmeRmw->numAttempts = 1;
  while(vmeRmw->numAttempts <= vmeRmw->maxAttempts){

    if((*vaDataPtr & vmeRmw->enableMask) == 
       (vmeRmw->swapData & vmeRmw->enableMask)){

          break;
 
    }
    vmeRmw->numAttempts++;
  }

  pTempe->lcsr.vmctrl &= ~0x100000;
  vmeSyncData();

  // If no success, set num Attempts to be greater than max attempts
  if(vmeRmw->numAttempts > vmeRmw->maxAttempts){
    vmeRmw->numAttempts = vmeRmw->maxAttempts +1;
  }

  return(0);
}


//-----------------------------------------------------------------------------
// Function   : dmaSrcAttr, dmaDstAttr
// Description: Helper functions which setup common portions of the 
//      DMA source and destination attribute registers.
//-----------------------------------------------------------------------------
int
dmaSrcAttr( vmeDmaPacket_t *vmeCur)
{
  
  int dsatreg = 0;
  // calculate source attribute register
  switch(vmeCur->srcBus){
    case VME_DMA_PATTERN_BYTE:
        dsatreg = 0x23000000;
        break;
    case VME_DMA_PATTERN_BYTE_INCREMENT:
        dsatreg = 0x22000000;
        break;
    case VME_DMA_PATTERN_WORD:
        dsatreg = 0x21000000;
        break;
    case VME_DMA_PATTERN_WORD_INCREMENT:
        dsatreg = 0x20000000;
        break;
    case VME_DMA_PCI:
        dsatreg = 0x00000000;
        break;
    case VME_DMA_VME:
        dsatreg = 0x10000000;
        dsatreg |= tempeSetupAttribute(
             vmeCur->srcVmeAttr.addrSpace,
             vmeCur->srcVmeAttr.userAccessType,
             vmeCur->srcVmeAttr.dataAccessType,
             vmeCur->srcVmeAttr.maxDataWidth,
             vmeCur->srcVmeAttr.xferProtocol,
             vmeCur->srcVmeAttr.xferRate2esst);
        break;
    default:
        dsatreg = -EINVAL;
        break;
  }
  return(dsatreg); 
}

int
dmaDstAttr( vmeDmaPacket_t *vmeCur)
{
  int ddatreg = 0;
  // calculate destination attribute register
  switch(vmeCur->dstBus){
    case VME_DMA_PCI:
        ddatreg = 0x00000000;
        break;
    case VME_DMA_VME:
        ddatreg = 0x10000000;
        ddatreg |= tempeSetupAttribute(
             vmeCur->dstVmeAttr.addrSpace,
             vmeCur->dstVmeAttr.userAccessType,
             vmeCur->dstVmeAttr.dataAccessType,
             vmeCur->dstVmeAttr.maxDataWidth,
             vmeCur->dstVmeAttr.xferProtocol,
             vmeCur->dstVmeAttr.xferRate2esst);
        break;
    default:
        ddatreg = -EINVAL;
        break;
  }
  return(ddatreg);
}

//-----------------------------------------------------------------------------
// Function   : tempeStartDma
// Description: Write the DMA controller registers with the contents
//    needed to actually start the DMA operation.
//    returns starting time stamp.
//
//    Starts either direct or chained mode as appropriate (based on dctlreg)
//-----------------------------------------------------------------------------
unsigned int
tempeStartDma(
int channel,
unsigned int dctlreg,
tsi148DmaDescriptor_t *vmeLL
)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  unsigned int val;
    
  // Setup registers as needed for direct or chained.
  if(dctlreg & 0x800000){

#if	0
printk("dsal = %08x\n", vmeLL->dsal);
printk("deal = %08x\n", vmeLL->ddal);
printk("dsat = %08x\n", vmeLL->dsat);
printk("ddat = %08x\n", vmeLL->ddat);
printk("dctl = %08x\n", dctlreg);
#endif

     // Write registers 
     pTempe->lcsr.dma[channel].dsau = vmeLL->dsau;
     pTempe->lcsr.dma[channel].dsal = vmeLL->dsal;
     pTempe->lcsr.dma[channel].ddau = vmeLL->ddau;
     pTempe->lcsr.dma[channel].ddal = vmeLL->ddal;
     pTempe->lcsr.dma[channel].dsat = vmeLL->dsat;
     pTempe->lcsr.dma[channel].ddat = vmeLL->ddat;
     pTempe->lcsr.dma[channel].dcnt = vmeLL->dcnt;
     pTempe->lcsr.dma[channel].ddbs = vmeLL->ddbs;
  } else {
     pTempe->lcsr.dma[channel].dnlau = 0;
     pTempe->lcsr.dma[channel].dnlal = (unsigned int)vmeLL;
  }
  vmeSyncData();

  // Start the operation
  pTempe->lcsr.dma[channel].dctl = dctlreg | 0x2000000;
  val = vmeGetTime();
  vmeSyncData();
  return(val);
}

//-----------------------------------------------------------------------------
// Function   : tempeSetupDma
// Description: Create a linked list (possibly only 1 element long) of 
//   Tempe DMA descriptors which will perform the DMA operation described
//   by the DMA packet.  Flush descriptors from cache.
//   Return pointer to beginning of list (or 0 on failure.).
//-----------------------------------------------------------------------------
tsi148DmaDescriptor_t *
tempeSetupDma(vmeDmaPacket_t *vmeDma)
{
  vmeDmaPacket_t *vmeCur;
  int	maxPerPage;
  int	currentLLcount;
  tsi148DmaDescriptor_t *startLL;
  tsi148DmaDescriptor_t *currentLL;
  tsi148DmaDescriptor_t *nextLL;
  

  maxPerPage = PAGESIZE/sizeof(tsi148DmaDescriptor_t)-1;
  startLL = (tsi148DmaDescriptor_t *)__get_free_pages(GFP_KERNEL, 0);
  if(startLL == 0){ return(startLL); }

  // First allocate pages for descriptors and create linked list
  vmeCur = vmeDma;
  currentLL = startLL;
  currentLLcount = 0;
  while(vmeCur != 0){
     if(vmeCur->pNextPacket != 0){
        currentLL->dnlau = (unsigned int)0;
        currentLL->dnlal = (unsigned int)(currentLL + 1);
        currentLLcount++;
        if(currentLLcount >= maxPerPage){
              currentLL->dnlal = __get_free_pages(GFP_KERNEL,0);
              currentLLcount = 0;
        } 
        currentLL = (tsi148DmaDescriptor_t *)currentLL->dnlal;
     } else {
        currentLL->dnlau = (unsigned int)0;
        currentLL->dnlal = (unsigned int)0;
     }
     vmeCur = vmeCur->pNextPacket;
  }
  
  // Next fill in information for each descriptor
  vmeCur = vmeDma;
  currentLL = startLL;
  while(vmeCur != 0){
     currentLL->dsau = vmeCur->srcAddrU;
     currentLL->dsal = vmeCur->srcAddr;
     currentLL->ddau = vmeCur->dstAddrU;
     currentLL->ddal = vmeCur->dstAddr;
     currentLL->dsat = dmaSrcAttr(vmeCur);
     currentLL->ddat = dmaDstAttr(vmeCur);
     currentLL->dcnt = vmeCur->byteCount;
     currentLL->ddbs = vmeCur->bcastSelect2esst;

     currentLL = (tsi148DmaDescriptor_t *)currentLL->dnlal;
     vmeCur = vmeCur->pNextPacket;
  }
  
  // Convert Links to PCI addresses.
  currentLL = startLL;
  while(currentLL != 0){
     nextLL = (tsi148DmaDescriptor_t *)currentLL->dnlal;
     currentLL->dnlal = (unsigned int)virt_to_bus(nextLL);
     if(nextLL == 0) currentLL->dnlal |= 1;
     vmeFlushLine(currentLL);
     vmeFlushLine(((char *)currentLL)+LINESIZE);
     currentLL = nextLL;
  }
  
  // Return pointer to descriptors list 
  return(startLL);
}

//-----------------------------------------------------------------------------
// Function   : tempeFreeDma
// Description: Free all memory that is used to hold the DMA 
//    descriptor linked list.  
//
//-----------------------------------------------------------------------------
int
tempeFreeDma(tsi148DmaDescriptor_t *startLL)
{
  tsi148DmaDescriptor_t *currentLL;
  tsi148DmaDescriptor_t *prevLL;
  tsi148DmaDescriptor_t *nextLL;
  
  // Convert Links to virtual addresses.
  currentLL = startLL;
  while(currentLL != 0){
     if(currentLL->dnlal & 1) {
        currentLL->dnlal = 0;
     } else {
        currentLL->dnlal = (unsigned int)bus_to_virt(currentLL->dnlal);
     }
     currentLL = (tsi148DmaDescriptor_t *)currentLL->dnlal;
  }

  // Free all pages associated with the descriptors.
  currentLL = startLL;
  prevLL = currentLL;
  while(currentLL != 0){
     nextLL = (tsi148DmaDescriptor_t *)currentLL->dnlal;
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
// Function   : tempeDoDma
// Description:  
//   Sanity check the DMA request. 
//   Setup DMA attribute register. 
//   Create linked list of DMA descriptors.
//   Invoke actual DMA operation.
//   Wait for completion.  Record ending time.
//   Free the linked list of DMA descriptor.
//-----------------------------------------------------------------------------
int
tempeDoDma(vmeDmaPacket_t *vmeDma)
{
  tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
  unsigned int dctlreg=0;
  int val;
  char buf[100];
  int channel, x;
  vmeDmaPacket_t *curDma;
  tsi148DmaDescriptor_t *dmaLL;
    
  // Sanity check the VME chain.
  channel = vmeDma->channel_number;
  if(channel > 1) { return(-EINVAL); }
  curDma = vmeDma;
  while(curDma != 0){
    if(curDma->byteCount == 0){ return(-EINVAL); }
    if(dmaSrcAttr(curDma) < 0) { return(-EINVAL); }
    if(dmaDstAttr(curDma) < 0) { return(-EINVAL); }

    curDma = curDma->pNextPacket;
    if(curDma == vmeDma) {          // Endless Loop!
       return(-EINVAL);
    }
  }

  // calculate control register
  if(vmeDma->pNextPacket != 0){
    dctlreg = 0;
  } else {
    dctlreg = 0x800000;
  }

  for(x = 0; x < 8; x++){	// vme block size
    if((32 << x) >= vmeDma->maxVmeBlockSize) { break; }
  }
  if (x == 8) x = 7;
  dctlreg |= (x << 12);

  for(x = 0; x < 8; x++){	// pci block size
    if((32 << x) >= vmeDma->maxPciBlockSize) { break; }
  }
  if (x == 8) x = 7;
  dctlreg |= (x << 4);

  if(vmeDma->vmeBackOffTimer){
    for(x = 1; x < 8; x++){	// vme timer
      if((1 << (x-1)) >= vmeDma->vmeBackOffTimer) { break; }
    }
    if (x == 8) x = 7;
    dctlreg |= (x << 8);
  }

  if(vmeDma->pciBackOffTimer){
    for(x = 1; x < 8; x++){	// pci timer
      if((1 << (x-1)) >= vmeDma->pciBackOffTimer) { break; }
    }
    if (x == 8) x = 7;
    dctlreg |= (x << 0);
  }

  // Setup the dma chain
  dmaLL = tempeSetupDma(vmeDma);

  // Start the DMA
  if(dctlreg & 0x800000){
      vmeDma->vmeDmaStartTick = 
                      tempeStartDma( channel, dctlreg, dmaLL);
  } else {
      vmeDma->vmeDmaStartTick = 
                      tempeStartDma( channel, dctlreg,
                                 (tsi148DmaDescriptor_t *)virt_to_phys(dmaLL));
  }

  wait_event_interruptible(dma_queue[channel], 
          (pTempe->lcsr.dma[channel].dsta & 0x1000000) == 0);
  val = pTempe->lcsr.dma[channel].dsta;

  vmeDma->vmeDmaStopTick = tempeDmaIrqTime[channel];
  vmeDma->vmeDmaStatus = val;
  if(vmeDma->vmeDmaStopTick < vmeDma->vmeDmaStartTick){
     vmeDma->vmeDmaElapsedTime = 
        (0xFFFFFFFF - vmeDma->vmeDmaStartTick) + vmeDma->vmeDmaStopTick;
  } else {
     vmeDma->vmeDmaElapsedTime = 
        vmeDma->vmeDmaStopTick - vmeDma->vmeDmaStartTick;
  }
  vmeDma->vmeDmaElapsedTime -= vmechipIrqOverHeadTicks;
  vmeDma->vmeDmaElapsedTime /= (tb_speed/1000000);

  vmeDma->vmeDmaStatus = 0;
  if (val & 0x10000000) {
      sprintf(buf,"<<tsi148>> DMA Error in DMA_tempe_irqhandler DSTA=%08X\n",val);  
      printk(buf);
      vmeDma->vmeDmaStatus = val;
  
  }

  // Free the dma chain
  tempeFreeDma(dmaLL);
  
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : tempe_shutdown
// Description: Put VME bridge in quiescent state.  Disable all decoders,
// clear all interrupts.
//-----------------------------------------------------------------------------
void
tempe_shutdown()
{
    tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
    int i;

   /*
    *  Shutdown all inbound and outbound windows.
    */
    for(i =0; i < 8; i++){
        pTempe->lcsr.inboundTranslation[i].itat = 0;
        pTempe->lcsr.outboundTranslation[i].otat = 0;
    }

   /*
    *  Shutdown Location monitor.
    */
   pTempe->lcsr.lmat = 0;
   
   /*
    *  Shutdown CRG map.
    */
   pTempe->lcsr.csrat = 0;

   /*
    *  Clear error status.
    */
   pTempe->lcsr.edpat = 0xFFFFFFFF;
   pTempe->lcsr.veat = 0xFFFFFFFF;
   pTempe->lcsr.pstat = 0x07000700;

   /*
    *  Remove VIRQ interrupt (if any)
    */
   if(pTempe->lcsr.vicr & 0x800){
      pTempe->lcsr.vicr = 0x8000;
   }

   /*
    *  Disable and clear all interrupts.
    */
   pTempe->lcsr.inteo = 0;
   vmeSyncData();
   pTempe->lcsr.intc = 0xFFFFFFFF;
   vmeSyncData();
   pTempe->lcsr.inten = 0xFFFFFFFF;
   
   /*
    *  Map all Interrupts to PCI INTA
    */
   pTempe->lcsr.intm1 = 0;
   pTempe->lcsr.intm2 = 0;

   vmeSyncData();
}

//-----------------------------------------------------------------------------
// Function   : tempe_init()
// Description: Initialize the VME chip as needed by the driver.
//    Quiesce VME bridge.
//    Setup default decoders.
//    Connect IRQ handler and enable interrupts.
//    Conduct quick sanity check of bridge.
//-----------------------------------------------------------------------------
int
tempe_init()
{
    tsi148_t *pTempe = (tsi148_t *)vmechip_baseaddr;
    int result;
    unsigned int tmp;
    unsigned int crcsr_addr;
    unsigned int irqOverHeadStart;
    int overHeadTicks;

    tempe_shutdown();

    // Determine syscon and slot number.
    tmp = pTempe-> lcsr.vstat;
    if(tmp & 0x100){
        vme_syscon = 1;
    } else {
        vme_syscon = 0;
    }

    // Initialize crcsr map
    if(vme_slotnum != -1){
      pTempe-> crcsr.cbar = vme_slotnum << 3;
      crcsr_addr = vme_slotnum*512*1024;
      pTempe->lcsr.crol = virt_to_bus(vmechip_interboard_data);
      pTempe->lcsr.crou = 0;
      vmeSyncData();
      pTempe->lcsr.crat = 0xFFFFFFFF;
    }

    // Connect the interrupt handler.
    result = request_irq(vmechip_irq, 
                         tempe_irqhandler, 
                         SA_INTERRUPT, 
                         "VMEBus (Tsi148)", NULL);
    if (result) {
      printk("  tsi148: can't get assigned pci irq vector %02X\n", vmechip_irq);
      return(0);
    } 

    // Enable DMA, mailbox, VIRQ (syscon only) & LM Interrupts
    if(vme_syscon)
       tmp = 0x03FF20FE;
    else 
       tmp = 0x03FF2000;
    pTempe->lcsr.inteo = tmp;
    pTempe->lcsr.inten = tmp;

    // Do a quick sanity test of the bridge
    if(pTempe->lcsr.inteo != tmp){
      return(0);
    }
    // Sanity check register access.
    for(tmp = 1; tmp < 0x80000000; tmp = tmp << 1){
      pTempe->lcsr.rmwen = tmp;
      pTempe->lcsr.rmwc = ~tmp;
      vmeSyncData();
      if(pTempe->lcsr.rmwen != tmp) {
        return(0);
      }
      if(pTempe->lcsr.rmwc != ~tmp) {
        return(0);
      }
    }

    // Calculate IRQ overhead
    irqOverHeadStart = vmeGetTime();
    pTempe->gcsr.mbox[0] = 0;
    vmeSyncData();
    for(tmp = 0; tmp < 10; tmp++) { vmeSyncData(); }

    irqOverHeadStart = vmeGetTime();
    pTempe->gcsr.mbox[0] = 0;
    vmeSyncData();
    for(tmp = 0; tmp < 10; tmp++) { vmeSyncData(); }

    overHeadTicks = tempeIrqTime - irqOverHeadStart;
    if(overHeadTicks > 0){
       vmechipIrqOverHeadTicks = overHeadTicks;
    } else {
       vmechipIrqOverHeadTicks = 1;
    }
    return(1);
}
