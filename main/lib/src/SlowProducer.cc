#include "eudaq/TransportClient.hh"
#include "eudaq/SlowProducer.hh"

namespace eudaq {

    SlowProducer::SlowProducer(const std::string &name, const std::string &runcontrol)
        : CommandReceiver("Producer", name, runcontrol),
          DataSender("Producer", name) {}

    void SlowProducer::OnData(const std::string &param) { Connect(param); }
}
