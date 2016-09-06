
#include"Processor.hh"
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"


#include <chrono>
#include <thread>
#include <iostream>
using namespace eudaq;

int main(int argn, char **argc){
  ProcessorManager &psMan = *ProcessorManager::GetInstance();
  
  std::cout<<"xxxxxxx"<<std::endl;
  {uint32_t i; std::cin>>i;}

  PSSP p10 = Processor::MakePSSP("ExamplePS", "SYS:PSID=10");
  PSSP p1 = Processor::MakePSSP("EventFileReaderPS", "SYS:PSID=1;FILE=../data/run000703.raw;SYS:SLEEP=1000");
  
  std::cout<<"xxxxxxx"<<std::endl;
  {uint32_t i; std::cin>>i;}
  
  // psMan
  //   >>"EventReceiverPS(SYS:PSID=7;SETSERVER=tcp://40000;SYS:PD:RUN)"
  //   ;
  
  psMan
    >>p1
    // >>"EV(ADD=_DET)"
    // >>p10
    >>"EV(ADD=_DET)"
    >>"DetEventUnpackInsertTimestampPS(SYS:PSID=100)"
    >>"EV(ADD=_RAW)"
    >>"SyncByTimestampPS(SYS:PSID=101)"
    // >>"ExamplePS(SYS:PSID=11)"
    // >>"EventSenderPS(SYS:PSID=6;CONNECT=Producer,p6,tcp://127.0.0.1:40000)"
    ;

  std::cout<<"xxxxxxx"<<std::endl;
  {uint32_t i; std::cin>>i;}

  
  p1<<"SYS:PD:RUN";

  std::cout<<"xxxxxxx"<<std::endl;
  {uint32_t i; std::cin>>i;}

  // p1.reset();
  
  std::cout<<"xxxxxxx"<<std::endl;
  
  uint32_t i;
  std::cin>>i;  
}

// "EventFileReaderPS(SYS:PSID=0;FILE=/opt/eudaq/run000703.raw;SYS:SLEEP=1000;SYS:PD:RUN)"
// "EventReceiverPS(SYS:PSID=7;SETSERVER=tcp://40000;SYS:PD:RUN)"
// "EventSenderPS(SYS:PSID=6;CONNECT=Producer,p6,tcp://127.0.0.1:40000)"
// "ExamplePS(SYS:PSID=2222)"
// "EV(ADD=_DET)"
