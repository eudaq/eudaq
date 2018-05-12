#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <cctype>

namespace eudaq{
  class CliRunControl : public RunControl{
  public:
    CliRunControl(const std::string & listenaddress);
    void DoStatus(std::shared_ptr<const ConnectionInfo> id,
		  std::shared_ptr<const Status> status) override final;
    void DoConnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void DoDisconnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void Exec() override final;

    static const uint32_t m_id_factory = eudaq::cstr2hash("CliRunControl");
  };

  namespace{
    auto dummy0 = Factory<RunControl>::
      Register<CliRunControl, const std::string&>(CliRunControl::m_id_factory);
  }

  CliRunControl::CliRunControl(const std::string & listenaddress)
    : RunControl(listenaddress){

  }

  void CliRunControl::DoStatus(std::shared_ptr<const ConnectionInfo> id,
			       std::shared_ptr<const Status> status) {
  }

  void CliRunControl::DoConnect(std::shared_ptr<const ConnectionInfo> id) {
    std::cout << "Connect:    " << *id << std::endl;
  }

  void CliRunControl::DoDisconnect(std::shared_ptr<const ConnectionInfo> id) {
    std::cout << "Disconnect: " << *id << std::endl;
  }

  void CliRunControl::Exec(){
    StartRunControl();
    bool help = true;
    while(IsActiveRunControl()){
      if (help) {
	help = false;
	std::cout << "--- Commands ---\n"
		  << "p        Print connections and status\n"
		  << "l [msg]  Send log message\n"
		  << "i [file] Initialize clients (with file 'file')\n"
		  << "c [file] Configure clients (with file 'file')\n"
		  << "r        Reset\n"
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
      case 'i':
	if(!line.empty())
	  ReadInitializeFile(line);
	Initialise();
	break;
      case 'c':
	if(!line.empty())
	  ReadConfigureFile(line);
	Configure();
	break;
      case 'r':
	Reset();
	break;
      case 'b':
	if(line.length())
	  SetRunN(std::stoul(line));
	StartRun();
	break;
      case 'e':
	StopRun();
	break;
      case 'q':
	Terminate();
	break;
      case 'p':
	{
	  auto map_conn_status= GetActiveConnectionStatusMap();
	  for(auto &conn_status: map_conn_status){
	    std::cout<<*conn_status.first<<std::endl;
	    conn_status.second->Print(std::cout);
	  }
	}
      	break;
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
