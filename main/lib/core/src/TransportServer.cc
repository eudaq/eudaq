#include "eudaq/TransportServer.hh"

namespace eudaq {
  template class DLLEXPORT Factory<TransportServer>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<TransportServer>::UP_BASE (*)(const std::string&)>&
  Factory<TransportServer>::Instance<const std::string&>();
  
  TransportServer::~TransportServer() {}

  TransportServer *TransportServer::CreateServer(const std::string &name) {
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
}
