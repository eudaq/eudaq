#include "eudaq/TransportClient.hh"

namespace eudaq {
  template class DLLEXPORT Factory<TransportClient>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<TransportClient>::UP_BASE (*)(const std::string&)>&
  Factory<TransportClient>::Instance<const std::string&>();
  
  TransportClient::~TransportClient() {}

  TransportClient *TransportClient::CreateClient(const std::string &name) {
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
