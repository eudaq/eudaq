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
  eudaq::Option<std::string> addr(op, "a", "listen-address", "tcp://44002", "address",
				  "The address on which to listen for Log connections");
  eudaq::Option<std::string> directory (op, "d", "directory", ".", "directory",
					"The path in which the log files should be stored");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
				   "The initial level for displaying log messages");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::LogCollector>::MakeShared<const std::string&, const std::string&,
							     const std::string&, const int&>
      (eudaq::cstr2hash("GuiLogCollector"), rctrl.Value(), addr.Value(), directory.Value(),
       eudaq::Status::String2Level(level.Value()));
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
