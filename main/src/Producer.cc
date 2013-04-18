#include "eudaq/Producer.hh"

namespace eudaq {

  Producer::Producer(const std::string & name, const std::string & runcontrol)
    : CommandReceiver("Producer", name, runcontrol),
    DataSender("Producer", name)
  {
  }

  void Producer::OnData(const std::string & param) {
    Connect(param);
  }
}
