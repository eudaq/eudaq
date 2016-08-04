#include"ExamplePS.hh"

#include<iostream>
#include"RawDataEvent.hh"

using namespace eudaq;


extern template class DLLEXPORT Factory<Processor>;
extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>& Factory<Processor>::Instance<std::string&>();
extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>& Factory<Processor>::Instance<std::string&&>();


namespace{
  auto dummy0 = Factory<Processor>::Register<ExamplePS, std::string&>(eudaq::cstr2hash("ExamplePS"));
  auto dummy1 = Factory<Processor>::Register<ExamplePS, std::string&&>(eudaq::cstr2hash("ExamplePS"));
  auto dummy2 = Factory<Processor>::Register<ExamplePS, std::string&&>(0);
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
