

/* VME Linux driver test/demo code */
/* Outbound window test */
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
	close(fdInfo);
	return(0);
}


addressMode_t addrmodes[] =  { 
	VME_A16, VME_A24, VME_A32, VME_A64};


addressMode_t addrsize[] =  { 
	16, 24, 32, 64 };


addressMode_t testdatasize[] =  { 
	0, 128 * 1024, 1024 * 1024, 1024 * 1024 };


addressMode_t testdataarea[] =  { 
	-1, 0x10000, 0x40000000, -1 };


addressMode_t testdataoffset[] =  { 
	0, 0, 0, 0 };


int	readdata[1024*1024];

char	devnode[20];

int
main(int argc, char *argv[])
{
	int	fdOut;
	vmeOutWindowCfg_t vmeOut;
	vmeOutWindowCfg_t vmeOutGet;
	int	status;
	int	maxmode = 3;
	int	i, j, window;
	int	n;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	/*
     * Exercise IO through all the windows.
     * Assumes 128K bytes available in A24 space starting at 0x00010000
     * Assumes 1Meg bytes available in A32 space starting at 0x40100000
     * Assumes 1Meg bytes available in A64 space starting at 0x00000000
     * Do not run concurrently with other programs which use outbound
     * windows.
     */
	for (window = 0; window < 7; window++) {
		printf("doing window %d\n", window);
		sprintf(devnode, "/dev/vme_m%d", window);
		fdOut = open(devnode, O_RDWR);
		if (fdOut < 0) {
			printf("%s: Open window #%d failed.  Errno = %d\n", 
			    argv[0], window, errno);
			_exit(1);
		}

		if (myVmeInfo.vmeControllerID == 0x014810e3) {
			maxmode = 4;
		}
		for (i = 0; i < maxmode; i++) {
                        printf("mode %d\n", addrmodes[i]);
			j = 16;
			for (; j < addrsize[i]; j++) {
				memset(&vmeOut, 0, sizeof(vmeOut));
				vmeOut.windowNbr = window;
				vmeOut.addrSpace =  addrmodes[i];
				vmeOut.userAccessType = VME_SUPER;
				vmeOut.dataAccessType = VME_DATA;
				vmeOut.windowEnable = 1;
				vmeOut.windowSizeL = 0x200000;
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
					printf("%s: VME_IOCTL_SET_OUTBOUND failed.  Errno = %d\n", argv[0], errno);
					_exit(1);
				}

				memset(&vmeOutGet, 0, sizeof(vmeOutGet));
				vmeOutGet.windowNbr = window;
				status = ioctl(fdOut, VME_IOCTL_GET_OUTBOUND, &vmeOutGet);
				if (status < 0) {
					printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
					_exit(1);
				}
				if (vmeOutGet.addrSpace != vmeOut.addrSpace ) {
					printf("%08x %08x\n", vmeOutGet.addrSpace, vmeOut.addrSpace);
					printf("addrSpace was not configured \n");
					printf("%d %d %d, addrSpace was not configured \n", window, i, j);
					_exit(1);
				}
				if (vmeOutGet.userAccessType != vmeOut.userAccessType) {
					printf("userAccessType was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.dataAccessType != vmeOut.dataAccessType) {
					printf("dataAccessType was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.addrSpace != vmeOut.addrSpace ) {
					printf("addrSpace was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.windowEnable != vmeOut.windowEnable) {
					printf("windowEnable was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.windowSizeL != vmeOut.windowSizeL) {
					printf("windowSizeL was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.xlatedAddrU != vmeOut.xlatedAddrU) {
					printf("xlatedAddrU was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.xlatedAddrL != vmeOut.xlatedAddrL) {
					printf("xlatedAddrL was not configured \n");
					printf("%08x %08x\n", 
					    vmeOutGet.xlatedAddrL, vmeOut.xlatedAddrL);
					_exit(1);
				}
				if (vmeOutGet.maxDataWidth != vmeOut.maxDataWidth) {
					printf("maxDataWidth was not configured \n");
					_exit(1);
				}
				if (vmeOutGet.xferProtocol != vmeOut.xferProtocol) {
					printf("%08x %08x\n", vmeOutGet.xferProtocol, vmeOut.addrSpace);
					printf("xferProtocol was not configured \n");
					_exit(1);
				}

				/*
               * Verify we can read data at the areas expected to
               * respond.
               */
				if (testdataarea[i] == vmeOut.xlatedAddrL) {
                                        lseek(fdOut, testdataoffset[i], SEEK_SET);
					n = read(fdOut, readdata, testdatasize[i]);
					if (n != testdatasize[i]) {
						printf("read failed errno = %d at address %08x\n",
						    errno, vmeOut.xlatedAddrL);
						printf("n = %d, window = %d, mode = %d\n",
						    n, window, i);
#if	0
						_exit(1);
#endif
					}
printf("First read location: %08x%08x\n", readdata[0], readdata[1]);
				}

			}
		}
		close(fdOut);
	}
	return(0);
}


