/* TODO list:

I:
 ECONNRESET / WSAECONNRESET
 this error should be catch
 it indicates that the connection to some client got lost

qute:
WSAECONNRESET --> connection reset by peer.

An existing connection was forcibly closed by the remote host. This normally
results if the peer application on the remote host is suddenly stopped, the host
is rebooted, the host or remote network interface is disabled, or the remote
host uses a hard close (see setsockopt for more information on the SO_LINGER
option on the remote socket). This error may also result if a connection was
broken due to keep-alive activity detecting a failure while one or more
operations are in progress. Operations that were in progress fail with
WSAENETRESET. Subsequent operations fail with WSAECONNRESET.

II:
at some places we have constructions like:
 if (m_srvsock == (SOCKET)-1)


 Maybe we can replace (SOCKET)-1 with a macro that has a understandable name

 III:

 we should check if the return Value on Linux is really SOCKET


 like for example in this line:
 SOCKET result = select(static_cast<int>(m_sock+1), &tempset, NULL, NULL,
 &timeremain);

 since we have defined SOCKET on Linux to be int it will work as an int but is
 is it from the context correct?
 if not we should make it to int.

*/

#include "eudaq/TransportTCP.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Time.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>

#if EUDAQ_PLATFORM_IS(WIN32) || EUDAQ_PLATFORM_IS(MINGW)
#include "eudaq/TransportTCP_WIN32.hh"
#pragma comment(lib, "Ws2_32.lib")
#else
#include "eudaq/TransportTCP_POSIX.hh"
#endif

// print debug messages that are optimized out if DEBUG_TRANSPORT is not set:
// source and details:
// http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing
#ifndef DEBUG_TRANSPORT
#define DEBUG_TRANSPORT 0
#endif
#define debug_transport(fmt, ...)                                              \
  do {                                                                         \
    if (DEBUG_TRANSPORT)                                                       \
      fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __FUNCTION__,    \
              __VA_ARGS__);                                                    \
  } while (0)

namespace eudaq {
  const std::string TCPServer::name = "tcp";
  const std::string TCPClient::name = "tcp";

  namespace{
    auto d0=Factory<TransportServer>::Register<TCPServer, const std::string&>
      (str2hash(TCPServer::name));
    auto d1=Factory<TransportClient>::Register<TCPClient, const std::string&>
      (str2hash(TCPClient::name));
  }
  
  namespace {
    static const int MAXPENDING = 16;
    static const int MAX_BUFFER_SIZE = 10000;
    static int to_int(char c) { return static_cast<unsigned char>(c); }
#ifdef MSG_NOSIGNAL
    // On Linux (and cygwin?) send(...) can be told to
    // ignore signals by setting the flag below
    static const int FLAGS = MSG_NOSIGNAL;
    static void setup_signal() {}
#else
#include <signal.h>
    // On Mac OS X (BSD?) this flag is not available,
    // so we have to set the signal handler to ignore
    static const int FLAGS = 0;
    static void setup_signal() {
      // static so that it is only done once
      static sig_t sig = signal(SIGPIPE, SIG_IGN);
      (void)sig;
    }
#endif

    static void do_send_data(SOCKET sock, const unsigned char *data,
                             size_t len) {
      size_t sent = 0;
      do {
        int result = send(sock, reinterpret_cast<const char *>(data + sent),
                          static_cast<int>(len - sent), FLAGS);

        if (result > 0) {
          sent += result;
        }
	else if (result < 0 &&
		 (LastSockError() == EUDAQ_ERROR_Resource_temp_unavailable ||
		  LastSockError() == EUDAQ_ERROR_Interrupted_function_call)){
          // continue
        }
	else if (result == 0) {
          EUDAQ_THROW_NOLOG("TransportTCP:: Connection reset by peer");
        }
	else if (result < 0) {
          EUDAQ_THROW_NOLOG(LastSockErrorString("TransportTCP:: Error sending data"));
        }
      } while (sent < len);
    }

    static void do_send_packet(SOCKET sock, const unsigned char *data,
                               size_t length){
      if (length < 1020) {
        size_t len = length;
        std::string buffer(len + 4, '\0');
        for (int i = 0; i < 4; ++i) {
          buffer[i] = static_cast<char>(len & 0xff);
          len >>= 8;
        }
        std::copy(data, data + length, &buffer[4]);
        do_send_data(sock, reinterpret_cast<const unsigned char *>(&buffer[0]),
                     buffer.length());
      } else {
        size_t len = length;
        unsigned char buffer[4] = {0};
        for (int i = 0; i < 4; ++i) {
          buffer[i] = static_cast<unsigned char>(len & 0xff);
          len >>= 8;
        }
        do_send_data(sock, buffer, 4);
        do_send_data(sock, data, length);
      }
    }

  } // anonymous namespace

  bool ConnectionInfoTCP::Matches(const ConnectionInfo &other) const {
    const ConnectionInfoTCP *ptr =
        dynamic_cast<const ConnectionInfoTCP *>(&other);
    if (ptr && (ptr->m_fd == m_fd))
      return true;
    return false;
  }

  void ConnectionInfoTCP::Print(std::ostream &os, size_t offset) const {
    os << std::string(offset, ' ') << "<ConnectionTCP>\n";
    os << std::string(offset + 2, ' ') << "<FD>" << m_host <<"</FD>\n";
    ConnectionInfo::Print(os, offset+2);
    os << std::string(offset, ' ') << "</ConnectionTCP>\n";
  }

  void ConnectionInfoTCP::append(size_t length, const char *data) {
    m_buf += std::string(data, length);
    update_length();
  }

  bool ConnectionInfoTCP::havepacket() const {
    return m_buf.length() >= m_len + 4;
  }

  std::string ConnectionInfoTCP::getpacket() {
    if (!havepacket())
      EUDAQ_THROW_NOLOG("TransprotTCP:: No packet available");
    std::string packet(m_buf, 4, m_len);
    m_buf.erase(0, m_len + 4);
    update_length(true);
    return packet;
  }

  void ConnectionInfoTCP::update_length(bool force) {
    if (force || m_len == 0) {
      m_len = 0;
      if (m_buf.length() >= 4) {
        for (int i = 0; i < 4; ++i) {
          m_len |= to_int(m_buf[i]) << (8 * i);
        }
      }
    }
  }

  TCPServer::TCPServer(const std::string &param)
      : m_port(from_string(param, 0)),
        m_srvsock(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)),
        m_maxfd(m_srvsock) {
    if (m_srvsock == (SOCKET)-1)
      EUDAQ_THROW_NOLOG(LastSockErrorString("TCPServer:: Failed to create socket")); //$$ check if (SOCKET)-1 is correct
    setup_signal();
    FD_ZERO(&m_fdset);
    FD_SET(m_srvsock, &m_fdset);

    setup_socket(m_srvsock);

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    if (bind(m_srvsock, (sockaddr *)&addr, sizeof addr)) {
      closesocket(m_srvsock);
      EUDAQ_THROW_NOLOG(LastSockErrorString("TCPServer:: Failed to bind socket: " + param));
    }
    socklen_t addr_len = sizeof addr;
    if(m_port == 0){
      getsockname(m_srvsock, (sockaddr *)&addr, &addr_len);
      m_port = ntohs(addr.sin_port);
      EUDAQ_INFO("TCPServer:: Listening on port " + std::to_string(m_port));
    }
    if (listen(m_srvsock, MAXPENDING)){
      closesocket(m_srvsock);
      EUDAQ_THROW_NOLOG(
          LastSockErrorString("Failed to listen on socket: " + param));
    }
  }

  TCPServer::~TCPServer() {
    for(auto &conn : m_conn){
      if(conn){
        closesocket(conn->GetFd());
      }
    }
    closesocket(m_srvsock);
  }

  std::shared_ptr<ConnectionInfoTCP> TCPServer::GetInfo(SOCKET fd) const {
    const ConnectionInfoTCP tofind(fd);
    for(auto &conn: m_conn){
      if (conn && tofind.Matches(*conn) && conn->GetState() >= 0) {
	return conn;
      }
    }
    EUDAQ_THROW_NOLOG("BUG: please report it");
  }

  
  std::vector<ConnectionSPC> TCPServer::GetConnections () const{
    std::vector<ConnectionSPC> conns;
    for(auto &conn: m_conn){
      if(conn)
	conns.push_back(conn);
    }
    return conns;
  }
  
  void TCPServer::Close(const ConnectionInfo &id) {
    for(auto &conn: m_conn){
      if(conn && id.Matches(*conn)){
          SOCKET fd = conn->GetFd();
          FD_CLR(fd, &m_fdset);
          closesocket(fd);
	  conn.reset();
      }	
    }
  }
  
  void TCPServer::SendPacket(const unsigned char *data, size_t len,
                             const ConnectionInfo &id, bool duringconnect) { 
    for(auto &conn: m_conn){
      if(conn && id.Matches(*conn)){
        if(conn->GetState() > 0 || duringconnect) {
          do_send_packet(conn->GetFd(), data, len);
        }
      }
    }
  }

  void TCPServer::ProcessEvents(int timeout) {
#if DEBUG_NOTIMEOUT == 0
    Time t_start = Time::Current(); /*t_curr = t_start,*/
#endif
    Time t_remain = Time(0, timeout);
    bool done = false;
    do {
      fd_set tempset;
      memcpy(&tempset, &m_fdset, sizeof(tempset));
      timeval timeremain = t_remain;
      int result = select(static_cast<int>(m_maxfd + 1), &tempset, NULL, NULL,
                          &timeremain);
      if (result == 0) {
        // std::cout << "timeout" << std::endl;
      } else if (result < 0 &&
                 LastSockError() != EUDAQ_ERROR_Interrupted_function_call) {
        EUDAQ_THROW_NOLOG(LastSockErrorString("Error in select()"));
      } else if (result > 0) {

        if (FD_ISSET(m_srvsock, &tempset)) {
          sockaddr_in addr;
          socklen_t len = sizeof(addr);
          SOCKET peersock = accept(static_cast<int>(m_srvsock), (sockaddr *)&addr, &len);
          if (peersock == INVALID_SOCKET) {
	    EUDAQ_THROW_NOLOG(LastSockErrorString("Error in accept()"));
          }
	  else {
            FD_SET(peersock, &m_fdset);
            m_maxfd = (m_maxfd < peersock) ? peersock : m_maxfd;
            setup_socket(peersock);
            std::string host = inet_ntoa(addr.sin_addr);
            host = "tcp://"+host+":" + to_string(ntohs(addr.sin_port));
            auto conn_new = std::make_shared<ConnectionInfoTCP>(peersock, host);
            bool inserted = false;
            for(auto &conn: m_conn) {
              if(!conn) {
                conn = conn_new;
                inserted = true;
		break;
              }
            }
            if (!inserted)
              m_conn.push_back(conn_new);
            m_events.push(TransportEvent(TransportEvent::CONNECT, conn_new));
            FD_CLR(m_srvsock, &tempset);
          }
        }
        for (SOCKET j = 0; j < m_maxfd + 1; j++) {
          if (FD_ISSET(j, &tempset)) {
            char buffer[MAX_BUFFER_SIZE + 1];

            do {
              result = recv(j, buffer, MAX_BUFFER_SIZE, 0);
            } while (result == EUDAQ_ERROR_NO_DATA_RECEIVED &&
                     LastSockError() == EUDAQ_ERROR_Interrupted_function_call);

            if (result > 0) {
              buffer[result] = 0;
	      auto m = GetInfo(j);
              m->append(result, buffer);
              while (m->havepacket()) {
                done = true;
                m_events.push(
                    TransportEvent(TransportEvent::RECEIVE, m, m->getpacket()));
              }
            }
            else if (result == 0) {
              debug_transport(
                  "Server #%d, return=%d, WSAError:%d (%s) Disconnected.\n", j,
                  result, errno, strerror(errno));
	      auto m = GetInfo(j);
              m_events.push(TransportEvent(TransportEvent::DISCONNECT, m));
	      Close(*m);
            } else if (result == EUDAQ_ERROR_NO_DATA_RECEIVED) {
              debug_transport(
                  "Server #%d, return=%d, WSAError:%d (%s) No Data Received.\n",
                  j, result, errno, strerror(errno));
            } else {
              debug_transport("Server #%d, return=%d, WSAError:%d (%s) \n", j,
                              result, errno, strerror(errno));
            }
          }
        }
      }

// optionally disable timeout at compile time by setting DEBUG_NOTIMEOUT to 1
#if DEBUG_NOTIMEOUT
      t_remain = Time(0, timeout);
#else
      t_remain = Time(0, timeout) + t_start - Time::Current();
#endif
    } while (!done && t_remain > Time(0));
  }

  std::string TCPServer::ConnectionString() const{
#ifdef _WIN32
    const char *host = std::getenv("computername");
#else
    const char *host = std::getenv("HOSTNAME");
#endif
    if (!host)
      host = "localhost";
    // gethostname(buf, sizeof buf);
    return name + "://" + to_string(m_port);
  }

  TCPClient::TCPClient(const std::string &param)
      : m_server(param), m_port(44000),
        m_sock(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)),
        m_buf(std::make_shared<ConnectionInfoTCP>(m_sock, param)) {
    if (m_sock == (SOCKET)-1)
      EUDAQ_THROW_NOLOG(LastSockErrorString(
          "Failed to create socket")); //$$ check if (SOCKET)-1 is correct

    size_t i = param.find(':');
    if (i != std::string::npos) {
      m_server = trim(std::string(param, 0, i));
      m_port = from_string(std::string(param, i + 1), 44000);
    }
    if (m_server == "")
      m_server = "localhost";

    OpenConnection();
  }

  void TCPClient::OpenConnection() {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);

    hostent *host = gethostbyname(m_server.c_str());
    if (!host) {
      closesocket(m_sock);
      EUDAQ_THROW_NOLOG(
          LastSockErrorString("Error looking up address \'" + m_server + "\'"));
    }
    memcpy((char *)&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
    if (connect(m_sock, (sockaddr *)&addr, sizeof(addr)) &&
        LastSockError() != EUDAQ_ERROR_Operation_progress &&
        LastSockError() != EUDAQ_ERROR_Resource_temp_unavailable) {
      EUDAQ_THROW_NOLOG(
          LastSockErrorString("Are you sure the server is running? - Error " +
                              to_string(LastSockError()) + " connecting to " +
                              m_server + ":" + to_string(m_port)));
    }
    setup_socket(m_sock); // set to non-blocking
  }

  void TCPClient::SendPacket(const unsigned char *data, size_t len,
                             const ConnectionInfo &id, bool) {
    if(id.Matches(*m_buf)) {
      do_send_packet(m_buf->GetFd(), data, len);
    }
  }

  void TCPClient::ProcessEvents(int timeout) {
#if DEBUG_NOTIMEOUT == 0
    Time t_start = Time::Current(); /*t_curr = t_start,*/
#endif
    Time t_remain = Time(0, timeout);
    bool done = false;
    do {
      fd_set tempset;
      FD_ZERO(&tempset);
      FD_SET(m_sock, &tempset);
      timeval timeremain = t_remain;
      auto result = select(static_cast<int>(m_sock + 1), &tempset, NULL, NULL,
			   &timeremain);
      bool donereading = false;
      do {
        char buffer[MAX_BUFFER_SIZE + 1];

        do {
          result = recv(m_sock, buffer, MAX_BUFFER_SIZE, 0);
        } while (result == EUDAQ_ERROR_NO_DATA_RECEIVED &&
                 LastSockError() == EUDAQ_ERROR_Interrupted_function_call);

        if (result == EUDAQ_ERROR_NO_DATA_RECEIVED &&
            LastSockError() == EUDAQ_ERROR_Resource_temp_unavailable) {
          debug_transport("Client, return=%d, WSAError:%d (%s) Nothing to do\n",
                          result, errno, strerror(errno));
          donereading = true;
        } else if (result == EUDAQ_ERROR_NO_DATA_RECEIVED) {
          debug_transport("Client, return=%d, WSAError:%d (%s) Time Out\n",
                          result, errno, strerror(errno));
        } else if (result == 0) {
          debug_transport("Client, return=%d, WSAError:%d (%s) Disconnect (?)\n", result,
			  errno, strerror(errno));
          donereading = true;
          EUDAQ_THROW_NOLOG(LastSockErrorString(
              "SocketClient Error (" + to_string(LastSockError()) + ")"));
        } else if (result > 0) {
          m_buf->append(result, buffer);
          while (m_buf->havepacket()) {
            m_events.push(TransportEvent(TransportEvent::RECEIVE, m_buf,
                                         m_buf->getpacket()));
            done = true;
          }
        }
      } while (!donereading);

// optionally disable timeout at compile time by setting DEBUG_NOTIMEOUT to 1
#if DEBUG_NOTIMEOUT
      t_remain = Time(0, timeout);
#else
      t_remain = Time(0, timeout) + t_start - Time::Current();
#endif
      if (!(t_remain > Time(0))) {
        debug_transport("%s\n", "Reached Timeout in ProcessEvents().");
      }
    } while (!done && t_remain > Time(0));
  }

  TCPClient::~TCPClient() { closesocket(m_sock); }
}
