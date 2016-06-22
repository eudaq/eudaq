
#include"Processor.hh"
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"

#include"ExamplePS.hh"

#include <chrono>
#include <iostream>
using namespace eudaq;

int main(int argn, char **argc){

  auto psMan = ProcessorManager::GetInstance();
  ProcessorManager *ppsMan = psMan.get();
  // psMan->RegisterProcessorType("ExamplePS", &ExamplePS::Create, 0);
  // psMan->RegisterEventType("RawDataEvent", &CreateEV_rawdata, 0);

  PSSP p1(std::move(ProcessorClassFactory::Create("ExamplePS",1)));
  PSSP p2(std::move(ProcessorClassFactory::Create("ExamplePS",2)));
  PSSP p3(std::move(ProcessorClassFactory::Create("ExamplePS",3)));
  PSSP p4(std::move(ProcessorClassFactory::Create("ExamplePS",4)));
  PSSP p5(std::move(ProcessorClassFactory::Create("ExamplePS",5)));

  std::cout<<"xxxxxxx"<<std::endl;

  ppsMan>>p1>>p2>>p3>>p4>>p5;


  p1->ProduceEvent();
  // auto pp1 = p1.get();
  // std::thread t(*pp1);
  // t.detach();
  std::cout<<"xxxxxxx"<<std::endl;

  
  uint32_t i;
  std::cin>>i;
  
  
  std::this_thread::sleep_for(std::chrono::seconds(20));
  
}
