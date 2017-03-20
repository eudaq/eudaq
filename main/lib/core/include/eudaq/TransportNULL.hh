#ifndef EUDAQ_INCLUDED_TransportNULL
#define EUDAQ_INCLUDED_TransportNULL

#include "eudaq/TransportServer.hh"
#include "eudaq/TransportClient.hh"

#include <vector>
#include <string>
#include <map>

namespace eudaq {

  class NULLServer : public TransportServer {
  public:
    NULLServer(const std::string &param);
    ~NULLServer() override;

    void Close(const ConnectionInfo &id) override;
    void SendPacket(const unsigned char *data, size_t len,
		    const ConnectionInfo &id = ConnectionInfo::ALL,
		    bool duringconnect = false) override;
    void ProcessEvents(int timeout) override;
    std::vector<ConnectionSPC> GetConnections() const override;
    std::string ConnectionString() const override;
    bool IsNull() const override{ return true; }
    static const std::string name;
  };

  class NULLClient : public TransportClient {
  public:
    NULLClient(const std::string &param);
    virtual ~NULLClient();

    virtual void SendPacket(const unsigned char *data, size_t len,
                            const ConnectionInfo &id = ConnectionInfo::ALL,
                            bool = false);
    virtual void ProcessEvents(int timeout = -1);
    virtual bool IsNull() const { return true; }

  private:
    ConnectionInfo m_buf;
  };
}

#endif // EUDAQ_INCLUDED_TransportNULL
