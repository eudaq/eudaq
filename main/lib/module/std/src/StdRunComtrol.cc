#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <cctype>

namespace eudaq{
  class StdRunControl : publicRunControl {
  public:
    StdRunControl(const std::string & listenaddress);
    void OnReceive(const ConnectionInfo & id, std::shared_ptr<eudaq::Status> status) override final;
    void OnConnect(const ConnectionInfo & id) override final;
    void OnDisconnect(const ConnectionInfo & id) override final;
    void Exec() override final;

    static const uint32_t m_id_factory = eudaq::cstr2hash("StdRunControl");
  };

  namespace{
    auto dummy0 = eudaq::Factory<eudaq::RunControl>::
      Register<StdRunControl, const std::string&>(StdRunControl::m_id_factory);
  }

  StdRunControl::StdRunControl(const std::string & listenaddress)
    : eudaq::RunControl(listenaddress){

  }

  void StdRunControl::OnReceive(const eudaq::ConnectionInfo & id, std::shared_ptr<eudaq::Status> status) {
    std::cout << "Receive:    " << *status << " from " << id << std::endl;
  }

  void StdRunControl::OnConnect(const eudaq::ConnectionInfo & id) {
    std::cout << "Connect:    " << id << std::endl;
  }

  void StdRunControl::OnDisconnect(const eudaq::ConnectionInfo & id) {
    std::cout << "Disconnect: " << id << std::endl;
  }


  void StdRunControl::Exec(){
    bool done = false;
    bool help = true;
    do {
      if (help) {
	help = false;
	std::cout << "--- Commands ---\n"
		  << "p        Print connections\n"
		  << "l [msg]  Send log message\n"
	  // << "f [file] Configure clients (with file 'file')\n"
		  << "c [cnf]  Configure clients (with configuration 'cnf')\n"
		  << "r        Reset\n"
		  << "s        Status\n"
		  << "b [msg]  Begin Run (with run comment 'msg')\n"
		  << "e        End Run\n"
		  << "x        Terminate clients\n"
		  << "q        Quit\n"
		  << "?        Display this help\n"
		  << "----------------" << std::endl;
      }
      std::string line;
      std::getline(std::cin, line);
      char cmd = '\0';
      if (line.length() > 0) {
	cmd = tolower(line[0]);
	line = eudaq::trim(std::string(line, 1));
      }
      else {
	line = "";
      }
      switch (cmd) {
      case '\0': // ignore
	break;
      case 'l':
	EUDAQ_USER(line);
	break;
	// case 'f':
	//   break;
      case 'c':
	Configure(line);
	break;
      case 'r':
	Reset();
	break;
      case 's':
	GetStatus();
	break;
      case 'b':
	StartRun(line);
	break;
      case 'e':
	StopRun();
	break;
      case 'x':
	Terminate();
	break;
      case 'q':
	Terminate();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	done = true;
	break;
      case 'p':
	std::cout << "Connections (" << NumConnections() << ")" << std::endl;
	for (unsigned i = 0; i < NumConnections(); ++i) {
	  std::cout << "  " << GetConnection(i) << std::endl;
	}
	break;
      case '?':
	help = true;
	break;
      default:
	std::cout << "Unrecognized command, type ? for help" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (!done);
  }
}
