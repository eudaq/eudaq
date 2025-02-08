#ifndef EUDAQ_INCLUDED_TransportTCP
#define EUDAQ_INCLUDED_TransportTCP

#include "eudaq/TransportServer.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/Platform.hh"

#if EUDAQ_PLATFORM_IS(WIN32) || EUDAQ_PLATFORM_IS(MINGW)
#ifndef __CINT__
#define NOMINMAX
#include <winsock.h> // using winsock2.h here would cause conflicts when including Windows4Root.h
                     // required e.g. by the ROOT online monitor
#endif
#else
#include <sys/select.h>
typedef int SOCKET;
#endif

#include <vector>
#include <string>
#include <map>

namespace eudaq {
  class ConnectionInfoTCP : public ConnectionInfo {
  public:
    ConnectionInfoTCP() = delete;
    ConnectionInfoTCP(const ConnectionInfoTCP&) = delete;
    ConnectionInfoTCP& operator = (const ConnectionInfoTCP&) = delete;   
    ConnectionInfoTCP(SOCKET fd, const std::string &host = "")
      : m_fd(fd), m_host(host), m_len(0), m_buf(""), ConnectionInfo("") {}
    void append(size_t length, const char *data);
    bool havepacket() const;
    std::string getpacket();
    SOCKET GetFd() const { return m_fd; }
    bool Matches(const ConnectionInfo &other) const override;
    void Print(std::ostream &, size_t) const override;
    std::string GetRemote() const override { return m_host; }

  private:
    void update_length(bool = false);
    SOCKET m_fd;
    std::string m_host;
    size_t m_len;
    std::string m_buf;
  };
  
  class TCPServer : public TransportServer {
  public:
    TCPServer(const std::string &param);
    ~TCPServer() override;
    void Close(const ConnectionInfo &id) override;
    void SendPacket(const unsigned char *data, size_t len,
		    const ConnectionInfo &id = ConnectionInfo::ALL,
		    bool duringconnect = false) override;
    void ProcessEvents(int timeout) override;
    std::string ConnectionString() const override;
    std::vector<ConnectionSPC> GetConnections() const  override;
    static const std::string name;
  private:
    std::vector<std::shared_ptr<ConnectionInfoTCP>> m_conn;
    std::mutex m_mtx_conn;
    
    int m_port;
    SOCKET m_srvsock;
    SOCKET m_maxfd;
    fd_set m_fdset;

    std::shared_ptr<ConnectionInfoTCP> GetInfo(SOCKET fd) const;
  };

  class TCPClient : public TransportClient {
  public:
    TCPClient(const std::string &param);
    virtual ~TCPClient();

    virtual void SendPacket(const unsigned char *data, size_t len,
                            const ConnectionInfo &id = ConnectionInfo::ALL,
                            bool = false);
    virtual void ProcessEvents(int timeout = -1);
    static const std::string name;
  private:
    void OpenConnection();
    std::string m_server;
    int m_port;
    SOCKET m_sock;
    std::shared_ptr<ConnectionInfoTCP> m_buf;
  };
}

#endif // EUDAQ_INCLUDED_TransportTCP
