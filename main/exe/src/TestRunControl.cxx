#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <istream>
#include <cctype>
#include <vector>
#ifndef WIN32
#include <unistd.h>
#endif



class TestRunControl : public eudaq::RunControl {
  public:
    TestRunControl(const std::string & listenaddress)
      : eudaq::RunControl(listenaddress)
    {}
    void OnReceive(const eudaq::ConnectionInfo & id, std::shared_ptr<eudaq::ConnectionState> connectionstate) {
      std::cout << "Receive:    " << *connectionstate << " from " << id << std::endl;
    }
    void OnConnect(const eudaq::ConnectionInfo & id) {
      std::cout << "Connect:    " << id << std::endl;
    }
    void OnDisconnect(const eudaq::ConnectionInfo & id) {
      std::cout << "Disconnect: " << id << std::endl;
    }
};

struct rcHandler
{
  rcHandler(TestRunControl *rc, const std::vector<std::string>& conf, uint64_t duration, uint64_t runs) :m_rc(rc), m_conf(conf), m_duration(duration), m_runs(runs){}
  TestRunControl *m_rc;
  std::vector<std::string> m_conf;
  uint64_t m_duration, m_runs;
};

void * worker_automatic_run(void * arg) {
  std::cout << "starting worker " << std::endl;
  
  auto rcH = static_cast<rcHandler *>(arg);
  TestRunControl * rc = rcH->m_rc;
      auto duration=rcH->m_duration;
      auto runs = rcH->m_runs;
 
      for (auto&conf : rcH->m_conf)
      {

        rc->Configure(conf);

        eudaq::mSleep(10000);

        for (uint64_t i = 0; i < runs; ++i){
          std::cout << "starting run " << i << " of " << runs << std::endl;


          rc->StartRun("");
          std::cout << "sleep for " << duration << std::endl;
          eudaq::mSleep(duration);

          rc->StopRun();

        }

      }

  std::cout << "stopping thread" << std::endl;
  rc->Terminate();
  eudaq::mSleep(5000);
  exit(0);
  return 0;
}

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Run Control", "1.0", "A command-line version of the Run Control");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44000", "address",
      "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level",      "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::OptionFlag automatic(op, "s", "scripted", "runs the test run control in automatic mode");
  eudaq::Option<std::string> configName(op, "c", "cnf", "", "configuration",
    "determines which configuration gets loaded automatically");
  eudaq::Option<uint64_t> duration(op, "d", "duration", 60, "uint64_t", "determines the length of a run in seconds");
  eudaq::Option<uint64_t> runs(op, "r", "runs", 1, "uint64_t", "how many runs you want to take with this configuration");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    TestRunControl rc(addr.Value());
    auto conf = eudaq::split(configName.Value(), ",");
    rcHandler rcH(&rc, conf, duration.Value()*1000, runs.Value());
    bool done = false;
    bool help = true;

    if (automatic.IsSet())
    {
      
      eudaq::mSleep(10000); //30 seconds sleep to give the producer time to startup
 
     
      

      std::thread m(worker_automatic_run,&rcH);
      m.detach();
    }


    do {
      if (help) {
        help = false;
        std::cout << "--- Commands ---\n"
          << "p       Print connections\n"
          << "l [msg] Send log message\n"
          << "c [cnf] Configure clients (with configuration 'cnf')\n"
          << "r       Reset\n"
          << "s       Connection State\n"
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
          rc.GetConnectionState();
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
