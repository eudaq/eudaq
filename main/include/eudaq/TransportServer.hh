#ifndef EUDAQ_INCLUDED_TransportServer
#define EUDAQ_INCLUDED_TransportServer

#include "eudaq/TransportBase.hh"
#include <string>
#include <memory>

namespace eudaq {

  class TransportServer : public TransportBase {
    public:
      virtual ~TransportServer();
      virtual std::string ConnectionString() const = 0;
      size_t NumConnections() const { return m_conn.size(); }
      const ConnectionInfo & GetConnection(size_t i) const { return *m_conn[i]; }
    protected:
      std::vector<std::shared_ptr<ConnectionInfo> > m_conn;
  };

}

#endif // EUDAQ_INCLUDED_TransportServer
