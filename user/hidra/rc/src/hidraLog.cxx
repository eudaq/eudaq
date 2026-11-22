#include "HidraLogCollectorGUI.hh"

#include "eudaq/OptionParser.hh"

#include <QApplication>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
  std::unique_ptr<QCoreApplication> qapp(new QApplication(argc, argv));

  eudaq::OptionParser op("HiDRa EUDAQ Log Collector", "2.0",
                         "A HiDRa Qt log collector with configurable output "
                         "thresholds");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> listen(op, "a", "listen-address", "", "address",
                                    "The address on which to listen for log "
                                    "connections");

  try {
    op.Parse(argv);
  } catch (...) {
    std::ostringstream err;
    return op.HandleMainException(err);
  }

  HidraLogCollectorGUI app("log", rctrl.Value());
  if (!listen.Value().empty()) {
    app.SetServerAddress(listen.Value());
  }
  app.Exec();

  return 0;
}
