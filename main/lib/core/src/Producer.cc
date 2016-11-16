#include "eudaq/TransportClient.hh"
#include "eudaq/Producer.hh"

namespace eudaq {

  Producer::Producer(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Producer", name, runcontrol), m_name(name){
  }

  void Producer::OnData(const std::string &param){
    //TODO: decode param
    Connect(param);
  }

  void Producer::Connect(const std::string & server){
    auto it = m_senders.find(server);
    if(it==m_senders.end()){
      std::unique_ptr<DataSender> sender(new DataSender("Producer", m_name));
      m_senders[server]= std::move(sender);
    }
    m_senders[server]->Connect(server);
  }

  void Producer::SendEvent(const Event &ev){
    for(auto &e: m_senders){
      e.second->SendEvent(ev);
    }
  }

}
