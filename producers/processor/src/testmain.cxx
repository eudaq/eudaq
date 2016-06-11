
#include"ProcessorManager.hh"
#include"RawDataEvent.hh"


Event* CreateEV_rawdata(){
  Event *ev = new RawDataEvent()

}


int main(int argn, char **argc){
  auto psMan = ProcessorManager::GetInstance();
  // psMan->RegisterProcessorType("ExamplePS", &ExamplePS::Create, 0);
  // psMan->RegisterEventType("RawDataEvent", &CreateEV_+, 0);

}
