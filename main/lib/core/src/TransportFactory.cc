#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include <iostream>
#include <ostream>
#include <string>
#include <map>

namespace eudaq {

  TransportServer *TransportFactory::CreateServer(const std::string &name) {
    std::string proto = "tcp", param = name;
    size_t i = name.find("://");
    if (i != std::string::npos) {
      proto = std::string(name, 0, i);
      param = std::string(name, i + 3);
    }

    auto server = Factory<TransportServer>::MakeUnique<const std::string&>(str2hash(proto), param);
    if(!server)
      EUDAQ_THROW("Unknown protocol: " + proto);
    return server.release();
  }

  TransportClient *TransportFactory::CreateClient(const std::string &name) {
    std::string proto = "tcp", param = name;
    size_t i = name.find("://");
    if (i != std::string::npos) {
      proto = std::string(name, 0, i);
      param = std::string(name, i + 3);
    }
    auto client = Factory<TransportClient>::MakeUnique<const std::string&>(str2hash(proto), param);
    if(!client)
      EUDAQ_THROW("Unknown protocol: " + proto);
    return client.release();
  }
  
}
