#include<ModV551.hh>

ModV551::ModV551(unsigned int adr){

  debug=5; udelay=0;  
  IRQ_Vector=0;

  VMEaddr=adr; 
  Len=0x10000; 

  vme = VMEFactory::Create(VMEaddr,Len,24,16);

  Vers      = 0xfe;
  Type      = 0xfc;
  Code      = 0xfa;
  DAC       = 0x18;
  T5        = 0x16;
  T4        = 0x14;
  T3        = 0x12;
  T2        = 0x10;
  T1        = 0x0e;
  Nchan     = 0x0c;
  Test      = 0x0a;
  Status    = 0x08;
  STrigger  = 0x06;
  SClear    = 0x04;
  IRQlevel  = 0x02;
  IRQvector = 0x00;

  ModuleInfo(); 
  SetIRQLevel(0); //disabled interrupts
  printf(" v551: Interrupts have been disabled \n"); 

  time1 = 150;
  time2 = 30; 
  time3 = 20; 
  time4 = 49; 
  time5 = 48;

}//ModV551
unsigned int ModV551::ReadVME16(unsigned int adr){
  
  unsigned int rc;
  rc = (unsigned int)(vme->Read(adr,data));
  return rc;

}//ReadVME16
void ModV551::WriteVME16(unsigned int adr, unsigned int dataW){

  unsigned short int tmp = (unsigned short int)dataW;
  tmp = ((tmp << 8)&0xFF00 | (tmp >> 8)&0x00FF);
  vme->Write(adr,tmp);

}//WriteVME16
void ModV551::ModuleInfo(){

  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|    VME Sequencer V 551 B                  \n");
  printf("|    Version: %d                            \n",(unsigned int)(vme->Read(Vers,data) >> 12)); 
  printf("|    Serial Number: %d                      \n",(unsigned int)(vme->Read(Vers,data) & 0xF));
  printf("|    Manufacturer number: %d                \n",(unsigned int)(vme->Read(Type,data) >> 10)); 
  printf("|    Type: %d                               \n",(unsigned int)(vme->Read(Type,data) & 0x3FF));  
  printf("|                                           \n");
  printf("--------------------------------------------\n");


}//ModuleInfo
void ModV551::SetIRQStatusID(unsigned int StatusID){

  WriteVME16(IRQvector,StatusID);

}//SetIRQStatusID
void ModV551::SetIRQLevel(unsigned int IRQL){

    WriteVME16(IRQlevel,IRQL);

}//SetIRQLevel
void ModV551::Clear(){

  WriteVME16(SClear,0);

}//SetIRQLevel
void ModV551::SoftwareTrigger(){

  WriteVME16(STrigger,0);

}//SoftwareTrigger
unsigned int ModV551::GetTrigger(){
	unsigned int tmp  =  (  ReadVME16(STrigger)  & 0x1 );
	//printf("trig551=%d ", tmp);
	return tmp;
}//GetTrigger
void ModV551::SetInternalDelay(unsigned int ID){

  unsigned int tmp = ReadVME16(Status) & 0x3F;
  if(ID) WriteVME16(Status,(tmp | 0x1));
  else WriteVME16(Status,(tmp & 0x3E));

}//SetInternalDelay
void ModV551::SetVeto(unsigned int V){

  unsigned int tmp = ReadVME16(Status) & 0x3F;
  if(V) WriteVME16(Status,(tmp | 0x2));
  else WriteVME16(Status,(tmp & 0x3D));

}//SetVeto
void ModV551::SetAutoTrigger(unsigned int AT){

  unsigned int tmp = ReadVME16(Status) & 0x3F;
  if(AT) WriteVME16(Status,(tmp | 0x4));
  else WriteVME16(Status,(tmp & 0x3B));

}//SetAutoTrigger
void ModV551::GetStatus(){
  
  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|     VME Sequencer 551 module STATUS       \n");
  printf("|                                           \n");
  printf("|    Interna Delay: %d                      \n",(int)(vme->Read(Status,data) & 0x1)); 
  printf("|    Veto: %d                               \n",(int)(vme->Read(Status,data) & 0x2) >> 1);
  printf("|    Auto Trigger: %d                       \n",(int)(vme->Read(Status,data) & 0x4) >> 2); 
  printf("|    Data Ready: %d                         \n",(int)(vme->Read(Status,data) & 0x8) >> 3);  
  printf("|    Busy: %d                               \n",(int)(vme->Read(Status,data) & 0x10) >> 4);  
  printf("|    Active Sequence: %d                    \n",(int)(vme->Read(Status,data) & 0x20) >> 5);  
  printf("|                                           \n");
  printf("--------------------------------------------\n");

}//GetStatus
unsigned int ModV551::GetDataRedy(){

  unsigned int tmp = ((ReadVME16(Status) & 0x8) >> 3);
  return tmp;

}//GetDataRedy
unsigned int ModV551::GetBusy(){

  unsigned int tmp = ((ReadVME16(Status) & 0x10) >> 4);
  return tmp;

}//GetBusy
unsigned int ModV551::GetActiveSequence(){
  unsigned int tmp = ((ReadVME16(Status) & 0x20) >> 5);
  while(tmp != 1);
  printf("A=%d ",tmp);
  return tmp;
}//GetActiveSequence
void ModV551::SetTestMode(unsigned int TM){

  unsigned int tmp = ReadVME16(Test) & 0xF;
  if(TM) WriteVME16(Test,(tmp | 0x1));
  else WriteVME16(Test,(tmp & 0xE));

}//SetTestMode
void ModV551::SetClockLevel(unsigned int CL){

  unsigned int tmp = ReadVME16(Test) & 0xF;
  if(CL) WriteVME16(Test,(tmp | 0x2));
  else WriteVME16(Test,(tmp & 0xD));

}//SetClockLevel
void ModV551::SetShiftLevel(unsigned int SL){

  unsigned int tmp = ReadVME16(Test) & 0xF;
  if(SL) WriteVME16(Test,(tmp | 0x4));
  else WriteVME16(Test,(tmp & 0xB));

}//SetShiftLevel
void ModV551::SetTestPulsLevel(unsigned int PL){

  unsigned int tmp = ReadVME16(Test) & 0xF;
  if(PL) WriteVME16(Test,(tmp | 0x8));
  else WriteVME16(Test,(tmp & 0x7));

}//SetTestPulsLevel
void ModV551::GetTest(){

  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|   VME Sequencer 551 module Tets Status    \n");
  printf("|                                           \n");
  printf("|    Test Mode: %d                          \n",(int)(vme->Read(Test,data) & 0x1)); 
  printf("|    Clock Level: %d                        \n",(int)(vme->Read(Test,data) & 0x2) >> 1);
  printf("|    Shift-in Level: %d                     \n",(int)(vme->Read(Test,data) & 0x4) >> 2); 
  printf("|    Test Pulse Level: %d                   \n",(int)(vme->Read(Test,data) & 0x8) >> 3);  
  printf("|                                           \n");
  printf("--------------------------------------------\n");

}//GetTest
unsigned int ModV551::GetTestMode(){

  unsigned int tmp = (ReadVME16(Test) & 0x1);
  return tmp;

}//GetTestMode
unsigned int ModV551::GetClockLevel(){

  unsigned int tmp = ((ReadVME16(Test) & 0x2) >> 1);
  return tmp;

}//GetClockLevel
unsigned int ModV551::GetShiftLevel(){

  unsigned int tmp = ((ReadVME16(Test) & 0x4) >> 2);
  return tmp;

}//GetShiftLevel
unsigned int ModV551::GetTestPulsLevel(){

  unsigned int tmp = ((ReadVME16(Test) & 0x8) >> 3);
  return tmp;

}//GetTestPulsLevel
void ModV551::SetNChannels(unsigned int NCh){

  WriteVME16(Nchan,NCh);

}//SetNChannels
void ModV551::GetNChannels(){

  unsigned int NCh = (ReadVME16(Nchan) & 0x7FF);
  printf("--------------------------------------------\n");
  printf("|                                           \n");
  printf("|   VME Sequencer 551                       \n");
  printf("|                                           \n");
  printf("|    Number of channels: %d                 \n",NCh); 
  printf("|                                           \n");
  printf("--------------------------------------------\n");

}//GetNChannels
void ModV551::SetTime(int t1, int t2, int t3, int t4, int t5){

  if(t1 < 0 || t1 > 255){
    printf(" !!!! Error v551: t1 value over range (0,255), t1=%d\n",t1); 
    exit(1);
  }

  if(t2 <10 || t2 > 511){
    printf(" !!!! Error v551: t2 value over range (10,511), t2=%d\n",t2); 
    exit(1);
  }

  if(t3 < 1 || t3 > 255 || t3 > t4){
    printf(" !!!! Error v551: t3 value over range (1,255) t3 <= t4, t3=%d t4=%d\n",t3,t4); 
    exit(1);
  }

  if(t4 < 1 || t4 > 511){
    printf(" !!!! Error v551: t4 value over range (1,511), t4=%d\n",t4); 
    exit(1);
  }

  if (t5 < 2 || t5 > 511){
    printf(" !!!! Error v551: t5 value over range (2,511), t5=%d\n",t5); 
    exit(1);
  }

  WriteVME16(T1,t1);
  WriteVME16(T2,t2);
  WriteVME16(T3,t3);
  WriteVME16(T4,t4);
  WriteVME16(T5,t5);

}//SetTime
void ModV551::GetTime(int* t1, int* t2, int* t3, int* t4, int* t5){

  *t1 = ReadVME16(T1) & 0xFF;
  *t2 = ReadVME16(T2) & 0x1FF;
  *t3 = ReadVME16(T3) & 0xFF;
  *t4 = ReadVME16(T4) & 0x1FF;
  *t5 = ReadVME16(T5) & 0x1FF;

}//GetTime
void ModV551::SetVCAL(unsigned int VCAL){

  WriteVME16(DAC,VCAL);

}//SetVCAL
/*void ModV551::CalibAutoTrig(){
  unsigned int HitNumberADC0;
  unsigned int HitNumberADC1;
  unsigned int OR;
  unsigned int V;
  int ChNum;
  int ChData;
  int SilcPointer;
//  v551->Init(NumOfChan);

//  for(int i = 0; i < NumOfADC; i++) {
//    v550[i]->Init(NumOfChan);
//    v550[i]->SetPedThrZero();
//  }//i

  int* Ped =  new int[BuffSize551];
  int* Ped2 = new int[BuffSize551];
  int* Noi = new int[BuffSize551];
  int* Norm = new int[BuffSize551];

  for(int tmp = 0; tmp < BuffSize551; tmp++) {
    Ped[tmp] = 0;
    Ped2[tmp] = 0;
    Noi[tmp] = 0;
    Norm[tmp] = 0;
  }//tmp
  printf("!!!!!=====Calibration=====!!!!! ");

  for(int i_trig = 0; i_trig < 1000; i_trig++) {
    m_modv551->SoftwareTrigger();
    if( (i_trig%100) == 0 ) printf("Calibration: Event - %d \n\r",i_trig);

    SilcPointer = 0;

    for(int i = 0; i < NumOfADC; i++) {
      while(!m_modv551->GetDataRedy());
      m_modv550[i]->GetWordCounter(HitNumberADC0,HitNumberADC1);
      for(int ADC_int = 0; ADC_int < 2; ADC_int++) {
    	if(ADC_int == 0){
    		  for(int chan = 0; chan < HitNumberADC0; chan++){
    			  m_modv550[i]->GetFifo0(OR,V,&ChNum,&ChData);
    			  Ped[ChNum + SilcPointer*NumOfChan] += ChData;
    			  Ped2[ChNum + SilcPointer*NumOfChan] += ChData*ChData;
    			  Norm[ChNum + SilcPointer*NumOfChan] += 1;
    		  }//chan
    		  SilcPointer++;
    	  }//ADC0
    	  if(ADC_int == 1){
    		  for(int chan = 0; chan < HitNumberADC1; chan++){
    			  m_modv550[i]->GetFifo1(OR,V,&ChNum,&ChData);
    			  Ped[ChNum + SilcPointer*NumOfChan] += ChData;
    			  Ped2[ChNum + SilcPointer*NumOfChan] += ChData*ChData;
    			  Norm[ChNum + SilcPointer*NumOfChan] += 1;
    		  }//chan
    		  SilcPointer++;
    	  }//ADC1
      }//ADC_int
    }//i
  }//i_trig

  for(int tmp = 0; tmp < BuffSize; tmp++){

    if(Norm[tmp] > 1){
      Ped[tmp] = Ped[tmp]/Norm[tmp];
      Ped2[tmp] = Ped2[tmp]/Norm[tmp];
      Noi[tmp] = (int) ( sqrt(  (double)  ( (Norm[tmp]/(Norm[tmp] - 1)) *(Ped2[tmp] - Ped[tmp]*Ped[tmp]) )  )  );
    }
    else{
      Ped[tmp] = 0;
      		Noi[tmp] = 0;
    	}

  }//tmp
//////////////////////////////////////////////////////////////////////////////
  int ped_tmp, thr_tmp;

  SilcPointer = 0;

  for(int i = 0; i < NumOfADC; i++){

    for(int ADC_int = 0; ADC_int < 2; ADC_int++){
    	//if(!ADC_Status[ADC_int + i*NumOfADC]) continue;
    	for(int chan = 0; chan < NumOfChan; chan++){
    	ped_tmp = Ped[ chan + SilcPointer*NumOfChan ];
    	thr_tmp = ped_tmp + 3*Noi[chan + SilcPointer*NumOfChan];
	m_modv550[i]->SetPedestalThreshold( ADC_int, chan, ped_tmp, thr_tmp );
     }//chan
    	SilcPointer++;
    }//ADC_int
	}//i
  delete [] Ped;
  delete [] Ped2;
  delete [] Noi;
  delete [] Norm;
}//CalibAutoTrig
 */
void ModV551::Init(unsigned int NChan){
  Clear();
  SetTestMode(0);
  SetClockLevel(0);
  SetShiftLevel(0);
  SetTestPulsLevel(0);
  SetInternalDelay(0);
  SetVeto(0);
  SetAutoTrigger(0);
  SetNChannels(NChan);
  SetTime(time1,time2,time3,time4,time5);

}//Init
ModV551::~ModV551(){

  printf("ModV551B destructor is working \n");
    
}//~ModV551B
