#include"FileReader.hh"
#include"Processor.hh"

#include <chrono>
#include <thread>
#include <iostream>


using namespace eudaq;

int main(int argn, char **argc){
  
  auto ptel = Processor::MakeShared("TelProducerPS", {{"SYS:PSID", "1"}});
  auto ptlu = Processor::MakeShared("TluProducerPS", {{"SYS:PSID", "2"}});

 
  
  // pds0<<"SYS:PD:RUN"<<"SYS:PD:DETACH";

  std::cout << "press any key to exit...\n"; getchar();
  // prd0<<"EMPTY";
  std::cout << "press any key to exit...\n"; getchar();
  return 0;
}
