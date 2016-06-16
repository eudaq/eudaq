
#include"Processor.hh"
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"

#include <chrono>
#include <iostream>
using namespace eudaq;

// Event* CreateEV_rawdata(){
//   Event *ev = new RawDataEvent()

// }


int main(int argn, char **argc){
  auto psMan = ProcessorManager::GetInstance();
  ProcessorManager *ppsMan = psMan.get();
  // psMan->RegisterProcessorType("ExamplePS", &ExamplePS::Create, 0);
  // psMan->RegisterEventType("RawDataEvent", &CreateEV_+, 0);
  PSSP p1(new Processor("base", 1));  
  PSSP p2(new Processor("base", 2));  

  std::cout<<"xxxxxxx"<<std::endl;

  ppsMan>>p1>>p2;
  // p1->SetPSHub(p1);

  uint32_t i;
  std::cin>>i;
  
  
  std::this_thread::sleep_for(std::chrono::seconds(20));
  
}
