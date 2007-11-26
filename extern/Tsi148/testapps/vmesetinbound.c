
#include <sys/ioctl.h> 

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
#include "../vmedrv.h"

vmeInfoCfg_t myVmeInfo;
int
getMyInfo()
{
    int fd, status;
    fd = open("/dev/vme_ctl", 0);
    if(fd < 0){
       return(1);
    }

    memset(&myVmeInfo, 0, sizeof(myVmeInfo));
    status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &myVmeInfo);
    if(status < 0){
       return(1);
    }
    close(fd);
    return(0);
}

void
usage(int argc, char *argv[])
{
   printf("Usage: %s window_number mode\n", argv[0]); 
}

int
main(int argc, char *argv[])
{
    int fd;
    int status;
    vmeInWindowCfg_t vmeIn;

    int window, mode;

    if(getMyInfo()){
	printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    if(argc != 3){
        usage(argc, argv);
        _exit(1);
    }
    
    window = -1;
    sscanf(argv[1], "%d", &window);
    if((window < 0) || (window > 7)){
        usage(argc, argv);
        _exit(1);
    }

    mode = -1;
    sscanf(argv[2], "%d", &mode);
    if((mode < 0) || (mode > 7)){
        usage(argc, argv);
        _exit(1);
    }

    printf("window = %d, mode = %d\n", window, mode);
    fd = open("/dev/vme_ctl", 0);
    if(fd < 0){
	printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }


    /*
     * Get the current requested window mode
     */
    memset(&vmeIn, 0, sizeof(vmeIn));
    vmeIn.windowNbr = window;
    status = ioctl(fd, VME_IOCTL_GET_INBOUND, &vmeIn);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    printf("current window = %d, mode = %d, addr = %08x\n", window, mode, vmeIn.pciAddrL);

    /*
     * Set the current requested window mode
     */
    vmeIn.addrSpace = mode;
    switch(mode){
       case 2:    // A32
          vmeIn.vmeAddrU = 0;
          vmeIn.vmeAddrL = 0x80000000 
                        + (myVmeInfo.vmeSlotNum * 0x2000000) 
                        + (0x400000*window);
          vmeIn.windowSizeL = 0x20000;
       break;
       case 1:    // A24
          vmeIn.vmeAddrU = 0;
          vmeIn.vmeAddrL = 0x80000 + myVmeInfo.vmeSlotNum * 0x20000;
          vmeIn.windowSizeL = 0x20000;
       break;
       case 0:    // A16
          vmeIn.vmeAddrU = 0;
          vmeIn.vmeAddrL = 0x8000;
          vmeIn.windowSizeL = 0x1000;
       break;
    }
    status = ioctl(fd, VME_IOCTL_SET_INBOUND, &vmeIn);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    printf("window = %d, mode = %d\n", window, mode);

    /*
     * Get the setting and make sure it matches.
     */
    memset(&vmeIn, 0, sizeof(vmeIn));
    vmeIn.windowNbr = window;
    status = ioctl(fd, VME_IOCTL_GET_INBOUND, &vmeIn);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }
    if(vmeIn.addrSpace != mode){
	printf("%s: mode was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }

  return(0);
}
