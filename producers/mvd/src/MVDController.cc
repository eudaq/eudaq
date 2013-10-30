#include "MVDController.hh"
#include "eudaq/Utils.hh"
/////////////////////////////
#include<sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <vector>


/////////////////////////////
using eudaq::to_string;
using namespace std;

MVDController::MVDController() : m_modv551(0) {
}
void MVDController::Clear() {
  delete m_modv551;
  m_modv551 = 0;
  for (unsigned i = 0; i < m_modv550.size(); ++i) {
    delete m_modv550[i];
    m_modv550[i] = 0;
  }
  m_modv550.resize(0);
  m_enabled.resize(0);
}
MVDController::~MVDController() {
  Clear();
}
void MVDController::Configure(const eudaq::Configuration & param) {
  Clear();
  NumOfChan = param.Get("NumOfChan", 1280);
  NumOfADC = param.Get("NumOfADC", 2);
  m_modv551 = new ModV551(param.Get("Base551", 0));
  m_modv551->Init(NumOfChan);

  for (unsigned i = 0; i < NumOfADC; ++i) {
    unsigned addr = param.Get("Base_" + to_string(i), 0);
    m_modv550.push_back(new ModV550(addr));
    m_modv550[i]->Init(NumOfChan);
    m_modv550[i]->SetPedThrZero();

    unsigned status = param.Get("Disable_" + to_string(i), 0);
    for (unsigned j = 0; j < MODV550_CHANNELS; ++j) {
      unsigned mask = 1 << j;
      m_enabled.push_back(!(status & mask));
      if (m_enabled.back()) NumOfSil++;
    }
  }
  //CalibAutoTrig();
  ReadCalFile();

}
bool MVDController::Enabled(unsigned adc, unsigned subadc) {
  return adc < NumOfADC && subadc < MODV550_CHANNELS && m_enabled[adc*MODV550_CHANNELS + subadc];
}
std::vector<unsigned int> MVDController::Time(unsigned adc){
	std::vector<unsigned int> result(4);
	result[0]=0xAABBCCD0 + adc;
    gettimeofday(&tv,NULL);
    sec = tv.tv_sec;
    microsec = tv.tv_usec;
    result[1]= sec;
    result[2] = microsec;
    result[3] = 0;
	return result;
}
std::vector<unsigned int> MVDController::Read(unsigned adc, unsigned subadc) {
  if (adc >= NumOfADC) EUDAQ_THROW("Invalid ADC (" + to_string(adc) + " >= " + to_string(NumOfADC) + ")");
  if (subadc >= MODV550_CHANNELS) EUDAQ_THROW("Invalid SubADC (" + to_string(subadc) + " >= " + to_string(MODV550_CHANNELS) + ")");
  unsigned hits = m_modv550[adc]->GetWordCounter(subadc);
  std::vector<unsigned int> result(hits+2);
  result[0] = ((0xAABBCC00 | adc << 1) | subadc);//ene plane
  for (unsigned i = 1; i < hits+1; ++i) {
    result[i] = m_modv550[adc]->GetEvent(subadc);
    if ( (adc == 1 ) & (subadc == 1) & (i == hits) ) { result[hits+1]=0xAABBCCDD;}//end events
  }
  if ( (adc == 1 ) & (subadc == 1) & ( hits < 1 ) ) { result[1]=0xAABBCCDD; }//end events
  //printf(" \n");
  return result;
}
bool MVDController::DataBusy(){
	//printf(" v551: GetBusy\n");
	return m_modv551->GetBusy();
}
bool MVDController:: ActiveSequence(){
	return m_modv551->GetActiveSequence();
}
bool MVDController::DataReady(){
  return m_modv551->GetDataRedy();
}
bool MVDController::GetStatTrigger(){
	unsigned int p = m_modv551->GetTrigger();
	//printf("trigComntrol=%d ",  p);
	return p;
	//return 1;
	//printf("tri");
}
int MVDController::swap4(int w4){
  unsigned int rw4;
  unsigned char b[4];
  for(int i = 0; i < 4; i++) b[i] = (w4>>i*8)&0xff;
  rw4 = 0;
  for(int i = 0; i < 4; i++) rw4+=b[i]<<(3-i)*8;
  return (int)rw4;
}
/*
void MVDController::WriteCalFile(){
	char OFName[200];
	int SilcPointer;
	int iev;
	int CalRunNum;
	unsigned int CalVal[BuffSize + 8];

	CalRunNum = 0;
	FILE* fp ;
	while(1){
	  sprintf(OFName,"./DATA/Calibartion_%d.dat",CalRunNum);
	  fp = fopen(OFName,"r");
	  if(fp){
		  CalRunNum++;
		  fclose(fp);
	  }//if
	  else{
		fp = fopen(OFName,"w");
		if(fp == NULL){
			printf("!!! Warning: Can not open file %s \n",OFName);
			CalRunNum++;
			fclose(fp);
		}
		else break;
	  }
	 }
	 printf("DAQ: You calibration file is: %d \n\n\r", CalRunNum);

	 printf("Note for calibration file:");
	 fgets(OFName,200,stdin);

	 //event time
	 struct timeval tv;
	 unsigned int sec;
	 unsigned int microsec;
	 time_t    timer;
	 struct tm *date;



	 time(&timer);
	 date = localtime(&timer);
	 fprintf(fp,"*Calibration file: %d \n", CalRunNum);
	 fprintf(fp,"*time %d:%d, date %d/%d/%d \n",date->tm_hour, date->tm_min, date->tm_mday, date->tm_mon+1, date->tm_year+1900);
	 fprintf(fp,"*init_SEQ: t1=%d, t2=%d, t3=%d, t4=%d, t5=%d \n",m_modv551->time1,m_modv551->time1,m_modv551->time3,m_modv551->time4,m_modv551->time5);
	 fprintf(fp,"*REM = %s",OFName);

	 int ped_tmp, thr_tmp, PedThr;
	 int chan;

	 ped_tmp = 0;
	 thr_tmp = 0;

	 int PedMax = 700;
	 int NoiMax = 100;

	 unsigned int hPed[NumOfChan];
	 unsigned int hThr[NumOfChan];
	 unsigned int hPedestal[PedMax];

     	iev = 0;
     	CalVal[iev] = 0xAABBCC00;
	 	for(int i = 0; i < NumOfADC; i++){//number of crate
			 for(int ADC_int = 0; ADC_int < 2; ADC_int++){//number of ADC
			 for(int chan = 0; chan < NumOfChan; chan++){
				 m_modv550[i]->GetPedestalThreshold(ADC_int,chan,&ped_tmp,&thr_tmp);
				 m_modv550[i]->GetPedestalThresholdFull(ADC_int, chan, &PedThr);
				 CalVal[iev++] = PedThr;
			 }//chan
			 CalVal[iev++] = ( (0xAABBCC00 | i << 1) | ADC_int);
		 }//ADC_int
	 }//i
	 CalVal[iev++] = 0xAABBCCDD;
	 fwrite(CalVal,4,iev + 1,fp);
	 fclose(fp);
}*/
void MVDController::ReadCalFile() {
    char CharTmp[20];
    int ped_tmp, thr_tmp;
    int tmp;
    int NumOfCrate;
    int ADC_int;
    FILE* SiFile;
    //char OFName[200];
    int string = 200;
    double 	Trigger[2];
    bool ADC_Status[4];

    //only 3 sillicon detectors
    ADC_Status[0] = true; //telescope module 1
    ADC_Status[1] = true; //telescope module 2
    ADC_Status[2] = false; //not active ADC in v550
    ADC_Status[3] = true; //telescope module 3

    printf("DAQ: Set the calibration number: ");
    //fgets(CharTmp,20,stdin);
    pather = new char[string];
    pather ="./DATA/Calibration_1.dat";
	//sprintf(OFName,"./DATA/Calibration_1.dat", atoi(CharTmp) );

	SiFile = fopen(pather,"r");

	if(!SiFile){
	  cout << "!!! ERROR !!!" << endl;
	  cout << "File does not exist" << endl;
	  return ;
	}
    //Run Header
    char Header[80];
    for(int i = 0; i < 4; i++){
      fgets(Header,80,SiFile); printf("%s",Header);
    }
    int chan = 0;
    NumOfCrate = 0;
    ADC_int=0;
    int plane =0;
	for(int i = 0; i < EventLength; i++){//For all channel of strip detector  //EventLength

		if(feof(SiFile) || ferror(SiFile)) return ;

	    ped_tmp = 0;
	    thr_tmp = 0;
	    fread(&tmp,4,1,SiFile);
		//printf(" %d", NumOfCrate);
	    //tmp = swap4(tmp);

		//printf(" tmp=%x", tmp);

	    //printf("TMP0=%x \n", tmp);
	    if(tmp == 0xAABBCCDD)  break;
	    //end of first telescope
	    if(tmp == 0xAABBCC00){
	    	NumOfCrate = 0;
	    	ADC_int = 0;
	    	chan = 0;
	    	printf("adc=1\n");
	    	continue;
	    }
	    //end of second telescope
	    if(tmp == 0xAABBCC01){
	    	NumOfCrate = 0;
	    	ADC_int = 1;
	    	chan = 0;
	    	printf("adc=2\n");
	    	continue;
	    }
	    //end of second telescope
	    if(tmp == 0xAABBCC03){
	    	NumOfCrate = 1;
	    	ADC_int = 0;
	    	chan = 0;
	    	printf("adc=3\n");
	    	continue;
	    }
	    ped_tmp = (tmp >> 12) & 0xFFF;
	    thr_tmp = tmp & 0xFFF;
	    //printf(" ped=%d", ped_tmp);
	    //printf(" thr=%d", thr_tmp);
	    //m_modv550[NumOfCrate]->SetPedestalThreshold(ADC_int,chan,ped_tmp,thr_tmp);
	    m_modv550[NumOfCrate]->ModV550::SetPedestalThresholdFull(ADC_int,chan, tmp);
	    if (NumOfCrate == 1){
	    	m_modv550[NumOfCrate]->SetPedestalThreshold(1,chan,ped_tmp,thr_tmp);
	    }
	    chan++;


	  }//for
	  if(feof(SiFile) || ferror(SiFile)) { return ; }
  return ;
}
/*
void MVDController::TimeEvents(){
	/////////////////////////////////////////////////
	//    	T i m e   o f    e v e n t
	/////////////////////////////////////////////////
	 printf("!!!!!=====T i m e=====!!!!! ");

	iev = 0;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;
	microsec = tv.tv_usec;
	EVENT[iev++] = ievent;
	EVENT[iev++] = sec;
	EVENT[iev++] = microsec;
	EVENT[iev++] = 0;
	/////////////////////////////////////////////////
	//return time;
}
*/
void MVDController::CalibAutoTrig(){
	unsigned int chan;
	int Ped[3840];
	int Ped2[3840];
	int Noi [3840];
	int Norm[3840];
//  m_modv551->Init(NumOfChan);

//  for(int i = 0; i < NumOfADC; i++) {
//	  m_modv550[i]->Init(NumOfChan);
//	  m_modv550[i]->SetPedThrZero();
//  }//i
/*
  int* Ped =  new int[BuffSize];
  int* Ped2 = new int[BuffSize];
  int* Noi = new int[BuffSize];
  int* Norm = new int[BuffSize];
  */
  for(int tmp = 0; tmp < 3840; tmp++) {
    Ped[tmp] = 0;
    Ped2[tmp] = 0;
    Norm[tmp] = 0;
    Noi[tmp] = 0;
  }//

  for(int i_trig = 0; i_trig < 5; i_trig++) {
	  printf("Calibration: Event - %d \n\r",i_trig);
	  m_modv551->SoftwareTrigger();
	  while (! DataBusy())	;
	  //printf(" v551: GetDataBusy DONE\n");
	  while (! DataReady())	;
	  //printf(" v551: GetDataRedy DONE\n");
	  //if( (i_trig%100) == 0 ) printf("Calibration: Event - %d \n\r",i_trig);
	  SilcPointer = 0;
	  for(unsigned int i = 0; i < NumOfADC; i++) {
		  printf("NumOfADC=%d ", i  );
		  for(int ADC_int = 0; ADC_int < 2; ADC_int++) {
////////////////////////////////
/*
			  //SilcPointer++;
			  //printf("ADC_int=%d ", ADC_int  );
			  unsigned hits = m_modv550[i]->GetWordCounter(ADC_int);
			  printf("HitNumberADC%d= %d ", i, hits);
			  chan=0;
			  while(chan < hits){
				  //for(unsigned int chan = 0; chan < hits; chan++){
				  ChTmp = m_modv550[i]->GetEvent(ADC_int);
				  ChNum = (ChTmp >> 12) & 0xfff;
				  ChData = (ChTmp & 0xfff);
				  printf("ChNum= %d \n", ChNum);
				  printf("ChData= %d \n", ChData);
				  Ped[  ChNum + (SilcPointer*1280) ] += ChData;
				  Ped2[ ChNum + (SilcPointer*1280) ] += ChData*ChData;
				  Norm[ ChNum + (SilcPointer*1280) ] += 1;
				  chan++;
			  }
    		SilcPointer++;
    		printf("SilcPointer=%d ", SilcPointer);
  */
    		if(ADC_int == 0){
    			unsigned hits = m_modv550[i]->GetWordCounter(ADC_int);
    			//printf("HitNumberADC%d= %d ", i, hits);
    			for(int chan = 0; chan < hits; chan++){
    				//printf( " %d ", chan );
    				//m_modv550[i]->GetFifo0(OR,V,&ChNum,&ChData);
    				ChTmp = m_modv550[i]->GetEvent(ADC_int);
    				ChNum = (ChTmp >> 12) & 0xfff;
    				ChData = (ChTmp & 0xfff);
    				Ped[  ChNum + (SilcPointer*NumOfCha) ] += ChData;
    				Ped2[ ChNum + (SilcPointer*NumOfCha) ] += ChData*ChData;
    				Norm[ ChNum + (SilcPointer*NumOfCha) ] += 1;
    			}//chan
    			SilcPointer++;
    		}//ADC0
    		if(ADC_int == 1){
    			unsigned hits = m_modv550[i]->GetWordCounter(ADC_int);
    			//printf("HitNumberADC%d= %d \n", i, hits);
    			for(int chan = 0; chan < hits; chan++){
    				//ChData = m_modv550[i]->GetEvent(ADC_int);
    				//printf( chan );
    				//m_modv550[i]->GetFifo1(OR,V,&ChNum,&ChData);
    				ChTmp = m_modv550[i]->GetEvent(ADC_int);
    				ChNum = (ChTmp >> 12) & 0xfff;
    				ChData = (ChTmp & 0xfff);
    				Ped[  ChNum + (SilcPointer*NumOfCha) ] += ChData;
    				Ped2[ ChNum + (SilcPointer*NumOfCha) ] += ChData*ChData;
    				Norm[ ChNum + (SilcPointer*NumOfCha) ] += 1;
    			}//chan
    			SilcPointer++;
    		}//ADC1

    	}//ADC_int
    }//i
  }//i_trig

  printf("!!!!!=====Calibration=====!!!!! ");
  /*
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
  delete [] Norm; */
}//CalibAutoTrig
