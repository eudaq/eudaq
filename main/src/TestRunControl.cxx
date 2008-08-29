#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <istream>
#include <cctype>
#include <unistd.h>

class TestRunControl : public eudaq::RunControl {
public:
  TestRunControl(const std::string & listenaddress)
    : eudaq::RunControl(listenaddress)
  {}
  void OnReceive(const eudaq::ConnectionInfo & id, counted_ptr<eudaq::Status> status) {
    std::cout << "Receive:    " << *status << " from " << id << std::endl;
  }
  void OnConnect(const eudaq::ConnectionInfo & id) {
    std::cout << "Connect:    " << id << std::endl;
  }
  void OnDisconnect(const eudaq::ConnectionInfo & id) {
    std::cout << "Disconnect: " << id << std::endl;
  }
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Run Control", "1.0", "A command-line version of the Run Control");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44000", "address",
                                   "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level",      "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    TestRunControl rc(addr.Value());
    bool done = false;
    bool help = true;
    do {
      if (help) {
        help = false;
        std::cout << "--- Commands ---\n"
                  << "p       Print connections\n"
                  << "l [msg] Send log message\n"
                  << "c [cnf] Configure clients (with configuration 'cnf')\n"
                  << "r       Reset\n"
                  << "s       Status\n"
                  << "b [msg] Begin Run (with run comment 'msg')\n"
                  << "e       End Run\n"
                  << "x       Terminate clients\n"
                  << "q       Quit\n"
                  << "?       Display this help\n"
                  << "----------------" << std::endl;
      }
      std::string line;
      std::getline(std::cin, line);
      //std::cout << "Line=\'" << line << "\'" << std::endl;
      char cmd = '\0';
      if (line.length() > 0) {
        cmd = std::tolower(line[0]);
        line = eudaq::trim(std::string(line, 1));
      } else {
        line = "";
      }
      switch (cmd) {
      case '\0': // ignore
        break;
      case 'l':
        EUDAQ_USER(line);
        break;
      case 'c':
        rc.Configure(line);
        break;
      case 'r':
        rc.Reset();
        break;
      case 's':
        rc.GetStatus();
        break;
      case 'b':
        rc.StartRun(line);
        break;
      case 'e':
        rc.StopRun();
        break;
      case 'x':
        rc.Terminate();
        break;
      case 'q':
        done = true;
        break;
      case 'p':
        std::cout << "Connections (" << rc.NumConnections() << ")" << std::endl;
        for (unsigned i = 0; i < rc.NumConnections(); ++i) {
          std::cout << "  " << rc.GetConnection(i) << std::endl;
        }
        break;
      case '?':
        help = true;
        break;
      default:
        std::cout << "Unrecognised command, type ? for help" << std::endl;
      }
      eudaq::mSleep(10);
    } while (!done);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
