#include "eudaq/TransportNULL.hh"
#include "eudaq/Utils.hh"

#include <iostream>

namespace eudaq {

  const std::string NULLServer::name = "null";


  namespace{
    auto d0=Factory<TransportServer>::Register<NULLServer, const std::string&>
      (cstr2hash("null"));
    auto d1=Factory<TransportClient>::Register<NULLClient, const std::string&>
      (cstr2hash("null"));
  }
  
  NULLServer::NULLServer(const std::string &) {}

  NULLServer::~NULLServer() {}

  void NULLServer::Close(const ConnectionInfo &) {}

  void NULLServer::SendPacket(const unsigned char *, size_t,
                              const ConnectionInfo &, bool) {}

  void NULLServer::ProcessEvents(int timeout) { mSleep(timeout); }

  std::vector<ConnectionSPC> NULLServer::GetConnections() const{
    return std::vector<ConnectionSPC>();
  }
  
  std::string NULLServer::ConnectionString() const { return "null://"; }

  NULLClient::NULLClient(const std::string & /*param*/):m_buf("") {}

  void NULLClient::SendPacket(const unsigned char *, size_t,
                              const ConnectionInfo &, bool) {}

  void NULLClient::ProcessEvents(int timeout) {
    mSleep(timeout);
  }

  NULLClient::~NULLClient() {}
}
