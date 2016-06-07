//------------------------------------------------------------------------------  
//title: VME PCI-VME Kernel Driver
//version: Linux 3.1
//date: February 2004
//original designer: Michael Wyrick
//original programmer: Michael Wyrick 
//  Signficant restructuring by Tom Armistead to add support of 
//  Tundra TSI148 and additional functionality.
//platform: Linux 2.4.x
//language: GCC 2.95 and 3.0
//module: vmemod.o
//------------------------------------------------------------------------------  
//  Purpose: Provides the device node interface from the user level program to 
//           the VME bridge.  Primary purpose is to convert user space
//           data to kernel and back.
//  Docs:                                  
//    This driver supports both the Tempe, Universe and Universe II chips                                     
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

static char Version[] = "3.1 02/10/2004";

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
#include <linux/iobuf.h>
#include <linux/highmem.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include "vmedrv.h"

#include "ca91c042.h"

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
static int vme_open(struct inode *, struct file *);
static int vme_release(struct inode *, struct file *);
static ssize_t vme_read(struct file *,char *, size_t, loff_t *);
static ssize_t vme_write(struct file *,const char *, size_t, loff_t *);
static unsigned int vme_poll(struct file *, poll_table *);
static int vme_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
//static int vme_mmap(struct inode *inode,struct file *file,struct vm_area_struct *vma);
//static int vme_select(struct inode *inode,struct file *file,int mode, select_table *table);
static int vme_procinfo(char *, char **, off_t, int, int *,void *);

extern	int tb_ticks_per_jiffy;


//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
static struct proc_dir_entry *vme_procdir;
static struct file_operations vme_fops = 
{
  read:     vme_read,
  write:    vme_write,
  poll:     vme_poll,     // vme_poll   
  ioctl:    vme_ioctl,
  open:     vme_open,
  release:  vme_release 
};

//----------------------------------------------------------------------------
// Vars and Defines
//----------------------------------------------------------------------------
#define VME_MAJOR      221
#define MAX_MINOR 	    0xFF

#define DMATYPE_SINGLE  1
#define DMATYPE_LLIST   2

static int opened[MAX_MINOR + 1];
struct semaphore devinuse[MAX_MINOR+1];

// Global VME controller information
extern int vmechip_irq;	// PCI irq
extern int vmechip_devid;	// PCI devID of VME bridge
extern int vmeparent_devid;	// PCI devID of VME bridge parent
extern int vmechip_revision;	// PCI revision
extern int vmechip_bus;	// PCI bus number.
extern char *vmechip_baseaddr;	// virtual address of chip registers
extern int vme_slotnum;	// VME slot num (-1 = unknown)
extern int vme_syscon;	// VME sys controller (-1 = unknown)

char *in_image_ba[0x8];         // Base PCI Address
int  in_image_size[0x8];         // Size

struct resource out_resource[0x8];
extern unsigned int out_image_va[8];    // Base virtual address
extern unsigned int out_image_valid[8];    // validity

/*
 * External functions
 */
extern void uni_shutdown();
extern void tempe_shutdown();

extern int vmeInit();
extern int vmeBusErrorChk(int);
extern int vmeGetSlotNum();
extern int vmeGetOutBound( vmeOutWindowCfg_t *);
extern int vmeSetOutBound( vmeOutWindowCfg_t *);
extern int vmeDoDma( vmeDmaPacket_t *);
extern int vmeGetSlotInfo( vmeInfoCfg_t *);
extern int vmeGetRequestor( vmeRequesterCfg_t *);
extern int vmeSetRequestor( vmeRequesterCfg_t *);
extern int vmeGetArbiter( vmeArbiterCfg_t *);
extern int vmeSetArbiter( vmeArbiterCfg_t *);
extern int vmeGenerateIrq( virqInfo_t *);
extern int vmeGetIrqStatus( virqInfo_t *);
extern int vmeClrIrqStatus( virqInfo_t *);
extern int vmeGetInBound( vmeInWindowCfg_t *);
extern int vmeSetInBound( vmeInWindowCfg_t *);
extern int vmeDoRmw( vmeRmwCfg_t *);
extern int vmeSetupLm( vmeLmCfg_t *);
extern int vmeWaitLm( vmeLmCfg_t *);

extern void vmeSyncData();
extern void vmeFlushLine(void *);
extern void vmeFlushRange(void *, void *);

int vme_irqlog[8][0x100];

#define	DEV_VALID	1
#define	DEV_EXCLUSIVE	2
#define	DEV_RW		4

static char vme_minor_dev_flags[] = { 
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m0 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m1 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m2 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m3 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m4 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m5 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m6 */
  DEV_VALID | DEV_EXCLUSIVE | DEV_RW,	/* /dev/vme_m7 */
  0,0,0,0,0,0,0,0,
  DEV_VALID,	/* /dev/vme_dma0 */
  DEV_VALID,	/* /dev/vme_dma1 */
  0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  DEV_VALID,	/* /dev/vme_ctl */
  DEV_VALID | DEV_RW,	/* /dev/vme_regs */
  DEV_VALID | DEV_EXCLUSIVE,	/* /dev/vme_rmw0 */
  DEV_VALID | DEV_EXCLUSIVE,	/* /dev/vme_lm0 */
  0,0,0,0,
  0,0,0,0,0,0,0,0,
  DEV_VALID | DEV_RW,	/* /dev/vme_slot0 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot1 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot2 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot3 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot4 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot5 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot6 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot7 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot8 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot9 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot10 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot11 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot12 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot13 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot14 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot15 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot16 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot17 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot18 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot19 */
  DEV_VALID | DEV_RW,	/* /dev/vme_slot20 */
  DEV_VALID | DEV_RW	/* /dev/vme_slot21 */
};

// Status Vars
static unsigned int reads  = 0;
static unsigned int writes = 0;
static unsigned int ioctls = 0;

// Shared global.
wait_queue_head_t	dma_queue[2];
wait_queue_head_t	lm_queue;
wait_queue_head_t	mbox_queue;
struct vmeSharedData *vmechip_interboard_data = NULL; // address of board data
static struct resource *vmepcimem;
int	tb_speed;
struct semaphore virq_inuse;

//----------------------------------------------------------------------------
// Module parms
//----------------------------------------------------------------------------
MODULE_PARM(vmechip_irq, "1-255i");
MODULE_PARM(vme_slotnum, "1-21i");

static int
vme_minor_type(minor)
{
  if(minor > sizeof(vme_minor_dev_flags)){
    return(0);
  }
  return(vme_minor_dev_flags[minor]);
}

//----------------------------------------------------------------------------
//  vme_alloc_buffers()
//----------------------------------------------------------------------------
  // Obtain a buffer for the data structure that will be 
  // advertized through CS/CSR space.  The CS/CSR decoder only
  // allows remapping on 512K boundaries so this buffer must be
  // aligned on a 512K byte boundary.  There is no foolproof
  // way to do this...  This code searches for a buffers with 
  // appropriate alignment until it either gets what it needs or
  // runs out of buffers.  In bound buffers are also allocated
  // here.
static int
vme_alloc_buffers()
{
  int discardi, bufferi;
  char *discardbuffers[100];
  char *tmpbuffer;

  // Initialize buffer information.
  vmechip_interboard_data  = NULL;
  for(discardi = 0; discardi < 100; discardi++){
    discardbuffers[discardi] = NULL;
  }
  for(bufferi = 0; bufferi < 8; bufferi++){
    in_image_ba[bufferi] = NULL;
    in_image_size[bufferi] = 0;
  }

  // Search for buffers with needed alignment.
  discardi = 0;
  bufferi = 0;
  while((tmpbuffer = kmalloc(128*1024, GFP_KERNEL)) != NULL){
    if((vmechip_interboard_data  == NULL) && 
      ((int)tmpbuffer & 0x7FFFF) == 0){
      vmechip_interboard_data = (struct vmeSharedData *)tmpbuffer;
    } else if (bufferi < 8) {
      in_image_ba[bufferi] = tmpbuffer;
      in_image_size[bufferi] = 128*1024;
      bufferi++;
    } else if (discardi < 100) {
      discardbuffers[discardi] = tmpbuffer;
      discardi++;
      break;
    } else {
      break;
    }
    if((bufferi == 8) && (vmechip_interboard_data != NULL)){
      break;
    }
  }

  // Return buffers allocated but not needed to free list.
  for(; discardi; discardi--){
    kfree(discardbuffers[discardi]);
  }

  return(0);
}

//----------------------------------------------------------------------------
//  vme_free_buffers()
//   Free all buffers allocated by this driver.
//----------------------------------------------------------------------------
void
vme_free_buffers()
{
  int i;

  // clear valid flags of interboard data
  // and free it
  if(vmechip_interboard_data  != NULL) {
    strcpy(vmechip_interboard_data->validity1,"VME");
    strcpy(vmechip_interboard_data->validity2,"OFF");
    kfree(vmechip_interboard_data);
  }

  // free inbound buffers
  for(i = 0; i < 8; i++){
    if(in_image_ba[i] != NULL){
      kfree(in_image_ba[i]);
    }
  }
}

//----------------------------------------------------------------------------
//  vme_init_test_data()
//   Initialize the test area of the interboard data structure 
//   with the expected data patterns.
//----------------------------------------------------------------------------
void
vme_init_test_data(int testdata[])
{
  int i;

  // walking 1 & walking 0
  for(i = 0; i < 32; i++){
    testdata[i*2] = 1 << i;
    testdata[i*2+1] = ~(1 << i);
  }

  // all 0, all 1's, 55's and AA's
  testdata[64] = 0x0;
  testdata[65] = 0xFFFFFFFF;
  testdata[66] = 0x55555555;
  testdata[67] = 0xAAAAAAAA;

  // Incrementing data.
  for(i = 68; i < 1024; i++){
     testdata[i] = i;
  }
}

//----------------------------------------------------------------------------
//  vme_init_interboard_data()
//    Initialize the interboard data structure.
//----------------------------------------------------------------------------
void
vme_init_interboard_data(struct	vmeSharedData *ptr)
{
  if(ptr == NULL) return;

  memset(ptr, 0, sizeof(struct vmeSharedData)); 
  vme_init_test_data(&ptr->readTestPatterns[0]);

  ptr->structureRev = 1;
  strcpy(&ptr->osname[0], "linux");
  
  ptr->driverRev = 1;

  strcpy(&ptr->boardString[0], "MVMETBD");  /* XXX Fix me */
  ptr->vmeControllerType = vmechip_devid;
  ptr->vmeControllerRev = vmechip_revision; 

  // Set valid strings "VME", "RDY"
  strcpy(ptr->validity1, "VME");
  strcpy(ptr->validity2, "RDY");
  vmeFlushRange(ptr, ptr+1);
}

//----------------------------------------------------------------------------
//  vme_procinfo()
//     /proc interface into this driver
//----------------------------------------------------------------------------
static int vme_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
  char *p;

  p = buf;
  p += sprintf(p,"VME driver %s\n",Version);

  p += sprintf(p,"  vmechip_baseaddr = %08X\n",(int)vmechip_baseaddr);
  p += sprintf(p,"  vmechip_chip device =  %08X\n",(int)vmechip_devid);
  p += sprintf(p,"  vme slot number  = %2d\n",(int)vme_slotnum);
  if(vme_syscon)
    p += sprintf(p,"  vme system controller = TRUE\n");
  else
    p += sprintf(p,"  vme system controller = FALSE\n");
  p += sprintf(p,"  Stats  reads = %i  writes = %i  ioctls = %i\n",
                 reads,writes,ioctls);

/* XXX add tempe & universe info */

  *eof = 1;
  return p - buf;
}

//----------------------------------------------------------------------------
//  register_proc()
//----------------------------------------------------------------------------
static void register_proc()
{
  vme_procdir = create_proc_entry("vmeinfo", S_IFREG | S_IRUGO, 0);
  vme_procdir->read_proc = vme_procinfo;
}

//----------------------------------------------------------------------------
//  unregister_proc()
//----------------------------------------------------------------------------
static void unregister_proc()
{
  remove_proc_entry("vmeinfo",0);
}

//----------------------------------------------------------------------------
//  vmeChipRegRead
//    Read a VME chip register.
//
//    Note that Tempe swizzles registers at offsets > 0x100 on its own.
//----------------------------------------------------------------------------
unsigned int
vmeChipRegRead(unsigned int *ptr)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      return(readl(ptr));
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      if(((char *)ptr) - ((char *)vmechip_baseaddr) > 0x100){
         return(*ptr);
      }
      return(readl(ptr));
      break;
  }
  return(0);
}

//----------------------------------------------------------------------------
//  vmeChipRegWrite
//    Write a VME chip register.
//
//    Note that Tempe swizzles registers at offsets > 0x100 on its own.
//----------------------------------------------------------------------------
void
vmeChipRegWrite(unsigned int *ptr, unsigned int value)
{
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      writel(value, ptr);
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      if(((char *)ptr) - ((char *)vmechip_baseaddr) > 0x100){
         *ptr = value;
      } else {
         writel(value, ptr);
      }
      break;
  }
}

//-----------------------------------------------------------------------------
// Function   : vmeIoctlUserFreeDma
// Description: Free the pages that comprise the linked list of DMA packets.
//-----------------------------------------------------------------------------
int
vmeIoctlUserFreeDma(
vmeDmaPacket_t *userStartPkt,
struct kiobuf *srcKiobuf,
struct kiobuf *dstKiobuf
)
{
  vmeDmaPacket_t *currentPkt;
  vmeDmaPacket_t *prevPkt;
  vmeDmaPacket_t *nextPkt;
  int pageno = 0;
  
  // Free all pages associated with the packets.
  currentPkt = userStartPkt;
  prevPkt = currentPkt;
  while(currentPkt != 0){
     if(srcKiobuf != 0){
        kunmap(srcKiobuf->maplist[pageno]);
     }
     if(dstKiobuf != 0){
        kunmap(dstKiobuf->maplist[pageno]);
     }
     nextPkt = currentPkt->pNextPacket;
     if(currentPkt+1 != nextPkt) {
         free_pages((int)prevPkt,0);
         prevPkt = nextPkt;
     }
     currentPkt = nextPkt;
     pageno++;
  }
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : vmeIoctlUserSetupDma
// Description: Create DMA linked list that corresponds to a buffer
// in user space.
//-----------------------------------------------------------------------------
vmeDmaPacket_t *
vmeIoctlUserSetupDma(
vmeDmaPacket_t *vmeDma, 
char *srcAddr,
struct kiobuf *srcKiobuf,
char *dstAddr,
struct kiobuf *dstKiobuf,
int *returnStatus
)
{
  int	maxPerPage;
  int	currentPktcount;
  vmeDmaPacket_t *userStartPkt;
  vmeDmaPacket_t *currentPkt;
  int pageno = 0;
  int remainingbytecount;
  int thispacketbytecount;

  maxPerPage = PAGESIZE/sizeof(vmeDmaPacket_t)-1;
  userStartPkt = (vmeDmaPacket_t *)__get_free_pages(GFP_KERNEL, 0);
  if(userStartPkt == 0){ *returnStatus = -ENOMEM; return(0); }


  // First allocate pages for packets and create linked list
  remainingbytecount = vmeDma->byteCount;
  currentPkt = userStartPkt;
  currentPktcount = 0;
  while(remainingbytecount > 0){
     memcpy(currentPkt, vmeDma, sizeof(vmeDmaPacket_t));
     currentPkt->pNextPacket = 0;

     // Calculate # of bytes for this DMA packet.
     thispacketbytecount = PAGESIZE;
     if(srcKiobuf != 0){
        if(srcKiobuf->offset != 0){
           thispacketbytecount = PAGESIZE - srcKiobuf->offset;
        }
     }
     if(dstKiobuf != 0){
        if(dstKiobuf->offset != 0){
           thispacketbytecount = PAGESIZE - dstKiobuf->offset;
        }
     }
     if(thispacketbytecount > remainingbytecount){
         thispacketbytecount = remainingbytecount;
     }

     // Setup Source address for this packet
     switch(vmeDma->srcBus){
        case VME_DMA_USER:
           currentPkt->srcAddr = (unsigned int)
                   kmap(srcKiobuf->maplist[pageno]) + srcKiobuf->offset;
           srcKiobuf->offset += thispacketbytecount;
           if(srcKiobuf->offset == PAGESIZE){ 
              srcKiobuf->offset = 0;
           }
           currentPkt->srcBus = VME_DMA_KERNEL;
           break;
        case VME_DMA_PATTERN_BYTE:
        case VME_DMA_PATTERN_WORD:
           break;
        case VME_DMA_PATTERN_WORD_INCREMENT:
           currentPkt->srcAddr = (unsigned int)srcAddr;
           srcAddr += (thispacketbytecount/4);
           break;
        case VME_DMA_PATTERN_BYTE_INCREMENT:
        default:
           currentPkt->srcAddr = (unsigned int)srcAddr;
           srcAddr += thispacketbytecount;
           break;
     }
     if(currentPkt->srcBus == VME_DMA_KERNEL) {
        currentPkt->srcAddr = virt_to_bus((void *)currentPkt->srcAddr);
        currentPkt->srcBus = VME_DMA_PCI;
     }

     // Setup destination address for this packet
     switch(vmeDma->dstBus){
        case VME_DMA_USER:
           currentPkt->dstAddr = (unsigned int)
                   kmap(dstKiobuf->maplist[pageno]) + dstKiobuf->offset;
           dstKiobuf->offset += thispacketbytecount;
           if(dstKiobuf->offset == PAGESIZE){ 
              dstKiobuf->offset = 0;
           }
           currentPkt->dstBus = VME_DMA_KERNEL;
        break;
        default:
           currentPkt->dstAddr = (unsigned int)dstAddr;
           dstAddr += thispacketbytecount;
        break;
     }
     if(currentPkt->dstBus == VME_DMA_KERNEL) {
        currentPkt->dstAddr = virt_to_bus((void *)currentPkt->dstAddr);
        currentPkt->dstBus = VME_DMA_PCI;
     }

     currentPkt->byteCount = thispacketbytecount;

     remainingbytecount  -= thispacketbytecount;
     if(remainingbytecount > 0){ 
        currentPkt->pNextPacket = currentPkt + 1;
        currentPktcount++;
        if(currentPktcount >= maxPerPage){
              currentPkt->pNextPacket = 
                   (vmeDmaPacket_t *)__get_free_pages(GFP_KERNEL,0);
              currentPktcount = 0;
        } 
     }
     currentPkt = currentPkt->pNextPacket;
     pageno++;
  }
  
  // Return to packets list 
  *returnStatus = 0;
  return(userStartPkt);
}

//-----------------------------------------------------------------------------
// Function   : vmeIoctlDoUserDma
// Description: Setup and perform a VME DMA to/from a buffer in user space.
//-----------------------------------------------------------------------------
int
vmeDoUserDma(vmeDmaPacket_t *startPkt)
{
   struct kiobuf *srcIobuf = 0;
   struct kiobuf *dstIobuf = 0;
   int result = 0;
   vmeDmaPacket_t *newstartPkt;

   // allocate kiovecs as needed for source & destination
   if(startPkt->srcBus == VME_DMA_USER) {
      if(alloc_kiovec(1, &srcIobuf)) {
          return(-ENOMEM);
      }
   }
   if(startPkt->dstBus == VME_DMA_USER) {
      if(alloc_kiovec(1, &dstIobuf)) {
          if(srcIobuf != 0) free_kiovec(1, &srcIobuf);
          return(-ENOMEM);
      }
   }

   if(srcIobuf != 0) result = map_user_kiobuf(WRITE,  srcIobuf, 
                         startPkt->srcAddr, startPkt->byteCount);
   if(result) {
      if(srcIobuf != 0) free_kiovec(1, &srcIobuf);
      if(dstIobuf != 0) free_kiovec(1, &dstIobuf);
      return(-ENOMEM);
   }
   if(dstIobuf != 0) result = map_user_kiobuf(READ, dstIobuf,
                         startPkt->dstAddr, startPkt->byteCount);
   if(result) {
      if(srcIobuf != 0) unmap_kiobuf(srcIobuf);
      if(srcIobuf != 0) free_kiovec(1, &srcIobuf);
      if(dstIobuf != 0) free_kiovec(1, &dstIobuf);
      return(-ENOMEM);
   }

   newstartPkt = vmeIoctlUserSetupDma(startPkt, 
           (char *)startPkt->srcAddr, srcIobuf,
           (char *)startPkt->dstAddr, dstIobuf,
           &result);

   if(result == 0) {
      result = vmeDoDma(newstartPkt);
      memcpy(startPkt, newstartPkt, sizeof(vmeDmaPacket_t));
      vmeIoctlUserFreeDma(newstartPkt, srcIobuf, dstIobuf);
   }

   if(srcIobuf != 0) unmap_kiobuf(srcIobuf);
   if(dstIobuf != 0) unmap_kiobuf(dstIobuf);
   if(srcIobuf != 0) free_kiovec(1, &srcIobuf);
   if(dstIobuf != 0) free_kiovec(1, &dstIobuf);

   return(result);
}

//-----------------------------------------------------------------------------
// Function   : vmeIoctlFreeDma
// Description: Free the pages associated with the linked list of DMA packets.
//-----------------------------------------------------------------------------
int
vmeIoctlFreeDma(vmeDmaPacket_t *startPkt)
{
  vmeDmaPacket_t *currentPkt;
  vmeDmaPacket_t *prevPkt;
  vmeDmaPacket_t *nextPkt;
  
  // Free all pages associated with the packets.
  currentPkt = startPkt;
  prevPkt = currentPkt;
  while(currentPkt != 0){
     nextPkt = currentPkt->pNextPacket;
     if(currentPkt+1 != nextPkt) {
         free_pages((int)prevPkt,0);
         prevPkt = nextPkt;
     }
     currentPkt = nextPkt;
  }
  return(0);
}


//-----------------------------------------------------------------------------
// Function   : vmeIoctlSetupDma
// Description: Read descriptors from user space and create a linked list
//    of DMA packets.
//  
//-----------------------------------------------------------------------------
vmeDmaPacket_t *
vmeIoctlSetupDma(vmeDmaPacket_t *vmeDma, int *returnStatus)
{
  vmeDmaPacket_t *vmeCur;
  int	maxPerPage;
  int	currentPktcount;
  vmeDmaPacket_t *startPkt;
  vmeDmaPacket_t *currentPkt;
  

  maxPerPage = PAGESIZE/sizeof(vmeDmaPacket_t)-1;
  startPkt = (vmeDmaPacket_t *)__get_free_pages(GFP_KERNEL, 0);
  if(startPkt == 0){ *returnStatus = -ENOMEM; return(0); }

  // First allocate pages for packets and create linked list
  vmeCur = vmeDma;
  currentPkt = startPkt;
  currentPktcount = 0;

  while(vmeCur != 0){
     if (copy_from_user(currentPkt, vmeCur,  sizeof(vmeDmaPacket_t))){
        currentPkt->pNextPacket = 0;
        vmeIoctlFreeDma(startPkt);
        *returnStatus = -EFAULT;
        return(0);
     }
     if(currentPkt != startPkt){
        if((currentPkt->srcBus == VME_DMA_USER) ||
           (currentPkt->dstBus == VME_DMA_USER)){
           currentPkt->pNextPacket = 0;
           vmeIoctlFreeDma(startPkt);
           *returnStatus = -EINVAL;
           return(0);
        }
     }
     if(currentPkt->srcBus == VME_DMA_KERNEL){
        currentPkt->srcAddr = virt_to_bus((void *)currentPkt->srcAddr);
     }
     if(currentPkt->dstBus == VME_DMA_KERNEL){
        currentPkt->dstAddr = virt_to_bus((void *)currentPkt->dstAddr);
     }
     if(currentPkt->pNextPacket == NULL) { break; }
     vmeCur = currentPkt->pNextPacket;

     currentPkt->pNextPacket = currentPkt + 1;
     currentPktcount++;
     if(currentPktcount >= maxPerPage){
           currentPkt->pNextPacket = 
                (vmeDmaPacket_t *)__get_free_pages(GFP_KERNEL,0);
           currentPktcount = 0;
     } 
     currentPkt = currentPkt->pNextPacket;
  }
  
  // Return to packets list 
  *returnStatus = 0;
  return(startPkt);
}

//-----------------------------------------------------------------------------
// Function   : vmeIoctlDma()
// Inputs     : 
//   User pointer to DMA packet (possibly chained)
//   DMA channel number.
// Outputs    : returns 0 on success, error code on failure.
// Description: 
//   Copy DMA chain into kernel space.
//   Verify validity of chain.
//   Wait, if needed, for DMA channel to be available.
//   Do the DMA operation.
//   Free the DMA channel.
//   Copy the DMA packet with status back to user space.
//   Free resources allocated by this operation.
// Remarks    : 
//    Note, due to complexity, DMA to/from a user space buffer is dealt with 
//    via a separate routine.
// History    : 
//-----------------------------------------------------------------------------
int
vmeIoctlDma(vmeDmaPacket_t *vmeDma, int channel)
{

  vmeDmaPacket_t *startPkt;
  int status = 0;


  // Read the (possibly linked) descriptors from the user
  startPkt = vmeIoctlSetupDma(vmeDma, &status);
  if(status < 0){
     return(status);
  }

  startPkt->channel_number = channel;
  if(startPkt->byteCount <= 0){
     vmeIoctlFreeDma(startPkt);
     return(-EINVAL);
  }

  // If user to user mode, buffers must be page aligned.
  if((startPkt->srcBus == VME_DMA_USER) && 
     (startPkt->dstBus == VME_DMA_USER)){
     if((startPkt->dstAddr & (PAGESIZE-1)) != (startPkt->srcAddr & (PAGESIZE-1))){
         vmeIoctlFreeDma(startPkt);
         return(-EINVAL);
     }
  }

  // user mode DMA can only support one DMA packet.
  if((startPkt->srcBus == VME_DMA_USER) || 
     (startPkt->dstBus == VME_DMA_USER)){
     if(startPkt->pNextPacket != NULL) {
         vmeIoctlFreeDma(startPkt);
         return(-EINVAL);
     }
  }

  // Wait for DMA channel to be available.
  if(down_interruptible(&devinuse[channel+VME_MINOR_DMA])) {
    vmeIoctlFreeDma(startPkt);
    return(-ERESTARTSYS);
  }

  // Do the DMA.
  if((startPkt->srcBus == VME_DMA_USER) || 
     (startPkt->dstBus == VME_DMA_USER)){
     status = vmeDoUserDma(startPkt);
  } else {
     status = vmeDoDma(startPkt);
  }

  // Free the DMA channel.
  up(&devinuse[channel+VME_MINOR_DMA]);


  // Copy the result back to the user.
  if (copy_to_user(vmeDma, startPkt, sizeof(vmeDmaPacket_t))){
    vmeIoctlFreeDma(startPkt);
    return(-EFAULT);
  }

  // Free the linked list.
  vmeIoctlFreeDma(startPkt);
  return(status);
}

//----------------------------------------------------------------------------
//
//  vme_poll()
//  Place holder - this driver function not implemented.
//
//----------------------------------------------------------------------------
static unsigned int vme_poll(struct file* file, poll_table* wait)
{
  return 0;
}

//-----------------------------------------------------------------------------
// Function   : vme_open()
// Inputs     : standard Linux device driver open arguments.
// Outputs    : returns 0 on success, error code on failure.
// Description: 
//   Verify device is valid,
//   If device is exclusive use and already open, return -EBUSY
//   increment device use count.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static int vme_open(struct inode *inode,struct file *file)
{
  unsigned int minor = MINOR(inode->i_rdev);
  int minor_flags;

  /* Check for valid minor num */
  minor_flags = vme_minor_type(minor);
  if (!minor_flags){
    return(-ENODEV);
  }

  /* Restrict exclusive use devices to one open */
  if((minor_flags & DEV_EXCLUSIVE) && opened[minor]){
    return(-EBUSY);
  }
  opened[minor]++;
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : vme_release()
// Inputs     : standard Linux device driver release arguments.
// Outputs    : returns 0.
// Description: 
//  Decrement use count and return.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static int vme_release(struct inode *inode,struct file *file)
{
  unsigned int minor = MINOR(inode->i_rdev);

  opened[minor]--;

  return 0;
}

//-----------------------------------------------------------------------------
// Function   : vme_read()
// Inputs     : standard linux device driver read() arguments.
// Outputs    : count of bytes read or error code.
// Description: 
//    Sanity check inputs,
//    If register read,  write user buffer from chip registers
//    If VME window read, find the outbound window and read data
//    through the window.  Return number of bytes successfully read.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static ssize_t vme_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  int x = 0;
  int decoderNum;
  int okcount = 0;
  char *pImage;
  unsigned int v,numt,remain;
  char *temp = buf;
  unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);  
  unsigned int foffset;

  unsigned char  vc;
  unsigned short vs;
  unsigned int   vl;
  int minor_flags;
  int image_size;

  minor_flags = vme_minor_type(minor);
  if(!(minor_flags & DEV_RW)){
    return(-EINVAL);
  }

  if(*ppos > 0xFFFFFFFF){
    return(-EINVAL);
  }
  foffset = *ppos;

  if (minor == VME_MINOR_REGS){
    if((count != 4) || (foffset & (4-1))) { /* word access only */
      return(-EINVAL);
    }
    if(foffset >= 0x1000) { /* Truncate at end of reg image */
      return(0);
    }
    v = vmeChipRegRead((unsigned int *)(vmechip_baseaddr + foffset));    
    __copy_to_user(temp,&v,4);
  } 

  if ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_OUT) {

    decoderNum = minor & 0xF;	/* Get outbound window num */
    if(out_image_va[decoderNum] == 0){
      return(-EINVAL);
    }
    image_size = out_resource[decoderNum].end -out_resource[decoderNum].start+1;
    if(foffset >= image_size){
      return(0);
    }
    if((foffset + count) > image_size){
        count = image_size - foffset;
    }
    reads++;                
    pImage = (char *)(out_image_va[decoderNum] + foffset);

    // Calc the number of longs we need
    numt = count / 4;
    remain = count % 4;
    for (x=0;x<numt;x++) {
      vl = *(int *)(pImage);    

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      }
      okcount += 4;

      __copy_to_user(temp,&vl,4);
      pImage+=4;
      temp+=4;
    }  

    // Calc the number of Words we need
    numt = remain / 2;
    remain = remain % 2;
    for (x=0;x<numt;x++) {
      vs = *(short *)(pImage);    

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      } 
      okcount += 2;

      __copy_to_user(temp,&vs,2);
      pImage+=2;
      temp+=2;
    }  

    for (x=0;x<remain;x++) {
      vc = readb(pImage);    

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      } 
      okcount++;

      __copy_to_user(temp,&vc,1);
      pImage+=1;
      temp+=1;
    }  

  }  

  if ( ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_SLOTS1) ||
       ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_SLOTS2)) {
    return(-EINVAL);	/* Not implemented yet */
  }
  *ppos += count;
  return(count);
}

//-----------------------------------------------------------------------------
// Function   : vme_write()
// Inputs     : standard linux device driver write() arguments.
// Outputs    : count of bytes written or error code.
// Description: 
//    Sanity check inputs,
//    If register write,  write VME chip registers with the user data
//    If VME window write, find the outbound window and write user data
//    through the window.  Return number of bytes successfully written.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static ssize_t vme_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  int x;
  int decoderNum;
  char *pImage;
  int okcount = 0;
  unsigned int numt,remain;
  char *temp = (char *)buf;
  unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);
  unsigned int foffset;

  unsigned char  vc;
  unsigned short vs;
  unsigned int   vl;
  int minor_flags;
  int image_size;

  minor_flags = vme_minor_type(minor);
  if(!(minor_flags & DEV_RW)){
    return(-EINVAL);
  }

  if(*ppos > 0xFFFFFFFF){
    return(-EINVAL);
  }
  foffset = *ppos;

  writes++;
  if (minor == VME_MINOR_REGS) {
    if((count != 4) || (foffset & (4-1))) { /* word access only */
      return(-EINVAL);
    }
    if(foffset >= 0x1000) { /* Truncate at end of reg image */
      return(0);
    }
    __copy_from_user(&vl,temp,4);
    vmeChipRegWrite((unsigned int *)(vmechip_baseaddr + foffset), vl);
  }

  if ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_OUT) {

    decoderNum = minor & 0xF;	/* Get outbound window num */
    if(out_image_va[decoderNum] == 0){
      return(-EINVAL);
    }

    image_size = out_resource[decoderNum].end -out_resource[decoderNum].start;

    if(foffset >= image_size){
      return(0);
    }
    if((foffset + count) > image_size){
        count = image_size - foffset;
    }

    // Calc the number of longs we need
    numt = count / 4;
    remain = count % 4;
    pImage = (char *)(out_image_va[decoderNum] + foffset);

    for (x=0;x<numt;x++) {
      __copy_from_user(&vl,temp,4);
      *(int *)pImage = vl;

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      } else
        okcount += 4;

      pImage+=4;
      temp+=4;
    }  

    // Calc the number of Words we need
    numt = remain / 2;
    remain = remain % 2;

    for (x=0;x<numt;x++) {
      __copy_from_user(&vs,temp,2);
      writew(vs,pImage);

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      } else
        okcount += 2;

      pImage+=2;
      temp+=2;
    }  

    for (x=0;x<remain;x++) {
      __copy_from_user(&vc,temp,1);
      writeb(vc,pImage);

      // Lets Check for a Bus Error
      if(vmeBusErrorChk(1)){
        return(okcount);
      } else
        okcount += 2;

      pImage+=1;
      temp+=1;
    }  

  }  

  if ( ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_SLOTS1) ||
       ((minor & VME_MINOR_TYPE_MASK) == VME_MINOR_SLOTS2)) {
    return(-EINVAL);	/* Not implemented yet */
  }

  *ppos += count;
  return(count);
}

//-----------------------------------------------------------------------------
// Function   : vme_ioctl()
// Inputs     : 
//    cmd -> the IOCTL command
//    arg -> pointer (if any) to the user ioctl buffer 
// Outputs    : 0 for sucess or errorcode for failure.
// Description: 
//    Copy in, if needed, the user input buffer pointed to by arg.
//    Call vme driver to perform actual ioctl
//    Copy out, if needed, the user output buffer pointed to be arg.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static int vme_ioctl(struct inode *inode,struct file *file,unsigned int cmd, unsigned long arg)
{
  unsigned int minor = MINOR(inode->i_rdev);
  vmeOutWindowCfg_t vmeOut;
  vmeInfoCfg_t      vmeInfo;
  vmeRequesterCfg_t vmeReq;
  vmeArbiterCfg_t   vmeArb;
  vmeInWindowCfg_t  vmeIn;
  virqInfo_t        vmeIrq;
  vmeRmwCfg_t       vmeRmw;
  vmeLmCfg_t        vmeLm;
  int	status;


  ioctls++;

  switch (minor & VME_MINOR_TYPE_MASK) {
    case VME_MINOR_OUT:
      switch (cmd) {
        case VME_IOCTL_GET_OUTBOUND:
          status = vmeGetOutBound(&vmeOut);
          if(status != 0) return(status);
          if (copy_to_user((void *)arg, &vmeOut, sizeof(vmeOut)))
            return(-EFAULT);
          break;  
        case VME_IOCTL_SET_OUTBOUND:
          if (copy_from_user(&vmeOut, (void *)arg, sizeof(vmeOut)))
            return(-EFAULT);
          status = vmeSetOutBound(&vmeOut);
          if(status != 0) return(status);
          break;  
        default:
          return(-EINVAL);
      }
      break;  

    case VME_MINOR_DMA:
      switch (cmd) {
        case VME_IOCTL_PAUSE_DMA:
        case VME_IOCTL_CONTINUE_DMA:
        case VME_IOCTL_ABORT_DMA:
        case VME_IOCTL_WAIT_DMA:
          return(-EINVAL);	/* Not supported for now. */
          break;  
        case VME_IOCTL_START_DMA:
          status = vmeIoctlDma((vmeDmaPacket_t *)arg, minor - VME_MINOR_DMA);
          if(status != 0) return(status);
          break;  
        default:
          return(-EINVAL);
      }
      break;  

    case VME_MINOR_MISC:
      switch (minor) {
        case VME_MINOR_CTL:
          switch (cmd) {
            case VME_IOCTL_GET_SLOT_VME_INFO:
              if (copy_from_user(&vmeInfo, (void *)arg, sizeof(vmeInfo)))
                return(-EFAULT);
              status = vmeGetSlotInfo(&vmeInfo);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeInfo, sizeof(vmeInfo)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_SET_REQUESTOR:
              if (copy_from_user(&vmeReq, (void *)arg, sizeof(vmeReq)))
                return(-EFAULT);
              status = vmeSetRequestor(&vmeReq);
              if(status != 0) return(status);
              break;  
            case VME_IOCTL_GET_REQUESTOR:
              if (copy_from_user(&vmeReq, (void *)arg, sizeof(vmeReq)))
                return(-EFAULT);
              status = vmeGetRequestor(&vmeReq);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeReq, sizeof(vmeReq)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_SET_CONTROLLER:
              if (copy_from_user(&vmeArb, (void *)arg, sizeof(vmeArb)))
                return(-EFAULT);
              status = vmeSetArbiter(&vmeArb);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeArb, sizeof(vmeArb)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_GET_CONTROLLER:
              status = vmeGetArbiter(&vmeArb);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeArb, sizeof(vmeArb)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_SET_INBOUND:
              if (copy_from_user(&vmeIn, (void *)arg, sizeof(vmeIn)))
                return(-EFAULT);
              status = vmeSetInBound( &vmeIn);
              if(status != 0) return(status);
              break;  
            case VME_IOCTL_GET_INBOUND:
              if (copy_from_user(&vmeIn, (void *)arg, sizeof(vmeIn)))
                return(-EFAULT);
              status = vmeGetInBound(&vmeIn);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeIn, sizeof(vmeIn)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_GENERATE_IRQ:

              if (copy_from_user(&vmeIrq, (void *)arg, sizeof(vmeIrq)))
                return(-EFAULT);

              // Obtain access to VIRQ generator.
              if(down_interruptible(&virq_inuse))
                  return(-ERESTARTSYS);
              status = vmeGenerateIrq(&vmeIrq);
              up(&virq_inuse); // Release VIRQ generator.

              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeIrq, sizeof(vmeIrq)))
                return(-EFAULT);
              break;  

            case VME_IOCTL_GET_IRQ_STATUS:
              if (copy_from_user(&vmeIrq, (void *)arg, sizeof(vmeIrq)))
                return(-EFAULT);
              status = vmeGetIrqStatus(&vmeIrq);
              if(status != 0) return(status);
              if (copy_to_user((void *)arg, &vmeIrq, sizeof(vmeIrq)))
                return(-EFAULT);
              break;  
            case VME_IOCTL_CLR_IRQ_STATUS:
              if (copy_from_user(&vmeIrq, (void *)arg, sizeof(vmeIrq)))
                return(-EFAULT);
              status = vmeClrIrqStatus(&vmeIrq);
              if(status != 0) return(status);
              break;  
            default:
              return(-EINVAL);
          }
          break;  
        case VME_MINOR_REGS:
          return(-EINVAL);
        case VME_MINOR_RMW:
          if (copy_from_user(&vmeRmw, (void *)arg, sizeof(vmeRmw)))
            return(-EFAULT);
          switch (cmd) {
            case VME_IOCTL_DO_RMW:
              status = vmeDoRmw(&vmeRmw);
              if(status != 0) return(status);
              break;  
            default:
              return(-EINVAL);
          }
          if (copy_to_user((void *)arg, &vmeRmw, sizeof(vmeRmw)))
            return(-EFAULT);
          break;  

        case VME_MINOR_LM:
          if (copy_from_user(&vmeLm, (void *)arg, sizeof(vmeLm)))
            return(-EFAULT);
          switch (cmd) {
            case VME_IOCTL_SETUP_LM:
              status = vmeSetupLm(&vmeLm);
              if(status != 0) return(status);
              break;  
            case VME_IOCTL_WAIT_LM:
              status = vmeWaitLm(&vmeLm);
              if(status != 0) return(status);
              break;  
            default:
              return(-EINVAL);
          }
          if (copy_to_user((void *)arg, &vmeLm, sizeof(vmeLm)))
            return(-EFAULT);
          break;  

        default:
          return(-EINVAL);
      }
      break;  

#if	0	// interboard data channels not implemented yet
    case VME_MINOR_SLOTS1:
    case VME_MINOR_SLOTS3:
      break;  
#endif
    default:
      return(-EINVAL);
  }	// masked minor
  return(status);
}

//-----------------------------------------------------------------------------
// Function   : findVmeBridge()
// Inputs     : void
// Outputs    : failed flag.
// Description: 
//    Search for a supported VME bridge chip.  
//    Initialize VME config registers.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static
struct pci_dev *
findVmeBridge()
{
  struct pci_dev *vme_pci_dev = NULL;

  if ((vme_pci_dev = pci_find_device(PCI_VENDOR_ID_TUNDRA,
                                     PCI_DEVICE_ID_TUNDRA_CA91C042, 
                                     vme_pci_dev))) {
    vmechip_devid = PCI_DEVICE_ID_TUNDRA_CA91C042;
  }
  if( vme_pci_dev == NULL) {
    if ((vme_pci_dev = pci_find_device(PCI_VENDOR_ID_TUNDRA,
                                       PCI_DEVICE_ID_TUNDRA_TEMPE,
                                       vme_pci_dev))) {
      vmechip_devid = PCI_DEVICE_ID_TUNDRA_TEMPE;
    }
  }
  if( vme_pci_dev == NULL) {
    printk("  VME bridge not found on PCI Bus.\n");
    return(vme_pci_dev);
  }

  // read revision.
  pci_read_config_dword(vme_pci_dev, PCI_CLASS, &vmechip_revision);
  printk("  PCI_CLASS = %08x\n", vmechip_revision);
  vmechip_revision &= 0xFF;

  // Unless user has already specified it, 
  // determine the VMEchip IRQ number.
  if(vmechip_irq == 0){
    pci_read_config_dword(vme_pci_dev, PCI_INTERRUPT_LINE, &vmechip_irq);
    printk("  PCI_INTLINE = %08x\n", vmechip_irq);
    vmechip_irq &= 0x000000FF;                    // Only a byte in size
    if(vmechip_irq == 0) vmechip_irq = 0x00000050;    // Only a byte in size
  }
  if((vmechip_irq == 0) || (vmechip_irq == 0xFF)){
    printk("Bad VME IRQ number: %02x\n", vmechip_irq);
    return(NULL);
  }

  // Lets turn Latency off
#if	0
  pci_write_config_dword(vme_pci_dev, PCI_MISC0, 0x8000);  /* XXX Why?? */
#endif


  return(vme_pci_dev);
}

//-----------------------------------------------------------------------------
// Function   : mapInVmeBridge()
// Inputs     : void
// Outputs    : failed flag.
// Description: 
//    Map VME chip registers in.  Perform basic sanity check of chip ID.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
int
mapInVmeBridge( struct pci_dev *vme_pci_dev)
{
  unsigned int temp, ba;

  // Setup VME Bridge Register Space
  // This is a 4k wide memory area that need to be mapped into the kernel
  // virtual memory space so we can access it.
  ba = pci_resource_start (vme_pci_dev, 0);
  vmechip_baseaddr = (char *)ioremap(ba,4096);
  if (!vmechip_baseaddr) {
    printk("  ioremap failed to map VMEchip to Kernel Space.\r");
    return 1;
  }

  // Check to see if the Mapping Worked out
  temp = readl(vmechip_baseaddr) & 0x0000FFFF;
  if (temp != PCI_VENDOR_ID_TUNDRA) {
    printk("  VME Chip Failed to Return PCI_ID in Memory Map.\n");
    iounmap(vmechip_baseaddr);
    return 1;
  }
  return(0);
}

//-----------------------------------------------------------------------------
// Function   : vmeDriverSetupWindows()
// Inputs     : void
// Outputs    : void
// Description: 
//    Creates initial inbound & outbound VME windows.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
static void
vmeDriverSetupWindows()
{
  vmeOutWindowCfg_t vmeDriverWindow;
  vmeInWindowCfg_t vmeInWindow;
  unsigned int temp, x;

  // Setup the outbound CRCSR window for use by the driver.
  memset(&vmeDriverWindow, 0, sizeof(vmeDriverWindow));
  vmeDriverWindow.windowNbr = 7;
  vmeDriverWindow.windowEnable = 1;
  vmeDriverWindow.windowSizeL = (512*1024)*32;
  vmeDriverWindow.xlatedAddrL = 0;
  vmeDriverWindow.wrPostEnable = 1;
  vmeDriverWindow.addrSpace = VME_CRCSR;
  vmeDriverWindow.maxDataWidth = VME_D32;
  vmeDriverWindow.xferProtocol = 0;
  vmeDriverWindow.dataAccessType = VME_DATA;
  vmeDriverWindow.userAccessType = VME_SUPER;
  vmeSetOutBound(&vmeDriverWindow);

  // Setup the inbound Windows
  if(vme_slotnum > 0){
    for(x = 0; x < 8; x++){
      memset(&vmeInWindow, 0, sizeof(vmeInWindow));
      if(in_image_size[x] != 0){
        memset(in_image_ba[x], 0, in_image_size[x]);
        vmeFlushRange(in_image_ba[x], in_image_ba[x]+in_image_size[x]);
        vmeInWindow.windowNbr = x;
        vmeInWindow.windowEnable = 1;
        vmeInWindow.windowSizeL = in_image_size[x];
	temp = virt_to_bus(in_image_ba[x]);
        vmeInWindow.pciAddrL = temp;
        temp = 0x80000000 + (vme_slotnum * 0x2000000) + (0x400000*x);
        vmeInWindow.vmeAddrL = temp;
        vmeInWindow.wrPostEnable = 1;
        vmeInWindow.addrSpace = VME_A32;
        vmeInWindow.xferProtocol = 
               VME_SCT | VME_BLT | VME_MBLT | VME_2eVME | VME_2eSST ;
	vmeInWindow.xferRate2esst = VME_SST320;	/*  2eSST Transfer Rate */
        vmeInWindow.dataAccessType = VME_DATA | VME_PROG;
        vmeInWindow.userAccessType = VME_SUPER | VME_USER;
        vmeSetInBound(&vmeInWindow);

      }
    }
  }
}

//-----------------------------------------------------------------------------
// Function   : cleanup_module
// Inputs     : void
// Outputs    : void
// Description: 
//    Initialize VME chip to quiescent state.
//    Free driver resources.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
void cleanup_module(void)
{          
  int x;


  // Queicse the hardware.
  switch(vmechip_devid) {
    case PCI_DEVICE_ID_TUNDRA_CA91C042:
      uni_shutdown();
      break;
    case PCI_DEVICE_ID_TUNDRA_TEMPE:
      tempe_shutdown();
      break;
  }

  // Free  & unmap resources
  free_irq(vmechip_irq,NULL);
  iounmap(vmechip_baseaddr);
  vme_free_buffers();

  for(x = 0; x < 8; x++){
    if(out_image_va[x] != 0) {
      iounmap((void *)out_image_va[x]);
      out_image_va[x] = 0;
    }
    if(out_resource[x].end - out_resource[x].start){
       iounmap((void *)out_image_va[x]);
       release_resource(&out_resource[x]);
       memset(&out_resource[x], 0, sizeof(struct resource));
    }
  }

  unregister_proc();
  unregister_chrdev(VME_MAJOR, "vme");
}

//-----------------------------------------------------------------------------
// Function   : init_module()
// Inputs     : void
// Outputs    : module initialization fail flag
// Description: 
//  top level module initialization.
//  find VME bridge
//  initialize VME bridge chip.
//  Sanity check bridge chip.
//  allocate and initialize driver resources.
// Remarks    : 
// History    : 
//-----------------------------------------------------------------------------
int init_module(void)
{
  int x;
  unsigned int temp;
  char vstr[80];
  struct resource pcimemres;
  struct pci_dev *vme_pci_dev = NULL;
  struct pci_dev *vme_ptp_pci_dev = NULL;
  unsigned int status;
  unsigned int *mv64360;

  sprintf(vstr,"\nTundra PCI-VME Bridge Driver %s\n",Version);
  printk(vstr);

  // find VME bridge
  vme_pci_dev = findVmeBridge();
  if (vme_pci_dev == NULL) {
    return 1;
  }

#define	DISABLE_MV64360_SNOOP_WORKAROUND 1
#ifdef	DISABLE_MV64360_SNOOP_WORKAROUND
  // This is ugly but needed for now.  Disable snoop at 64360.
  if(vmechip_devid == PCI_DEVICE_ID_TUNDRA_TEMPE){
     mv64360 = (unsigned int *)ioremap(0xF1000000,0x100000);
     mv64360[0x1E00/4] = mv64360[0x1E00/4] & ~0x0C000000;
     vmeSyncData();
     iounmap(mv64360);
  }
#endif

  // Get parent PCI resource & verify enough space is available.
  memset(&pcimemres,0,sizeof(pcimemres));
  pcimemres.flags = IORESOURCE_MEM;
  vmepcimem = pci_find_parent_resource(vme_pci_dev, &pcimemres);
  if(vmepcimem == 0){
    printk("Can't get VME parent device PCI resource\n");
    return(1);
  }

#define	VME_BEHIND_BRIDGE_WORKAROUND 1
#ifdef	VME_BEHIND_BRIDGE_WORKAROUND
  // If the VME bridge is behind a PCI bridge, only a meg of PCI MEM space
  // has been allocated.  Bump it up.  This is ugly too but is needed
  // esp on the MVME5500.
  if((vmepcimem->end - vmepcimem->start + 1 ) == 0x100000){
      // find bridge VME is under info
      vme_ptp_pci_dev = vme_pci_dev->bus->self;
      pci_write_config_word(vme_ptp_pci_dev, PCI_MEMORY_BASE, 0x8000);
      vmepcimem->start = 0x80000000;
  }
#endif

  if((vmepcimem->end - vmepcimem->start) < 0x2000000){
    printk("  Not enough PCI memory space available %08x\n", 
           (unsigned int)(vmepcimem->end - vmepcimem->start));
    return(1);
  }

  // Map in VME Bridge registers.
  if(mapInVmeBridge(vme_pci_dev)){
    return(1);
  }

  // Unless, user has already specified it, 
  // determine the VME slot that we are in.
  if(vme_slotnum == -1){
     vme_slotnum = vmeGetSlotNum();
  }
  if(vme_slotnum <= 0){
    printk("Bad VME slot #%02d\n", vme_slotnum);
    return(1);
  }

  // Initialize wait queues & mutual exclusion flags
  init_waitqueue_head(&dma_queue[0]);
  init_waitqueue_head(&dma_queue[1]);
  init_waitqueue_head(&lm_queue);
  init_waitqueue_head(&mbox_queue);
  sema_init(&virq_inuse, 1);
  for (x=0;x<MAX_MINOR+1;x++) {
    opened[x]    = 0;             
    sema_init(&devinuse[x], 1);
  }  

  // allocate buffers needed by the driver.
  vme_alloc_buffers();

  // Initialize the interboard data structure.
  if(vmechip_interboard_data != NULL){
    vme_init_interboard_data(vmechip_interboard_data);
  }

  // Display VME information  
  pci_read_config_dword(vme_pci_dev, PCI_CSR, &status);
  printk("  Vendor = %04X  Device = %04X  Revision = %02X Status = %08X\n",
         vme_pci_dev->vendor,vme_pci_dev->device, vmechip_revision, status);
  printk("  Class = %08X\n",vme_pci_dev->class);

  pci_read_config_dword(vme_pci_dev, PCI_MISC0, &temp);
  printk("  Misc0 = %08X\n",temp);      
  printk("  Irq = %04X\n",vmechip_irq);      
  if(vme_slotnum == -1){
     printk("VME slot number: unknown\n");
  } else {
     printk("VME slot number: %d\n", vme_slotnum);
  }


  // Initialize chip registers and register module.
  if(!vmeInit(vmechip_interboard_data)){
     printk("  VME Chip Initialization failed.\n");
     iounmap(vmechip_baseaddr);
     vme_free_buffers();
     return(1);
  }
  if (register_chrdev(VME_MAJOR, "vme", &vme_fops)) {
    printk("  Error getting Major Number for Drivers\n");
    iounmap(vmechip_baseaddr);
    vme_free_buffers();
    return(1);
  }

  if(vme_syscon == 0){
     printk("Is not VME system controller\n");
  } 
  if(vme_syscon == 1){
     printk("Is VME system controller\n");
  } 
  printk("  Tundra VME Loaded.\n\n"); 

  register_proc();

  // Get time base speed.  Needed to support DMA throughput measurements
  tb_speed = tb_ticks_per_jiffy * HZ;

  // Open up the default VME windows.
  vmeDriverSetupWindows();

  return 0;
}
