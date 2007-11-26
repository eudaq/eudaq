
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


unsigned int	tempeExpected[] = {  /* registers to read with expected contents */
	/* address,  expected,     mask */
	0x00000000, 0x014810e3, 0xFFFFFFFF,
	0x00000004, 0x02000000, 0xC6800288,
	0x00000008, 0x06800000, 0xFFFFFF00,
	0xFFFFFFFF
};


unsigned int	uniExpected[] = {	/* registers to read with expected contents */
	/* address,  expected,     mask */
	0x00000000, 0x000010e3, 0xFFFFFFFF,
	0x00000004, 0x02000000, 0xC6800288,
	0x00000008, 0x06800000, 0xFFFFFF00,
	0xFFFFFFFF
};


int
main(int argc, char *argv[])
{
	int	fd;
	int	n;
	int	imatempe = 0;
	unsigned int	*regptr;
	unsigned int	expected, actual;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	fd = open("/dev/vme_regs", 0);
	if (fd < 0) {
		printf("%s: Open failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	regptr = uniExpected;
	if (myVmeInfo.vmeControllerID == 0x014810e3) {
		imatempe = 1;
		regptr = tempeExpected;
	}
	while (*regptr < 0x1000) {
		lseek(fd, *regptr, SEEK_SET);
		n = read(fd, &actual, 4);
		if (n != 4) {
			printf("%s: Read at address %08x failed.  Errno = %d\n", 
			    argv[0], *regptr, errno);
			_exit(1);
		}
		actual &= *(regptr + 2);

		expected = *(regptr + 1);
		expected &= *(regptr + 2);

		if (expected != actual) {
			printf("%s: Unexpected data at address %08x.\n", 
			    argv[0], *regptr);
			printf("%s: Expected: %08x Actual: %08x.\n", 
			    argv[0], expected, actual);
			_exit(1);
		}
		regptr += 3;

	}
	return(0);
}


