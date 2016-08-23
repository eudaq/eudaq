#include"ExamplePS.hh"

#include<iostream>
#include"RawDataEvent.hh"

using namespace eudaq;

namespace{
  auto dummy0 = Factory<Processor>::Register<ExamplePS, std::string&>(eudaq::cstr2hash("ExamplePS"));
  auto dummy1 = Factory<Processor>::Register<ExamplePS, std::string&&>(eudaq::cstr2hash("ExamplePS"));
}

ExamplePS::ExamplePS(std::string cmd)
  :Processor("ExamplePS", ""){
  *this<<cmd; 
}


void ExamplePS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  ForwardEvent(std::move(ev));
}


void ExamplePS::ProduceEvent(){
  // EVUP ev = EventClassFactory::Create("RawDataEvent");
  for(int i =0; i<10; i++){
    EVUP ev(new RawDataEvent("data", 0, i));
    Processing(std::move(ev));
  }
}
