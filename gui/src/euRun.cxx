#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "euRun.hh"
#include <iostream>
#include <QApplication>

int main(int argc, char **argv) {
  QCoreApplication *qapp = new QApplication(argc, argv );  

  eudaq::OptionParser op("EUDAQ Run Control", "2", "A Qt Launcher of the Run Control");
  eudaq::Option<std::string> sname(op, "n", "name", "RunControl", "Static Name",
				   "Static Name of RunControl");
  eudaq::Option<std::string> addr(op, "a", "listen-address", "tcp://44000", "address",
				  "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  op.Parse(argv);
  EUDAQ_LOG_LEVEL(level.Value());
  auto app=eudaq::Factory<eudaq::RunControl>::MakeUnique<const std::string&>(eudaq::str2hash(sname.Value()), addr.Value());
  RunControlGUI gui;
  gui.SetInstance(std::move(app));
  gui.Exec();
  return 0;
}

