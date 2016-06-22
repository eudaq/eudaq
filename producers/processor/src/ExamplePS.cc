#include"ExamplePS.hh"

#include<iostream>
#include"RawDataEvent.hh"

using namespace eudaq;

namespace{
  static RegisterDerived<Processor, typename std::string, ExamplePS, uint32_t> reg_EXAMPLEPS("ExamplePS");
}

void ExamplePS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  ForwardEvent(std::move(ev));
}


void ExamplePS::ProcessCmdEvent(EVUP ev){
  
  
}

void ExamplePS::ProduceEvent(){
  // EVUP ev = EventClassFactory::Create("RawDataEvent");
  for(int i =0; i<10; i++){
    EVUP ev(new RawDataEvent("data", 0, i));
    Processing(std::move(ev));
  }
}



Processor* ExamplePS::Create(uint32_t psid){
  Processor *ps;
  ps = new ExamplePS(psid); 
  return ps;
}
