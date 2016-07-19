
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

  PSSP p1 = psMan.CreateProcessor("ExamplePS", 1);
  PSSP p2 = psMan.CreateProcessor("ExamplePS", 2);
  PSSP p3 = psMan.CreateProcessor("ExamplePS", 3);
  PSSP p4 = psMan.CreateProcessor("ExamplePS", 4);
  PSSP p5 = psMan.CreateProcessor("ExamplePS", 5);
  PSSP p6 = psMan.CreateProcessor("EventSenderPS", 6);
  PSSP p7 = psMan.CreateProcessor("EventReceiverPS", 7);
  PSSP p8 = psMan.CreateProcessor("ExamplePS", 8);
  PSSP p0 = psMan.CreateProcessor("EventFileReaderPS", 0);
  
  std::cout<<"xxxxxxx"<<std::endl;
  
  psMan>>p0>>p2;
  psMan>>p3>>p4>>p5>>p6;
  psMan>>p7>>p8;
  p2>>p4;

  p7<<"SETSERVER=tcp://40000";
  p7->RunProducerThread();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  p6<<"CONNECT=Producer,p6,tcp://127.0.0.1:40000";
  p0<<"FILE=/opt/eudaq/run000703.raw";  
  p0->RunProducerThread();  
  std::cout<<"xxxxxxx"<<std::endl;
  
  uint32_t i;
  std::cin>>i;  
}
