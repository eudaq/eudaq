#include "eudaq/OptionParser.hh"
#include "eudaq/RunControl.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line RunControl", "2.0", "The RunControl lauhcher of EUDAQ");
  eudaq::Option<std::string> name(op, "n", "name", "", "string",
				  "The eudaq application to be launched");
  eudaq::Option<std::string> listen(op, "a", "listen-port", "tcp://44000", "address",
				    "The listenning port this RunControl");
  op.Parse(argv);
  std::string app_name = name.Value();
  if(app_name.find("RunControl") != std::string::npos){
    auto app=eudaq::Factory<eudaq::RunControl>::
      MakeShared<const std::string&>(eudaq::str2hash(name.Value()), listen.Value());
    if(!app){
      std::cout<<"unknow RunControl"<<std::endl;
      return -1;
    }
    app->Exec();
  }
  else{
    std::cout<<"unknow application"<<std::endl;
    return -1;
  }
  return 0;
}
