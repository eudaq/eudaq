#include "eudaq/TransportNULL.hh"
#include "eudaq/Utils.hh"

#include <iostream>

namespace eudaq {

  namespace {

    static const std::string PROTO_NAME = "null";

    /** This registers the Transport with the TransportFactory
     */
    static RegisterTransport<NULLServer, NULLClient> reg(PROTO_NAME);

  } // anonymous namespace

  NULLServer::NULLServer(const std::string &) {
  }

  NULLServer::~NULLServer() {
  }

  void NULLServer::Close(const ConnectionInfo &) {
  }

  void NULLServer::SendPacket(const unsigned char *, size_t, const ConnectionInfo &, bool) {
  }

  void NULLServer::ProcessEvents(int timeout) {
    mSleep(timeout);
  }

  std::string NULLServer::ConnectionString() const {
    return PROTO_NAME + "://";
  }

  NULLClient::NULLClient(const std::string & /*param*/) {
  }

  void NULLClient::SendPacket(const unsigned char *, size_t, const ConnectionInfo &, bool) {
  }

  void NULLClient::ProcessEvents(int timeout) {
    //std::cout << "NULLClient::ProcessEvents " << timeout << std::endl;
    mSleep(timeout);
    //std::cout << "ok" << std::endl;
  }

  NULLClient::~NULLClient() {
  }

}
