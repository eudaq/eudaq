#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

class PyRunControl : public eudaq::RunControl {
  public:
    PyRunControl(const std::string & listenaddress)
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

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  PyRunControl* PyRunControl_new(char *listenaddress){return new PyRunControl(std::string(listenaddress));}
  void PyRunControl_GetStatus(PyRunControl *prc){prc->GetStatus();}
  void PyRunControl_StartRun(PyRunControl *prc){prc->StartRun();}
  void PyRunControl_StopRun(PyRunControl *prc){prc->StopRun();}
  void PyRunControl_Configure(PyRunControl *prc, char *cfg){prc->Configure(std::string(cfg));}
  size_t PyRunControl_NumConnections(PyRunControl *prc){prc->NumConnections();}
}
