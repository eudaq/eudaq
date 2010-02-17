/*--------------------------------------------------------------------------------------
 *Title          :       TSI148 (TUNDRA) Driver library
 *Date:          :       27-10-2006
 *Programmer     :       Chiarelli Lorenzo
 *Platform       :       Linux (ELinOS) PowerPC
 *File           :       vmelib.c
 *Version        :       1.3
 *--------------------------------------------------------------------------------------
 *License        :       You are free to use this source files for your own development as long
 *                               as it stays in a public research context. You are not allowed to use it
 *                               for commercial purpose. You must put this header with
 *                               authors names in all development based on this library.
 *--------------------------------------------------------------------------------------
 */

#include <sys/ioctl.h>  /*ioctl()*/
#include <unistd.h> /*close() read() write()*/
#include <sys/types.h> /*open()*/
#include <sys/stat.h> /*open()*/
#include <fcntl.h>  /*open()*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "vmedrv.h" /*header file del driver vmelinux */
#include "vmelib.h"/* header file della mia libreria*/

static void outbound_ctl(int fdOut, vmeOutWindowCfg_t *vmeOutSet);
/*static short int byte_swap(short int num);*/
static void flushLine(void *ramptr);
static int dodmaxfer(int bytecount,unsigned long int srcaddress,unsigned long int dstaddress,int xfer);
void set_parameters(int fdOut,int window_number,unsigned long int address,int xferRate2esst,int addrSpace,int maxDataWidth,int xferProtocol,int userAccessType,int dataAccessType);

#define WINDOW_SIZE 0x01000000
#define MASK_LOW    (WINDOW_SIZE - 1)
#define MASK_HIGH   (0xffffffff - MASK_LOW)

/*
 *       Utilizzata per controllare se i campi della struttura vmeOutWindowCfg_t 
 *       sono stati impostati correttamente dopo una  chiamata ioctl VME_IOCTL_SET_OUTBOUND
 */
static void outbound_ctl(int fdOut,vmeOutWindowCfg_t *vmeOutSet)
{
  vmeOutWindowCfg_t vmeOutGet;
  int status;
  memset(&vmeOutGet, 0, sizeof(vmeOutGet));
  vmeOutGet.windowNbr = vmeOutSet->windowNbr;
  status = ioctl(fdOut, VME_IOCTL_GET_OUTBOUND, &vmeOutGet);
  if (status < 0) 
    {
      printf("Ioctl failed.  Errno = %d\n", errno);
      _exit(1);
    }
  if (vmeOutGet.addrSpace != vmeOutSet->addrSpace ) 
    {
      printf("%08x %08x\n", vmeOutGet.addrSpace, vmeOutSet->addrSpace);
      printf("addrSpace was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.userAccessType != vmeOutSet->userAccessType) 
    {
      printf("userAccessType was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.dataAccessType != vmeOutSet->dataAccessType) 
    {
      printf("dataAccessType was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.addrSpace != vmeOutSet->addrSpace ) 
    {
      printf("addrSpace was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.windowEnable != vmeOutSet->windowEnable) 
    {
      printf("windowEnable was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.windowSizeL != vmeOutSet->windowSizeL) 
    {
      printf("windowSizeL was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.xlatedAddrU != vmeOutSet->xlatedAddrU) 
    {
      printf("xlatedAddrU was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.xlatedAddrL != vmeOutSet->xlatedAddrL) 
    {
      printf("xlatedAddrL was not configured \n");
      printf("%08x %08x\n",vmeOutGet.xlatedAddrL, vmeOutSet->xlatedAddrL);
      _exit(1);
    }
  if (vmeOutGet.maxDataWidth != vmeOutSet->maxDataWidth) 
    {
      printf("maxDataWidth was not configured \n");
      _exit(1);
    }
  if (vmeOutGet.xferProtocol != vmeOutSet->xferProtocol) 
    {
      printf("%08x %08x\n", vmeOutGet.xferProtocol,vmeOutSet->addrSpace);
      printf("xferProtocol was not configured \n");
      _exit(1);
    }
}

/*
 *       Utilizzata per invertire i byte per una corretta scrittura D16
 */

/*
static short int byte_swap(short int num)
{
  short int temp=num;
  return((temp&0xff00)>>8|(num&0x00ff)<<8);
}
*/

/*
 *       Utilizzata per i trasferimenti BLT MBLT 2eVME 2eSST 
 *       DCBF RA,RB Data Chache Block Flush 
 *       SYNC
 *       Operazioni assembler per PowerPC
 *       Queste istruzioni servono per l'utilizzo del DMA
 *
 *       Secondo me serve a trasferire il contenuto della cache perche' alloco il vettore di
 *       destinazione poi lo porto in ram effettuo il trasferimento
 *
 *       il TSI148 prende il controllo della RAM ed effettua un trasferimento ad alta banda
 *       poi quando il processore riprende il controllo della ram bisogna svuotare prima la cache del
 *       processore e poi copiare i dati dalla ram alla cache
 */

static void flushLine(void * ramptr)
{
  (void)ramptr; /* Disable unused parameter warning */
#if defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC) || \
    defined(__powerpc__) || defined(__powerpc)
  __asm__("dcbf 0,3");
  __asm__("sync");
#else
# warning "Old VME library is only compatible with Motorola CPUs"
#endif
}

/*
 *       Utilizzata per i trasferimenti BLT MBLT 2eVME 2eSST 
 *       Esegue il trasferimento dei dati dal VMEbus al bus PCI-X
 *       
 */

static int dodmaxfer(int bytecount,unsigned long int srcaddress,unsigned long int dstaddress,int xfer)
{
  int fd;               /*File descriptor del device aperto(vme_dma#)*/
  int status;           /*Per controllare se ioctl e' andata a buon fine*/

  vmeDmaPacket_t vmeDma;        /*Struttura da riempire (vedere vmedrv.h)*/

  fd = open("/dev/vme_dma0",O_RDWR);
  if (fd < 0) {
    printf("Open failed.  Errno = %d\n",errno);
    _exit(1);
  }

  memset(&vmeDma, 0, sizeof(vmeDma));     /*Inizializzazione di vmeDma */
        
  vmeDma.maxPciBlockSize = 4096;          /*Grandezza massima dei blocchi VME e PCI*/
  vmeDma.maxVmeBlockSize = 2048;
        
  vmeDma.byteCount = bytecount;           /*Numero di byte da leggere/scrivere*/
        
  /*
   *Impostazioni della struttura   
   *per il sorgente
   */
  vmeDma.srcBus = VME_DMA_VME;                    /*Bus sorgente VME*/
  vmeDma.srcAddr = srcaddress;                    /*Indirizzo sorgente o Buffer sorgente*/
  vmeDma.srcVmeAttr.maxDataWidth = VME_D32;       /*Numero di bit per i dati*/
  vmeDma.srcVmeAttr.addrSpace = VME_A32;          /*Numero di bit per gli indirizzi*/
  vmeDma.srcVmeAttr.userAccessType = VME_USER;    /*Tipo di accesso*/
  vmeDma.srcVmeAttr.dataAccessType = VME_DATA;    
  vmeDma.srcVmeAttr.xferProtocol = xfer;          /*Protocollo di trasferimento
                                                   *MBLT,2eVME,2eSST hanno 64 bit per i dati
                                                   *ma non bisogna cambiare maxDataWidth=VME_D32
                                                   */
  /*
   *Impostazioni della struttura
   *per la destinazione 
   */
  vmeDma.dstBus = VME_DMA_USER;                   /*Bus destinazione USER*/
  vmeDma.dstAddr = dstaddress;                    /*Indirizzo destinazione o Buffer di destinazione*/
  vmeDma.dstVmeAttr.maxDataWidth = VME_D32;       /*Numero di bit per i dati*/
  vmeDma.dstVmeAttr.addrSpace = VME_A32;          /*Numero di bit per gli indirizzi*/
  vmeDma.dstVmeAttr.userAccessType = VME_USER;    /*Tipo di accesso*/
  vmeDma.dstVmeAttr.dataAccessType = VME_DATA;
  vmeDma.dstVmeAttr.xferProtocol = xfer;          /*Protocollo di trasferimento
                                                   *MBLT,2eVME,2eSST hanno 64 bit per i dati
                                                   *ma non bisogna cambiare maxDataWidth=VME_D32
                                                   */
        
  status = ioctl(fd, VME_IOCTL_START_DMA, &vmeDma);
        
  if (status < 0) {
    printf("VME_IOCTL_START_DMA failed.  Errno = %d\n", errno);
    _exit(1);
  }
        
  close(fd);
  return(vmeDma.vmeDmaElapsedTime);
}

/*
 *       Utilizzata da tutte le funzioni SCT 
 *       per il settaggio dei parametri delle finestre
 */

void set_parameters(int fdOut,int window_number,unsigned long int address,int xferRate2esst,int addrSpace,int maxDataWidth,int xferProtocol,int userAccessType,int dataAccessType)
{
  int status = 0;
  static unsigned long vme_prevaddr = (unsigned long)-1;
  static int vme_prevspace = -65536;
  vmeOutWindowCfg_t vmeOutSet;            /*
                                           * Struttura da riempire per fare 
                                           * VME_IOCTL_SET_OUTBOUND con ioctl
                                           * Imposta i parametri della modalita' di lettura/scrittura
                                           */ 
                                                
  /*off_t offset;                                 //Offset della lseek*/

  if (vme_prevspace != addrSpace || vme_prevaddr != (address & MASK_HIGH)) {
    vme_prevspace = addrSpace;
    vme_prevaddr = (address & MASK_HIGH);

    memset(&vmeOutSet, 0, sizeof(vmeOutSet));       /*Inizializzazione di vmeOutSet*/
    
    /*
     *       Preparazione dei della struttura per effettuare 
     *       il settaggio dei parametri di accesso 
     */
    vmeOutSet.windowNbr = window_number;  /* "Numero della Finestra"
                                           *  E' il numero del registro OTAT [0...7] del TSI148
                                           *  a livello logico e' meglio usare window numer=vme_m#
                                           */                      
    vmeOutSet.windowEnable = 1;           /* 
                                           *  Abilitazione del registro OTAT 
                                           *  0 disabilitato per default
                                           */
    
    vmeOutSet.windowSizeL = WINDOW_SIZE;             /* Grandezza della finestra impostato a 16M*/
    
    /* Indirizzo di base dello slave su cui 
     *  si vuole leggere/scrivere
     *  xlatedAddrU parte alta 
     *  di un indirizzo a 64-bit (a63...a32)
     */
    vmeOutSet.xlatedAddrU = 0;             
    /*
     *  Indirizzo di base dello slave su cui 
     *  si vuole leggere/scrivere
     *  xlatedAddrL parte bassa 
     *  di un indirizzo a 64-bit (a31...a1)
     */
    vmeOutSet.xlatedAddrL = address & MASK_HIGH;
    
    vmeOutSet.xferRate2esst = xferRate2esst;        /* Abilitazione del clock 
                                                     *  per effettuare un trasferimento sincrono 2eSST
                                                     */   
    vmeOutSet.addrSpace = addrSpace;                        /*Numero di bit per gli indirizzi       OTAT*/
    vmeOutSet.maxDataWidth = maxDataWidth;          /*Numero di bit per i dati      OTAT*/
    vmeOutSet.xferProtocol = xferProtocol;          /* 
                                                     * Protocollo di trasferimento 
                                                     * Single Cycle Transfer OTAT
                                                     */
    vmeOutSet.userAccessType = userAccessType;      /* Accesso di tipo User/Supervisor OTAT*/
    vmeOutSet.dataAccessType = dataAccessType;      /* Accesso di tipo Data/Program OTAT*/
    
    /*printf("DEBUG: Setting parameters, addr = 0x%8lx, space=%d\n", address, addrSpace);*/
    /*
     * Scrittura sul registro OTAT attraverso ioctl dei parametri impostati
     */
    status = ioctl(fdOut, VME_IOCTL_SET_OUTBOUND, &vmeOutSet);
    if (status < 0) 
      {
        printf(" VME_IOCTL_SET_OUTBOUND failed.  Errno = %d\n", errno);
        _exit(1);
      }
    outbound_ctl(fdOut,&vmeOutSet);                 /*Controllo se i parametri settati sono stati
                                                     * correttamente scritti sul registro OTAT
                                                     * rileggendoli e facendo un match
                                                     */
  }
}


/*
 *       Utilizzata per ottenere informazioni riguardo la scheda MVME6100 
 *       con la chiamata ioctl VME_IOCTL_GET_SLOT_VME_INFO
 */

int getMyInfo()
{
  vmeInfoCfg_t myVmeInfo; 

  int fdInfo;    /*File descriptor del device aperto(vme_ctl) */
  int status;    /*Per controllare se ioctl e' andata a buon fine*/
  fdInfo = open("/dev/vme_ctl", O_RDONLY); /*Costante O_RDONLY=0*/
  if (fdInfo == -1) {
    return 1;
  }

  memset(&myVmeInfo, 0, sizeof(myVmeInfo)); 
        
  status = ioctl(fdInfo, VME_IOCTL_GET_SLOT_VME_INFO, &myVmeInfo); 
        
  close(fdInfo);

  if (status == -1) {
    return 1;
  }
        
  return 0;
}

/*
 *       Elenco Funzioni di lettura Single Cyle Tranfer
 */

int vme_A32_D32_User_Data_SCT_read(int fdOut,unsigned long int *readdata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di read e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A32,VME_D32,VME_SCT,VME_USER,VME_DATA);
/*
 *       Operazione di lettura dal bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di lettura di 4 byte D32
 */
  nbyte=4; /*Impostazione del numero di byte da leggere*/
        
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET); /*Impostazione dell'offset dal base address dello slave*/
        
  n = read(fdOut,readdata,nbyte); /*Lettura*/
        
  if (n != nbyte) 
    printf("read failed errno = %d at address %08lx\n",errno,address);
        
  return(n); 
}

int vme_A32_D16_User_Data_SCT_read(int fdOut,unsigned short int *readdata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di read e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
        
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A32,VME_D16,VME_SCT,VME_USER,VME_DATA);
/*
 *       Operazione di lettura dal bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di lettura di 4 byte D32
 */
  nbyte=2; /*Impostazione del numero di byte da leggere*/
        
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET); /*Impostazione dell'offset dal base address dello slave*/
        
  n = read(fdOut,readdata,nbyte); /*Lettura*/
        
  if (n != nbyte) 
    printf("read failed errno = %d at address %08lx\n",errno,address);
        
  return(n); 
}

int vme_A24_D16_User_Data_SCT_read(int fdOut,unsigned  short int *readdata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di read e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A24,VME_D16,VME_SCT,VME_USER,VME_DATA);
/*
 *       Operazione di lettura dal bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di lettura di 4 byte D32
 */
  nbyte=2; /*Impostazione del numero di byte da leggere*/
        
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET); /*Impostazione dell'offset dal base address dello slave*/
        
  n = read(fdOut,readdata,nbyte); /*Lettura*/
        
  if (n != nbyte) 
    printf("read failed errno = %d at address %08lx\n",errno,address);
        
  return(n); 
}

int vme_A24_D32_User_Data_SCT_read(int fdOut,unsigned long int *readdata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di read e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A24,VME_D32,VME_SCT,VME_USER,VME_DATA);
/*
 *       Operazione di lettura dal bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di lettura di 4 byte D32
 */
  nbyte=4; /*Impostazione del numero di byte da leggere*/
        
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET); /*Impostazione dell'offset dal base address dello slave*/
        
  n = read(fdOut,readdata,nbyte); /*Lettura*/
        
  if (n != nbyte) 
    printf("read failed errno = %d at address %08lx\n",errno,address);
        
  return(n); 
}



/*
 *       Elenco Funzioni di scrittura Single Cyle Tranfer
 */


int vme_A32_D32_User_Data_SCT_write(int fdOut,unsigned long int writedata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di write e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A32,VME_D32,VME_SCT,VME_USER,VME_DATA);
        
/*
 *       Operazione di scrittura sul bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di scrittura di 4 byte D32
 */      
  nbyte=4;        /*Impostazione del numero di byte da scrivere*/
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET);/*Impostazione dell'offset dal base address dello slave*/
        
  n = write(fdOut,&writedata,nbyte);      /*Scrittura*/
        
  if (n != nbyte) 
    {
      printf("write failed errno = %d at address %08lx\n",errno, address);
                
    }
  return(n);
}

int vme_A32_D16_User_Data_SCT_write(int fdOut,unsigned short int writedata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di write e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A32,VME_D16,VME_SCT,VME_USER,VME_DATA);
        
/*
 *       Operazione di scrittura sul bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di scrittura di 4 byte D32
 */      
  nbyte=2;        /*Impostazione del numero di byte da scrivere*/
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET);/*Impostazione dell'offset dal base address dello slave*/
        
  n = write(fdOut,&writedata,nbyte);      /*Scrittura*/
        
  if (n != nbyte) 
    {
      printf("write failed errno = %d at address %08lx\n",errno, address);
                
    }
  return(n);
}

int vme_A24_D32_User_Data_SCT_write(int fdOut,unsigned long int writedata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di write e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A24,VME_D32,VME_SCT,VME_USER,VME_DATA);
        
/*
 *       Operazione di scrittura sul bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di scrittura di 4 byte D32
 */      
  nbyte=4;        /*Impostazione del numero di byte da scrivere*/
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET);/*Impostazione dell'offset dal base address dello slave*/
        
  n = write(fdOut,&writedata,nbyte);      /*Scrittura*/
        
  if (n != nbyte) 
    {
      printf("write failed errno = %d at address %08lx\n",errno, address);
                
    }
  return(n);
}

int vme_A24_D16_User_Data_SCT_write(int fdOut,unsigned short int writedata,unsigned long int address)
{
  size_t n=0;                              /*Per controllare se l'operazione di write e' andata a buon fine*/
  size_t nbyte=0;                               /*Byte da leggere per la read*/
  off_t offset;                                 /*Offset della lseek*/
  set_parameters(fdOut,0,address,VME_SSTNONE,VME_A24,VME_D16,VME_SCT,VME_USER,VME_DATA);
        
/*
 *       Operazione di scrittura sul bus VME 
 *       1-Impostazione della parte meno significativa
 *         dell'indirizzo con lseek dall'inizio del file dev_m#
 *       2-Operazione di scrittura di 4 byte D32
 */      
  nbyte=2;        /*Impostazione del numero di byte da scrivere*/
  offset=lseek(fdOut,address & MASK_LOW,SEEK_SET);/*Impostazione dell'offset dal base address dello slave*/
        
  n = write(fdOut,&writedata,nbyte);      /*Scrittura*/
        
  if (n != nbyte) 
    {
      printf("write failed errno = %d at address %08lx\n",errno, address);
                
    }
  return(n);
}


/*
 *       Elenco Funzioni di lettura BLT MBLT 32 byte only
 */


int  vme_A32_D32_User_Data_MBLT_read(int bytecount,unsigned long int srcaddress,unsigned long int *dmadstbuffer)
{
  int i=0;
  /*Inizializzazione della memoria*/

  for (i = 0; i < bytecount/4; i++)
    dmadstbuffer[i] = -99;
  for (i = 0; i <bytecount/4; i++)
    flushLine(&dmadstbuffer[i]); 
        
  /*leggo in modalita' MBLT*/
  /*byte da leggere,indirizzo VMEsorgente,buffer di destinazione*/
        
  dodmaxfer(bytecount,srcaddress,(unsigned long int )dmadstbuffer,VME_MBLT);
                
  return(0);
}

/* Funzioni di lettura BLT MBLT 2eVME 2eSST 
 */


int  vme_A32_D64_User_Data_MBLT_read(int bytecount,unsigned long int srcaddress,unsigned long int *dmadstbuffer)
{
  int i=0;
  /*Inizializzazione della memoria*/

  for (i = 0; i < bytecount/4; i++)
    dmadstbuffer[i] = -99;
  for (i = 0; i <bytecount/4; i++)
    flushLine(&dmadstbuffer[i]); 
        
  /*leggo in modalita' MBLT*/
  /*byte da leggere,indirizzo VMEsorgente,buffer di destinazione*/
        
  dodmaxfer(bytecount,srcaddress,(unsigned long int )dmadstbuffer,VME_2eVME);
                
  return(0);
}

