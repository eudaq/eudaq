

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


/*
 * Sequentially generate all even vectors on all levels
 */
int
main(int argc, char *argv[])
{
	int	fd;
	int	status;
	int	level, vec;

	virqInfo_t vmeVirq;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	fd = open("/dev/vme_ctl", 0);
	if (fd < 0) {
		printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	for (level = 1; level < 8; level++) {
		for ( vec = 2; vec < 256; vec += 2) {
			memset(&vmeVirq, 0, sizeof(vmeVirq));
			vmeVirq.level = level;
			vmeVirq.vector = vec;
			vmeVirq.waitTime = 10;
			status = ioctl(fd, VME_IOCTL_GENERATE_IRQ, &vmeVirq);
			if (status < 0) {
				printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
				_exit(1);
			}
			if (vmeVirq.timeOutFlag) {
				printf("%s: VIRQ level %d, VEC %02x not acknowledged\n", 
				    argv[0], level, vec);
				_exit(1);
			}
		}
	}

	return(0);
}


