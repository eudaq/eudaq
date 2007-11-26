

/* VME Linux driver test/demo code */
/* 
 * Include files
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <string.h> 
#include <errno.h> 
#include <sys/ioctl.h> 
#include "../vmedrv.h"

vmeInfoCfg_t myVmeInfo;
int	imatempe=0;
int
getMyInfo()
{
	int	fdInfo, status;
	fdInfo = open("/dev/vme_ctl", 0);
	if (fdInfo < 0) {
		return(1);
	}

	memset(&myVmeInfo, 0, sizeof(myVmeInfo));
	status = ioctl(fdInfo, VME_IOCTL_GET_SLOT_VME_INFO, &myVmeInfo);
	if (status < 0) {
		return(1);
	}

	if (myVmeInfo.vmeControllerID == 0x014810e3) {
		imatempe = 1;
	}

	close(fdInfo);
	return(0);
}

#define	TABLEEND	0x87654321

unsigned int	rmwtestdata[] = {
	/* vmedata   enables     compare     swapdata */
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x12345678,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x12345678,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xF0F0F0F0, 0x00000000, 0x12345678,
	0x00000000, 0x0F0F0F0F, 0x00000000, 0x12345678,
	0xF0F0F0F0, 0x0F0F0F0F, 0xF0F0F0F0, 0x12345678,
	TABLEEND  , 0x0F0F0F0F, 0xF0F0F0F0, TABLEEND,    /* End of table marker. */
};


unsigned int	
getExpectedRmwData(
unsigned int vmedata, 
unsigned int enablemask, 
unsigned int comparedata, 
unsigned int swapdata)
{
	unsigned int	affectedbits;

	affectedbits = (~(vmedata ^ comparedata)) & enablemask;
	vmedata = (vmedata & ~affectedbits) | (swapdata & affectedbits);
	return(vmedata);
}


int
doRmwTest(int fdOut, int fdRmw, vmeRmwCfg_t *vmeRmw)
{
	unsigned int	*rmwdataptr, vmedata, enableMask, compareData, swapData;
	unsigned int	postRmwData;
	int	n;
	int	status;

	rmwdataptr =  rmwtestdata;
	while (*rmwdataptr != TABLEEND) {
		vmedata = *rmwdataptr++;
		enableMask = *rmwdataptr++;
		compareData = *rmwdataptr++;
		swapData = *rmwdataptr++;
#if	0
printf("testing %08x %08x %08x %08x\n",
		vmedata,
		enableMask,
		compareData,
		swapData
);
#endif

		lseek(fdOut, 0, SEEK_SET);
		n = write(fdOut, &vmedata, 4);
		if (n != 4) {
			printf("write failed at address %08x, errno = %d\n", vmeRmw->targetAddr, errno);
			return(1);
		}
		vmeRmw->enableMask = enableMask;
		vmeRmw->compareData = compareData;
		vmeRmw->swapData = swapData;
		vmeRmw->maxAttempts = 2;

		status = ioctl(fdRmw, VME_IOCTL_DO_RMW, vmeRmw);
		if (status < 0) {
			printf("IOCTL_DO_RMW failed errno = %d\n", errno);
			return(1);
		}
		lseek(fdOut, 0, SEEK_SET);
		n = read(fdOut, &postRmwData, 4);
		if (n != 4) {
			printf("read failed at address %08x\n", vmeRmw->targetAddr);
			return(1);
		}
		if (getExpectedRmwData(vmedata, enableMask, compareData, swapData) != 
		    postRmwData) {
			printf("rmw results wrong at at address %08x\n", vmeRmw->targetAddr);
			printf("expected %08x, actual %08x\n",
			    getExpectedRmwData(vmedata, enableMask, compareData, swapData),
			    postRmwData);
			return(1);
		}
	}
	return(0);
}


addressMode_t addrmodes[] =  { 
	VME_A24, VME_A32, VME_A64};


int addrsize[] =  { 
	24, 32, 64 };


unsigned int testdataarea[] =  { 
	0, 0x40000000, -1 };


char	devnode[20];

int
main(int argc, char *argv[])
{
	int	fdOut, fdRmw;
	vmeOutWindowCfg_t vmeOut;
	vmeRmwCfg_t vmeRmw;
	int	status;
	int	maxmode = 2;
	int	i, j, window;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	fdRmw = open("/dev/vme_rmw0", 0);
	if (fdRmw < 0) {
		printf("%s: Open /dev/vme_rmw0 failed.  Errno = %d\n", 
		    argv[0], errno);
		_exit(1);
	}

	/*
     * Exercise RMW through all the windows.
     * Assumes 128K bytes available in A24 space starting at 0x00000000
     * Assumes 1Meg bytes available in A32 space starting at 0x40000000
     * Assumes 1Meg bytes available in A64 space starting at 0x00000000
     * Do not run concurrently with other programs which use outbound
     * windows.
     */
	for (window = 0; window < 7; window++) {
		sprintf(devnode, "/dev/vme_m%d", window);
		fdOut = open(devnode, O_RDWR);
		if (fdOut < 0) {
			printf("%s: Open window #%d failed.  Errno = %d\n", 
			    argv[0], window, errno);
			_exit(1);
		}
		if(imatempe) maxmode = 3;
		for (i = 0; i < maxmode; i++) {
			for (j = 16; j < addrsize[i]; j++) {
				memset(&vmeOut, 0, sizeof(vmeOut));
				vmeOut.windowNbr =  window;
				vmeOut.addrSpace =  addrmodes[i];
				vmeOut.userAccessType = VME_SUPER;
				vmeOut.dataAccessType = VME_DATA;
				vmeOut.windowEnable = 1;
				vmeOut.windowSizeL = 0x100000;
				if (j > 31) {
					vmeOut.xlatedAddrU = 1 << (j - 32);
					vmeOut.xlatedAddrL = 0;
				} else {
					vmeOut.xlatedAddrU = 0;
					vmeOut.xlatedAddrL = 1 << j;
				}
				vmeOut.maxDataWidth = VME_D32;
				vmeOut.xferProtocol = VME_SCT;
				status = ioctl(fdOut, VME_IOCTL_SET_OUTBOUND, &vmeOut);
				if (status < 0) {
					printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
					_exit(1);
				}

				/*
				 * Test RMW at areas expected to respond
				 */
				if (testdataarea[i] == vmeOut.xlatedAddrL) {
					printf("testing at %08x %08x\n",
vmeOut.xlatedAddrL,vmeOut.addrSpace);
					memset(&vmeRmw, 0, sizeof(vmeRmw));
					vmeRmw.addrSpace =  addrmodes[i];
					if (j > 31) {
						vmeRmw.targetAddrU = 1 << (j - 32);
						vmeRmw.targetAddr = 0;
					} else {
						vmeRmw.targetAddrU = 0;
						vmeRmw.targetAddr = 1 << j;
					}
					if (doRmwTest(fdOut, fdRmw, &vmeRmw) != 0) {
						printf("rmw failed at address %08x\n", vmeOut.xlatedAddrL);
						_exit(1);
					}
				}

			}
		}
		close(fdOut);
	}
	return(0);
}
