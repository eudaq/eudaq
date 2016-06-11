#include"ExamplePS.hh"

#include<iostream>

#include"ProcessorManager.hh"


using namespace eudaq;

void ExamplePS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<std::endl;
  
}


void ExamplePS::ProcessCmdEvent(EVUP ev){
  
  
}


Processor* ExamplePS::Create(uint32_t psid, uint32_t psid_mother){
  Processor *ps;
  ps = new ExamplePS(psid, psid_mother); 
  return ps;
}
