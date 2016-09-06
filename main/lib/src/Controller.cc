#include "eudaq/TransportClient.hh"
#include "eudaq/Controller.hh"

namespace eudaq {

  Controller::Controller(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Controller", name, runcontrol) {}
  
}
