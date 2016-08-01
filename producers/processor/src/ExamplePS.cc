#include"ExamplePS.hh"

#include<iostream>
#include"RawDataEvent.hh"
#include "Factory.hh"

using namespace eudaq;

namespace{
  static auto dummy_val = Factory<Processor>::Register<ExamplePS, std::string&>(eudaq::cstr2hash("ExamplePS"));
}

ExamplePS::ExamplePS(std::string cmd)
  :Processor("ExamplePS", ""){
  *this<<cmd; 
}

ExamplePS::ExamplePS(uint32_t psid, std::string cmd)
  :Processor("ExamplePS", psid, ""){
  *this<<cmd;
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
