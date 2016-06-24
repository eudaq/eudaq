
#include"Processor.hh"
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"

#include"ExamplePS.hh"

#include <chrono>
#include <iostream>
using namespace eudaq;

int main(int argn, char **argc){

  auto psMan = ProcessorManager::GetInstance();

  PSSP p1 = psMan->CreateProcessor("ExamplePS", 1);
  PSSP p2 = psMan->CreateProcessor("ExamplePS", 2);
  PSSP p3 = psMan->CreateProcessor("ExamplePS", 3);
  PSSP p4 = psMan->CreateProcessor("ExamplePS", 4);
  PSSP p5 = psMan->CreateProcessor("ExamplePS", 5);
  
  std::cout<<"xxxxxxx"<<std::endl;
  *psMan>>p1>>p2;
  *psMan>>p3>>p4>>p5;
  p2>>p4;
  
  p1->RunProducerThread();
  
  std::cout<<"xxxxxxx"<<std::endl;
  
  uint32_t i;
  std::cin>>i;
  
  
  std::this_thread::sleep_for(std::chrono::seconds(20));
  
}
