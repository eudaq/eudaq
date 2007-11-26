/*
 * Module name: vmedrv.h
 *	Application interface to VME device nodes.
 */

/*
 * The reserved elements in the following structures are 
 * intended to allow future versions of the driver to 
 * maintain backwards compatibility.  Applications should 
 * initialize reserved locations to zero.
 */

#ifndef	VMEDRV_H
#define	VMEDRV_H

#define	VMEDRV_REV	0x0301

#define	VME_MINOR_TYPE_MASK	0xF0
#define	VME_MINOR_OUT	0x00
#define	VME_MINOR_DMA	0x10
#define	VME_MINOR_MISC	0x20
#define	VME_MINOR_SLOTS1	0x30
#define	VME_MINOR_SLOTS2	0x40

#define	VME_MINOR_CTL	0x20
#define	VME_MINOR_REGS	0x21
#define	VME_MINOR_RMW	0x22
#define	VME_MINOR_LM	0x23
#ifndef	PAGESIZE
#define	PAGESIZE 4096
#endif
#ifndef	LINESIZE
#define	LINESIZE 0x20
#endif
#define	VME_MAX_WINDOWS	8
#ifndef	PCI_DEVICE_ID_TUNDRA_TEMPE
#define	PCI_DEVICE_ID_TUNDRA_TEMPE 0x148
#endif

/************************************************************************
 * COMMON definitions 
 */

/*
 *  VME access type definitions
 */
#define	VME_DATA		1
#define	VME_PROG		2
#define	VME_USER		4
#define	VME_SUPER		8

/*
 *  VME address type definitions
 */
typedef enum {
        VME_A16,
        VME_A24,
        VME_A32,
        VME_A64,
        VME_CRCSR,
        VME_USER1,
        VME_USER2,
        VME_USER3,
        VME_USER4
} addressMode_t;

/*
 *  VME Transfer Protocol Definitions
 */
#define	VME_SCT			0x1
#define	VME_BLT			0x2
#define	VME_MBLT		0x4
#define	VME_2eVME		0x8
#define	VME_2eSST		0x10
#define	VME_2eSSTB		0x20

/*
 *  Data Widths 
 */
typedef enum {
        VME_D8 = 8,
        VME_D16 = 16,
        VME_D32 = 32,
        VME_D64 = 64
} dataWidth_t;

/*
 *  2eSST Data Transfer Rate Codes
 */
typedef enum {
	VME_SSTNONE = 0,
	VME_SST160 = 160,
	VME_SST267 = 267,
	VME_SST320 = 320
} vme2esstRate_t;


/*
 *  Arbitration Scheduling Modes
 */
typedef enum {
	VME_R_ROBIN_MODE,
	VME_PRIORITY_MODE
} vme2ArbMode_t;


/************************************************************************
 *  /dev/vme_m* Outbound Window Ioctl Commands
 */
#define	VME_IOCTL_SET_OUTBOUND		0x10
#define	VME_IOCTL_GET_OUTBOUND		0x11
/*
 *  VMEbus OutBound Window Arg Structure
 *  NOTE:
 *	If pciBusAddr[U,L] are 0, then kernel will dynamically assign
 *	pci start address on PCI bus.
 */
struct	vmeOutWindowCfg
{
	int		windowNbr;	/*  Window Number */
	char		windowEnable;	/*  State of Window */
	unsigned int	pciBusAddrU;	/*  Start Address on the PCI Bus */
	unsigned int	pciBusAddrL;	/*  Start Address on the PCI Bus */
	unsigned int	windowSizeU;	/*  Window Size */
	unsigned int	windowSizeL;	/*  Window Size */
	unsigned int	xlatedAddrU;	/*  Starting Address on the VMEbus */
	unsigned int	xlatedAddrL;	/*  Starting Address on the VMEbus */
	int		bcastSelect2esst;	/*  2eSST Broadcast Select */
	char		wrPostEnable;		/*  Write Post State */
	char		prefetchEnable;	/*  Prefetch Read Enable State */
	int		prefetchSize;	/*  Prefetch Read Size (in Cache Lines) */
	vme2esstRate_t	xferRate2esst;	/*  2eSST Transfer Rate */
	addressMode_t	addrSpace;	/*  Address Space */
	dataWidth_t	maxDataWidth;	/*  Maximum Data Width */
	int		xferProtocol;	/*  Transfer Protocol */
	int		userAccessType;	/*  User/Supervisor Access Type */
	int		dataAccessType;	/*  Data/Program Access Type */
	int		reserved;	/* For future use */

};
typedef	struct	vmeOutWindowCfg	vmeOutWindowCfg_t;



/************************************************************************
 *  /dev/vme_dma* DMA commands
 */
#define	VME_IOCTL_START_DMA		0x30
#define	VME_IOCTL_PAUSE_DMA		0x31
#define	VME_IOCTL_CONTINUE_DMA		0x32
#define	VME_IOCTL_ABORT_DMA		0x33
#define	VME_IOCTL_WAIT_DMA		0x34

typedef enum {
	/* NOTE: PATTERN entries only valid as source of data */
	VME_DMA_PATTERN_BYTE,
	VME_DMA_PATTERN_BYTE_INCREMENT,
	VME_DMA_PATTERN_WORD,
	VME_DMA_PATTERN_WORD_INCREMENT,
	VME_DMA_USER,
	VME_DMA_KERNEL,
	VME_DMA_PCI,
	VME_DMA_VME
} dmaData_t;

/*
 *  VMEbus Transfer Attributes 
 */
struct	vmeAttr
{
	dataWidth_t	maxDataWidth;	/*  Maximum Data Width */
	vme2esstRate_t	xferRate2esst;	/*  2eSST Transfer Rate */
	int		bcastSelect2esst; /*  2eSST Broadcast Select */
	addressMode_t	addrSpace;	/*  Address Space */
	int		userAccessType;	/*  User/Supervisor Access Type */
	int		dataAccessType;	/*  Data/Program Access Type */
	int		xferProtocol;	/*  Transfer Protocol */
	int		reserved;	/* For future use */
};
typedef	struct	vmeAttr	vmeAttr_t;

/*
 *  DMA arg info
 *  NOTE: 
 *	structure contents relating to VME are don't care for 
 *       PCI transactions
 *	structure contents relating to PCI are don't care for 
 *       VME transactions
 *	If source or destination is user memory and transaction
 *	will cross page boundary, the DMA request will be split
 *	into multiple DMA transactions.
 */
typedef	struct	vmeDmaPacket
{
	int		vmeDmaToken;	/*  Token for driver use */
	int		vmeDmaWait;	/*  Time to wait for completion */
	unsigned int	vmeDmaStartTick; /*  Time DMA started */
	unsigned int	vmeDmaStopTick; /*  Time DMA stopped */
	unsigned int	vmeDmaElapsedTime; /*  Elapsed time */
	int		vmeDmaStatus;   /*  DMA completion status */

  	int  	byteCount;	/*  Byte Count */
	int	bcastSelect2esst;	/*  2eSST Broadcast Select */

	/*
  	 *  DMA Source Data
 	 */
	dmaData_t	srcBus;
	unsigned int		srcAddrU;
	unsigned int		srcAddr;
	int		pciReadCmd;
	struct		vmeAttr	srcVmeAttr;
	char		srcfifoEnable;

	/*
  	 *  DMA Destination Data
 	 */
	dmaData_t	dstBus;
	unsigned int		dstAddrU;
	unsigned int		dstAddr;
	int		pciWriteCmd;
	struct		vmeAttr	dstVmeAttr;
	char		dstfifoEnable;


	/*
  	 *  BUS usage control
 	 */
	int		maxCpuBusBlkSz;	/*  CPU Bus Maximum Block Size */
	int		maxPciBlockSize;	/* PCI Bus Maximum Block Size */
	int		pciBackOffTimer;	/* PCI Bus Back-Off Timer */
	int		maxVmeBlockSize;	/* VMEbus Maximum Block Size */
	int		vmeBackOffTimer;	/* VMEbus Back-Off Timer */

	int		channel_number;	/* Channel number */
	int		reserved;	/* For future use */
	/*
	 *	Ptr to next Packet
	 *	(NULL == No more Packets)
	 */
  	struct		vmeDmaPacket *  pNextPacket;

} vmeDmaPacket_t;


/************************************************************************
 *  /dev/vme_ctl ioctl Commands
 */
#define	VME_IOCTL_GET_SLOT_VME_INFO	0x41
/*
 *  VMEbus GET INFO Arg Structure
 */
struct	vmeInfoCfg
{
	int		vmeSlotNum;	/*  VME slot number of interest */
	int		boardResponded;	/* Board responded */
	char		sysConFlag;	/*  System controller flag */
	int		vmeControllerID; /*  Vendor/device ID of VME bridge */
	int		vmeControllerRev; /*  Revision of VME bridge */
	char		osName[8];	/*  Name of OS e.g. "Linux" */
	int		vmeSharedDataValid;	/*  Validity of data struct */
	int		vmeDriverRev;	/*  Revision of VME driver */
	unsigned	int vmeAddrHi[8];  /* Address on VME bus */
	unsigned	int vmeAddrLo[8];  /* Address on VME bus */
	unsigned	int vmeSize[8];  /* Size on VME bus */
	unsigned	int vmeAm[8];  /* Address modifier on VME bus */
	int		reserved;	/* For future use */
};
typedef	struct	vmeInfoCfg	vmeInfoCfg_t;

#define	VME_IOCTL_SET_REQUESTOR		0x42
#define	VME_IOCTL_GET_REQUESTOR		0x43
/*
 *  VMEbus Requester Arg Structure
 */
struct	vmeRequesterCfg
{
	int		requestLevel;	/*  Requester Bus Request Level */
	char		fairMode;	/*  Requester Fairness Mode Indicator */
	int		releaseMode;	/*  Requester Bus Release Mode */
	int		timeonTimeoutTimer;	/*  Master Time-on Time-out Timer */
	int		timeoffTimeoutTimer;	/*  Master Time-off Time-out Timer */
	int		reserved;	/* For future use */
};
typedef	struct	vmeRequesterCfg	vmeRequesterCfg_t;

#define	VME_IOCTL_SET_CONTROLLER	0x44
#define	VME_IOCTL_GET_CONTROLLER	0x45
/*
 *  VMEbus Arbiter Arg Structure
 */
struct	vmeArbiterCfg
{
	vme2ArbMode_t	arbiterMode;	/*  Arbitration Scheduling Algorithm */
	char		arbiterTimeoutFlag;	/*  Arbiter Time-out Timer Indicator */
	int		globalTimeoutTimer;	/*  VMEbus Global Time-out Timer */
	char		noEarlyReleaseFlag;	/*  No Early Release on BBUSY */
	int		reserved;	/* For future use */
};
typedef	struct	vmeArbiterCfg	vmeArbiterCfg_t;

#define	VME_IOCTL_GENERATE_IRQ		0x46
#define	VME_IOCTL_GET_IRQ_STATUS	0x47
#define	VME_IOCTL_CLR_IRQ_STATUS	0x48
/*
 *  VMEbus IRQ Info
 */
typedef	struct	virqInfo
{
	/*
	 *  Time to wait for Event to occur (in clock ticks)
	 */
	short			waitTime;
	short			timeOutFlag;

	/*
	 *  VMEbus Interrupt Level and Vector Data
	 */
	int		level;
	int		vector;
	int		reserved;	/* For future use */

} virqInfo_t;

#define	VME_IOCTL_SET_INBOUND		0x49
#define	VME_IOCTL_GET_INBOUND		0x50
/*
 *  VMEbus InBound Window Arg Structure
 *  NOTE:
 *	If pciBusAddr[U,L] and windowSize[U,L] are 0, then kernel 
 *      will dynamically assign inbound window to map to a kernel
 *	supplied buffer.
 */
struct	vmeInWindowCfg
{
	int		windowNbr;	/*  Window Number */
	char		windowEnable;	/*  State of Window */
	unsigned int		vmeAddrU;	/*  Start Address responded to on the VMEbus */
	unsigned int		vmeAddrL;	/*  Start Address responded to on the VMEbus */
	unsigned int		windowSizeU;	/*  Window Size */
	unsigned int		windowSizeL;	/*  Window Size */
	unsigned int		pciAddrU;	/*  Start Address appearing on the PCI Bus */
	unsigned int		pciAddrL;	/*  Start Address appearing on the PCI Bus */
	char		wrPostEnable;	/*  Write Post State */
	char		prefetchEnable;	/*  Prefetch Read State */
	char		prefetchThreshold;	/*  Prefetch Read Threshold State */
	int		prefetchSize;	/*  Prefetch Read Size */
	char		rmwLock;	/*  Lock PCI during RMW Cycles */
	char		data64BitCapable;	/*  non-VMEbus capable of 64-bit Data */
	addressMode_t	addrSpace;	/*  Address Space */
	int		userAccessType;	/*  User/Supervisor Access Type */
	int		dataAccessType;	/*  Data/Program Access Type */
	int		xferProtocol;	/*  Transfer Protocol */
	vme2esstRate_t	xferRate2esst;	/*  2eSST Transfer Rate */
	char		bcastRespond2esst;	/*  Respond to 2eSST Broadcast */
	int		reserved;	/* For future use */

};
typedef	struct	vmeInWindowCfg	vmeInWindowCfg_t;


/************************************************************************
 *  /dev/vme_rmw RMW Ioctl Commands
 */
#define	VME_IOCTL_DO_RMW		0x60
/*
 *  VMEbus RMW Configuration Data
 */
struct	vmeRmwCfg
{
	unsigned int	targetAddrU;	/*  VME Address (Upper) to trigger RMW cycle */
	unsigned int	targetAddr;	/*  VME Address (Lower) to trigger RMW cycle */
	addressMode_t	addrSpace;	/*  VME Address Space */
	int	enableMask;	/*  Bit mask defining the bits of interest */
	int	compareData;	/*  Data to be compared with the data read */
	int	swapData;	/*  Data written to the VMEbus on success */
	int	maxAttempts;	/*  Maximum times to try */
	int	numAttempts;	/*  Number of attempts before success */
	int	reserved;	/* For future use */

} ;
typedef	struct vmeRmwCfg	vmeRmwCfg_t;


/************************************************************************
 *  /dev/vme_lm location Monitor Ioctl Commands
 */
#define	VME_IOCTL_SETUP_LM		0x70
#define	VME_IOCTL_WAIT_LM		0x71
/*
 *  VMEbus Location Monitor Arg Structure
 */
struct	vmeLmCfg
{
	unsigned int		addrU;	/*  Location Monitor Address upper */
	unsigned int		addr;	/*  Location Monitor Address lower */
	addressMode_t	addrSpace;	/*  Address Space */
	int		userAccessType;	/*  User/Supervisor Access Type */
	int		dataAccessType;	/*  Data/Program Access Type */
	int		lmWait;		/* Time to wait for access */
	int		lmEvents;	/* Lm event mask */
	int		reserved;	/* For future use */
};
typedef	struct	vmeLmCfg	vmeLmCfg_t;

/*
 *  Data structure created for each board in CS/CSR space.  
 */
struct	vmeSharedData {
	/*
	 * Public elements
	 */
	char    validity1[4];	/* "VME" when contents are valid */
	char	validity2[4];	/* "RDY" when contents are valid */
	int	structureRev;	/* Revision of this structure */
	int	reserved1;

	char	osname[8];	/* OS name string */
	int	driverRev;	/* Revision of VME driver */
	int	reserved2;

	char	boardString[16];	/* type of board */

	int	vmeControllerType;
	int	vmeControllerRev;
	int	boardSemaphore[8];	/* for use by remote */
	unsigned int	inBoundVmeAddrHi[8];	/* This boards VME windows */
	unsigned int	inBoundVmeAddrLo[8];	/* This boards VME windows */
	addressMode_t	inBoundVmeAM[8];	/* Address modifier */
	int	inBoundVmeSize[8];	/* size available to remotes */
	char	reserved3[0x1000-248];	/* Pad to 4k boundary */

	int	readTestPatterns[1024];
	int	remoteScratchArea[24][256];	/* 1k scratch for each remote*/
	/*
	 * Private areas for use by driver only.
	 */
	char	driverScratch[4096];
        struct  {
		char	Eye[4];
		int	Offset;
		int	Head;
		int	Tail;
		int	Size;
		int	Reserved1;
		int	Reserved2;
		int	Reserved3;
		char	Data[4096-32];
	} boardFifo[23];
};


/*
 *  Driver errors reported back to the Application (other than the
 *  stanard Linux errnos...).
 */
#define	VME_ERR_VERR		1	/* VME bus error detected */
#define	VME_ERR_PERR		2	/* PCI bus error detected */

#endif	/* VMEDRV_H */

