/*-----------------------------------------------------------------------------------
 *Title          :       Eudet Readout Board (EUDRB) Library
 *Date           :       05-04-2007
 *Programmer     :       Chiarelli Lorenzo
 *Platform       :       Linux (ELinOS) PowerPC
 *File           :       eudrblib.c
 *Version        :       1.0
 *------------------------------------------------------------------------------------
 *License        : You are free to use this source files for your own development as long
 *                       : as it stays in a public research context. You are not allowed to use it
 *                       : for commercial purpose. You must put this header with
 *                       : authors names in all development based on this library.
 *------------------------------------------------------------------------------------
 */


#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>  /*ioctl()*/
#include <sys/types.h> /*open()*/
#include <sys/stat.h> /*open()*/
#include <fcntl.h>  /*open()*/
#include <stdlib.h> /*strtol()*/
#include <string.h>
#include <errno.h>
#include "vmedrv.h" /*driver vmelinux*/
#include "vmelib.h"
#include "eudrblib.h"


/*
 *       EUDRB_CSR Control_Status_Register.eudrb_csr_reg=0x02000000
 *       !!!!    THIS ASSIGNMENT IT'S NOT POSSIBLE USE THE EUDRB_CSR_Default() FUNCTION TO INITIALIZE THE FUNCTIONAL Control_Status_Register VARIABLE;
 */

/*
 *       FUNCTIONS TO SET THE EUDRB FUNCTIONAL CONTROL STATUS REGISTER
 */

/*
 *       MAIN VARIABLE
 */

static EUDRB_CSR Control_Status_Register;

void EUDRB_CSR_Default(int fdOut, unsigned long int baseaddress)
{
  Control_Status_Register.eudrb_csr_reg=0x00000000;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
}

void EUDRB_NIOSII_InterruptServeEna(int fdOut, unsigned long int baseaddress)
{
  Control_Status_Register.Bit.NIOS_IntrptServEnable=1;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
}

void EUDRB_FakeTriggerEnable_Set(int fdOut, unsigned long int baseaddress)
{
  Control_Status_Register.Bit.FakeTriggerEnable=1;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
}

void EUDRB_FakeTriggerEnable_Reset(int fdOut, unsigned long int baseaddress)
{
  /*    Control_Status_Register.Bit.FakeTriggerEnable=1;*/
  Control_Status_Register.Bit.FakeTriggerEnable=0; /* dhaas: must be 0, I suppose! */
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
}

void EUDRB_NIOSII_Reset(int fdOut, unsigned long int baseaddress)
{
  Control_Status_Register.Bit.NIOS_Reset=1;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
  sleep(1);
}

void EUDRB_ZS_On(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=baseaddress;
  unsigned long int readdata32=0, newdata32=0;
  /* read address first and only set the reset bit */
  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  newdata32=readdata32|0x20;
  vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
}
void EUDRB_ZS_Off(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=baseaddress;
  unsigned long int readdata32=0, newdata32=0;
  /* read address first and only set the reset bit */
  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  newdata32=readdata32&~0x20;
  vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
}

/*
  void EUDRB_ZS_On(int fdOut, unsigned long int baseaddress)
  {
  Control_Status_Register.Bit.ZS_Enabled=1;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
  }

  void EUDRB_ZS_Off(int fdOut, unsigned long int baseaddress)
  {
  Control_Status_Register.Bit.ZS_Enabled=0;
  vme_A32_D32_User_Data_SCT_write(fdOut,Control_Status_Register.eudrb_csr_reg,baseaddress);
  }
*/

unsigned long EventDataReady_size(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|0x00400004);
  unsigned long int readdata32=0;
  int i;
  unsigned long int olddata = 0;
  for (i = 0; i <= 100; ++i) {
    vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
    if (readdata32 & 0x80000000) {
      return readdata32 & 0xfffff;
    }
    usleep(1);
    if (readdata32 != olddata) {
      printf("W %d, r=%ld (%08lx)\n", i, readdata32, readdata32);
      olddata = readdata32;
    }
    if (i > 0 && i % 50 == 0)  printf("waiting for ready %d cycles\n",i);
  }
  return 0;
}

void EventDataReady_wait(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|0x00400004);
  unsigned long int readdata32=0;
    int i=0; 
  while((readdata32&0x80000000)!=0x80000000)
    {
      vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
      usleep(1);
      i++; 
      if (i%1000==0)  printf("waiting for ready %d cycles\n",i); 
    }

}

void EUDRB_TriggerProcessingUnit_Reset_Check(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address = baseaddress|0x00400004;
  unsigned long int readdata32=0;
  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  while((readdata32&0x80000000)==0x80000000) {
    vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  }
}

void EUDRB_Reset(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=baseaddress;
  unsigned long int readdata32=0, newdata32=0;
  /* read address first and only set the reset bit */
  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  newdata32=readdata32|0x8;
  vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
  vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
}


/*
  Forse la uso per la visualizzazione se si richiede di leggere lo stato dei vari flags
*/
void EUDRB_CSR_Read(int fdOut, unsigned long int baseaddress)
{
  vme_A32_D32_User_Data_SCT_read(fdOut,&Control_Status_Register.eudrb_csr_reg,baseaddress);
}


/*
 *       NIOS II COMMAND FUNCTIONS
 */

void EUDRB_Fake_Trigger(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|CommandToMCU);
  vme_A32_D32_User_Data_SCT_write(fdOut,FakeTrig_Generate,address);
}

void EUDRB_TriggerProcessingUnit_Reset(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=baseaddress;
  unsigned long int readdata32=0, newdata32=0;
  /* read address first and only set the reset bit */
  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
  newdata32=readdata32|0x8;
  vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
  vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
/*  unsigned long int address=(baseaddress|CommandToMCU); */
/*  vme_A32_D32_User_Data_SCT_write(fdOut,ClearTrigProcUnits,address); */
}

void EUDRB_NIOSII_IsMasterOfSRAM_Set(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|CommandToMCU);
  vme_A32_D32_User_Data_SCT_write(fdOut,uCIsMasterOfSRAM_SET,address);
}

void EUDRB_NIOSII_IsMasterOfSRAM_Reset(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|CommandToMCU);
  vme_A32_D32_User_Data_SCT_write(fdOut,uCIsMasterOfSRAM_CLR,address);
}

void EUDRB_NIOSII_FakeTriggerEnable_Set(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|CommandToMCU);
  vme_A32_D32_User_Data_SCT_write(fdOut,FakeTriggerEnable_SET,address);
}

void EUDRB_NIOSII_FakeTriggerEnable_Reset(int fdOut, unsigned long int baseaddress)
{
  unsigned long int address=(baseaddress|CommandToMCU);
  vme_A32_D32_User_Data_SCT_write(fdOut,FakeTriggerEnable_CLR,address);
}



/*
 *       MIMO*2 CONFIGURATION
 */
void PedThrInitialize(void)
{

  FILE *outputFile_a=NULL;
  FILE *outputFile_b=NULL;
  FILE *outputFile_c=NULL;
  FILE *outputFile_d=NULL;
  int i,temp_adr,TargetData;

  /*CHANNEL A     */
  outputFile_a = fopen("PedThrA.txt", "w");
  if(outputFile_a==NULL)
    {
      printf("Could not open PedThrA.txt\n");
    }
  else
    {
      for(i=0;i<QSRAM_SIZE;i++)
        {
          temp_adr = i & 0x7f;
          if ((temp_adr == 0xa) || (temp_adr == 0xb))
            {
              TargetData  = 0x000000A;       /* threshold*/
              TargetData |= 0x1F << 6;        /* pedestal*/
            }
          else
            {
              TargetData = QSRAM_DATAOUT_Default;
            }
          fprintf(outputFile_a,"%x\n",TargetData);
        }
      fclose(outputFile_a);
    }

  /*CHANNEL B             */
  outputFile_b = fopen("PedThrB.txt", "w");
  if(outputFile_b==NULL)
    {
      printf("Could not open PedThrB.txt\n");
    }
  else
    {

      for (i=0; i<QSRAM_SIZE; i++)
        {
          temp_adr = i & 0x7f;
          if (temp_adr == 0xb)
            {
              TargetData  = 0x000000A;       /* threshold*/
              TargetData |= 0x1F << 6;        /* pedestal*/
            }
          else
            {
              TargetData = QSRAM_DATAOUT_Default;
            }
          fprintf(outputFile_b,"%x\n",TargetData);
        }
      fclose(outputFile_b);
    }

  /*CHANNEL C     */
  outputFile_c = fopen("PedThrC.txt", "w");
  if(outputFile_c==NULL)
    {
      printf("Could not open PedThrC.txt\n");
    }
  else
    {
      for (i=0; i<QSRAM_SIZE; i++)
        {
          temp_adr = i & 0x7f;
          if (temp_adr == 0xc)
            {
              TargetData  = 0x000000A;       /* threshold*/
              TargetData |= 0x1F << 6;        /* pedestal*/
            }
          else
            {
              TargetData = QSRAM_DATAOUT_Default;
            }
          fprintf(outputFile_c,"%x\n",TargetData);
        }
      fclose(outputFile_c);
    }

  /*CHANNEL D*/
  outputFile_d = fopen("PedThrD.txt", "w");
  if(outputFile_d==NULL)
    {
      printf("Could not open PedThrD.txt\n");
    }
  else
    {
      for (i=0; i<QSRAM_SIZE; i++)
        {

          temp_adr = i & 0x7f;
          if (temp_adr == 0xd)
            {
              TargetData  = 0x000000A;       /* threshold*/
              TargetData |= 0x1F << 6;        /* pedestal*/
            }
          else
            {
              TargetData = QSRAM_DATAOUT_Default;
            }
          fprintf(outputFile_d,"%x\n",TargetData);
        }
      fclose(outputFile_d);
    }

}


/*void EUDRB_Init()
  {



  }*/


