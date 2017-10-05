#include "eudaq/OptionParser.hh"
#include "eudaq/Monitor.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line Monitor", "2.0", "The monitor launcher of EUDAQ");
  eudaq::Option<std::string> name(op, "n", "name", "", "string",
				  "The eudaq application to be launched");  
  eudaq::Option<std::string> tname(op, "t", "tname", "", "string",
				  "Runtime name of the eudaq application");
  eudaq::Option<std::string> listen(op, "a", "listen-port", "", "address",
				  "The port on which the data monitor is going to listen on");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
  				   "The address of the RunControl to connect to");
  op.Parse(argv);
  std::string app_name = name.Value();
  if(app_name.find("Monitor") != std::string::npos){
    auto app=eudaq::Factory<eudaq::Monitor>::MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(name.Value()), tname.Value(), rctrl.Value());
    if(!app){
      std::cout<<"unknow Monitor"<<std::endl;
      return -1;
    }

    uint16_t port = static_cast<uint16_t>(eudaq::str2hash(name.Value()+tname.Value()+rctrl.Value()));
    std::string addr_listen = "tcp://"+std::to_string(port);
    if(!listen.Value().empty()){
      addr_listen = listen.Value();
    }
    app->SetServerAddress(addr_listen);
    app->Exec();
  }
  else{
    std::cout<<"unknow application"<<std::endl;
    return -1;
  }
  return 0;
}

