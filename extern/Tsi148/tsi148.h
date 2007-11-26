/*
 * Module name: %M%
 * Description:
 *      Tsi148 VME ASIC defines 
 * SCCS identification: %I%
 * Branch: %B%
 * Sequence: %S%
 * Date newest applied delta was created (MM/DD/YY): %G%
 * Time newest applied delta was created (HH:MM:SS): %U%
 * SCCS file name %F%
 * Fully qualified SCCS file name:
 *      %P%
 * Copyright:
 *      (C) COPYRIGHT MOTOROLA, INC. 2003
 *      ALL RIGHTS RESERVED
 * Notes:
 * History:
 * Date         Who
 *
 * 08/04/03     Tom A.
 * Initial release.
 *
 */


#ifndef TSI148_H
#define TSI148_H

/*
 *  Define the number of each that the Tsi148 supports.
 */
#define TSI148_MAX_OUT_WINDOWS        8        /* Max Outbound Windows */
#define TSI148_MAX_IN_WINDOWS         8        /* Max Inbound Windows */
#define TSI148_MAX_DMA_CNTRLS         2        /* Max DMA Controllers */
#define TSI148_MAX_MAILBOXES          4        /* Max Mail Box registers */
#define TSI148_MAX_SEMAPHORES         8        /* Max Semaphores */

#ifndef _ASMLANGUAGE

    /*
     * Misc Structs that are used as part of the larger structs
     */

    /*
     * Outbound Functions used in LCSR
     */
    typedef struct {
        unsigned int	otsau;	/* Starting address */
        unsigned int	otsal;
        unsigned int	oteau;	/* Ending address */
        unsigned int	oteal;
        unsigned int	otofu;	/* Translation offset */
        unsigned int	otofl;
        unsigned int	otbs;	/* 2eSST broadcast select */
        unsigned int	otat;	/* Attributes */
    } tsi148OutboundTranslation_t;

    /*
     * Inbound Functions used in LCSR
     */
    typedef struct {
        unsigned int	itsau;	/* Starting address */
        unsigned int	itsal;
        unsigned int	iteau;	/* Ending address */
        unsigned int	iteal;
        unsigned int	itofu;	/* Translation offset */
        unsigned int	itofl;
        unsigned int	itat;	/* Attributes */
        unsigned char	reserved10[4];
    } tsi148InboundTranslation_t;

    /*
     * DMA Controller used in LCSR
     */
    typedef struct {
        unsigned int	dctl;	/* Control */
        unsigned int	dsta;	/* Status */
        unsigned int	dcsau;	/* RO Current Source address */
        unsigned int	dcsal;
        unsigned int	dcdau;	/* RO Current Destination address */
        unsigned int	dcdal;
        unsigned int	dclau;	/* RO Current Link address */
        unsigned int	dclal;
        unsigned int	dsau;	/* Source Address */
        unsigned int	dsal;
        unsigned int	ddau;	/* Destination Address */
        unsigned int	ddal;
        unsigned int	dsat;	/* Source attributes */
        unsigned int	ddat;	/* Destination attributes */
        unsigned int	dnlau;	/* Next link address */
        unsigned int	dnlal;
        unsigned int	dcnt;	/* Byte count */
        unsigned int	ddbs;	/* 2eSST Broadcast select */
        unsigned char	reserved12[0x38];
    } tsi148Dma_t;

    /*
     *  Layout of a DMAC Linked-List Descriptor
     */
    struct tsi148DmaDescriptor {
        unsigned int	dsau;	/* Source Address */
        unsigned int	dsal;
        unsigned int	ddau;	/* Destination Address */
        unsigned int	ddal;
        unsigned int	dsat;	/* Source attributes */
        unsigned int	ddat;	/* Destination attributes */
        unsigned int	dnlau;	/* Next link address */
        unsigned int	dnlal;
        unsigned int	dcnt;	/* Byte count */
        unsigned int	ddbs;	/* 2eSST Broadcast select */
    };
    typedef struct tsi148DmaDescriptor tsi148DmaDescriptor_t;

    /*
     * PCFS Configuration - includes memory base address register MBAR
     */
    typedef struct {
        unsigned short	veni;		/* PCI header VENI/DEVI */
        unsigned short	devi;
        unsigned short	cmmd;		/* PCI header CMMD/STAT */
        unsigned short	stat;
        unsigned int	reviAndClas;	/* PCI header REVI/CLAS */
        unsigned char	clsz;		/* PCI header CLSZ/MLAT/HEAD/reserved */
        unsigned char	mlat;
        unsigned char	head;
        unsigned char	reserved0[1];
        unsigned int	mbarl;		/* PCI header MBARL */
        unsigned int	mbaru;		/* PCI header MBARU */
        unsigned char	reserved1[20];
        unsigned short	subv;		/* PCI header SUBV/SUBI */
        unsigned short	subi;
        unsigned char	reserved2[4];	/* PCI header reserved */
        unsigned char	capp;		/* PCI header CAPP/reserved */
        unsigned char	reserved3[7];	/* PCI header reserved */
        unsigned char	intl;		/* PCI header INTL/INTP/MNGM/MXLA */
        unsigned char	intp;
        unsigned char	mngn;
        unsigned char	mxla;

        /*
         * offset 40  040 - MSI Capp MSICD/NCAPP/MSCI
         * offset 44  044 - MSI Capp MSIAL
         * offset 48  048 - MSI Capp MSIAU
         * offset 4C  04C - MSI Capp MSIMD/reserved
         */
        unsigned char	msiCapID;
        unsigned char	msiNextCapPtr;
        unsigned short	msiControl;
        unsigned int	msiMsgAddrL;
        unsigned int	msiMsgAddrU;
        unsigned short	msiMsgData;
        unsigned char	reserved4[2];

        /*
         * offset 50  050 - PCI-X Capp PCIXCD/NCAPP/PCIXCAP
         * offset 54  054 - PCI-X Capp PCIXSTAT
         */
        unsigned char	pcixCapID;
        unsigned char	pcixNextCapPtr;
        unsigned short	pcixCommand;
        unsigned int	pcixStatus;

        /*
         * offset 58  058  Reserved
         * ...
         * offset FF  0FF  Reserved
         */
        unsigned char	reserved5[167];

    } tsi148Pcfs_t;


    /*
     * LCSR definitions
     */
    typedef struct {

        /*
         *    Outbound Translations
         */
        tsi148OutboundTranslation_t outboundTranslation[TSI148_MAX_OUT_WINDOWS];

        /*
         * VMEbus interupt ack
         * offset  200
         *
         *	Note:	Even though the registers are defined
         *		as 32-bits in the spec, we only want
         *		to issue 8-bit IACK cycles on the bus.
         */
        unsigned char	viack[8*4];

        /*
         * RMW
         * offset    220
         */
        unsigned int	rmwau;
        unsigned int	rmwal;
        unsigned int	rmwen;
        unsigned int	rmwc;
        unsigned int	rmws;

        /*
         * VMEbus control
         * offset    234
         */
        unsigned int	vmctrl;
        unsigned int	vctrl;
        unsigned int	vstat;

        /*
         * PCI status
         * offset  240
         */
        unsigned int	pstat;
        unsigned char	pstatrsvd[0xc];

        /*
         * VME filter.
         * offset  250
         */
        unsigned int	vmefl;
        unsigned char	vmeflrsvd[0xc];

        /*
         * VME exception.
         * offset  260
         */
        unsigned int	veau;
        unsigned int	veal;
        unsigned int	veat;
        unsigned char	vearsvd[0x4];

        /*
         * PCI error
         * offset  270
         */
        unsigned int	edpau;
        unsigned int	edpal;
        unsigned int	edpxa;
        unsigned int	edpxs;
        unsigned int	edpat;
        unsigned char	edparsvd[0x7c];

        /*
         * Inbound Translations
         * offset  300
         */
        tsi148InboundTranslation_t inboundTranslation[TSI148_MAX_IN_WINDOWS];

        /*
         * Inbound Translation GCSR
         * offset  400 
         */
        unsigned int	gbau;
        unsigned int	gbal;
        unsigned int	gcsrat;

        /*
         * Inbound Translation CRG
         * offset  40C 
         */
        unsigned int	cbau;
        unsigned int	cbal;
        unsigned int	csrat;

        /*
         * Inbound Translation CR/CSR
         *         CRG
         * offset  418
         */
        unsigned int	crou;
        unsigned int	crol;
        unsigned int	crat;

        /*
         * Inbound Translation Location Monitor
         * offset  424 
         */
        unsigned int	lmbau;
        unsigned int	lmbal;
        unsigned int	lmat;

        /*
         * VMEbus Interrupt Control.
         * offset  430
         */
        unsigned int	bcu;
        unsigned int	bcl;
        unsigned int	bpgtr;
        unsigned int	bpctr;
        unsigned int	vicr;

        unsigned char	vicrsvd[0x4];

        /*
         * Local Bus Interrupt Control.
         * offset  448
         */ 
        unsigned int	inten;
        unsigned int	inteo;
        unsigned int	ints;
        unsigned int	intc;
        unsigned int	intm1;
        unsigned int	intm2;

        unsigned char	reserved7[0xa0];

        /*
         * DMA Controllers
         * offset 500
         */
        tsi148Dma_t dma[TSI148_MAX_DMA_CNTRLS];

    } tsi148Lcsr_t;


    /*
     * GCSR Register Group
     */
    typedef struct {
        /*
         * Header
         *         GCSR    CRG
         * offset   00     600 - DEVI/VENI
         */
        unsigned short	devi;
        unsigned short	veni;

        /*
         * Control
         *         GCSR    CRG
         * offset   04     604 - CTRL/GA/REVID
         */
        unsigned short	ctrl;
        unsigned char	ga;
        unsigned char	revid;

        /*
         * Semaphore
         *         GCSR    CRG
         * offset   08     608 - Semaphore3/2/1/0
         * offset   0C     60C - Seamphore7/6/5/4
         */
        unsigned char	semaphore[TSI148_MAX_SEMAPHORES];

        /*
         * Mail Box
         *         GCSR    CRG
         * offset   10     610 - Mailbox0
         */
        unsigned int	mbox[TSI148_MAX_MAILBOXES];

    } tsi148Gcsr_t;


    /*
     * CR/CSR
     */
    typedef struct {
        /*
         *        CR/CSR   CRG
         * offset  7FFF4   FF4 - CSRBCR
         * offset  7FFF8   FF8 - CSRBSR
         * offset  7FFFC   FFC - CBAR
         */
        unsigned int	csrbcr;
        unsigned int	csrbsr;
        unsigned int	cbar;
    } tsi148Crcsr_t;


    /*
     *  TSI148 ASIC register structure overlays and bit field definitions.
     *
     *      Note:   Tsi148 Register Group (CRG) consists of the following
     *              combination of registers:
     *                      PCFS    - PCI Configuration Space Registers
     *                      LCSR    - Local Control and Status Registers
     *                      GCSR    - Global Control and Status Registers
     *                      CR/CSR  - Subset of Configuration ROM /
     *                                Control and Status Registers
     */

    /*
     *  Combined Register Group (CRG)
     */
    typedef struct {
        /*
         * PCFS
         */
        tsi148Pcfs_t pcfs;

        /*
         * LCSR
         */
        tsi148Lcsr_t lcsr;

        /*
         * GCSR
         */
        tsi148Gcsr_t gcsr;

        /*
         * Fill Space
         */
        unsigned char	reserved9[0xFF4-0x61C-4];

        /*
         * CR/CSR
         */
        tsi148Crcsr_t crcsr;

    } tsi148_t;


#endif  /* _ASMLANGUAGE */


    /*
     *  TSI148 Register Bit Definitions
     */

    /*
     *  PFCS Register Set
     */

    /*
     *  Command/Status Registers (CRG + $004)
     */
#define TSI148_PCFS_CMMD_SERR          (1<<8)  /* SERR_L out pin ssys err */
#define TSI148_PCFS_CMMD_PERR          (1<<6)  /* PERR_L out pin  parity */
#define TSI148_PCFS_CMMD_MSTR          (1<<2)  /* PCI bus master */
#define TSI148_PCFS_CMMD_MEMSP         (1<<1)  /* PCI mem space access  */
#define TSI148_PCFS_CMMD_IOSP          (1<<0)  /* PCI I/O space enable */

#define TSI148_PCFS_STAT_RCPVE         (1<<15) /* Detected Parity Error */
#define TSI148_PCFS_STAT_SIGSE         (1<<14) /* Signalled System Error */
#define TSI148_PCFS_STAT_RCVMA         (1<<13) /* Received Master Abort */
#define TSI148_PCFS_STAT_RCVTA         (1<<12) /* Received Target Abort */
#define TSI148_PCFS_STAT_SIGTA         (1<<11) /* Signalled Target Abort */
#define TSI148_PCFS_STAT_SELTIM        (3<<9)  /* DELSEL Timing */
#define TSI148_PCFS_STAT_DPAR          (1<<8)  /* Data Parity Err Reported */
#define TSI148_PCFS_STAT_FAST          (1<<7)  /* Fast back-to-back Cap */
#define TSI148_PCFS_STAT_P66M          (1<<5)  /* 66 MHz Capable */
#define TSI148_PCFS_STAT_CAPL          (1<<4)  /* Capab List - address $34 */


    /*
     *  Revision ID/Class Code Registers   (CRG +$008)
     */
#define TSI148_PCFS_CLAS_M             (0xFF<<24) /* Class ID */
#define TSI148_PCFS_SUBCLAS_M          (0xFF<<16) /* Sub-Class ID */
#define TSI148_PCFS_PROGIF_M           (0xFF<<8)  /* Sub-Class ID */
#define TSI148_PCFS_REVID_M            (0xFF<<0)  /* Rev ID */

    /*
     * Cache Line Size/ Master Latency Timer/ Header Type Registers (CRG + $00C)
     */
#define TSI148_PCFS_HEAD_M             (0xFF<<16) /* Master Lat Timer */
#define TSI148_PCFS_MLAT_M             (0xFF<<8)  /* Master Lat Timer */
#define TSI148_PCFS_CLSZ_M             (0xFF<<0)  /* Cache Line Size */

    /*
     *  Memory Base Address Lower Reg (CRG + $010)
     */
#define TSI148_PCFS_MBARL_BASEL_M      (0xFFFFF<<12) /* Base Addr Lower Mask */
#define TSI148_PCFS_MBARL_PRE          (1<<3)  /* Prefetch */
#define TSI148_PCFS_MBARL_MTYPE_M      (3<<1)  /* Memory Type Mask */
#define TSI148_PCFS_MBARL_IOMEM        (1<<0)  /* I/O Space Indicator */

    /*
     *  Message Signaled Interrupt Capabilities Register (CRG + $040)
     */
#define TSI148_PCFS_MSICAP_64BAC       (1<<7)  /* 64-bit Address Capable */
#define TSI148_PCFS_MSICAP_MME_M       (7<<4)  /* Multiple Msg Enable Mask */
#define TSI148_PCFS_MSICAP_MMC_M       (7<<1)  /* Multiple Msg Capable Mask */
#define TSI148_PCFS_MSICAP_MSIEN       (1<<0)  /* Msg signaled INT Enable */

    /*
     *  Message Address Lower Register (CRG +$044)
     */
#define TSI148_PCFS_MSIAL_M            (0x3FFFFFFF<<2)   /* Mask */

    /*
     *  Message Data Register (CRG + 4C)
     */
#define TSI148_PCFS_MSIMD_M            (0xFFFF<<0)       /* Mask */

    /*
     *  PCI-X Capabilities Register (CRG + $050)
     */
#define TSI148_PCFS_PCIXCAP_MOST_M     (7<<4)  /* Max outstanding Split Tran */
#define TSI148_PCFS_PCIXCAP_MMRBC_M    (3<<2)  /* Max Mem Read byte cnt */
#define TSI148_PCFS_PCIXCAP_ERO        (1<<1)  /* Enable Relaxed Ordering */
#define TSI148_PCFS_PCIXCAP_DPERE      (1<<0)  /* Data Parity Recover Enable */

    /*
     *  PCI-X Status Register (CRG +$054)
     */
#define TSI148_PCFS_PCIXSTAT_RSCEM     (1<<29) /* Recieved Split Comp Error */
#define TSI148_PCFS_PCIXSTAT_DMCRS_M   (7<<26) /* max Cumulative Read Size */
#define TSI148_PCFS_PCIXSTAT_DMOST_M   (7<<23) /* max outstanding Split Trans */
#define TSI148_PCFS_PCIXSTAT_DMMRC_M   (3<<21) /* max mem read byte count */
#define TSI148_PCFS_PCIXSTAT_DC        (1<<20) /* Device Complexity */
#define TSI148_PCFS_PCIXSTAT_USC       (1<<19) /* Unexpected Split comp */
#define TSI148_PCFS_PCIXSTAT_SCD       (1<<18) /* Split completion discard */
#define TSI148_PCFS_PCIXSTAT_133C      (1<<17) /* 133MHz capable */
#define TSI148_PCFS_PCIXSTAT_64D       (1<<16) /* 64 bit device */
#define TSI148_PCFS_PCIXSTAT_BN_M      (0xFF<<8) /* Bus number */
#define TSI148_PCFS_PCIXSTAT_DN_M      (0x1F<<3) /* Device number */
#define TSI148_PCFS_PCIXSTAT_FN_M      (7<<0)  /* Function Number */

    /*
     *  LCSR Registers
     */

    /*
     *  Outbound Translation Starting Address Lower
     */
#define TSI148_LCSR_OTSAL_M            (0xFFFF<<16)    /* Mask */

    /*
     *  Outbound Translation Ending Address Lower
     */
#define TSI148_LCSR_OTEAL_M            (0xFFFF<<16)    /* Mask */

    /*
     *  Outbound Translation Offset Lower
     */
#define TSI148_LCSR_OTOFFL_M           (0xFFFF<<16)    /* Mask */

    /*
     *  Outbound Translation 2eSST Broadcast Select
     */
#define TSI148_LCSR_OTBS_M             (0xFFFFF<<0)    /* Mask */

    /*
     *  Outbound Translation Attribute
     */
#define TSI148_LCSR_OTAT_EN            (1<<31) /* Window Enable */
#define TSI148_LCSR_OTAT_MRPFD         (1<<18) /* Prefetch Disable */

#define TSI148_LCSR_OTAT_PFS_M         (3<<16) /* Prefetch Size Mask */
#define TSI148_LCSR_OTAT_PFS_2         (0<<16) /* 2 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_4         (1<<16) /* 4 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_8         (2<<16) /* 8 Cache Lines P Size */
#define TSI148_LCSR_OTAT_PFS_16        (3<<16) /* 16 Cache Lines P Size */

#define TSI148_LCSR_OTAT_2eSSTM_M      (3<<11) /* 2eSST Xfer Rate Mask */
#define TSI148_LCSR_OTAT_2eSSTM_160    (0<<11) /* 160MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_OTAT_2eSSTM_267    (1<<11) /* 267MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_OTAT_2eSSTM_320    (2<<11) /* 320MB/s 2eSST Xfer Rate */

#define TSI148_LCSR_OTAT_TM_M          (7<<8)  /* Xfer Protocol Mask */
#define TSI148_LCSR_OTAT_TM_SCT        (0<<8)  /* SCT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_BLT        (1<<8)  /* BLT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_MBLT       (2<<8)  /* MBLT Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eVME      (3<<8)  /* 2eVME Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eSST      (4<<8)  /* 2eSST Xfer Protocol */
#define TSI148_LCSR_OTAT_TM_2eSSTB     (5<<8)  /* 2eSST Bcast Xfer Protocol */

#define TSI148_LCSR_OTAT_DBW_M         (3<<6)  /* Max Data Width */
#define TSI148_LCSR_OTAT_DBW_16        (0<<6)  /* 16-bit Data Width */
#define TSI148_LCSR_OTAT_DBW_32        (1<<6)  /* 32-bit Data Width */

#define TSI148_LCSR_OTAT_SUP           (1<<5)  /* Supervisory Access */
#define TSI148_LCSR_OTAT_PGM           (1<<4)  /* Program Access */

#define TSI148_LCSR_OTAT_AMODE_M       (0xf<<0) /* Address Mode Mask */
#define TSI148_LCSR_OTAT_AMODE_A16     (0<<0)  /* A16 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A24     (1<<0)  /* A24 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A32     (2<<0)  /* A32 Address Space */
#define TSI148_LCSR_OTAT_AMODE_A64     (4<<0)  /* A32 Address Space */
#define TSI148_LCSR_OTAT_AMODE_CRCSR   (5<<0)  /* CR/CSR Address Space */

    /*
     *  VME Master Control Register  CRG+$234
     */
#define TSI148_LCSR_VMCTRL_VSA         (1<<27) /* VMEbus Stop Ack */
#define TSI148_LCSR_VMCTRL_VS          (1<<26) /* VMEbus Stop */
#define TSI148_LCSR_VMCTRL_DHB         (1<<25) /* Device Has Bus */
#define TSI148_LCSR_VMCTRL_DWB         (1<<24) /* Device Wants Bus */

#define TSI148_LCSR_VMCTRL_RMWEN       (1<<20) /* RMW Enable */

#define TSI148_LCSR_VMCTRL_ATO_M       (7<<16) /* Master Access Time-out Mask */
#define TSI148_LCSR_VMCTRL_ATO_32      (0<<16) /* 32 us */
#define TSI148_LCSR_VMCTRL_ATO_128     (1<<16) /* 128 us */
#define TSI148_LCSR_VMCTRL_ATO_512     (2<<16) /* 512 us */
#define TSI148_LCSR_VMCTRL_ATO_2M      (3<<16) /* 2 ms */
#define TSI148_LCSR_VMCTRL_ATO_8M      (4<<16) /* 8 ms */
#define TSI148_LCSR_VMCTRL_ATO_32M     (5<<16) /* 32 ms */
#define TSI148_LCSR_VMCTRL_ATO_128M    (6<<16) /* 128 ms */
#define TSI148_LCSR_VMCTRL_ATO_DIS     (7<<16) /* Disabled */

#define TSI148_LCSR_VMCTRL_VTOFF_M     (7<<12) /* VMEbus Master Time off */
#define TSI148_LCSR_VMCTRL_VTOFF_0     (0<<12) /* 0us */
#define TSI148_LCSR_VMCTRL_VTOFF_1     (1<<12) /* 1us */
#define TSI148_LCSR_VMCTRL_VTOFF_2     (2<<12) /* 2us */
#define TSI148_LCSR_VMCTRL_VTOFF_4     (3<<12) /* 4us */
#define TSI148_LCSR_VMCTRL_VTOFF_8     (4<<12) /* 8us */
#define TSI148_LCSR_VMCTRL_VTOFF_16    (5<<12) /* 16us */
#define TSI148_LCSR_VMCTRL_VTOFF_32    (6<<12) /* 32us */
#define TSI148_LCSR_VMCTRL_VTOFF_64    (7<<12) /* 64us */

#define TSI148_LCSR_VMCTRL_VTON_M      (7<<8)  /* VMEbus Master Time On */
#define TSI148_LCSR_VMCTRL_VTON_4      (0<<8)  /* 8us */
#define TSI148_LCSR_VMCTRL_VTON_8      (1<<8)  /* 8us */
#define TSI148_LCSR_VMCTRL_VTON_16     (2<<8)  /* 16us */
#define TSI148_LCSR_VMCTRL_VTON_32     (3<<8)  /* 32us */
#define TSI148_LCSR_VMCTRL_VTON_64     (4<<8)  /* 64us */
#define TSI148_LCSR_VMCTRL_VTON_128    (5<<8)  /* 128us */
#define TSI148_LCSR_VMCTRL_VTON_256    (6<<8)  /* 256us */
#define TSI148_LCSR_VMCTRL_VTON_512    (7<<8)  /* 512us */

#define TSI148_LCSR_VMCTRL_VREL_M      (3<<3)  /* VMEbus Master Rel Mode Mask */
#define TSI148_LCSR_VMCTRL_VREL_T_D    (0<<3)  /* Time on or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_R_D  (1<<3)  /* Time on and REQ or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_B_D  (2<<3)  /* Time on and BCLR or Done */
#define TSI148_LCSR_VMCTRL_VREL_T_D_R  (3<<3)  /* Time on or Done and REQ */

#define TSI148_LCSR_VMCTRL_VFAIR       (1<<2)  /* VMEbus Master Fair Mode */
#define TSI148_LCSR_VMCTRL_VREQL_M     (3<<0)  /* VMEbus Master Req Level Mask */

    /*
     *  VMEbus Control Register CRG+$238
     */
#define TSI148_LCSR_VCTRL_LRE          (1<<31) /* Late Retry Enable */

#define TSI148_LCSR_VCTRL_DLT_M        (0xF<<24) /* Deadlock Timer */
#define TSI148_LCSR_VCTRL_DLT_OFF      (0<<24) /* Deadlock Timer Off */
#define TSI148_LCSR_VCTRL_DLT_16       (1<<24) /* 16 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_32       (2<<24) /* 32 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_64       (3<<24) /* 64 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_128      (4<<24) /* 128 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_256      (5<<24) /* 256 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_512      (6<<24) /* 512 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_1024     (7<<24) /* 1024 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_2048     (8<<24) /* 2048 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_4096     (9<<24) /* 4096 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_8192     (0xA<<24) /* 8192 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_16384    (0xB<<24) /* 16384 VCLKS */
#define TSI148_LCSR_VCTRL_DLT_32768    (0xC<<24) /* 32768 VCLKS */

#define TSI148_LCSR_VCTRL_NERBB        (1<<20) /* No Early Release of Bus Busy */

#define TSI148_LCSR_VCTRL_SRESET       (1<<17) /* System Reset */
#define TSI148_LCSR_VCTRL_LRESET       (1<<16) /* Local Reset */

#define TSI148_LCSR_VCTRL_SFAILAI      (1<<15) /* SYSFAIL Auto Slot ID */
#define TSI148_LCSR_VCTRL_BID_M        (0x1F<<8) /* Broadcast ID Mask */

#define TSI148_LCSR_VCTRL_ATOEN        (1<<7)  /* Arbiter Time-out Enable */
#define TSI148_LCSR_VCTRL_ROBIN        (1<<6)  /* VMEbus Round Robin */

#define TSI148_LCSR_VCTRL_GTO_M        (7<<0)  /* VMEbus Global Time-out Mask */
#define TSI148_LCSR_VCTRL_GTO_8	      (0<<0)  /* 8 us */
#define TSI148_LCSR_VCTRL_GTO_16	      (1<<0)  /* 16 us */
#define TSI148_LCSR_VCTRL_GTO_32	      (2<<0)  /* 32 us */
#define TSI148_LCSR_VCTRL_GTO_64	      (3<<0)  /* 64 us */
#define TSI148_LCSR_VCTRL_GTO_128      (4<<0)  /* 128 us */
#define TSI148_LCSR_VCTRL_GTO_256      (5<<0)  /* 256 us */
#define TSI148_LCSR_VCTRL_GTO_512      (6<<0)  /* 512 us */
#define TSI148_LCSR_VCTRL_GTO_DIS      (7<<0)  /* Disabled */


    /*
     *  VMEbus Status Register  CRG + $23C
     */
#define TSI148_LCSR_VSTAT_CPURST       (1<<15) /* Clear power up reset*/
#define TSI148_LCSR_VSTAT_BRDFL        (1<<14) /* Board fail */
#define TSI148_LCSR_VSTAT_PURSTS       (1<<12) /* Power up reset status*/
#define TSI148_LCSR_VSTAT_BDFAILS      (1<<11) /* Board Fail Status */
#define TSI148_LCSR_VSTAT_SYSFAILS     (1<<10) /* System Fail Status*/
#define TSI148_LCSR_VSTAT_ACFAILS      (1<<9)  /* AC fail status */
#define TSI148_LCSR_VSTAT_SCONS        (1<<8)  /* System Cont Status */
#define TSI148_LCSR_VSTAT_GAP          (1<<5)  /* Geographic Addr Parity */
#define TSI148_LCSR_VSTAT_GA_M         (0x1F<<0) /* Geographic Addr Mask */

    /*
     *  PCI Configuration Status Register CRG+$240
     */
#define TSI148_LCSR_PSTAT_REQ64S       (1<<6)  /* Request 64 status set */
#define TSI148_LCSR_PSTAT_M66ENS       (1<<5)  /* M66ENS 66Mhz enable */
#define TSI148_LCSR_PSTAT_FRAMES       (1<<4)  /* Frame Status */
#define TSI148_LCSR_PSTAT_IRDYS        (1<<3)  /* IRDY status */
#define TSI148_LCSR_PSTAT_DEVSELS      (1<<2)  /* DEVL status */
#define TSI148_LCSR_PSTAT_STOPS        (1<<1)  /* STOP status */
#define TSI148_LCSR_PSTAT_TRDYS        (1<<0)  /* TRDY status */

    /*
     *  Inbound Translation Starting Address Lower
     */
#define TSI148_LCSR_ITSAL6432_M        (0xFFFF<<16)      /* Mask */
#define TSI148_LCSR_ITSAL24_M          (0x00FFF<<12)     /* Mask */
#define TSI148_LCSR_ITSAL16_M          (0x0000FFF<<4)    /* Mask */

    /*
     *  Inbound Translation Ending Address Lower
     */
#define TSI148_LCSR_ITEAL6432_M        (0xFFFF<<16)      /* Mask */
#define TSI148_LCSR_ITEAL24_M          (0x00FFF<<12)     /* Mask */
#define TSI148_LCSR_ITEAL16_M          (0x0000FFF<<4)    /* Mask */

    /*
     *  Inbound Translation Offset Lower
     */
#define TSI148_LCSR_ITOFFL6432_M       (0xFFFF<<16)     /* Mask */
#define TSI148_LCSR_ITOFFL24_M         (0xFFFFF<<12)    /* Mask */
#define TSI148_LCSR_ITOFFL16_M         (0xFFFFFFF<<4)   /* Mask */

    /*
     *  Inbound Translation Attribute
     */
#define TSI148_LCSR_ITAT_EN            (1<<31) /* Window Enable */
#define TSI148_LCSR_ITAT_TH            (1<<18) /* Prefetch Threshold */

#define TSI148_LCSR_ITAT_VFS_M         (3<<16) /* Virtual FIFO Size Mask */
#define TSI148_LCSR_ITAT_VFS_64        (0<<16) /* 64 bytes Virtual FIFO Size */
#define TSI148_LCSR_ITAT_VFS_128       (1<<16) /* 128 bytes Virtual FIFO Sz */
#define TSI148_LCSR_ITAT_VFS_256       (2<<16) /* 256 bytes Virtual FIFO Sz */
#define TSI148_LCSR_ITAT_VFS_512       (3<<16) /* 512 bytes Virtual FIFO Sz */

#define TSI148_LCSR_ITAT_2eSSTM_M      (7<<12) /* 2eSST Xfer Rate Mask */
#define TSI148_LCSR_ITAT_2eSSTM_160    (0<<12) /* 160MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_ITAT_2eSSTM_267    (1<<12) /* 267MB/s 2eSST Xfer Rate */
#define TSI148_LCSR_ITAT_2eSSTM_320    (2<<12) /* 320MB/s 2eSST Xfer Rate */

#define TSI148_LCSR_ITAT_2eSSTB        (1<<11) /* 2eSST Bcast Xfer Protocol */
#define TSI148_LCSR_ITAT_2eSST         (1<<10) /* 2eSST Xfer Protocol */
#define TSI148_LCSR_ITAT_2eVME         (1<<9)  /* 2eVME Xfer Protocol */
#define TSI148_LCSR_ITAT_MBLT          (1<<8)  /* MBLT Xfer Protocol */
#define TSI148_LCSR_ITAT_BLT           (1<<7)  /* BLT Xfer Protocol */

#define TSI148_LCSR_ITAT_AS_M          (7<<4)  /* Address Space Mask */
#define TSI148_LCSR_ITAT_AS_A16        (0<<4)  /* A16 Address Space */
#define TSI148_LCSR_ITAT_AS_A24        (1<<4)  /* A24 Address Space */
#define TSI148_LCSR_ITAT_AS_A32        (2<<4)  /* A32 Address Space */
#define TSI148_LCSR_ITAT_AS_A64        (4<<4)  /* A64 Address Space */

#define TSI148_LCSR_ITAT_SUPR          (1<<3)  /* Supervisor Access */
#define TSI148_LCSR_ITAT_NPRIV         (1<<2)  /* Non-Priv (User) Access */
#define TSI148_LCSR_ITAT_PGM           (1<<1)  /* Program Access */
#define TSI148_LCSR_ITAT_DATA          (1<<0)  /* Data Access */

    /*
     *  GCSR Base Address Lower Address  CRG +$404
     */
#define TSI148_LCSR_GBAL_M             (0x7FFFFFF<<5)    /* Mask */

    /*
     *  GCSR Attribute Register CRG + $408
     */
#define TSI148_LCSR_GCSRAT_EN          (1<<7)  /* Enable access to GCSR */

#define TSI148_LCSR_GCSRAT_AS_M        (7<<4)  /* Address Space Mask */
#define TSI148_LCSR_GCSRAT_AS_16       (0<<4)  /* Address Space 16 */
#define TSI148_LCSR_GCSRAT_AS_24       (1<<4)  /* Address Space 24 */
#define TSI148_LCSR_GCSRAT_AS_32       (2<<4)  /* Address Space 32 */
#define TSI148_LCSR_GCSRAT_AS_64       (4<<4)  /* Address Space 64 */

#define TSI148_LCSR_GCSRAT_SUPR        (1<<3)  /* Sup set -GCSR decoder */
#define TSI148_LCSR_GCSRAT_NPRIV       (1<<2)  /* Non-Privliged set - CGSR */
#define TSI148_LCSR_GCSRAT_PGM         (1<<1)  /* Program set - GCSR decoder */
#define TSI148_LCSR_GCSRAT_DATA        (1<<0)  /* DATA set GCSR decoder */

    /*
     *  CRG Base Address Lower Address  CRG + $410
     */
#define TSI148_LCSR_CBAL_M             (0xFFFFF<<12)

    /*
     *  CRG Attribute Register  CRG + $414
     */
#define TSI148_LCSR_CRGAT_EN           (1<<7)  /* Enable PRG Access */

#define TSI148_LCSR_CRGAT_AS_M         (7<<4)  /* Address Space */
#define TSI148_LCSR_CRGAT_AS_16        (0<<4)  /* Address Space 16 */
#define TSI148_LCSR_CRGAT_AS_24        (1<<4)  /* Address Space 24 */
#define TSI148_LCSR_CRGAT_AS_32        (2<<4)  /* Address Space 32 */
#define TSI148_LCSR_CRGAT_AS_64        (4<<4)  /* Address Space 64 */

#define TSI148_LCSR_CRGAT_SUPR         (1<<3)   /* Supervisor Access */
#define TSI148_LCSR_CRGAT_NPRIV        (1<<2)   /* Non-Privliged(User) Access */
#define TSI148_LCSR_CRGAT_PGM          (1<<1)   /* Program Access */
#define TSI148_LCSR_CRGAT_DATA         (1<<0)   /* Data Access */

    /*
     *  CR/CSR Offset Lower Register  CRG + $41C
     */
#define TSI148_LCSR_CROL_M             (0x1FFF<<19)      /* Mask */

    /*
     *  CR/CSR Attribute register  CRG + $420
     */
#define TSI148_LCSR_CRAT_EN            (1<<7)  /* Enable access to CR/CSR */

    /*
     *  Location Monitor base address lower register  CRG + $428
     */
#define TSI148_LCSR_LMBAL_M            (0x7FFFFFF<<5)    /* Mask */

    /*
     *  Location Monitor Attribute Register  CRG + $42C
     */
#define TSI148_LCSR_LMAT_EN            (1<<7)  /* Enable Location Monitor */

#define TSI148_LCSR_LMAT_AS_M          (7<<4)  /* Address Space MASK  */
#define TSI148_LCSR_LMAT_AS_16         (0<<4)  /* A16 */
#define TSI148_LCSR_LMAT_AS_24         (1<<4)  /* A24 */
#define TSI148_LCSR_LMAT_AS_32         (2<<4)  /* A32 */
#define TSI148_LCSR_LMAT_AS_64         (4<<4)  /* A64 */

#define TSI148_LCSR_LMAT_SUPR          (1<<3)  /* Supervisor Access */
#define TSI148_LCSR_LMAT_NPRIV         (1<<2)  /* Non-Priv (User) Access */
#define TSI148_LCSR_LMAT_PGM           (1<<1)  /* Program Access */
#define TSI148_LCSR_LMAT_DATA          (1<<0)  /* Data Access  */

    /*
     *  Broadcast Pulse Generator Timer Register  CRG + $438
     */
#define TSI148_LCSR_BPGTR_BPGT_M       (0xFFFF<<0)     /* Mask */

    /*
     *  Broadcast Programmable Clock Timer Register  CRG + $43C
     */
#define TSI148_LCSR_BPCTR_BPCT_M       (0xFFFFFF<<0)   /* Mask */

    /*
     *  VMEbus Interrupt Control Register           CRG + $43C
     */
#define TSI148_LCSR_VICR_CNTS_M        (3<<22) /* Cntr Source MASK */
#define TSI148_LCSR_VICR_CNTS_DIS      (1<<22) /* Cntr Disable */
#define TSI148_LCSR_VICR_CNTS_IRQ1     (2<<22) /* IRQ1 to Cntr */
#define TSI148_LCSR_VICR_CNTS_IRQ2     (3<<22) /* IRQ2 to Cntr */

#define TSI148_LCSR_VICR_EDGIS_M       (3<<20) /* Edge interupt MASK */
#define TSI148_LCSR_VICR_EDGIS_DIS     (1<<20) /* Edge interupt Disable */
#define TSI148_LCSR_VICR_EDGIS_IRQ1    (2<<20) /* IRQ1 to Edge */
#define TSI148_LCSR_VICR_EDGIS_IRQ2    (3<<20) /* IRQ2 to Edge */

#define TSI148_LCSR_VICR_IRQIF_M       (3<<18) /* IRQ1* Function MASK */
#define TSI148_LCSR_VICR_IRQIF_NORM    (1<<18) /* Normal */
#define TSI148_LCSR_VICR_IRQIF_PULSE   (2<<18) /* Pulse Generator */
#define TSI148_LCSR_VICR_IRQIF_PROG    (3<<18) /* Programmable Clock */
#define TSI148_LCSR_VICR_IRQIF_1U      (4<<18) /* 1us Clock */

#define TSI148_LCSR_VICR_IRQ2F_M       (3<<16) /* IRQ2* Function MASK */
#define TSI148_LCSR_VICR_IRQ2F_NORM    (1<<16) /* Normal */
#define TSI148_LCSR_VICR_IRQ2F_PULSE   (2<<16) /* Pulse Generator */
#define TSI148_LCSR_VICR_IRQ2F_PROG    (3<<16) /* Programmable Clock */
#define TSI148_LCSR_VICR_IRQ2F_1U      (4<<16) /* 1us Clock */

#define TSI148_LCSR_VICR_BIP           (1<<15) /* Broadcast Interrupt Pulse */

#define TSI148_LCSR_VICR_IRQC          (1<<12) /* VMEbus IRQ Clear */
#define TSI148_LCSR_VICR_IRQS          (1<<11) /* VMEbus IRQ Status */

#define TSI148_LCSR_VICR_IRQL_M        (7<<8)  /* VMEbus SW IRQ Level Mask */
#define TSI148_LCSR_VICR_IRQL_1        (1<<8)  /* VMEbus SW IRQ Level 1 */
#define TSI148_LCSR_VICR_IRQL_2        (2<<8)  /* VMEbus SW IRQ Level 2 */
#define TSI148_LCSR_VICR_IRQL_3        (3<<8)  /* VMEbus SW IRQ Level 3 */
#define TSI148_LCSR_VICR_IRQL_4        (4<<8)  /* VMEbus SW IRQ Level 4 */
#define TSI148_LCSR_VICR_IRQL_5        (5<<8)  /* VMEbus SW IRQ Level 5 */
#define TSI148_LCSR_VICR_IRQL_6        (6<<8)  /* VMEbus SW IRQ Level 6 */
#define TSI148_LCSR_VICR_IRQL_7        (7<<8)  /* VMEbus SW IRQ Level 7 */

#define TSI148_LCSR_VICR_STID_M        (0xFF<<0) /* Status/ID Mask */

    /*
     *  Interrupt Enable Register   CRG + $440
     */
#define TSI148_LCSR_INTEN_DMA1EN       (1<<25) /* DMAC 1 */
#define TSI148_LCSR_INTEN_DMA0EN       (1<<24) /* DMAC 0 */
#define TSI148_LCSR_INTEN_LM3EN        (1<<23) /* Location Monitor 3 */
#define TSI148_LCSR_INTEN_LM2EN        (1<<22) /* Location Monitor 2 */
#define TSI148_LCSR_INTEN_LM1EN        (1<<21) /* Location Monitor 1 */
#define TSI148_LCSR_INTEN_LM0EN        (1<<20) /* Location Monitor 0 */
#define TSI148_LCSR_INTEN_MB3EN        (1<<19) /* Mail Box 3 */
#define TSI148_LCSR_INTEN_MB2EN        (1<<18) /* Mail Box 2 */
#define TSI148_LCSR_INTEN_MB1EN        (1<<17) /* Mail Box 1 */
#define TSI148_LCSR_INTEN_MB0EN        (1<<16) /* Mail Box 0 */
#define TSI148_LCSR_INTEN_VBEREN       (1<<13) /* VMEbus Error */
#define TSI148_LCSR_INTEN_VATOEN       (1<<12) /* VMEbus Access Time-out */
#define TSI148_LCSR_INTEN_VIEEN        (1<<11) /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTEN_IACKEN       (1<<10) /* IACK */
#define TSI148_LCSR_INTEN_SYSFLEN      (1<<9)  /* System Fail */
#define TSI148_LCSR_INTEN_ACFLEN       (1<<8)  /* AC Fail */
#define TSI148_LCSR_INTEN_IRQ7EN       (1<<7)  /* IRQ7 */
#define TSI148_LCSR_INTEN_IRQ6EN       (1<<6)  /* IRQ6 */
#define TSI148_LCSR_INTEN_IRQ5EN       (1<<5)  /* IRQ5 */
#define TSI148_LCSR_INTEN_IRQ4EN       (1<<4)  /* IRQ4 */
#define TSI148_LCSR_INTEN_IRQ3EN       (1<<3)  /* IRQ3 */
#define TSI148_LCSR_INTEN_IRQ2EN       (1<<2)  /* IRQ2 */
#define TSI148_LCSR_INTEN_IRQ1EN       (1<<1)  /* IRQ1 */

    /*
     *  Interrupt Enable Out Register CRG + $444
     */
#define TSI148_LCSR_INTEO_DMA1EO       (1<<25) /* DMAC 1 */
#define TSI148_LCSR_INTEO_DMA0EO       (1<<24) /* DMAC 0 */
#define TSI148_LCSR_INTEO_LM3EO        (1<<23) /* Loc Monitor 3 */
#define TSI148_LCSR_INTEO_LM2EO        (1<<22) /* Loc Monitor 2 */
#define TSI148_LCSR_INTEO_LM1EO        (1<<21) /* Loc Monitor 1 */
#define TSI148_LCSR_INTEO_LM0EO        (1<<20) /* Location Monitor 0 */
#define TSI148_LCSR_INTEO_MB3EO        (1<<19) /* Mail Box 3 */
#define TSI148_LCSR_INTEO_MB2EO        (1<<18) /* Mail Box 2 */
#define TSI148_LCSR_INTEO_MB1EO        (1<<17) /* Mail Box 1 */
#define TSI148_LCSR_INTEO_MB0EO        (1<<16) /* Mail Box 0 */
#define TSI148_LCSR_INTEO_VBEREO       (1<<13) /* VMEbus Error */
#define TSI148_LCSR_INTEO_VATOEO       (1<<12) /* VMEbus Access Time-out */
#define TSI148_LCSR_INTEO_VIEEO        (1<<11) /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTEO_IACKEO       (1<<10) /* IACK */
#define TSI148_LCSR_INTEO_SYSFLEO      (1<<9)  /* System Fail */
#define TSI148_LCSR_INTEO_ACFLEO       (1<<8)  /* AC Fail */
#define TSI148_LCSR_INTEO_IRQ7EO       (1<<7)  /* IRQ7 */
#define TSI148_LCSR_INTEO_IRQ6EO       (1<<6)  /* IRQ6 */
#define TSI148_LCSR_INTEO_IRQ5EO       (1<<5)  /* IRQ5 */
#define TSI148_LCSR_INTEO_IRQ4EO       (1<<4)  /* IRQ4 */
#define TSI148_LCSR_INTEO_IRQ3EO       (1<<3)  /* IRQ3 */
#define TSI148_LCSR_INTEO_IRQ2EO       (1<<2)  /* IRQ2 */
#define TSI148_LCSR_INTEO_IRQ1EO       (1<<1)  /* IRQ1 */

    /*
     *  Interrupt Status Register CRG + $448
     */
#define TSI148_LCSR_INTS_DMA1S         (1<<25) /* DMA 1 */
#define TSI148_LCSR_INTS_DMA0S         (1<<24) /* DMA 0 */
#define TSI148_LCSR_INTS_LM3S          (1<<23) /* Location Monitor 3 */
#define TSI148_LCSR_INTS_LM2S          (1<<22) /* Location Monitor 2 */
#define TSI148_LCSR_INTS_LM1S          (1<<21) /* Location Monitor 1 */
#define TSI148_LCSR_INTS_LM0S          (1<<20) /* Location Monitor 0 */
#define TSI148_LCSR_INTS_MB3S          (1<<19) /* Mail Box 3 */
#define TSI148_LCSR_INTS_MB2S          (1<<18) /* Mail Box 2 */
#define TSI148_LCSR_INTS_MB1S          (1<<17) /* Mail Box 1 */
#define TSI148_LCSR_INTS_MB0S          (1<<16) /* Mail Box 0 */
#define TSI148_LCSR_INTS_VBERS         (1<<13) /* VMEbus Error */
#define TSI148_LCSR_INTS_VATOS         (1<<12) /* VMEbus Access Time-out */
#define TSI148_LCSR_INTS_VIES          (1<<11) /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTS_IACKS         (1<<10) /* IACK */
#define TSI148_LCSR_INTS_SYSFLS        (1<<9)  /* System Fail */
#define TSI148_LCSR_INTS_ACFLS         (1<<8)  /* AC Fail */
#define TSI148_LCSR_INTS_IRQ7S         (1<<7)  /* IRQ7 */
#define TSI148_LCSR_INTS_IRQ6S         (1<<6)  /* IRQ6 */
#define TSI148_LCSR_INTS_IRQ5S         (1<<5)  /* IRQ5 */
#define TSI148_LCSR_INTS_IRQ4S         (1<<4)  /* IRQ4 */
#define TSI148_LCSR_INTS_IRQ3S         (1<<3)  /* IRQ3 */
#define TSI148_LCSR_INTS_IRQ2S         (1<<2)  /* IRQ2 */
#define TSI148_LCSR_INTS_IRQ1S         (1<<1)  /* IRQ1 */

    /*
     *  Interrupt Clear Register CRG + $44C
     */
#define TSI148_LCSR_INTC_DMA1C         (1<<25) /* DMA 1 */
#define TSI148_LCSR_INTC_DMA0C         (1<<24) /* DMA 0 */
#define TSI148_LCSR_INTC_LM3C          (1<<23) /* Location Monitor 3 */
#define TSI148_LCSR_INTC_LM2C          (1<<22) /* Location Monitor 2 */
#define TSI148_LCSR_INTC_LM1C          (1<<21) /* Location Monitor 1 */
#define TSI148_LCSR_INTC_LM0C          (1<<20) /* Location Monitor 0 */
#define TSI148_LCSR_INTC_MB3C          (1<<19) /* Mail Box 3 */
#define TSI148_LCSR_INTC_MB2C          (1<<18) /* Mail Box 2 */
#define TSI148_LCSR_INTC_MB1C          (1<<17) /* Mail Box 1 */
#define TSI148_LCSR_INTC_MB0C          (1<<16) /* Mail Box 0 */
#define TSI148_LCSR_INTC_VBERC         (1<<13) /* VMEbus Error */
#define TSI148_LCSR_INTC_VATOC         (1<<12) /* VMEbus Access Time-out */
#define TSI148_LCSR_INTC_VIEC          (1<<11) /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTC_IACKC         (1<<10) /* IACK */
#define TSI148_LCSR_INTC_SYSFLC        (1<<9)  /* System Fail */
#define TSI148_LCSR_INTC_ACFLC         (1<<8)  /* AC Fail */

    /*
     *  Interrupt Map Register 1 CRG + $458
     */
#define TSI148_LCSR_INTM1_DMA1M_M      (3<<18) /* DMA 1 */
#define TSI148_LCSR_INTM1_DMA0M_M      (3<<16) /* DMA 0 */
#define TSI148_LCSR_INTM1_LM3M_M       (3<<14) /* Location Monitor 3 */
#define TSI148_LCSR_INTM1_LM2M_M       (3<<12) /* Location Monitor 2 */
#define TSI148_LCSR_INTM1_LM1M_M       (3<<10) /* Location Monitor 1 */
#define TSI148_LCSR_INTM1_LM0M_M       (3<<8)  /* Location Monitor 0 */
#define TSI148_LCSR_INTM1_MB3M_M       (3<<6)  /* Mail Box 3 */
#define TSI148_LCSR_INTM1_MB2M_M       (3<<4)  /* Mail Box 2 */
#define TSI148_LCSR_INTM1_MB1M_M       (3<<2)  /* Mail Box 1 */
#define TSI148_LCSR_INTM1_MB0M_M       (3<<0)  /* Mail Box 0 */

    /*
     *  Interrupt Map Register 2 CRG + $45C
     */
#define TSI148_LCSR_INTM2_PERRM_M      (3<<26) /* PCI Bus Error */
#define TSI148_LCSR_INTM2_VERRM_M      (3<<24) /* VMEbus Error */
#define TSI148_LCSR_INTM2_VIEM_M       (3<<22) /* VMEbus IRQ Edge */
#define TSI148_LCSR_INTM2_IACKM_M      (3<<20) /* IACK */
#define TSI148_LCSR_INTM2_SYSFLM_M     (3<<18) /* System Fail */
#define TSI148_LCSR_INTM2_ACFLM_M      (3<<16) /* AC Fail */
#define TSI148_LCSR_INTM2_IRQ7M_M      (3<<14) /* IRQ7 */
#define TSI148_LCSR_INTM2_IRQ6M_M      (3<<12) /* IRQ6 */
#define TSI148_LCSR_INTM2_IRQ5M_M      (3<<10) /* IRQ5 */
#define TSI148_LCSR_INTM2_IRQ4M_M      (3<<8)  /* IRQ4 */
#define TSI148_LCSR_INTM2_IRQ3M_M      (3<<6)  /* IRQ3 */
#define TSI148_LCSR_INTM2_IRQ2M_M      (3<<4)  /* IRQ2 */
#define TSI148_LCSR_INTM2_IRQ1M_M      (3<<2)  /* IRQ1 */

    /*
     *  DMA Control (0-1) Registers CRG + $500
     */
#define TSI148_LCSR_DCTL_ABT           (1<<27) /* Abort */
#define TSI148_LCSR_DCTL_PAU           (1<<26) /* Pause */
#define TSI148_LCSR_DCTL_DGO           (1<<25) /* DMA Go */

#define TSI148_LCSR_DCTL_MOD           (1<<23) /* Mode */

#define TSI148_LCSR_DCTL_VBKS_M        (7<<12) /* VMEbus block Size MASK */
#define TSI148_LCSR_DCTL_VBKS_32       (0<<12) /* VMEbus block Size 32 */
#define TSI148_LCSR_DCTL_VBKS_64       (1<<12) /* VMEbus block Size 64 */
#define TSI148_LCSR_DCTL_VBKS_128      (2<<12) /* VMEbus block Size 128 */
#define TSI148_LCSR_DCTL_VBKS_256      (3<<12) /* VMEbus block Size 256 */
#define TSI148_LCSR_DCTL_VBKS_512      (4<<12) /* VMEbus block Size 512 */
#define TSI148_LCSR_DCTL_VBKS_1024     (5<<12) /* VMEbus block Size 1024 */
#define TSI148_LCSR_DCTL_VBKS_2048     (6<<12) /* VMEbus block Size 2048 */
#define TSI148_LCSR_DCTL_VBKS_4096     (7<<12) /* VMEbus block Size 4096 */

#define TSI148_LCSR_DCTL_VBOT_M        (7<<8)  /* VMEbus back-off MASK */
#define TSI148_LCSR_DCTL_VBOT_0        (0<<8)  /* VMEbus back-off  0us */
#define TSI148_LCSR_DCTL_VBOT_1        (1<<8)  /* VMEbus back-off 1us */
#define TSI148_LCSR_DCTL_VBOT_2        (2<<8)  /* VMEbus back-off 2us */
#define TSI148_LCSR_DCTL_VBOT_4        (3<<8)  /* VMEbus back-off 4us */
#define TSI148_LCSR_DCTL_VBOT_8        (4<<8)  /* VMEbus back-off 8us */
#define TSI148_LCSR_DCTL_VBOT_16       (5<<8)  /* VMEbus back-off 16us */
#define TSI148_LCSR_DCTL_VBOT_32       (6<<8)  /* VMEbus back-off 32us */
#define TSI148_LCSR_DCTL_VBOT_64       (7<<8)  /* VMEbus back-off 64us */

#define TSI148_LCSR_DCTL_PBKS_M        (7<<4)  /* PCI block size MASK */
#define TSI148_LCSR_DCTL_PBKS_32       (0<<4)  /* PCI block size 32 bytes*/
#define TSI148_LCSR_DCTL_PBKS_64       (1<<4)  /* PCI block size 64 bytes*/
#define TSI148_LCSR_DCTL_PBKS_128      (2<<4)  /* PCI block size 128 bytes*/
#define TSI148_LCSR_DCTL_PBKS_256      (3<<4)  /* PCI block size 256 bytes*/
#define TSI148_LCSR_DCTL_PBKS_512      (4<<4)  /* PCI block size 512 bytes*/
#define TSI148_LCSR_DCTL_PBKS_1024     (5<<4)  /* PCI block size 1024 bytes*/
#define TSI148_LCSR_DCTL_PBKS_2048     (6<<4)  /* PCI block size 2048 bytes*/
#define TSI148_LCSR_DCTL_PBKS_4096     (7<<4)  /* PCI block size 4096 bytes*/

#define TSI148_LCSR_DCTL_PBOT_M        (7<<0)  /* PCI back off MASK */
#define TSI148_LCSR_DCTL_PBOT_0        (0<<0)  /* PCI back off 0us */
#define TSI148_LCSR_DCTL_PBOT_1        (1<<0)  /* PCI back off 1us */
#define TSI148_LCSR_DCTL_PBOT_2        (2<<0)  /* PCI back off 2us */
#define TSI148_LCSR_DCTL_PBOT_4        (3<<0)  /* PCI back off 3us */
#define TSI148_LCSR_DCTL_PBOT_8        (4<<0)  /* PCI back off 4us */
#define TSI148_LCSR_DCTL_PBOT_16       (5<<0)  /* PCI back off 8us */
#define TSI148_LCSR_DCTL_PBOT_32       (6<<0)  /* PCI back off 16us */
#define TSI148_LCSR_DCTL_PBOT_64       (7<<0)  /* PCI back off 32us */

    /*
     *  DMA Status Registers (0-1)  CRG + $504
     */
#define TSI148_LCSR_DSTA_SMA           (1<<31) /* PCI Signalled Master Abt*/
#define TSI148_LCSR_DSTA_RTA           (1<<30) /* PCI Received Target Abt */
#define TSI148_LCSR_DSTA_MRC           (1<<29) /* PCI Max Retry Count */
#define TSI148_LCSR_DSTA_VBE           (1<<28) /* VMEbus error */
#define TSI148_LCSR_DSTA_ABT           (1<<27) /* Abort */
#define TSI148_LCSR_DSTA_PAU           (1<<26) /* Pause */
#define TSI148_LCSR_DSTA_DON           (1<<25) /* Done */
#define TSI148_LCSR_DSTA_BSY           (1<<24) /* Busy */

    /*
     *  DMA Current Link Address Lower (0-1)
     */
#define TSI148_LCSR_DCLAL_M            (0x3FFFFFF<<6)    /* Mask */

    /*
     *  DMA Source Attribute (0-1) Reg
     */
#define TSI148_LCSR_DSAT_TYP_M         (3<<28) /* Source Bus Type */
#define TSI148_LCSR_DSAT_TYP_PCI       (0<<28) /* PCI Bus */
#define TSI148_LCSR_DSAT_TYP_VME       (1<<28) /* VMEbus */
#define TSI148_LCSR_DSAT_TYP_PAT       (2<<28) /* Data Pattern */

#define TSI148_LCSR_DSAT_PSZ           (1<<25) /* Pattern Size */
#define TSI148_LCSR_DSAT_NIN           (1<<24) /* No Increment */

#define TSI148_LCSR_DSAT_2eSSTM_M      (3<<11) /* 2eSST Trans Rate Mask */
#define TSI148_LCSR_DSAT_2eSSTM_160    (0<<11) /* 160 MB/s */
#define TSI148_LCSR_DSAT_2eSSTM_267    (1<<11) /* 267 MB/s */
#define TSI148_LCSR_DSAT_2eSSTM_320    (2<<11) /* 320 MB/s */

#define TSI148_LCSR_DSAT_TM_M          (7<<8)  /* Bus Transfer Protocol Mask */
#define TSI148_LCSR_DSAT_TM_SCT        (0<<8)  /* SCT */
#define TSI148_LCSR_DSAT_TM_BLT        (1<<8)  /* BLT */
#define TSI148_LCSR_DSAT_TM_MBLT       (2<<8)  /* MBLT */
#define TSI148_LCSR_DSAT_TM_2eVME      (3<<8)  /* 2eVME */
#define TSI148_LCSR_DSAT_TM_2eSST      (4<<8)  /* 2eSST */
#define TSI148_LCSR_DSAT_TM_2eSSTB     (5<<8)  /* 2eSST Broadcast */

#define TSI148_LCSR_DSAT_DBW_M         (3<<6)  /* Max Data Width MASK */
#define TSI148_LCSR_DSAT_DBW_16        (0<<6)  /* 16 Bits */
#define TSI148_LCSR_DSAT_DBW_32        (1<<6)  /* 32 Bits */

#define TSI148_LCSR_DSAT_SUP           (1<<5)  /* Supervisory Mode*/
#define TSI148_LCSR_DSAT_PGM           (1<<4)  /* Program Mode */

#define TSI148_LCSR_DSAT_AMODE_M       (0xf<<0) /* Address Space Mask */
#define TSI148_LCSR_DSAT_AMODE_16      (0<<0)  /* A16 */
#define TSI148_LCSR_DSAT_AMODE_24      (1<<0)  /* A24 */
#define TSI148_LCSR_DSAT_AMODE_32      (2<<0)  /* A32 */
#define TSI148_LCSR_DSAT_AMODE_64      (4<<0)  /* A64 */
#define TSI148_LCSR_DSAT_AMODE_CRCSR   (5<<0)  /* CR/CSR */
#define TSI148_LCSR_DSAT_AMODE_USER1   (8<<0)  /* User1 */
#define TSI148_LCSR_DSAT_AMODE_USER2   (9<<0)  /* User2 */
#define TSI148_LCSR_DSAT_AMODE_USER3   (0xa<<0) /* User3 */
#define TSI148_LCSR_DSAT_AMODE_USER4   (0xb<<0) /* User4 */

    /*
     *  DMA Destination Attribute Registers (0-1)
     */
#define TSI148_LCSR_DDAT_TYP_VME       (1<<28) /* Destination VMEbus */
#define TSI148_LCSR_DDAT_TYP_PCI       (0<<28) /* Destination PCI Bus  */

#define TSI148_LCSR_DDAT_NIN           (1<<24) /* No Increment */

#define TSI148_LCSR_DDAT_2eSSTM_M      (3<<11) /* 2eSST Transfer Rate Mask */
#define TSI148_LCSR_DDAT_2eSSTM_160    (0<<11) /* 160 MB/s */
#define TSI148_LCSR_DDAT_2eSSTM_267    (1<<11) /* 267 MB/s */
#define TSI148_LCSR_DDAT_2eSSTM_320    (2<<11) /* 320 MB/s */

#define TSI148_LCSR_DDAT_TM_M          (7<<8)  /* Bus Transfer Protocol Mask */
#define TSI148_LCSR_DDAT_TM_SCT        (0<<8)  /* SCT */
#define TSI148_LCSR_DDAT_TM_BLT        (1<<8)  /* BLT */
#define TSI148_LCSR_DDAT_TM_MBLT       (2<<8)  /* MBLT */
#define TSI148_LCSR_DDAT_TM_2eVME      (3<<8)  /* 2eVME */
#define TSI148_LCSR_DDAT_TM_2eSST      (4<<8)  /* 2eSST */
#define TSI148_LCSR_DDAT_TM_2eSSTB     (5<<8)  /* 2eSST Broadcast */

#define TSI148_LCSR_DDAT_DBW_M         (3<<6)  /* Max Data Width MASK */
#define TSI148_LCSR_DDAT_DBW_16        (0<<6)  /* 16 Bits */
#define TSI148_LCSR_DDAT_DBW_32        (1<<6)  /* 32 Bits */

#define TSI148_LCSR_DDAT_SUP           (1<<5)  /* Supervisory/User Access */
#define TSI148_LCSR_DDAT_PGM           (1<<4)  /* Program/Data Access */

#define TSI148_LCSR_DDAT_AMODE_M       (0xf<<0) /* Address Space Mask */
#define TSI148_LCSR_DDAT_AMODE_16      (0<<0)  /* A16 */
#define TSI148_LCSR_DDAT_AMODE_24      (1<<0)  /* A24 */
#define TSI148_LCSR_DDAT_AMODE_32      (2<<0)  /* A32 */
#define TSI148_LCSR_DDAT_AMODE_64      (4<<0)  /* A64 */
#define TSI148_LCSR_DDAT_AMODE_CRCSR   (5<<0)  /* CRC/SR */
#define TSI148_LCSR_DDAT_AMODE_USER1   (8<<0)  /* User1 */
#define TSI148_LCSR_DDAT_AMODE_USER2   (9<<0)  /* User2 */
#define TSI148_LCSR_DDAT_AMODE_USER3   (0xA<<0) /* User3 */
#define TSI148_LCSR_DDAT_AMODE_USER4   (0xB<<0) /* User4 */

    /*
     *  DMA Next Link Address Lower
     */
#define TSI148_LCSR_DNLAL_DNLAL_M      (0x3FFFFFF<<6)    /* Address Mask */
#define TSI148_LCSR_DNLAL_LLA          (1<<0)  /* Last Link Address Indicator */

    /*
     *  DMA 2eSST Broadcast Select
     */
#define TSI148_LCSR_DBS_M              (0x1FFFFF<<0)     /* Mask */

    /*
     *  GCSR Register Group
     */

    /*
     *  GCSR Control and Status Register  CRG + $604
     */
#define TSI148_GCSR_GCTRL_LRST         (1<<15) /* Local Reset */
#define TSI148_GCSR_GCTRL_SFAILEN      (1<<14) /* System Fail enable */
#define TSI148_GCSR_GCTRL_BDFAILS      (1<<13) /* Board Fail Status */
#define TSI148_GCSR_GCTRL_SCON         (1<<12) /* System Copntroller */
#define TSI148_GCSR_GCTRL_MEN          (1<<11) /* Module Enable (READY) */

#define TSI148_GCSR_GCTRL_LMI3S        (1<<7)  /* Loc Monitor 3 Int Status */
#define TSI148_GCSR_GCTRL_LMI2S        (1<<6)  /* Loc Monitor 2 Int Status */
#define TSI148_GCSR_GCTRL_LMI1S        (1<<5)  /* Loc Monitor 1 Int Status */
#define TSI148_GCSR_GCTRL_LMI0S        (1<<4)  /* Loc Monitor 0 Int Status */
#define TSI148_GCSR_GCTRL_MBI3S        (1<<3)  /* Mail box 3 Int Status */
#define TSI148_GCSR_GCTRL_MBI2S        (1<<2)  /* Mail box 2 Int Status */
#define TSI148_GCSR_GCTRL_MBI1S        (1<<1)  /* Mail box 1 Int Status */
#define TSI148_GCSR_GCTRL_MBI0S        (1<<0)  /* Mail box 0 Int Status */

#define TSI148_GCSR_GAP                (1<<5)  /* Geographic Addr Parity */
#define TSI148_GCSR_GA_M               (0x1F<<0) /* Geographic Address Mask */

    /*
     *  CR/CSR Register Group
     */

    /*
     *  CR/CSR Bit Clear Register CRG + $FF4
     */
#define TSI148_CRCSR_CSRBCR_LRSTC      (1<<7)  /* Local Reset Clear */
#define TSI148_CRCSR_CSRBCR_SFAILC     (1<<6)  /* System Fail Enable Clear */
#define TSI148_CRCSR_CSRBCR_BDFAILS    (1<<5)  /* Board Fail Status */
#define TSI148_CRCSR_CSRBCR_MENC       (1<<4)  /* Module Enable Clear */
#define TSI148_CRCSR_CSRBCR_BERRSC     (1<<3)  /* Bus Error Status Clear */

    /*
     *  CR/CSR Bit Set Register CRG+$FF8
     */
#define TSI148_CRCSR_CSRBSR_LISTS      (1<<7)  /* Local Reset Clear */
#define TSI148_CRCSR_CSRBSR_SFAILS     (1<<6)  /* System Fail Enable Clear */
#define TSI148_CRCSR_CSRBSR_BDFAILS    (1<<5)  /* Board Fail Status */
#define TSI148_CRCSR_CSRBSR_MENS       (1<<4)  /* Module Enable Clear */
#define TSI148_CRCSR_CSRBSR_BERRS      (1<<3)  /* Bus Error Status Clear */

    /*
     *  CR/CSR Base Address Register CRG + FFC
     */
#define TSI148_CRCSR_CBAR_M            (0x1F<<3) /* Mask */

#endif  /* TSI148_H */
