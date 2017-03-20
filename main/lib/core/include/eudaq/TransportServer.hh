#ifndef EUDAQ_INCLUDED_TransportServer
#define EUDAQ_INCLUDED_TransportServer

#include "eudaq/TransportBase.hh"
#include "eudaq/Factory.hh"
#include <string>
#include <memory>

namespace eudaq {
  class TransportServer;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<TransportServer>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<TransportServer>::UP_BASE (*)(const std::string&)>&
  Factory<TransportServer>::Instance<const std::string&>();
#endif
  
  class DLLEXPORT TransportServer : public TransportBase {
  public:
    ~TransportServer() override;
    virtual std::string ConnectionString() const = 0;
    virtual std::vector<ConnectionSPC> GetConnections() const = 0;
    static TransportServer* CreateServer(const std::string &name);
  };
}

#endif // EUDAQ_INCLUDED_TransportServer
