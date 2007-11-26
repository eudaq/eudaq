

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


addressMode_t addrmodes[] =  { 
	VME_A16, VME_A24, VME_A32, VME_A64};


addressMode_t addrsize[] =  { 
	16, 24, 32, 64 };


int
main(int argc, char *argv[])
{
	int	fdLm, fdOut;
	vmeOutWindowCfg_t vmeOut;
	vmeLmCfg_t vmeLm;
	int	status;
	int	maxmode = 3;
	int	i, j;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	fdLm = open("/dev/vme_lm0", 0);
	if (fdLm < 0) {
		printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	fdOut = open("/dev/vme_m0", 0);
	if (fdOut < 0) {
		printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	if(imatempe) maxmode = 4;
	for (i = 0; i < maxmode; i++) {
		printf("testing mode %d\n", addrmodes[i]);
		for (j = 16; j < addrsize[i]; j++) {
			memset(&vmeLm, 0, sizeof(vmeLm));
			vmeLm.addrSpace =  addrmodes[i];
			if (j > 31) {
				vmeLm.addrU = 1 << (j - 32);
				vmeLm.addr = 0;
			} else {
				vmeLm.addrU = 0;
				vmeLm.addr = 1 << j;
			}
			vmeLm.userAccessType = VME_USER | VME_SUPER;
			vmeLm.dataAccessType = VME_DATA | VME_PROG;
			vmeLm.lmWait = 1;
			status = ioctl(fdLm, VME_IOCTL_SETUP_LM, &vmeLm);
			if (status < 0) {
				printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
				_exit(1);
			}

			memset(&vmeOut, 0, sizeof(vmeOut));
			vmeOut.addrSpace =  addrmodes[i];
			vmeOut.userAccessType = VME_SUPER;
			vmeOut.dataAccessType = VME_DATA;
			vmeOut.windowNbr = 0;
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
                        lseek(fdOut, 0, SEEK_SET);
			read(fdOut, &status, 2);	/* Don't care about data... */

			status = ioctl(fdLm, VME_IOCTL_WAIT_LM, &vmeLm);
			if (status < 0) {
				printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
				_exit(1);
			}
			if (vmeLm.lmEvents == 0) {
				printf("%s: failed. LM did not detect access at address %08x\n", 
				    argv[0], vmeLm.addr);
				_exit(1);
			}
		}
	}
	return(0);
}


