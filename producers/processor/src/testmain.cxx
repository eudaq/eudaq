
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

  PSSP p0 = psMan.MakePSSP("EventFileReaderPS", "SYS:PSID=0");
  PSSP p100 = psMan.MakePSSP("ExamplePS", "SYS:PSID=100");
  PSSP p200 = psMan.MakePSSP("ExamplePS", "SYS:PSID=200");
  // PSSP p6 = psMan.MakePSSP("EventSenderPS","SYS:PSID=6");
  // PSSP p7 = psMan.MakePSSP("EventReceiverPS", "SYS:PSID=7");
  
  std::cout<<"xxxxxxx"<<std::endl;

  psMan
    >>p100
    >>"EventReceiverPS(SYS:PSID=7;SETSERVER=tcp://40000;SYS:PD:RUN)"
    // >>"EV(ADD=_DET)"
    // >>"ExamplePS(SYS:PSID=8)"
    // >>"EV(ADD=_DET)"
    ;
  
  // p7<<"SETSERVER=tcp://40000;SYS:PD:RUN";
  // p6<<"CONNECT=Producer,p6,tcp://127.0.0.1:40000";
  
  psMan
    >>p200
    >>"EventFileReaderPS(SYS:PSID=0;FILE=/opt/eudaq/run000703.raw;SYS:SLEEP=1000;SYS:PD:RUN)"
    >>"EV(ADD=_DET)"
    // >>"ExamplePS(SYS:PSID=1)"
    // >>"EV(ADD=_DET)"
    // >>"ExamplePS(SYS:PSID=2222)"
    // >>"EV(ADD=_DET)"
    >>"EventSenderPS(SYS:PSID=6;CONNECT=Producer,p6,tcp://127.0.0.1:40000)"
    // >>p6
    ;

  // p0<<"FILE=/opt/eudaq/run000703.raw;SYS:SLEEP=1000;SYS:PD:RUN";

  std::cout<<"xxxxxxx"<<std::endl;
  
  uint32_t i;
  std::cin>>i;  
}
