#ifndef EUDAQ_INCLUDED_TransportClient
#define EUDAQ_INCLUDED_TransportClient

#include "eudaq/TransportBase.hh"
#include "eudaq/Factory.hh"
#include <string>

namespace eudaq {
  class TransportClient;
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<TransportClient>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<TransportClient>::UP_BASE (*)(const std::string&)>&
  Factory<TransportClient>::Instance<const std::string&>();
#endif
  
  class TransportClient : public TransportBase {
  public:
    // virtual void SendPacket(const std::string & packet) = 0;
    // virtual bool ReceivePacket(std::string * packet, int timeout = -1) = 0;

    virtual ~TransportClient();
    static TransportClient* CreateClient(const std::string &name);
  };
}

#endif // EUDAQ_INCLUDED_TransportClient
