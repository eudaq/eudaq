
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
   printf("Usage: %s requestlevel fair|demand [rwd|ror] timeon timeoff\n", argv[0]); 
}

int
main(int argc, char *argv[])
{
    int fd;
    int status;
    vmeRequesterCfg_t vmeReq;
    vmeRequesterCfg_t vmeReqGet;

    if(getMyInfo()){
	printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    if(argc != 6){
        usage(argc, argv);
        _exit(1);
    }

    memset(&vmeReq, 0, sizeof(vmeReq));

    vmeReq.requestLevel = -2;
    sscanf(argv[1], "%d", &vmeReq.requestLevel);
    if((vmeReq.requestLevel) < 0 || (vmeReq.requestLevel > 3)){
        usage(argc, argv);
        _exit(1);
    }

    if(!strcmp(argv[2], "demand")) {
        vmeReq.fairMode = 0;
    } else if (!strcmp(argv[2], "fair")) {
        vmeReq.fairMode = 1;
    } else {
        usage(argc, argv);
        _exit(1);
    }

    if(!strcmp(argv[3], "rwd")) {
        vmeReq.releaseMode = 0;
    } else if (!strcmp(argv[3], "ror")) {
        vmeReq.releaseMode = 1;
    } else {
        usage(argc, argv);
        _exit(1);
    }

    vmeReq.timeonTimeoutTimer = -2;
    sscanf(argv[4], "%d", &vmeReq.timeonTimeoutTimer);
    if(vmeReq.timeonTimeoutTimer < 0) {
        usage(argc, argv);
        _exit(1);
    }

    vmeReq.timeoffTimeoutTimer = -2;
    sscanf(argv[5], "%d", &vmeReq.timeoffTimeoutTimer);
    if(vmeReq.timeoffTimeoutTimer < 0) {
        usage(argc, argv);
        _exit(1);
    }

    fd = open("/dev/vme_ctl", 0);
    if(fd < 0){
	printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }


    /*
     * Set the requested arbiter settings
     */
    status = ioctl(fd, VME_IOCTL_SET_REQUESTOR, &vmeReq);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    /*
     * Get the arbiter settings and make sure they match.
     */
    memset(&vmeReqGet, 0, sizeof(vmeReqGet));
    status = ioctl(fd, VME_IOCTL_GET_REQUESTOR, &vmeReqGet);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }
    if(vmeReqGet.requestLevel != vmeReq.requestLevel) {
	printf("%s: requestLevel was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
    if(vmeReqGet.fairMode != vmeReq.fairMode) {
	printf("%s: fairMode was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
    if(vmeReqGet.releaseMode != vmeReq.releaseMode) {
	printf("%s: releaseMode was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
#if	0	/* Not universally supported */
    if(vmeReqGet.timeonTimeoutTimer != vmeReq.timeonTimeoutTimer) {
	printf("%s: timeonTimeoutTimer was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
    if(vmeReqGet.timeoffTimeoutTimer != vmeReq.timeoffTimeoutTimer) {
	printf("%s: timeoffTimeoutTimer was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
#endif


  return(0);
}
