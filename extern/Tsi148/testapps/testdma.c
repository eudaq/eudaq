
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
#include <malloc.h>
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


#define	NUMDMATYPES	3
dmaData_t dmasrctypes[NUMDMATYPES] = {
	VME_DMA_USER,
	VME_DMA_VME, 
	VME_DMA_VME
};
dmaData_t dmadsttypes[NUMDMATYPES] = {
	VME_DMA_VME,
	VME_DMA_USER,
	VME_DMA_VME
};

int
dodmaxfer(
int channel, 
int bytecount, 
dmaData_t srcbus,
addressMode_t srcmode,
unsigned int srcaddress, 
int srcprotocol,
dmaData_t dstbus,
addressMode_t dstmode,
unsigned int dstaddress,
int dstprotocol
)
{
	int fd;
	int	status;
	char devnode[20];
	vmeDmaPacket_t vmeDma;

	sprintf(devnode, "/dev/vme_dma%d", channel);
	fd = open(devnode, 0);
	if (fd < 0) {
		printf("%s: Open failed.  Errno = %d\n", devnode, errno);
		_exit(1);
	}

	memset(&vmeDma, 0, sizeof(vmeDma));
	vmeDma.maxPciBlockSize = 4096;
	vmeDma.maxVmeBlockSize = 4096;
	vmeDma.byteCount = bytecount;
	vmeDma.srcBus = srcbus;
	vmeDma.srcAddr = srcaddress;
	vmeDma.srcVmeAttr.maxDataWidth = VME_D32;
	vmeDma.srcVmeAttr.addrSpace = srcmode;
	vmeDma.srcVmeAttr.userAccessType = VME_SUPER;
	vmeDma.srcVmeAttr.dataAccessType = VME_DATA;
	vmeDma.srcVmeAttr.xferProtocol = srcprotocol;
	vmeDma.dstBus = dstbus;
	vmeDma.dstAddr = dstaddress;
	vmeDma.dstVmeAttr.maxDataWidth = VME_D32;
	vmeDma.dstVmeAttr.addrSpace = dstmode;
	vmeDma.dstVmeAttr.userAccessType = VME_SUPER;
	vmeDma.dstVmeAttr.dataAccessType = VME_DATA;
	vmeDma.dstVmeAttr.xferProtocol = dstprotocol;

	status = ioctl(fd, VME_IOCTL_START_DMA, &vmeDma);
	if (status < 0) {
		printf("VME_IOCTL_START_DMA failed.  Errno = %d\n", errno);
		_exit(1);
	}
	close(fd);
	return(vmeDma.vmeDmaElapsedTime);
}

#define	DMABUFFERSIZE	0x2000000
int	*dmadstbuffer;
int	*dmasrcbuffer;

int
dodmatest(
int channel
)
{
	double	rate;
	int	i, j;
	int	elapsedTime;

	for (i = 0; i < DMABUFFERSIZE/4; i++) 
		dmadstbuffer[i] = -i;
	for (i = 0; i < DMABUFFERSIZE/4; i++) 
		dmasrcbuffer[i] = i;

	for(j = 0x8; j < 0x10000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_PCI, VME_A16, 0, VME_MBLT, 
			VME_DMA_VME, VME_A16, 0x00000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: PCI to A16,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x100000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_PCI, VME_A24, 0, VME_MBLT, 
			VME_DMA_VME, VME_A24, 0x00000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: PCI to A24,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x200000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_PCI, VME_A32, 0, VME_MBLT, 
			VME_DMA_VME, VME_A32, 0x40000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: PCI to A32,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x10000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_USER, VME_A16, (int)dmasrcbuffer, VME_MBLT, 
			VME_DMA_VME, VME_A16, 0x00000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: USER to A16,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x100000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_USER, VME_A24, (int)dmasrcbuffer, VME_MBLT, 
			VME_DMA_VME, VME_A24, 0x00000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: USER to A24,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x200000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_USER, VME_A32, (int)dmasrcbuffer, VME_MBLT, 
			VME_DMA_VME, VME_A32, 0x40000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: USER to A32,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}

	printf("my id = %08x\n", myVmeInfo.vmeControllerID);
	if (myVmeInfo.vmeControllerID != 0x014810e3) {
		return(0);
	}
	for(j = 0x8; j < 0x8000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_VME, VME_A16, 0x0004000, VME_MBLT, 
			VME_DMA_VME, VME_A16, 0x0000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: A16 to A16,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x80000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_VME, VME_A24, 0x0040000, VME_MBLT, 
			VME_DMA_VME, VME_A24, 0x0000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: A24 to A24,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}
	for(j = 0x8; j < 0x200000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_VME, VME_A32, 0x40100000, VME_MBLT, 
			VME_DMA_VME, VME_A32, 0x40000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: A32 to A32,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}

	for(j = 0x8; j < 0x200000; j = j << 1){
		elapsedTime = dodmaxfer(channel, j, 
			VME_DMA_PATTERN_WORD, 0, 0, 0, 
			VME_DMA_VME, VME_A32, 0x40000000, VME_MBLT);
		rate = 1000000.0 / (double)elapsedTime * (double)j;
		printf("Mode: PATTERN to A32,");
		printf(" bytes: %08x,", j);
		printf(" time: %d usec,", elapsedTime);
		printf(" rate: %8.0f bps\n", rate);
	}

	return(0);
}


int
main(int argc, char *argv[])
{
 	int channel;
 	int maxchannel;

	dmadstbuffer = malloc(DMABUFFERSIZE);
	dmasrcbuffer = malloc(DMABUFFERSIZE);
	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	if (myVmeInfo.vmeControllerID != 0x014810e3) {
		maxchannel = 1;
	} else {
		maxchannel = 2;
	}

	for(channel = 0; channel < maxchannel; channel++){
		printf("Testing DMA channel #%d\n", channel);
		dodmatest(channel);
	}

	return(0);
}


