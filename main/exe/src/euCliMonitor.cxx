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
  				   "The address of the RunControl to connect to (using null://null to run it standalone without RunControl)");


  try{
    op.Parse(argv);
  }
  catch(...){
    std::ostringstream err;
    return op.HandleMainException(err);
  }
  std::string app_name = name.Value();
  if(app_name.find("Monitor") == std::string::npos){
    std::cout<<"unknown application"<<std::endl;
    return -1;
  }
  auto app=eudaq::Monitor::Make(name.Value(), tname.Value(), rctrl.Value());
  if(!app){
    std::cout<<"unknown Monitor: "<<name.Value()<<std::endl;
    return -1;
  }
  if(!listen.Value().empty()){
    app->SetServerAddress(listen.Value());
  }
  try{
    app->Connect();
  }
  catch (...){
    std::cout<<"Can not connect to RunControl at "<<rctrl.Value()<<std::endl;
    return -1;
  }
  while(app->IsConnected()){
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}

