
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


#define	HZ 60

char	irqs[8][256];

int
main(int argc, char *argv[])
{
	int	fd;
	int	status;
	int	i, j;

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

	memset(&vmeVirq, 0, sizeof(vmeVirq));
	status = ioctl(fd, VME_IOCTL_CLR_IRQ_STATUS, &vmeVirq);
	if (status < 0) {
		printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	printf("Start testgenirq on remote board...\n");
	sleep(2);

loop:

	memset(&vmeVirq, 0, sizeof(vmeVirq));
	vmeVirq.waitTime = 100 * 4;
	status = ioctl(fd, VME_IOCTL_GET_IRQ_STATUS, &vmeVirq);
	if (status < 0) {
		printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}
	if (vmeVirq.timeOutFlag) {
		printf("%s: Failed. Timeout waiting for IRQ\n", argv[0]);
		_exit(1);
	} else {
		status = ioctl(fd, VME_IOCTL_CLR_IRQ_STATUS, &vmeVirq);
	}
        printf("Got level %d vector %02x\r", vmeVirq.level, vmeVirq.vector);
	irqs[vmeVirq.level][vmeVirq.vector]++;
	for (i = 1; i < 8; i++) {
		for (j = 2; j < 256; j += 2) {
			if (irqs[i][j] == 0) {
				break;
			}
			if (irqs[i][j] > 1) {
				printf("%s: Failed. Unexpected multiple IRQ %d %02x\n", argv[0], i, j);
				_exit(1);
			}
		}
	}
	if ((i == 8) && (j == 256)) {
		printf("%s: Passed - all expected IRQs received\n", argv[0]);
		return(0);
	}
	goto loop;
}


