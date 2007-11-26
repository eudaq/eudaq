
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
   printf("Usage: %s [roundrobin|priority] [no]arbitertimeout bustimeout [no]earlyrelease\n", argv[0]); 
}

int
main(int argc, char *argv[])
{
    int fd;
    int status;
    vmeArbiterCfg_t vmeArb;
    vmeArbiterCfg_t vmeArbGet;

    if(getMyInfo()){
	printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    if(argc != 5){
        usage(argc, argv);
        _exit(1);
    }
    memset(&vmeArb, 0, sizeof(vmeArb));
    if(!strcmp(argv[1], "roundrobin")) {
        vmeArb.arbiterMode = VME_R_ROBIN_MODE;
    } else if (!strcmp(argv[1], "priority")) {
        vmeArb.arbiterMode = VME_PRIORITY_MODE;
    } else {
        usage(argc, argv);
        _exit(1);
    }

    if(!strcmp(argv[2], "noarbitertimeout")) {
        vmeArb.arbiterTimeoutFlag = 0;
    } else if (!strcmp(argv[2], "arbitertimeout")) {
        vmeArb.arbiterTimeoutFlag = 1;
    } else {
        usage(argc, argv);
        _exit(1);
    }

    vmeArb.globalTimeoutTimer = -2;
    sscanf(argv[3], "%d", &vmeArb.globalTimeoutTimer); 
    if(vmeArb.globalTimeoutTimer < 0){
        usage(argc, argv);
        _exit(1);
    }

    if(!strcmp(argv[4], "noearlyrelease")) {
        vmeArb.noEarlyReleaseFlag = 1;
    } else if (!strcmp(argv[4], "earlyrelease")) {
        vmeArb.noEarlyReleaseFlag = 0;
    } else {
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
    status = ioctl(fd, VME_IOCTL_SET_CONTROLLER, &vmeArb);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }

    /*
     * Get the arbiter settings and make sure they match.
     */
    memset(&vmeArbGet, 0, sizeof(vmeArbGet));
    status = ioctl(fd, VME_IOCTL_GET_CONTROLLER, &vmeArbGet);
    if(status < 0){
	printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
	_exit(1);
    }
    if(vmeArbGet.arbiterMode != vmeArb.arbiterMode) {
	printf("%s: arbiterMode was not configured.  Errno = %d\n", 
                argv[0], errno);
	_exit(1);
    }
    if(vmeArbGet.arbiterTimeoutFlag != vmeArb.arbiterTimeoutFlag) {
	printf("%s: arbiterTimeout was not configured.  Errno = %d\n",
                argv[0], errno);
	_exit(1);
    }
    if(vmeArbGet.globalTimeoutTimer < vmeArb.globalTimeoutTimer) {
	printf("%s: globalTimeoutTimer was not configured.  Errno = %d\n",
                argv[0], errno);
	_exit(1);
    }
    if(vmeArbGet.noEarlyReleaseFlag != vmeArb.noEarlyReleaseFlag) {
	printf("%s: noEarlyReleaseFlag was not configured.  Errno = %d\n",
                argv[0], errno);
	_exit(1);
    }

  return(0);
}
