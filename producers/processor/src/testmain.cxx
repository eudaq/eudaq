
#include"Processor.hh"
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"

#include"ExamplePS.hh"
#include"EventSenderPS.hh"
#include"EventReceiverPS.hh"
#include"EventFileReaderPS.hh"

#include <chrono>
#include <thread>
#include <iostream>
using namespace eudaq;

int main(int argn, char **argc){
    
  ProcessorManager& psMan = *ProcessorManager::GetInstance();

  PSSP p0 = psMan.MakePSSP("EventFileReaderPS", "SYS:PSID=0;SYS:EVTYPE:ADD=_DET");
  PSSP p1 = psMan.MakePSSP("ExamplePS", "SYS:PSID=1;SYS:EVTYPE:ADD=_DET");
  PSSP p2 = psMan.MakePSSP("ExamplePS", "SYS:PSID=2;SYS:EVTYPE:ADD=_DET");
  PSSP p3 = psMan.MakePSSP("ExamplePS", "SYS:PSID=3;SYS:EVTYPE:ADD=_DET");
  PSSP p4 = psMan.MakePSSP("ExamplePS", "SYS:PSID=4;SYS:EVTYPE:ADD=_DET");
  PSSP p5 = psMan.MakePSSP("ExamplePS", "SYS:PSID=5;SYS:EVTYPE:ADD=_DET");
  PSSP p6 = psMan.MakePSSP("EventSenderPS","SYS:PSID=6;SYS:EVTYPE:ADD=_DET");
  PSSP p7 = psMan.MakePSSP("EventReceiverPS", "SYS:PSID=7;SYS:EVTYPE:ADD=_DET");
  PSSP p8 = psMan.MakePSSP("ExamplePS", "SYS:PSID=8");
  
  std::cout<<"xxxxxxx"<<std::endl;
  
  psMan>>p0>>p2;
  psMan>>p3>>p4>>p5>>p6;
  psMan>>p7>>p8;
  p2>>p4;

  p7<<"SETSERVER=tcp://40000;SYS:PD:RUN";
  p6<<"SYS:SLEEP=1000;CONNECT=Producer,p6,tcp://127.0.0.1:40000";
  p0<<"SYS:SLEEP=1000;FILE=/opt/eudaq/run000703.raw;SYS:PD:RUN";

  std::cout<<"xxxxxxx"<<std::endl;
  
  uint32_t i;
  std::cin>>i;  
}
