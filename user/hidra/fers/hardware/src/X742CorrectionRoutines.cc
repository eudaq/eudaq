/******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/

#include "X742CorrectionRoutines.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_READ_CHAR               1000
#define MAX_BASE_INPUT_FILE_LENGTH  1000

static void PeakCorrection(CAEN_DGTZ_X742_GROUP_t *dataout) {
	int offset;
	int chaux_en;
	unsigned int i;
	int j;

	chaux_en = (dataout->ChSize[8] == 0)? 0:1;
	for(j=0; j<(8+chaux_en); j++){
		dataout->DataChannel[j][0] = dataout->DataChannel[j][1];
	}
	for(i=1; i<dataout->ChSize[0]; i++){
		offset=0;
		for(j=0; j<8; j++){
			if (i==1){
				if ((dataout->DataChannel[j][2]- dataout->DataChannel[j][1])>30){								  
					offset++;
				}
				else {
					if (((dataout->DataChannel[j][3]- dataout->DataChannel[j][1])>30)&&((dataout->DataChannel[j][3]- dataout->DataChannel[j][2])>30)){								  
						offset++;
					}
				}
			}
			else{
				if ((i==dataout->ChSize[j]-1)&&((dataout->DataChannel[j][dataout->ChSize[j]-2]- dataout->DataChannel[j][dataout->ChSize[j]-1])>30)){  
					offset++;										 							        
				}
				else{
					if ((dataout->DataChannel[j][i-1]- dataout->DataChannel[j][i])>30){ 
						if ((dataout->DataChannel[j][i+1]- dataout->DataChannel[j][i])>30)
							offset++;
						else {
							if ((i==dataout->ChSize[j]-2)||((dataout->DataChannel[j][i+2]-dataout->DataChannel[j][i])>30))
								offset++;
						} 							        
					}
				}
			}
		}								

		if (offset==8){
			for(j=0; j<(8+chaux_en); j++){
				if (i==1){
					if ((dataout->DataChannel[j][2]- dataout->DataChannel[j][1])>30) {
						dataout->DataChannel[j][0]=dataout->DataChannel[j][2];
						dataout->DataChannel[j][1]=dataout->DataChannel[j][2];
					}
					else{
						dataout->DataChannel[j][0]=dataout->DataChannel[j][3];
						dataout->DataChannel[j][1]=dataout->DataChannel[j][3];
						dataout->DataChannel[j][2]=dataout->DataChannel[j][3];
					}
				}
				else{
					if (i==dataout->ChSize[j]-1){
						dataout->DataChannel[j][dataout->ChSize[j]-1]=dataout->DataChannel[j][dataout->ChSize[j]-2];
					}
					else{
						if ((dataout->DataChannel[j][i+1]- dataout->DataChannel[j][i])>30)
							dataout->DataChannel[j][i]=((dataout->DataChannel[j][i+1]+dataout->DataChannel[j][i-1])/2);
						else {
							if (i==dataout->ChSize[j]-2){
								dataout->DataChannel[j][dataout->ChSize[j]-2]=dataout->DataChannel[j][dataout->ChSize[j]-3];
								dataout->DataChannel[j][dataout->ChSize[j]-1]=dataout->DataChannel[j][dataout->ChSize[j]-3];
							}
							else {
								dataout->DataChannel[j][i]=((dataout->DataChannel[j][i+2]+dataout->DataChannel[j][i-1])/2);
								dataout->DataChannel[j][i+1]=( (dataout->DataChannel[j][i+2]+dataout->DataChannel[j][i-1])/2);
							}
						}
					}
				}
			}
		}								
	}
}

/*! \brief   Corrects 'data' depending on the informations contained in 'CTable'
*
*   \param   CTable              :  Pointer to the Table containing the Data Corrections
*   \param   frequency           :  The operational Frequency of the board
*   \param   CorrectionLevelMask :  Mask of Corrections to be applied
*   \param   data                :  Data to be corrected
*/
void ApplyDataCorrection(CAEN_DGTZ_DRS4Correction_t* CTable, CAEN_DGTZ_DRS4Frequency_t frequency, int CorrectionLevelMask, CAEN_DGTZ_X742_GROUP_t *data) {

    int i, j, size1, trg = 0, k;
    float Time[1024], t0; 
    float Tsamp; 
	float vcorr; 
    uint16_t st_ind = 0;
    float wave_tmp[1024];
	int cellCorrection = CorrectionLevelMask & 0x1;
	int nsampleCorrection = (CorrectionLevelMask & 0x2) >> 1;
	int timeCorrection = (CorrectionLevelMask & 0x4) >> 2;
   
	switch(frequency) {
		case CAEN_DGTZ_DRS4_2_5GHz:
			Tsamp =(float)((1.0/2500.0)*1000.0);
			break;
		case CAEN_DGTZ_DRS4_1GHz:
			Tsamp =(float)((1.0/1000.0)*1000.0);
			break;
		case CAEN_DGTZ_DRS4_5GHz:
			Tsamp =(float)((1.0/5000.0)*1000.0);
			break;
		default:
			Tsamp =(float)((1.0/750.0)*1000.0);
			break;
	}

	if (data->ChSize[8] != 0) trg = 1;
	st_ind =(uint16_t)(data->StartIndexCell);
	for (i=0;i<MAX_X742_CHANNEL_SIZE;i++) {
		size1  = data->ChSize[i];
        for (j=0;j<size1;j++) {
			if (cellCorrection)
                data->DataChannel[i][j]  -=  CTable->cell[i][((st_ind+j) % 1024)]; 
			if (nsampleCorrection)
                data->DataChannel[i][j] -= CTable->nsample[i][j];
		}
	}
	
	if (cellCorrection)
        PeakCorrection(data);
	if (!timeCorrection)
        return;

	t0 = CTable->time[st_ind];                       
	Time[0]=0.0;
	for(j=1; j < 1024; j++) {
		 t0= CTable->time[(st_ind+j)%1024]-t0;
		 if  (t0 >0) 
		   Time[j] =  Time[j-1]+ t0;
		 else
		   Time[j] =  Time[j-1]+ t0 + (Tsamp*1024);

		 t0 = CTable->time[(st_ind+j)%1024];
	}
	for (j=0;j<8+trg;j++) {
		data->DataChannel[j][0] = data->DataChannel[j][1];
		wave_tmp[0] = data->DataChannel[j][0];
		vcorr = 0.0;
		k=0;
		i=0;

		for(i=1; i<1024; i++)  {
			while ((k<1024-1) && (Time[k]<(i*Tsamp)))  k++;
			vcorr =(((float)(data->DataChannel[j][k] - data->DataChannel[j][k-1])/(Time[k]-Time[k-1]))*((i*Tsamp)-Time[k-1]));
			wave_tmp[i]= data->DataChannel[j][k-1] + vcorr;
			if (wave_tmp[i] < 0)
				wave_tmp[i] = 0;
			if(wave_tmp[i] > 4095)
				wave_tmp[i] = 4095;
			k--;								
		}
		memcpy(data->DataChannel[j],wave_tmp,1024*sizeof(float));
	}
}

/*! \brief   Write the correction table of a x742 boards into the output files
*
*   \param   Filename of output file
*   \param   Group Mask of Tables to be saved
*   \param   Pointer to the DataCorrection group tables
*/
int SaveCorrectionTables(char *outputFileName, uint32_t groupMask, CAEN_DGTZ_DRS4Correction_t *tables) {
    char fnStr[MAX_BASE_INPUT_FILE_LENGTH + 1];
    int ch,i,j, gr;
    FILE *outputfile;

    if(strlen(outputFileName) > MAX_BASE_INPUT_FILE_LENGTH + 17)
        return -1; // Too long base filename

    for(gr = 0; gr < MAX_X742_GROUP_SIZE; gr++) {
        CAEN_DGTZ_DRS4Correction_t *tb;

        if(!((groupMask>>gr)&0x1))
            continue;
        tb = &tables[gr];
        sprintf(fnStr, "%s_gr%d_cell.txt", outputFileName, gr);
        printf("Saving correction table cell values to %s\n", fnStr);
        if((outputfile = fopen(fnStr, "w")) == NULL)
            return -2;
        for(ch=0; ch<MAX_X742_CHANNEL_SIZE; ch++) {
            fprintf(outputfile, "Calibration values from cell 0 to 1024 for channel %d:\n\n", ch);
            for(i=0; i<1024; i+=8) {
                for(j=0; j<8; j++)
                    fprintf(outputfile, "%d\t", tb->cell[ch][i+j]);
                fprintf(outputfile, "cell = %d to %d\n", i, i+7);
            }
        }
        fclose(outputfile);

        sprintf(fnStr, "%s_gr%d_nsample.txt", outputFileName, gr);
        printf("Saving correction table nsamples values to %s\n", fnStr);
        if((outputfile = fopen(fnStr, "w")) == NULL)
            return -3;
        for(ch=0; ch<MAX_X742_CHANNEL_SIZE; ch++) {
            fprintf(outputfile, "Calibration values from cell 0 to 1024 for channel %d:\n\n", ch);
            for(i=0; i<1024; i+=8) {
                for(j=0; j<8; j++)
                    fprintf(outputfile, "%d\t", tb->nsample[ch][i+j]);
                fprintf(outputfile, "cell = %d to %d\n", i, i+7);
            }
        }
        fclose(outputfile);

        sprintf(fnStr, "%s_gr%d_time.txt", outputFileName, gr);
        printf("Saving correction table time values to %s\n", fnStr);
        if((outputfile = fopen(fnStr, "w")) == NULL)
            return -4;
        fprintf(outputfile, "Calibration values (ps) from cell 0 to 1024 :\n\n");
        for(i=0; i<1024; i+=8) {
            for(ch=0; ch<8; ch++)
                fprintf(outputfile, "%09.3f\t", tb->time[i+ch]);
            fprintf(outputfile, "cell = %d to %d\n", i, i+7);
        }
        fclose(outputfile);
    }
    return 0;
}

/*! \brief   Reads the correction table of a x742 boards from txt files
*
*   \param   Base Filename of input file. Actual filenames loaded will be:
*               a) baseInputFileName + "_cell.txt"
*               b) baseInputFileName + "_nsample.txt"
*               c) baseInputFileName + "_time.txt"
*   \param   DataCorrection table to be filled
*/

int LoadCorrectionTable(char *baseInputFileName, CAEN_DGTZ_DRS4Correction_t *tb) {
    char fnStr[MAX_BASE_INPUT_FILE_LENGTH + 1];
    int ch, i, j, read;
    FILE *inputfile;
    char Buf[MAX_READ_CHAR + 1], *pread;

    if(strlen(baseInputFileName) > MAX_BASE_INPUT_FILE_LENGTH + 13)
        return -1; // Too long base filename

    strcpy(fnStr, baseInputFileName);
    strcat(fnStr, "_cell.txt");
    printf("Loading correction table cell values from %s\n", fnStr);
    if((inputfile = fopen(fnStr, "r")) == NULL)
        return -2;
    for(ch=0; ch<MAX_X742_CHANNEL_SIZE; ch++) {
        while(strstr(Buf, "Calibration") != Buf)
            pread = fgets(Buf, MAX_READ_CHAR, inputfile);
        
        for(i=0; i<1024; i+=8) {
            for(j=0; j<8; j++)
                read = fscanf(inputfile, "%hd", &(tb->cell[ch][i+j]));
            pread = fgets(Buf, MAX_READ_CHAR, inputfile);
        }
    }
    fclose(inputfile);

    strcpy(fnStr, baseInputFileName);
    strcat(fnStr, "_nsample.txt");
    printf("Loading correction table nsamples values from %s\n", fnStr);
    if((inputfile = fopen(fnStr, "r")) == NULL)
        return -3;
    for(ch=0; ch<MAX_X742_CHANNEL_SIZE; ch++) {
        while(strstr(Buf, "Calibration") != Buf)
            pread = fgets(Buf, MAX_READ_CHAR, inputfile);
        
        for(i=0; i<1024; i+=8) {
            for(j=0; j<8; j++)
                read = fscanf(inputfile, "%hhd", &(tb->nsample[ch][i+j]));
            pread = fgets(Buf, MAX_READ_CHAR, inputfile);
        }
    }
    fclose(inputfile);

    strcpy(fnStr, baseInputFileName);
    strcat(fnStr, "_time.txt");
    printf("Loading correction table time values from %s\n", fnStr);
    if((inputfile = fopen(fnStr, "r")) == NULL)
        return -4;
    while(strstr(Buf, "Calibration") != Buf)
        pread = fgets(Buf, MAX_READ_CHAR, inputfile);
    pread = fgets(Buf, MAX_READ_CHAR, inputfile);
        
    for(i=0; i<1024; i+=8) {
        for(j=0; j<8; j++)
            read = fscanf(inputfile, "%f", &(tb->time[i+j]));
        pread = fgets(Buf, MAX_READ_CHAR, inputfile);
    }
    fclose(inputfile);

    return 0;
}
