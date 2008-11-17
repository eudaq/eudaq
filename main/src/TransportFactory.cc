#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include <iostream>
#include <ostream>
#include <string>
#include <map>

namespace eudaq {

  namespace {

    typedef std::map<std::string, TransportInfo> map_t;

    static map_t & TransportMap() {
      static map_t m;
      return m;
    }
  }

  void TransportFactory::Register(const TransportInfo & info) {
    //std::cout << "DEBUG: TransportFactory::Register " << info.name << std::endl;
    TransportMap()[info.name] = info;
  }

  TransportServer * TransportFactory::CreateServer(const std::string & name) {
    std::string proto = "tcp", param = name;
    size_t i = name.find("://");
    if (i != std::string::npos) {
      proto = std::string(name, 0, i);
      param = std::string(name, i+3);
    }
    map_t::const_iterator it = TransportMap().find(proto);
    if (it == TransportMap().end()) EUDAQ_THROW("Unknown protocol: " + proto);
    return (it->second.serverfactory)(param);
  }

  TransportClient * TransportFactory::CreateClient(const std::string & name) {
    std::string proto = "tcp", param = name;
    size_t i = name.find("://");
    if (i != std::string::npos) {
      proto = std::string(name, 0, i);
      param = std::string(name, i+3);
    }
    map_t::const_iterator it = TransportMap().find(proto);
    if (it == TransportMap().end()) EUDAQ_THROW("Unknown protocol: " + proto);
    return (it->second.clientfactory)(param);
  }

}
