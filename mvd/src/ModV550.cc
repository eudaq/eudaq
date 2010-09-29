#include<ModV550.hh>
#include "eudaq/Exception.hh"

ModV550::ModV550(unsigned int adr){

  debug=5;

  VMEaddr=adr;
  Len=0x10000;

  vme = VMEFactory::Create(VMEaddr,Len,24,16);
  vme32 = VMEFactory::Create(VMEaddr,Len,24,32);

  //  printf("Size: unsigned int=%d vmelong=%d \n",sizeof(unsigned int),sizeof(unsigned int));
  PT1    = 0x4000;
  PT0    = 0x2000;
  Vers   = 0xfe;
  Type   = 0xfc;
  Code   = 0xfa;
  Test1  = 0x16;
  Test0  = 0x14;
  WC1    = 0x12;
  WC0    = 0x10;
  Fifo_1 = 0x0c;
  Fifo_0 = 0x08;
  MClear = 0x06;
  Nchan  = 0x04;
  Status = 0x02;
  IRQ    = 0x00;

  ModuleInfo();

}//ModV550
void ModV550::ModuleInfo(){

  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|    VME ADC V550 B                         \n");
  printf("|    Version: %d                            \n",(int)(vme->Read(Vers,data) >> 12)); 
  printf("|    Serial Number: %d                      \n",(int)(vme->Read(Vers,data) & 0xF));
  printf("|    Manufacturer number: %d                \n",(int)(vme->Read(Type,data) >> 10)); 
  printf("|    Type: %d                               \n",(int)(vme->Read(Type,data) & 0x3FF));  
  printf("|                                           \n");
  printf("--------------------------------------------\n");


}//ModuleInfo
void ModV550::SetTestPattern(unsigned short int tst0, unsigned short int tst1){

  unsigned short int tmp = tst0;
  tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF); 
  vme->Write(Test0,tmp);

  tmp = tst1;
  tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF); 
  vme->Write(Test1,tmp);

}//SetTestPattern
void ModV550::GetWordCounter(unsigned int& NoOTD0, unsigned int& NoOTD1){

  NoOTD0 = vme->Read(WC0,data) & 0x7FF;
  NoOTD1 = vme->Read(WC1,data) & 0x7FF;

}//GetWordCounter
unsigned int ModV550::GetWordCounter(unsigned int n){
  return vme->Read(n ? WC1 : WC0,data) & 0x7FF;
}//GetWordCounter
void ModV550::GetFifo0(unsigned int OR, unsigned int V, int* ChNum, int* ChData){

  unsigned int tmp = vme32->Read(Fifo_0, 0U);

  OR = tmp >> 31;
  V = (tmp >> 30) & 0x1;
  *ChNum = (tmp >> 12) & 0x7FF;
  *ChData =  tmp & 0xFFF;

}//GetFifo0
void ModV550::GetFifo1(unsigned int OR, unsigned int V, int* ChNum, int* ChData){

  unsigned int tmp = vme32->Read(Fifo_1, 0U);
  
  OR = tmp >> 31;
  V = (tmp >> 30) & 0x1;
  *ChNum = (tmp >> 12) & 0x7FF;
  *ChData =  tmp & 0xFFF;

}//GetFifo1
void ModV550::Clear(){

  vme->Write((MClear), (unsigned short int)(0));

}//Clear
void ModV550::SetNumberOfChannels(unsigned short int ChN0, unsigned short int ChN1){

  unsigned short int tmp = (((ChN1 & 0x3F) << 6) | (ChN0 & 0x3F));
  tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
  vme->Write(Nchan,tmp);

}//SetNumberOfChannels
void ModV550::GetNumberOfChannels(unsigned int& ChN0, unsigned int& ChN1){

  ChN0 = ((vme->Read(Nchan,data) >> 6) & 0x3F);
  ChN1 = (vme->Read(Nchan,data) & 0x3F);

}//GetNumberOfChannels
void ModV550::GetStatus(){

  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|     VME ADC V 550 B module STATUS         \n");
  printf("|                                           \n");
  printf("|    Test mode: %d                          \n",(int)(vme->Read(Status,data) & 0x1));
  printf("|    Memory Owner: %d                       \n",(int)(vme->Read(Status,data) & 0x2) >> 1);
  printf("|    DRDY Channel 0: %d                     \n",(int)(vme->Read(Status,data) & 0x4) >> 2);
  printf("|    DRDY Channel 1: %d                     \n",(int)(vme->Read(Status,data) & 0x8) >> 3);
  printf("|    FIFO 0 Empty: %d                       \n",(int)(vme->Read(Status,data) & 0x10) >> 4);
  printf("|    FIFO 1 Empty: %d                       \n",(int)(vme->Read(Status,data) & 0x20) >> 5);
  printf("|    FIFO 0 HALF FULL: %d                   \n",(int)(vme->Read(Status,data) & 0x40) >> 6);
  printf("|    FIFO 1 HALF FULL: %d                   \n",(int)(vme->Read(Status,data) & 0x80) >> 7);
  printf("|    FIFO 0 FULL: %d                        \n",(int)(vme->Read(Status,data) & 0x100) >> 8);  
  printf("|    FIFO 1 FULL: %d                        \n",(int)(vme->Read(Status,data) & 0x200) >> 9); 
  printf("|                                           \n");
  printf("--------------------------------------------\n");

}//GetStatus
unsigned int ModV550::GetTestMode(){

  return vme->Read(Status,data) & 0x1;

}//GetTestMode
unsigned int ModV550::GetMemoryOwner(){

  return (vme->Read(Status,data) & 0x2) >> 1;

}//GetMemoryOwner
unsigned int ModV550::GetDRDY0(){

  return (vme->Read(Status,data) & 0x4) >> 2;

}//GetDRDY0
unsigned int ModV550::GetDRDY1(){

  return (vme->Read(Status,data) & 0x8) >> 3;

}//GetDRDY1
unsigned int ModV550::GetFifoEmpty0()
{

  return (vme->Read(Status,data) & 0x10) >> 4;

}//GetFifoEmpty0
unsigned int ModV550::GetFifoEmpty1(){

  return (vme->Read(Status,data) & 0x20) >> 5;

}//GetFifoEmpty1
unsigned int ModV550::GetFifoHF0(){

  return (vme->Read(Status,data) & 0x40) >> 6;

}//GetFifoHF0
unsigned int ModV550::GetFifoHF1(){

  return (vme->Read(Status,data) & 0x80) >> 7;

}//GetFifoHF1
unsigned int ModV550::GetFifoFull0(){

  unsigned int tmp;
  tmp = (vme->Read(Status,data) & 0x100) >> 8;
  return tmp;

}//GetFifoFull0
unsigned int ModV550::GetFifoFull1(){

  return (vme->Read(Status,data) & 0x200) >> 9;

}//GetFifoFull1
void ModV550::SetTestMode(unsigned int TM){

  unsigned short int tmp = vme->Read(Status,data);

  if(TM){
    tmp = (tmp | 0x1);
    tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
    vme->Write((Status),tmp);
  }
  else{
    tmp = tmp & 0xFFFE;
    tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
    vme->Write((Status),tmp);
  }

}//SetTestMode
void ModV550::SetMemoryOwner(unsigned int MO){

  unsigned short int tmp = vme->Read(Status,data);

  if(MO){
    tmp = tmp | 0x2;
    tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
    vme->Write((Status),tmp);
  }
  else{
    tmp = tmp & 0xFFFD;
    tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
    vme->Write((Status),tmp);
  }

}//SetMemoryOwner
void ModV550::SetInterrupt(unsigned short int IntLev, unsigned short int StatID){

  unsigned short int tmp = (((IntLev & 0x7) << 8) | (StatID & 0xFF)); 
  tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
  vme->Write((IRQ),tmp);

}//SetInterrupt
void ModV550::SetPedestalThresholdFull(int adc, int chan, int PedThr){
	  if(adc > 1){
	    printf(" !!! Error v550: There are only 2 adc modules in the module");
	    exit(1);
	  }

	  SetMemoryOwner(0); //memory owner VME

	  if(adc == 0){
	    vme32->Write((PT0 + chan*sizeof(PedThr)),PedThr);
	  }
	  if(adc == 1){
	    vme32->Write((PT1 + chan*sizeof(PedThr)),PedThr);
	  }

	  SetMemoryOwner(1); //memory owner Convertion Logick

}//SetPedestalThresholdFull
void ModV550::SetPedestalThreshold(int adc, int chan, int ped, int thr){

  if(adc > 1){
    printf(" !!! Error v550: There are only 2 adc modules in the module");
    exit(1);
  }

  SetMemoryOwner(0); //memory owner VME

  vme32->Write(((adc ? PT1 : PT0) + chan*sizeof(unsigned)), (ped << 12) | thr);

  SetMemoryOwner(1); //memory owner Conversion Logic 

}//SetPedestalThreshold
void ModV550::GetPedestalThreshold(int adc, int chan, int* ped, int* thr){

  SetMemoryOwner(0); //memory owner VME

  unsigned int tmp = vme32->Read((adc ? PT1 : PT0) + chan*sizeof(tmp), 0U);
  
  *ped = (tmp >> 12) & 0xFFF;
  *thr = tmp & 0xFFF;

  SetMemoryOwner(1); //memory owner Conversion Logic

}//GetPedestalThreshold
void ModV550::GetPedestalThresholdFull(int adc, int chan, int* PedThr){
  unsigned int tmp;

  SetMemoryOwner(0); //memory owner VME

  if(adc == 0) tmp = vme32->Read(PT0 + chan*sizeof(tmp),tmp);
  if(adc == 1) tmp = vme32->Read(PT1 + chan*sizeof(tmp),tmp);

  *PedThr = tmp;

  SetMemoryOwner(1); //memory owner Convertion Logick

}//GetPedestalThresholdFull
void ModV550::SetPedThrZero(){

  SetMemoryOwner(0);

  printf(" v550: All pedestals and thresholds set to 0\n");

  for(int i = 0; i < 2047; i++){
    
    vme32->Write(PT0 + i*4,0);
    vme32->Write(PT1 + i*4,0);

  }//i

  SetMemoryOwner(1);

}//SetPedThrZero
void ModV550::Init(unsigned int NChan){

  unsigned int tmp = NChan/32;

  Clear();
  SetTestMode(0);
  SetNumberOfChannels(tmp,tmp);

}//Init
unsigned int ModV550::GetEvent(unsigned int adc){

  return vme32->Read(adc ? Fifo_1 : Fifo_0, 0U);

}//GetEvent
ModV550::~ModV550(){

  printf(" v550: ModV550 destructor is working \n");

}//~ModV550
