#ifndef EUDAQ_INCLUDED_TransportServer
#define EUDAQ_INCLUDED_TransportServer

#include "eudaq/TransportBase.hh"
#include <string>
#include <memory>

namespace eudaq {

  class DLLEXPORT TransportServer : public TransportBase {
  public:
    ~TransportServer() override;
    virtual std::string ConnectionString() const = 0;
    virtual std::vector<ConnectionSPC> GetConnections() const = 0;
  };
}

#endif // EUDAQ_INCLUDED_TransportServer
