#include "eudaq/OptionParser.hh"
#include "eudaq/Status.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/LogCollector.hh"
#include <QApplication>

namespace{
  QCoreApplication* GetQApplication(){
    QCoreApplication* qapp = QApplication::instance();
    if(!qapp){
      int argc = 1;
      char *argv[] = {(char*)"euGUI"};
      qapp = new QApplication(argc, argv );  
    }
    return qapp;
  }
  auto qapp = GetQApplication();
}


int main(int argc, char **argv) {
  eudaq::OptionParser op("EUDAQ Log Collector", "1.0",
                         "A Qt version of the Log Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> listen(op, "a", "listen-address", "", "address",
				    "The address on which to listen for Log connections");
  try {
    op.Parse(argv);
    // EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::LogCollector>::
      MakeShared<const std::string&, const std::string&>
      (eudaq::cstr2hash("GuiLogCollector"), "log", rctrl.Value());
    uint16_t port = static_cast<uint16_t>(eudaq::str2hash("log"+rctrl.Value()));
    std::string addr_listen = "tcp://"+std::to_string(port);
    if(!listen.Value().empty()){
      addr_listen = listen.Value();
    }
    app->SetServerAddress(addr_listen);
    app->Exec();
  } catch (...) {
    std::cout << "euLog exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    std::cerr<<"Exception"<< err.str()<<"\n";
    return result;
  }
  return 0;
}
