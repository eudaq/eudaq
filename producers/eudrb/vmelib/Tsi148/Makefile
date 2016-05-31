all: vmemod.o tests
install: all devices

INCLUDE = /usr/src/linux/include/

CFLAGS = -O2 -Wall -DSIMPCIMEMBASE

vmemod.o: vmelinux.o vmedrv.o ca91c042.o tsi148.o Makefile
	ld -r vmelinux.o vmedrv.o ca91c042.o tsi148.o -o vmemod.o

vmelinux.o: vmelinux.c vmedrv.h Makefile
	gcc -I${INCLUDE} ${CFLAGS} -D__KERNEL__ -c vmelinux.c

vmedrv.o: vmedrv.c vmedrv.h Makefile
	gcc -I${INCLUDE} ${CFLAGS} -D__KERNEL__ -c vmedrv.c

ca91c042.o: ca91c042.c ca91c042.h vmedrv.h Makefile
	gcc -I${INCLUDE} ${CFLAGS} -D__KERNEL__ -c ca91c042.c

tsi148.o: tsi148.c tsi148.h vmedrv.h Makefile
	gcc -I${INCLUDE} ${CFLAGS} -D__KERNEL__ -c tsi148.c

clean:
	rm -f probeslots
	rm -f testdata testdma testgenirq testirq testlm
	rm -f testout testreg testrmw testlocaldma
	rm -f vmeregread vmeregwrite
	rm -f vmesetarbiter
	rm -f vmesetrequestor
	rm -f vmesetinbound testslotdata
	rm -f ca91c042.o tsi148.o vmedrv.o vmemod.o vmelinux.o

tests: 
	gcc -Wall testapps/probeslots.c -o probeslots
	gcc -Wall testapps/testdata.c -o testdata
	gcc -Wall testapps/testdma.c -o testdma
	gcc -Wall testapps/testgenirq.c -o testgenirq
	gcc -Wall testapps/testirq.c -o testirq
	gcc -Wall testapps/testlm.c -o testlm
	gcc -Wall testapps/testout.c -o testout
	gcc -Wall testapps/testreg.c -o testreg
	gcc -Wall testapps/testrmw.c -o testrmw
	gcc -Wall testapps/testlocaldma.c -o testlocaldma
	gcc -Wall testapps/vmeregread.c -o vmeregread
	gcc -Wall testapps/vmeregwrite.c -o vmeregwrite
	gcc -Wall testapps/testslotdata.c -o testslotdata
	gcc -Wall testapps/vmesetarbiter.c -o vmesetarbiter
	gcc -Wall testapps/vmesetrequestor.c -o vmesetrequestor
	gcc -Wall testapps/vmesetinbound.c -o vmesetinbound

devices:
	mknod /dev/vme_m0  c 221 0
	mknod /dev/vme_m1  c 221 1
	mknod /dev/vme_m2  c 221 2
	mknod /dev/vme_m3  c 221 3
	mknod /dev/vme_m4  c 221 4
	mknod /dev/vme_m5  c 221 5
	mknod /dev/vme_m6  c 221 6
	mknod /dev/vme_m7  c 221 7
	mknod /dev/vme_dma0 c 221 16
	mknod /dev/vme_dma1 c 221 17
	mknod /dev/vme_ctl  c 221 32
	mknod /dev/vme_regs c 221 33
	mknod /dev/vme_rmw0 c 221 34
	mknod /dev/vme_lm0  c 221 35
	chown root /dev/vme*
	chgrp users /dev/vme*
	chmod 666 /dev/vme*
	chmod 600 /dev/vme_m7
