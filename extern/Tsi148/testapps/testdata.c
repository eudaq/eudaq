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


int
testDataReadOnly(int slotnum, int fd)
{
	int	n, i;
	struct vmeSharedData slotdata;

	lseek(fd, slotnum * 512 * 1024, SEEK_SET);
	n = read(fd, &slotdata, sizeof(slotdata));
	if (n != sizeof(slotdata)) {
		printf("Read of slot %d data failed\n", slotnum);
		return(1);
	}


	 // walking 1 & walking 0
	    for (i = 0; i < 32; i++) {
		if (slotdata.readTestPatterns[i*2] != (1 << i)) {
			printf("Read data incorrect at pattern %d\n", i);
		}
		if (slotdata.readTestPatterns[i*2+1] != ~(1 << i)) {
			printf("Read data incorrect at pattern %d\n", i);
		}
	}

	 // all 0, all 1's, 55's and AA's
	  if(slotdata.readTestPatterns[64] != 0x0) {
	        printf("Read data incorrect at pattern 64\n");
	return(1);
	  }
	  if(slotdata.readTestPatterns[65] != 0xFFFFFFFF) {
	        printf("Read data incorrect at pattern 65\n");
	return(1);
	  }
	  if(slotdata.readTestPatterns[66] != 0x55555555) {
	        printf("Read data incorrect at pattern 66\n");
	return(1);
	  }
	  if(slotdata.readTestPatterns[67] != 0xAAAAAAAA) {
	        printf("Read data incorrect at pattern 67\n");
	return(1);
	  }
	
	  // Incrementing data.
	  for(i = 68; i < 1024; i++){
	     if(slotdata.readTestPatterns[i] != i) {
	        printf("Read data incorrect at pattern %d\n", i);
	return(1);
	     }
	  }
	
	   return(0);
	}
	
int
testDataReadWrite(int slotnum, int fd)
{
   int i,n;
   int writetestarea[256];
   int readtestarea[256];
   struct vmeSharedData *slotdata = 0;

   for(i = 0; i < 256; i++){
      writetestarea[i] = i;
   }
   /* 
    * Write the test patterns
    */
   lseek(fd,slotnum*512*1024, SEEK_SET);
   lseek(fd,(int)&slotdata->remoteScratchArea[myVmeInfo.vmeSlotNum][0], SEEK_CUR);
   n = write(fd, writetestarea, sizeof(writetestarea));
   if(n != sizeof(writetestarea)){
       printf("Write of slot %d scratch area failed %d\n", slotnum, errno);
       return(1);
   }

   /* 
    * Read the test patterns back
    */
   lseek(fd,slotnum*512*1024, SEEK_SET);
   lseek(fd,(int)&slotdata->remoteScratchArea[myVmeInfo.vmeSlotNum][0], SEEK_CUR);
   n = read(fd, readtestarea, sizeof(readtestarea));
   if(n != sizeof(writetestarea)){
       printf("Read of slot %d scratch area failed\n", slotnum);
       return(1);
   }

   /* 
    * Verify the read data matches the write data.
    */
   for(i = 0; i < 256; i++){
      if(readtestarea[i] != i) {
         printf("Read data does not match written data at index %i.\n", i);
         printf("Expected %08x, Actual %08x\n", i, readtestarea[i]);
         return(1);
      }
   }
   
   return(0);
}

/*
 * Verify driver interboard data at each populated slot.
 */
int
main(int argc, char *argv[])
{
    int i,fdOut, fdCtl, status;
    vmeInfoCfg_t vmeSlotInfo;
    int tested=0;

    if(getMyInfo()){
	printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    fdOut = open("/dev/vme_m7", O_RDWR);
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
       if(testDataReadOnly(i,fdOut)){
	   printf("%s: read only data failure at slot %d. \n", argv[0], i);
	   _exit(1);
       }
       if(testDataReadWrite(i,fdOut)){
	   printf("%s: read/write data failure at slot %d. \n", argv[0], i);
	   _exit(1);
       }
    }
   if(tested == 0){
      printf("%s: failed to find any boards to test\n", argv[0]);
   }

  return(0);
}
