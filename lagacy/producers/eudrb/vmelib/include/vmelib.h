/*--------------------------------------------------------------------------------------
*Title		:	TSI148 (TUNDRA) Driver library
*Date:		:	27-10-2006
*Programmer	:	Chiarelli Lorenzo
*Platform	:	Linux (ELinOS) PowerPC
*File 		:	vmelib.h
*Version 	:	1.3
*--------------------------------------------------------------------------------------
*License	: 	You are free to use this source files for your own development as long
*			 	as it stays in a public research context. You are not allowed to use it
*			 	for commercial purpose. You must put this header with 
*			 	authors names in all development based on this library.
*--------------------------------------------------------------------------------------
*/


#ifndef _vmelib
#define _vmelib

#ifdef __cplusplus
extern "C" {
#endif



int getMyInfo();

int vme_A32_D32_User_Data_SCT_read(int fdOut,unsigned long int *readdata,unsigned long int address);
int vme_A32_D16_User_Data_SCT_read(int fdOut,unsigned short int  *readdata,unsigned long int address);

int vme_A24_D32_User_Data_SCT_read(int fdOut,unsigned long int *readdata,unsigned long int address);
int vme_A24_D16_User_Data_SCT_read(int fdOut,unsigned short int *readdata,unsigned long int address);

int vme_A32_D32_User_Data_SCT_write(int fdOut,unsigned long int writedata,unsigned long int address);
int vme_A32_D16_User_Data_SCT_write(int fdOut,unsigned short int writedata,unsigned long int address);

int vme_A24_D16_User_Data_SCT_write(int fdOut,unsigned short int writedata,unsigned long int address);
int vme_A24_D32_User_Data_SCT_write(int fdOut,unsigned long int writedata,unsigned long address);

int vme_A32_D32_User_Data_MBLT_read(int bytecount,unsigned long int srcaddress,unsigned long int *dmadstbuffer);
int vme_A32_D64_User_Data_MBLT_read(int bytecount,unsigned long int srcaddress,unsigned long int *dmadstbuffer);

#ifdef __cplusplus
}
#endif


#endif
