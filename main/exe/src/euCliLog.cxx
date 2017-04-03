#include "eudaq/OptionParser.hh"
#include "eudaq/LogCollector.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line LogCollector", "2.0", "The LogCollcector lauhcher of EUDAQ");
  eudaq::Option<std::string> name(op, "n", "name", "", "string",
				  "The eudaq application to be launched");  
  eudaq::Option<std::string> listen(op, "a", "listen-port", "tcp://44001", "address",
				  "The listenning port this ");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
  				   "The address of the RunControl to connect");
  eudaq::Option<std::string> logpath(op, "p", "log-path", "./", "directory",
  				     "The path in which the log files should be stored");
  eudaq::Option<std::string> loglevel(op, "l", "log-level", "NONE", "level",
				      "The minimum level for displaying log messages locally");
  op.Parse(argv);
  std::string app_name = name.Value();
  if(app_name.find("LogCollector") != std::string::npos){
    auto app=eudaq::Factory<eudaq::LogCollector>::MakeShared<const std::string&,const std::string&,
							     const std::string&,const int&>
      (eudaq::str2hash(name.Value()), rctrl.Value(), listen.Value(), logpath.Value(),
       eudaq::Status::String2Level(loglevel.Value()));
    if(!app){
      std::cout<<"unknow LogCollector"<<std::endl;
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
