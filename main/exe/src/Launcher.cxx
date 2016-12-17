#include "eudaq/OptionParser.hh"
#include "eudaq/RunControl.hh"
#include "eudaq/Producer.hh"
#include "eudaq/DataCollector.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Processor.hh"

#include <iostream>

using namespace eudaq;

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ application launcher", "2.0", "The lauhcher of EUDAQ");
  eudaq::Option<std::string> name(op, "n", "name", "", "string",
				  "The eudaq application to be launched");
  eudaq::Option<std::string> listen(op, "a", "listen-port", "tcp://44001", "address",
				  "The listenning port this ");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
  				   "The address of the RunControl to connect");
  eudaq::Option<std::string> loglevel(op, "l", "log-level", "NONE", "level",
				      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> runfile(op, "u", "run-file", "../data/runnumber.dat", "string",
  				     "The file containing the run number of the last run.");  
  eudaq::Option<std::string> logfile(op, "o", "log-file", "", "filename",
  				     "A log file to load at startup");
  eudaq::Option<std::string> logpath(op, "p", "log-path", "../logs", "directory",
  				     "The path in which the log files should be stored");


  std::cout<<"parsing;\n";
  op.Parse(argv);
  std::cout<<"endl;\n";
  
  std::string app_name = name.Value();
  if(app_name.find("Producer") != std::string::npos){
    auto app=Factory<Producer>::MakeShared<const std::string&,const std::string&>
      (str2hash(name.Value()), name.Value(), rctrl.Value());
    app->Exec();
  }else if(app_name.find("DataCollector") != std::string::npos){
    auto app=Factory<DataCollector>::MakeShared<const std::string&,const std::string&>
      (str2hash(name.Value()), name.Value(), rctrl.Value());
    app->Exec();
  }else if(app_name.find("LogCollector") != std::string::npos){
    auto app=Factory<LogCollector>::MakeShared<const std::string&,const std::string&,
					       const std::string&,const int&>
      (str2hash(name.Value()), rctrl.Value(), listen.Value(), logpath.Value(),
       eudaq::Status::String2Level(loglevel.Value()));
    app->Exec();
  }else if(app_name.find("RunControl") != std::string::npos){
    auto app=Factory<RunControl>::MakeShared<const std::string&>(str2hash(name.Value()), listen.Value());
    app->Exec();
  }
  else{
    std::cout<<"unknow application"<<std::endl;
    return -1;
  }
  return 0;
}
