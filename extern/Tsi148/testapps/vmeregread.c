
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
main(int argc, char *argv[])
{
	int	fd;
	int	n;
	int	regvalue, regoffset;

	if(argc != 2){
		printf("usage: %s regoffset\n", argv[0]);
		_exit(1);
	}

	n = sscanf(argv[1], "%x", &regoffset);
	if((regoffset < 0) || (regoffset > 0xFFC) || (regoffset & 3)){
		printf("usage: %s regoffset\n", argv[0]);
		_exit(1);
	}

	fd = open("/dev/vme_regs", 0);
	if (fd < 0) {
		printf("%s: /dev/vme_regs open failed.  Errno = %d\n", 
			argv[0], errno);
		_exit(1);
	}

	lseek(fd, regoffset, SEEK_SET);
	n = read(fd, &regvalue, 4);

	if (n != 4) {
		printf("%s: Read at address %08x failed.  Errno = %d\n", 
		    argv[0], regoffset, errno);
		_exit(1);
	}

	printf("%08x\n", regvalue);
	return(0);
}
