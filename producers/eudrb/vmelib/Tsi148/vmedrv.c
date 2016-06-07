//------------------------------------------------------------------------------
//title: PCI-VME Kernel Driver
//version: 3.1
//date: February 2004
//designer:  Tom Armistead
//programmer: Tom Armistead
//platform: Linux 2.4.XX
//language:
//------------------------------------------------------------------------------  
//  Purpose:  Provides generic (non bridge specific) subroutine
//            entry points.
//            Many of these entry points do little more than call Universe
//            or Tempe specific routines and return the result.
//  Docs:                                  
//------------------------------------------------------------------------------  
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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/poll.h>

#include "vmedrv.h"
#include "ca91c042.h"

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
extern struct resource *vmepcimem;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Vars and Defines
//----------------------------------------------------------------------------

// Global VME controller information
int vmechip_irq = 0;	// PCI irq
int vmechipIrqOverHeadTicks = 0;	// Interrupt overhead
int vmechip_devid = 0;	// PCI devID
int vmeparent_devid = 0;	// VME bridge parent ID
int vmechip_revision = 0;	// PCI revision
int vmechip_bus = 0;	// PCI bus number.
char *vmechip_baseaddr  = 0;	// virtual address of chip registers
int vme_slotnum = -1;	// VME slot num (-1 = unknown)
int vme_syscon = -1;	// VME sys controller (-1 = unknown)

// address of board data
extern struct vmeSharedData *vmechip_interboard_data;

struct resource out_resource[VME_MAX_WINDOWS];
unsigned int out_image_va[VME_MAX_WINDOWS];    // Base virtual address

/*
 * External functions 
 */
extern int uni_init();
extern void uni_shutdown();
extern int uniSetArbiter( vmeArbiterCfg_t *vmeArb);
extern int uniGetArbiter( vmeArbiterCfg_t *vmeArb);
extern int uniSetRequestor( vmeRequesterCfg_t *vmeReq);
extern int uniGetRequestor( vmeRequesterCfg_t *vmeReq);
extern int uniSetInBound( vmeInWindowCfg_t *vmeIn);
extern int uniGetInBound( vmeInWindowCfg_t *vmeIn);
extern int uniSetOutBound( vmeOutWindowCfg_t *vmeOut);
extern int uniGetOutBound( vmeOutWindowCfg_t *vmeOut);
extern int uniSetupLm( vmeLmCfg_t *vmeLm);
extern int uniWaitLm( vmeLmCfg_t *vmeLm);
extern int uniDoRmw( vmeRmwCfg_t *vmeRmw);
extern int uniDoDma(vmeDmaPacket_t *vmeDma);
extern int uniGenerateIrq(virqInfo_t *);
extern int uniBusErrorChk(int);

extern int tempe_init();
extern void tempe_shutdown();
extern int tempeSetArbiter( vmeArbiterCfg_t *vmeArb);
extern int tempeGetArbiter( vmeArbiterCfg_t *vmeArb);
extern int tempeSetRequestor( vmeRequesterCfg_t *vmeReq);
extern int tempeGetRequestor( vmeRequesterCfg_t *vmeReq);
extern int tempeSetInBound( vmeInWindowCfg_t *vmeIn);
extern int tempeGetInBound( vmeInWindowCfg_t *vmeIn);
extern int tempeSetOutBound( vmeOutWindowCfg_t *vmeOut);
extern int tempeGetOutBound( vmeOutWindowCfg_t *vmeOut);
extern int tempeSetupLm( vmeLmCfg_t *vmeLm);
extern int tempeWaitLm( vmeLmCfg_t *vmeLm);
extern int tempeDoRmw( vmeRmwCfg_t *vmeRmw);
extern int tempeDoDma(vmeDmaPacket_t *vmeDma);
extern int tempeGenerateIrq(virqInfo_t *);
extern int tempeBusErrorChk(int);

int vme_irqlog[8][0x100];

//----------------------------------------------------------------------------
//  Some kernels don't supply pci_bus_mem_base_phys()... this subroutine
//  provides that functionality.
//----------------------------------------------------------------------------

#ifdef	SIMPCIMEMBASE

unsigned int pciBusMemBasePhys=0;
unsigned int
pci_bus_mem_base_phys(int bus)
{
  return(pciBusMemBasePhys);
}

#endif

//----------------------------------------------------------------------------
//  vmeSyncData()
//    Ensure completion of all previously issued loads/stores
//----------------------------------------------------------------------------
void
vmeSyncData()
{
   asm("sync");		// PPC specific.
   asm("eieio");
}

//----------------------------------------------------------------------------
//  vmeGetTime()
//    Return time stamp.
//----------------------------------------------------------------------------
void
vmeGetTime()
{
  asm("mfspr 3,268");   // PPC specific.
}

//----------------------------------------------------------------------------
//  vmeFlushLine(void *ptr)
//    Flush & invalidate a cache line 
//----------------------------------------------------------------------------
void 
vmeFlushLine()
{
  asm("dcbf 0,3");   // PPC specific.
  asm("sync");   // PPC specific.
}

//----------------------------------------------------------------------------
//  vmeFlushRange()
//    Flush & invalidate a range of memory
//----------------------------------------------------------------------------
void 
vmeFlushRange(char *start, char *end)
{
  while(start < end) {
    vmeFlushLine(start);
    start += LINESIZE;
  }
}

//----------------------------------------------------------------------------
//  vmeBusErrorChk()
//     Return 1 if VME bus error occured, 0 otherwise.
//     Optionally clear VME bus error status.
//----------------------------------------------------------------------------
int
vmeBusErrorChk(clrflag)
{
  vmeSyncData();
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniBusErrorChk(clrflag));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeBusErrorChk(clrflag));
      break;
  }
  return(0);
}

//----------------------------------------------------------------------------
//  vmeGetSlotNum
//    Determine the slot number that we are in.  Uses CR/CSR base 
//    address reg if it contains a valid value.  If not, obtains
//    slot number from board level register.
//----------------------------------------------------------------------------
int
vmeGetSlotNum()
{
  int slotnum = 0;

  // get slot num from VME chip reg if it is set.
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      slotnum = readl(vmechip_baseaddr+VCSR_BS);
      slotnum = (slotnum >> 27) & 0x1F;
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      slotnum = *(int *)(vmechip_baseaddr+0x23C);
      slotnum = slotnum & 0x1F;
      return(slotnum);
      break;
  }

  // Following is board specific!!

#ifdef	MCG_MVME2600
  if(slotnum == 0)
    slotnum = ~inb(0x1006) & 0x1F;	// Genesis2 slot register
#endif
#ifdef	MCG_MVME2400
  if(slotnum == 0)
    slotnum = ~inb(0x1006) & 0x1F;	// Genesis2 slot register
#endif
#ifdef	MCG_MVME2300
  if(slotnum == 0)
    slotnum = ~inb(0x1006) & 0x1F;	// Genesis2 slot register
#endif
  
  return(slotnum);
}

//----------------------------------------------------------------------------
//  vmeGetOutBound
//    Return attributes of an outbound window.
//----------------------------------------------------------------------------
int
vmeGetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  int windowNumber;

  windowNumber = vmeOut->windowNbr;
  memset(vmeOut, 0, sizeof(vmeOutWindowCfg_t));
  vmeOut->windowNbr = windowNumber;
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniGetOutBound(vmeOut));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeGetOutBound(vmeOut));
      break;
  }
  return(0);
}

//----------------------------------------------------------------------------
//  vmeSetOutBound
//    Set attributes of an outbound window.
//----------------------------------------------------------------------------
int
vmeSetOutBound( vmeOutWindowCfg_t *vmeOut)
{
  int windowNbr = vmeOut->windowNbr;
  unsigned int existingSize;

  if(vmepcimem == 0)
    return(-ENOMEM);
  if(vmeOut->windowSizeL == 0)
    return(-EINVAL);

  // Allocate and map PCI memory space for this window.
  existingSize = out_resource[windowNbr].end - out_resource[windowNbr].start;
  if(existingSize != vmeOut->windowSizeL){
    if(existingSize != 0){
      iounmap((char *)out_image_va[windowNbr]);
      release_resource(&out_resource[windowNbr]);
      memset(&out_resource[windowNbr], 0, sizeof(struct resource));
    }
    if(allocate_resource(vmepcimem, &out_resource[windowNbr], 
         vmeOut->windowSizeL, 0, 0xFFFFFFFF, 0x10000, 0,0)) {
         printk("allocation failed for %x\n", vmeOut->windowSizeL);
         return(-ENOMEM);
    }
    out_image_va[windowNbr] = 
      (int )ioremap(out_resource[windowNbr].start, vmeOut->windowSizeL);
    if(out_image_va[windowNbr] == 0){
         release_resource(&out_resource[windowNbr]);
         memset(&out_resource[windowNbr], 0, sizeof(struct resource));
         return(-ENOMEM);
    }
  }

  vmeOut->pciBusAddrL = out_resource[windowNbr].start 
                        - pci_bus_mem_base_phys(vmechip_bus);

  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniSetOutBound(vmeOut));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeSetOutBound(vmeOut));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeGetInBound
//    Return attributes of an inbound window.
//----------------------------------------------------------------------------
int
vmeGetInBound( vmeInWindowCfg_t *vmeIn)
{
  int windowNumber;

  windowNumber = vmeIn->windowNbr;
  memset(vmeIn, 0, sizeof(vmeInWindowCfg_t));
  vmeIn->windowNbr = windowNumber;
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniGetInBound(vmeIn));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeGetInBound(vmeIn));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeSetInBound
//    Set attributes of an inbound window.
//----------------------------------------------------------------------------
int
vmeSetInBound( vmeInWindowCfg_t *vmeIn)
{
  int status = -ENODEV;

  // Mask off VME address bits per address space.
  switch(vmeIn->addrSpace){
    case VME_A16:
	vmeIn->vmeAddrU = 0;
	vmeIn->vmeAddrL &= 0xFFFF;
        break;
    case VME_A24:
    case VME_CRCSR:
	vmeIn->vmeAddrU = 0;
	vmeIn->vmeAddrL &= 0xFFFFFF;
        break;
    case VME_A32:
	vmeIn->vmeAddrU = 0;
        break;
    case VME_A64:
    case VME_USER1:
    case VME_USER2:
    case VME_USER3:
    case VME_USER4:
        break;
  }
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      status = uniSetInBound(vmeIn);
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      status = tempeSetInBound(vmeIn);
      break;
  }

  if(!status){
    vmechip_interboard_data->inBoundVmeSize[vmeIn->windowNbr] = 
        vmeIn->windowSizeL;
    vmeFlushLine(
        (char *)&vmechip_interboard_data->inBoundVmeSize[vmeIn->windowNbr]);

    vmechip_interboard_data->inBoundVmeAddrHi[vmeIn->windowNbr] = 
        vmeIn->vmeAddrU;
    vmeFlushLine(
        (char *)&vmechip_interboard_data->inBoundVmeAddrHi[vmeIn->windowNbr]);

    vmechip_interboard_data->inBoundVmeAddrLo[vmeIn->windowNbr] = 
        vmeIn->vmeAddrL;
    vmeFlushLine(
        (char *)&vmechip_interboard_data->inBoundVmeAddrLo[vmeIn->windowNbr]);

    vmechip_interboard_data->inBoundVmeAM[vmeIn->windowNbr] = 
        vmeIn->addrSpace;
    vmeFlushLine(
        (char *)&vmechip_interboard_data->inBoundVmeAM[vmeIn->windowNbr]);
  }

  return(status);
}

//----------------------------------------------------------------------------
//  vmeDoDma()
//     Perform the requested DMA operation.
//     Caller must assure exclusive access to the DMA channel.  
//----------------------------------------------------------------------------
int
vmeDoDma( vmeDmaPacket_t *vmeDma)
{
  int retval = -ENODEV;

  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      retval = uniDoDma(vmeDma);
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      retval = tempeDoDma(vmeDma);
      break;
  }

  return(retval);
}

//----------------------------------------------------------------------------
//  vmeGetSlotInfo()
//     Obtain information about the board (if any) that is present in
//     a specified VME bus slot.   
//
//     (Note VME slot numbers start at 1, not 0).
//     Depends on board mapping in CS/CSR space.  Boards which do not 
//     support or are not configured to respond in CS/CSR space will 
//     not be detected by this routine.
//----------------------------------------------------------------------------
int
vmeGetSlotInfo( vmeInfoCfg_t *vmeInfo)
{
  int slotOfInterest;
  unsigned int temp;
  int i;
  
  struct vmeSharedData *remoteData;
  unsigned int *remoteCsr;
  
  slotOfInterest = vmeInfo->vmeSlotNum;
  memset(vmeInfo, 0, sizeof(vmeInfoCfg_t));

  if(slotOfInterest > 21){
     return(-EINVAL);
  }

  // Fill in information for our own board.
  if((slotOfInterest == 0) || (slotOfInterest == vme_slotnum)){
     vmeInfo->vmeSlotNum = vme_slotnum;
     vmeInfo->boardResponded = 1;
     vmeInfo->sysConFlag = vme_syscon; 
     vmeInfo->vmeControllerID = PCI_VENDOR_ID_TUNDRA | (vmechip_devid << 16);
     vmeInfo->vmeControllerRev = vmechip_revision;
     strcpy(vmeInfo->osName, "Linux");
     vmeInfo->vmeDriverRev = VMEDRV_REV;
     return(0);
  }

  // Fill in information for another board in the chassis.
  vmeInfo->vmeSlotNum = slotOfInterest;
  if(out_image_va[7] == 0){
     return(-ENODEV);
  }

  // See if something responds at that slot.
  remoteData = (struct vmeSharedData *)
               (vmeInfo->vmeSlotNum*512*1024+out_image_va[7]);

  remoteCsr = (unsigned int *)remoteData;
  temp = remoteCsr[0x7FFFC/4];
  if(vmeBusErrorChk(1)){
     return(0);    // no response.
  }
  if(temp != (slotOfInterest << 3)) {
     return(0);    // non-sensical response.
  }
  vmeInfo->boardResponded = 1;
  vmeInfo->vmeControllerID = readl(&remoteCsr[0x7F000/4]);
  vmeInfo->vmeControllerRev = readl(&remoteCsr[0x7F008/4]) & 0xFF;

  // If there is no valid driver data structure there, 
  // nothing left to do
    
  if(strcmp("VME", remoteData->validity1) ||
     strcmp("RDY", remoteData->validity2) ){
     return(0);
  }
  // Copy information from struct 
  vmeInfo->vmeSharedDataValid = 1;
  vmeInfo->vmeDriverRev = remoteData->driverRev;
  strncpy(vmeInfo->osName, remoteData->osname, 8);
  for(i = 0; i < 8; i++){
    vmeInfo->vmeAddrHi[i] =remoteData->inBoundVmeAddrHi[i]; 
    vmeInfo->vmeAddrLo[i] =remoteData->inBoundVmeAddrLo[i];
    vmeInfo->vmeAm[i] =remoteData->inBoundVmeAM[i];   
    vmeInfo->vmeSize[i] =remoteData->inBoundVmeSize[i];
  }

  return(0);
}

//----------------------------------------------------------------------------
//  vmeGetRequestor
//    Return current VME bus requestor attributes.
//----------------------------------------------------------------------------
int
vmeGetRequestor( vmeRequesterCfg_t *vmeReq)
{
  memset(vmeReq, 0, sizeof(vmeRequesterCfg_t));
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniGetRequestor(vmeReq));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeGetRequestor(vmeReq));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeSetRequestor
//    Set VME bus requestor attributes.
//----------------------------------------------------------------------------
int
vmeSetRequestor( vmeRequesterCfg_t *vmeReq)
{
  if(vmeReq->requestLevel > 3){
     return(-EINVAL);
  }

  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniSetRequestor(vmeReq));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeSetRequestor(vmeReq));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeGetArbiter
//    Return VME bus arbiter attributes.
//----------------------------------------------------------------------------
int
vmeGetArbiter( vmeArbiterCfg_t *vmeArb)
{

  // Only valid for system controller.
  if(vme_syscon == 0){
     return(-EINVAL);
  } 
  memset(vmeArb, 0, sizeof(vmeArbiterCfg_t));
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniGetArbiter(vmeArb));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeGetArbiter(vmeArb));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeSetArbiter
//    Set VME bus arbiter attributes.
//----------------------------------------------------------------------------
int
vmeSetArbiter( vmeArbiterCfg_t *vmeArb)
{
  // Only valid for system controller.
  if(vme_syscon == 0){
     return(-EINVAL);
  } 
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniSetArbiter(vmeArb));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeSetArbiter(vmeArb));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeGenerateIrq
//    Generate a VME bus interrupt at the specified level & vector.
//
//  Caller must assure exclusive access to the VIRQ generator.
//----------------------------------------------------------------------------
int
vmeGenerateIrq( virqInfo_t *vmeIrq)
{
  int status = -ENODEV;

  // Only valid for non system controller.
  if(vme_syscon != 0){
     return(-EINVAL);
  } 
  if((vmeIrq->level < 1) || (vmeIrq->level > 7)) {
     return(-EINVAL);
  } 
  if((vmeIrq->vector < 0) || (vmeIrq->vector > 0xFF)) {
     return(-EINVAL);
  } 

  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      status = uniGenerateIrq(vmeIrq);
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      status = tempeGenerateIrq(vmeIrq);
      break;
  }

  return(status);
}

//----------------------------------------------------------------------------
//  vmeGetIrqStatus()
//    Wait for a specific or any VME bus interrupt to occur.
//----------------------------------------------------------------------------
int
vmeGetIrqStatus( virqInfo_t *vmeIrq)
{
  int timeout;
  int looptimeout = 1;
  int i,j;

  // Only valid for system controller.
  if(vme_syscon == 0){
     return(-EINVAL);
  } 
  if((vmeIrq->level >= 8) || (vmeIrq->vector > 0xFF)){
     return(-EINVAL);
  }
  if((vmeIrq->level == 0) && (vmeIrq->vector != 0)){
     return(-EINVAL);
  }

  vmeIrq->timeOutFlag = 0;
  timeout = vmeIrq->waitTime;
  do {
     if((vmeIrq->level == 0) && (vmeIrq->vector == 0)){
        for(i = 1; i < 8; i++){
            for(j = 0; j <= 0xFF; j++){
               if(vme_irqlog[i][j] != 0){
                   vmeIrq->level = i;
                   vmeIrq->vector = j;
                   return(0);
               }
            }
        }
     }
     if(vme_irqlog[vmeIrq->level][vmeIrq->vector] != 0){
         return(0);
     }

     if(timeout <= 0){
        vmeIrq->timeOutFlag = 1;
        return(0);
     }

     // Delay a bit before checking again
     set_current_state(TASK_INTERRUPTIBLE);
     schedule_timeout(looptimeout);
     timeout = timeout - looptimeout;
     
  } while (1);

  return(0);
}

//----------------------------------------------------------------------------
//  vmeClrIrqStatus()
//    Clear irq log of a specific or all VME interrupts.
//----------------------------------------------------------------------------
int
vmeClrIrqStatus( virqInfo_t *vmeIrq)
{

  int i,j;

  // Only valid for system controller.
  if(vme_syscon == 0){
     return(-EINVAL);
  } 

  // Sanity check input
  if((vmeIrq->level >= 8) || (vmeIrq->vector > 0xFF)){
     return(-EINVAL);
  }
  if((vmeIrq->level == 0) && (vmeIrq->vector != 0)){
     return(-EINVAL);
  }

  if((vmeIrq->level == 0) && (vmeIrq->vector == 0)){

     // Clear all irqs.
     for(i = 1; i < 8; i++){
         for(j = 0; j <= 0xFF; j++){
            vme_irqlog[i][j] = 0;
         }
     }
  }

  // Clear a single irq.
  vme_irqlog[vmeIrq->level][vmeIrq->vector] = 0;
  return(0);
}


//----------------------------------------------------------------------------
//  vmeDoRmw()
//    Perform a RMW operation on the VME bus.
//----------------------------------------------------------------------------
int
vmeDoRmw( vmeRmwCfg_t *vmeRmw)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniDoRmw(vmeRmw));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeDoRmw(vmeRmw));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeSetupLm()
//    Setup the VME bus location monitor to detect the specified access.
//----------------------------------------------------------------------------
int
vmeSetupLm( vmeLmCfg_t *vmeLm)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniSetupLm(vmeLm));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeSetupLm(vmeLm));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeWaitLm()
//    Wait for the location monitor to trigger.
//----------------------------------------------------------------------------
int
vmeWaitLm( vmeLmCfg_t *vmeLm)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uniWaitLm(vmeLm));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempeWaitLm(vmeLm));
      break;
  }
  return(-ENODEV);
}

//----------------------------------------------------------------------------
//  vmeInit()
//    Initialize the VME bridge.
//----------------------------------------------------------------------------
int
vmeInit(void *driverdata)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(uni_init(driverdata));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      return(tempe_init(driverdata));
      break;
  }
  return(0);
}
