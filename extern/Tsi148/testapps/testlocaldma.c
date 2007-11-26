
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


#define	DMABUFFERSIZE	0x2000000
int	dmadstbuffer[DMABUFFERSIZE/4];
int	dmasrcbuffer[DMABUFFERSIZE/4];

#define	NUMDMATYPES	5
dmaData_t dmatesttypes[NUMDMATYPES] = {
	VME_DMA_PATTERN_BYTE,
	VME_DMA_PATTERN_BYTE_INCREMENT,
	VME_DMA_PATTERN_WORD,
	VME_DMA_PATTERN_WORD_INCREMENT,
	VME_DMA_USER
#if	0
	VME_DMA_KERNEL,
	VME_DMA_PCI,
	VME_DMA_VME
#endif
};

void
flushLine(void *ramptr)
{
	asm("dcbf 0,3");
	asm("sync");
}

int
dodmatest(int channel)
{
	int fd;
	int	status;
	double	rate;
	int	i, j, source;
	char	*srcbyteptr;
	char	*dstbyteptr;
	char devnode[20];
	vmeDmaPacket_t vmeDma;

	printf("Testing DMA channel #%d\n", channel);
	sprintf(devnode, "/dev/vme_dma%d", channel);
	fd = open(devnode, 0);
	if (fd < 0) {
		printf("%s: Open failed.  Errno = %d\n", devnode, errno);
		_exit(1);
	}

	for(source = 0; source < NUMDMATYPES; source++) {
		for (j = 1; j <= DMABUFFERSIZE; j = j << 1) {
			for (i = 0; i < DMABUFFERSIZE/4; i++) {
				dmadstbuffer[i] = -i;
			}
			for (i = 0; i < DMABUFFERSIZE/4; i += 0x8) {
				flushLine(&dmadstbuffer[i]);
			}
			for (i = 0; i < DMABUFFERSIZE/4; i++) 
				dmasrcbuffer[i] = i;
			for (i = 0; i < DMABUFFERSIZE/4; i += 0x8) {
				flushLine(&dmasrcbuffer[i]);
			}
			memset(&vmeDma, 0, sizeof(vmeDma));
			vmeDma.byteCount = j;
			vmeDma.srcBus = dmatesttypes[source];
			vmeDma.srcAddr = (int)dmasrcbuffer;
			if(vmeDma.srcBus != VME_DMA_USER) {
				vmeDma.srcAddr = 0x00000001;
			}
			vmeDma.dstBus = VME_DMA_USER;
			vmeDma.dstAddr = (int)dmadstbuffer;
			vmeDma.maxPciBlockSize = 4096;
	
			status = ioctl(fd, VME_IOCTL_START_DMA, &vmeDma);
			if (status < 0) {
				printf("VME_IOCTL_START_DMA failed.  Errno = %d\n", errno);
				_exit(1);
			}
			if (status < 0) 
				continue;

			vmeDma.byteCount = j;

			if(dmatesttypes[source] == VME_DMA_USER){
				srcbyteptr = (char *)dmasrcbuffer;
				dstbyteptr = (char *)dmadstbuffer;
				for (i = 0; i < j; i++) {
					if (*srcbyteptr != *dstbyteptr) {
 					  printf("failed at %08x, %08x %08x\n",
					 i, *srcbyteptr, *dstbyteptr);
					 break;
					}
					srcbyteptr++;
					dstbyteptr++;
				}
			}
			if(dmatesttypes[source] == VME_DMA_PATTERN_BYTE_INCREMENT){
				dstbyteptr = (char *)dmadstbuffer;
				for (i = 0; i < j; i++) {
					if (*dstbyteptr != ((i+1) & 0xFF)) {
 					  printf("failed at %08x, %08x %08x\n",
					 i, 0, *dstbyteptr);
					 break;
					}
					dstbyteptr++;
				}
			}
			if(dmatesttypes[source] == VME_DMA_PATTERN_BYTE){
				dstbyteptr = (char *)dmadstbuffer;
				for (i = 0; i < j; i++) {
					if (*dstbyteptr != 1) {
 					  printf("failed at %08x, %08x %08x\n",
					 i, 0, *dstbyteptr);
					   break;
					}
					dstbyteptr++;
				}
			}
			if(dmatesttypes[source] == VME_DMA_PATTERN_WORD_INCREMENT){
				for (i = 0; i < j/4; i++) {
					dmadstbuffer[i] = 
			    		((dmadstbuffer[i] & 0xFF000000)>> 24) | 
			    		((dmadstbuffer[i] & 0x00FF0000)>> 8) | 
			    		((dmadstbuffer[i] & 0x0000FF00)<< 8) | 
			    		((dmadstbuffer[i] & 0x000000FF)<< 24);
					if (dmadstbuffer[i] != i+1) {
 					  printf("failed at %08x, %08x %08x\n",
					 i, i, dmadstbuffer[i]);
					 break;
					}
				}
			}
			if(dmatesttypes[source] == VME_DMA_PATTERN_WORD){
				dstbyteptr = (char *)dmadstbuffer;
				for (i = 0; i < j/4; i++) {
					dmadstbuffer[i] = 
			    		((dmadstbuffer[i] & 0xFF000000)>> 24) | 
			    		((dmadstbuffer[i] & 0x00FF0000)>> 8) | 
			    		((dmadstbuffer[i] & 0x0000FF00)<< 8) | 
			    		((dmadstbuffer[i] & 0x000000FF)<< 24);
					if (dmadstbuffer[i] != 1) {
 					  printf("failed at %08x, %08x %08x\n",
					 i, 0, dmadstbuffer[i]);
					   break;
					}
				}
			}

			rate = 1000000.0 / (double)vmeDma.vmeDmaElapsedTime * (double)vmeDma.byteCount;
printf("DMA mode: %d, bytes = %08x, time = %d usec, throughput = %8.0f bps\n",
source, vmeDma.byteCount, vmeDma.vmeDmaElapsedTime, rate);

		}
	}
	return(0);
}

int
main(int argc, char *argv[])
{
 	int channel;

	if (getMyInfo()) {
		printf("%s: getMyInfo failed.  Errno = %d\n", argv[0], errno);
		_exit(1);
	}

	if (myVmeInfo.vmeControllerID != 0x10e30148) {
           	printf("%s: not applicable to non-Tempe boards\n", argv[0]);
	}

	for(channel = 0; channel < 2; channel++){
		dodmatest(channel);
	}

	return(0);
}


