#ifndef TEST_H_
#define TEST_H_
#endif /*TEST_H_*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sis1100_var.h"
#include "sis3100_vme_calls.h"

void print_return_code (int returnCode);
void programV1495(int dev, u_int32_t baseV1495);
void Sample(int dev, u_int32_t base, u_int32_t baseV1495);
int readCommentLine(FILE *pConfigFile);
void samples2file(FILE *hFile, unsigned int *rawData, int channel, int samples, int iEvent);
void samples2fileBinary(FILE *hFile, unsigned int *rawData, int channel, int samples, int iEvent, int iTrigger);
void header2file(FILE *hFile, int iEvent, int iTrigger); 
void writeInfoFile(FILE *pInfoFile, int samples, int nChannels, int nEvents, int nFiles, char *channel_string);
