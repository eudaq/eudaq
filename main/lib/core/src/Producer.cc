#include "eudaq/TransportClient.hh"
#include "eudaq/Producer.hh"

namespace eudaq {

  Producer::Producer(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Producer", name, runcontrol), m_name(name){
  }

  void Producer::OnData(const std::string &param) {
    //TODO: decode param
    Connect(param);
  }

  void Producer::Connect(const std::string & server){
    std::unique_ptr<DataSender> sender(new DataSender("Producer", m_name));
    if(sender){
      sender->Connect(server);
      m_senders.push_back(std::move(sender));
    }
  }

  void Producer::SendEvent(const Event &ev){
    for(auto &e: m_senders){
      e->SendEvent(ev);
    }
  }

}
