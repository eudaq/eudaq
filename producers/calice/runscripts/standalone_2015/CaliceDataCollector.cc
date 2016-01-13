#include <ostream>
#include <cctype>

#include "CaliceDataCollector.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/BufferSerializer.hh"
#ifndef WIN32
#include <unistd.h>
#endif

// socket headers
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

CaliceDataCollector * g_dc = 0;
// void ctrlc_handler(int) {
//   if (g_ptr) g_ptr->done = 1;
// }

void CaliceDataCollector::OnConfigure(const eudaq::Configuration & param) {
  std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
  DataCollector::OnConfigure(param);
  
  _listenPort = param.Get("ListenPort", 8888);

  _thread = new std::thread(CaliceDataCollector::AcceptThreadEntry, this);
  
  std::cout << "...Configured (" << param.Name() << ")" << std::endl;
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}

void CaliceDataCollector::SendEvent(const eudaq::DetectorEvent &ev)
{
  eudaq::BufferSerializer ser;
  ev.Serialize(ser);
  
  for(unsigned int i=0;i<_connectfd.size();i++){
    std::cout << "CaliceDataCollector::SendEvent: send header to fd " << _connectfd[i] << std::endl;
    int ret = write(_connectfd[i], "BEGINEVENT", strlen("BEGINEVENT"));
    if(ret == -1){
      std::cout << "CaliceDataCollector::SendEvent: connection closed." << std::endl;
      _connectfd.erase(_connectfd.begin()+i);
      i -= 1;
      continue;
    }
    std::cout << "CaliceDataCollector::SendEvent: send " << ser.size() << " bytes to fd " << _connectfd[i] << std::endl;
    int nwrite = write(_connectfd[i],&ser[0], ser.size()); 
    std::cout << nwrite << " bytes written." << std::endl;
    std::cout << "CaliceDataCollector::SendEvent: send footer to fd " << _connectfd[i] << std::endl;
    write(_connectfd[i], "ENDEVENT", strlen("ENDEVENT"));
  }
}

void CaliceDataCollector::AcceptThread()
{
  // socket initialization
  _serverfd = socket(AF_INET, SOCK_STREAM, 0);
  
  // bind socket
  struct sockaddr_in srvAddr, dstAddr;
  socklen_t sizAddr = sizeof(dstAddr);
  memset(&srvAddr, 0, sizeof(srvAddr));
  memset(&dstAddr, 0, sizeof(dstAddr));
  srvAddr.sin_port = htons(_listenPort);
  srvAddr.sin_family = AF_INET;
  srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(_serverfd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
  
  // listen
  listen(_serverfd, 1);
  
  while(!done){
    int dstFd = accept(_serverfd, (struct sockaddr *)&dstAddr, &sizAddr);
    if(dstFd == -1)break;
    
    std::cout << "Connect fd " << dstFd << std::endl;
    _connectfd.push_back(dstFd);
  }
  
}

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Data Collector", "1.0", "A command-line version of the Data Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44001", "address",
      "The address on which to listen for Data connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "CaliceDataCollector", "string",
      "The name of this DataCollector");
  eudaq::Option<std::string> runnumberfile (op, "f", "runnumberfile", "../data/runnumber.dat", "string",
      "The path and name of the file containing the run number of the last run.");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    CaliceDataCollector fw(name.Value(), rctrl.Value(), addr.Value(), runnumberfile.Value());
    g_dc = &fw;
    //std::signal(SIGINT, &ctrlc_handler);
    do {
      eudaq::mSleep(10);
    } while (!fw.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    std::cout << "DataCollector exception handler" << std::endl;
    return op.HandleMainException();
  }
  return 0;
}
