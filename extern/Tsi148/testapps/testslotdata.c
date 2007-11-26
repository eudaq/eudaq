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
int
getMyInfo()
{
	int	fd, status;
	fd = open("/dev/vme_ctl", 0);
	if (fd < 0) {
		return(1);
	}

	memset(&myVmeInfo, 0, sizeof(myVmeInfo));
	status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &myVmeInfo);
	if (status < 0) {
		return(1);
	}
	close(fd);
	return(0);
}


int
getSlotInfo(int slotno, vmeInfoCfg_t *slotinfo)
{
	int	fd, status;
	fd = open("/dev/vme_ctl", 0);
	if (fd < 0) {
		return(1);
	}

	memset(slotinfo, 0, sizeof(vmeInfoCfg_t));
	status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &slotinfo);
	if (status < 0) {
		return(1);
	}
	close(fd);
	return(0);
}



/*
 * Verify driver access to test buffers at each populated slot.
 */
int
main(int argc, char *argv[])
{
    int i,j,n, fdOut, fdCtl, status, window;
    vmeInfoCfg_t vmeSlotInfo;
    int tested=0;
    vmeOutWindowCfg_t vmeOut;
    int readvalue;

    if(getMyInfo()){
	printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    fdOut = open("/dev/vme_m0", O_RDWR);
    if(fdOut < 0){
	printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    fdCtl = open("/dev/vme_ctl", 0);
    if(fdCtl < 0){
	printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    /*
     * Test data in each populated slot with a driver running
     */
    for(i = 1; i < 22; i++){
               vmeSlotInfo.vmeSlotNum = i;
       if(i == myVmeInfo.vmeSlotNum) { continue; }
       status = ioctl(fdCtl, VME_IOCTL_GET_SLOT_VME_INFO, &vmeSlotInfo);
       if(status < 0){
	   printf("%s: ioctl failed.  Errno = %d\n", argv[0], errno);
	   _exit(1);
       }
       if(vmeSlotInfo.boardResponded == 0) { continue; }
       if(vmeSlotInfo.vmeSharedDataValid == 0) { continue; }
       tested++;
       printf("Testing access to board in slot %d\n", i);
       for(window = 0; window < 8; window++){
           memset(&vmeOut, 0, sizeof(vmeOutWindowCfg_t));
           
           printf("window %d starts at %08x:%08x length %08x mode %x\n", 
		window,
		vmeSlotInfo.vmeAddrHi[window],
		vmeSlotInfo.vmeAddrLo[window],
		vmeSlotInfo.vmeSize[window],
		vmeSlotInfo.vmeAm[window]);
           vmeOut.windowNbr = 0;
           vmeOut.windowEnable = 1;
           vmeOut.windowSizeU = 0;
           vmeOut.windowSizeL = vmeSlotInfo.vmeSize[window] * 2;
           vmeOut.xlatedAddrU = vmeSlotInfo.vmeAddrHi[window];
           vmeOut.xlatedAddrL = vmeSlotInfo.vmeAddrLo[window];
           vmeOut.wrPostEnable = 1;
           vmeOut.prefetchEnable = 1;
           vmeOut.xferRate2esst =
           vmeOut.addrSpace = vmeSlotInfo.vmeAm[window];
           vmeOut.maxDataWidth = VME_D32;
           vmeOut.xferProtocol = VME_SCT;
           vmeOut.userAccessType = VME_SUPER;
           vmeOut.dataAccessType = VME_DATA;
           status = ioctl(fdOut, VME_IOCTL_SET_OUTBOUND, &vmeOut);
           if(status < 0){
	       printf("%s: ioctl failed.  Errno = %d\n", argv[0], errno);
	       _exit(1);
           }
           lseek(fdOut, 0, SEEK_SET);
           for(j = 0; j < vmeSlotInfo.vmeSize[window]; j += 4){
              n = write(fdOut, &j, 4);
              if(n != 4){
	          printf("%s: write failed at %0x.  Errno = %d\n", 
                         argv[0], j, errno);
	          _exit(1);
              }
           }

           lseek(fdOut, 0, SEEK_SET);
           for(j = 0; j < vmeSlotInfo.vmeSize[window]; j += 4){
              n = read(fdOut, &readvalue, 4);
              if(n != 4){
	          printf("%s: read failed at %0x.  Errno = %d\n", 
                         argv[0], j, errno);
	          _exit(1);
              }
              if(readvalue != j){
	          printf("%s: data mismatch.  Expected %08x, Actual %08x.\n", 
                         argv[0], j, readvalue);
	          _exit(1);
              }
           }

           n = read(fdOut, &readvalue, 4);
           if(n == 4){
                  printf("%s: Failed: read succeeded at offset %0x.\n", 
                         argv[0], j);
                  _exit(1);
           }

       }
   }

   if(tested == 0){
      printf("%s: failed to find any boards to test\n", argv[0]);
   }

  return(0);
}
