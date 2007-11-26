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
	int	status;
	int	i;

	vmeInfoCfg_t vmeInfo;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	printf("My slot = %d\n", myVmeInfo.vmeSlotNum);

	fd = open("/dev/vme_ctl", 0);
	if (fd < 0) {
		printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}
	for (i = 1; i < 21; i++) {
		memset(&vmeInfo, 0, sizeof(vmeInfo));
		vmeInfo.vmeSlotNum = i;
		status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &vmeInfo);
		if (status < 0) {
			printf("%s: Ioctl failed.  Errno = %d\n", argv[0], errno);
			_exit(1);
		}
		if (vmeInfo.boardResponded == 0) { 
			continue; 
		}
		printf("Slot: %d present, ctrl %08x, rev %08x\n", 
		    i, vmeInfo.vmeControllerID, vmeInfo.vmeControllerRev);

	}
	return(0);
}


