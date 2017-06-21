#include "eudaq/OptionParser.hh"
#include "eudaq/LogCollector.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line LogCollector", "2.0", "The LogCollcector launcher of EUDAQ");
  eudaq::Option<std::string> name(op, "n", "name", "FileLogCollector", "string",
				  "The eudaq application to be launched");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address",
  				   "The address of the RunControl to connect to");
  eudaq::Option<std::string> listen(op, "a", "listen-address", "", "address",
				    "The address on which to listen for log connections");
  try{
    op.Parse(argv);
    auto app=eudaq::Factory<eudaq::LogCollector>::
      MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(name.Value()), "Log" ,rctrl.Value());
    if(!app){
      std::cout<<"unknown LogCollector"<<std::endl;
      return -1;
    }
    uint16_t port = static_cast<uint16_t>(eudaq::str2hash(name.Value()+"Log"+rctrl.Value()));
    std::string addr_listen = "tcp://"+std::to_string(port);
    if(!listen.Value().empty()){
      addr_listen = listen.Value();
    }
    app->SetServerAddress(addr_listen);
    app->Exec();
  }
  catch(...){
    std::cout << "euLog exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    std::cerr<<"Exception"<< err.str()<<"\n";
    return result;
  }
  return 0;
}
