#ifndef EUDAQ_INCLUDED_TransportServer
#define EUDAQ_INCLUDED_TransportServer

#include "eudaq/TransportBase.hh"
#include <string>
#include <memory>

namespace eudaq {

  class DLLEXPORT TransportServer : public TransportBase {
  public:
    virtual ~TransportServer();
    virtual std::string ConnectionString() const = 0;
    virtual size_t NumConnections() const { return 0; }
    virtual std::vector<ConnectionSPC> GetConnections() const {
      std::vector<ConnectionSPC> cons;
      return cons;
    }
  };
}

#endif // EUDAQ_INCLUDED_TransportServer
