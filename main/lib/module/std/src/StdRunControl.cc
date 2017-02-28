#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <cctype>

namespace eudaq{
  class StdRunControl : public RunControl{
  public:
    StdRunControl(const std::string & listenaddress);
    void DoStatus(std::shared_ptr<const ConnectionInfo> id,
		  std::shared_ptr<const Status> status) override final;
    void DoConnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void DoDisconnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void Exec() override final;

    static const uint32_t m_id_factory = eudaq::cstr2hash("StdRunControl");
  };

  namespace{
    auto dummy0 = Factory<RunControl>::
      Register<StdRunControl, const std::string&>(StdRunControl::m_id_factory);
  }

  StdRunControl::StdRunControl(const std::string & listenaddress)
    : RunControl(listenaddress){

  }

  void StdRunControl::DoStatus(std::shared_ptr<const ConnectionInfo> id,
			       std::shared_ptr<const Status> status) {
    std::cout << "Receive:    " << *status << " from " << *id << std::endl;
  }

  void StdRunControl::DoConnect(std::shared_ptr<const ConnectionInfo> id) {
    std::cout << "Connect:    " << *id << std::endl;
  }

  void StdRunControl::DoDisconnect(std::shared_ptr<const ConnectionInfo> id) {
    std::cout << "Disconnect: " << *id << std::endl;
  }

  void StdRunControl::Exec(){
    StartRunControl();
    bool help = true;
    while(IsActiveRunControl()){
      if (help) {
	help = false;
	std::cout << "--- Commands ---\n"
		  << "p        Print connections\n"
		  << "l [msg]  Send log message\n"
		  << "f [file] Configure clients (with file 'file')\n"
		  << "r        Reset\n"
		  << "s        Status\n"
		  << "b [n]    Begin Run (with run number)\n"
		  << "e        End Run\n"
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
      case 'f':
	if(!line.empty())
	  ReadConfigureFile(line);
	Configure();
	break;
      case 'r':
	Reset();
	break;
      // case 's':
      // 	RemoteStatus();
      // 	break;
      case 'b':
	if(line.length())
	  SetRunNumber(std::stoul(line));
	StartRun();
	break;
      case 'e':
	StopRun();
	break;
      case 'q':
	Terminate();
	break;
      // case 'p':
      // 	std::cout << "Connections (" << NumConnections() << ")" << std::endl;
      // 	for (unsigned i = 0; i < NumConnections(); ++i) {
      // 	  std::cout << "  " << GetConnection(i) << std::endl;
      // 	}
      // 	break;
      case '?':
	help = true;
	break;
      default:
	std::cout << "Unrecognized command, type ? for help" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}
