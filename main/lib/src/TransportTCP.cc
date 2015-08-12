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
#include <iostream>

#if EUDAQ_PLATFORM_IS(WIN32) || EUDAQ_PLATFORM_IS(MINGW)
#include "eudaq/TransportTCP_WIN32.h" //$$ changed to ".h"
#pragma comment(lib, "Ws2_32.lib")

// defining error code more informations under
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms740668%28v=vs.85%29.aspx

// qute:
// "Resource temporarily unavailable.
//
// 	This error is returned from operations on nonblocking sockets that
//  cannot be completed immediately, for example recv when no data is queued
//  to be read from the socket. It is a nonfatal error, and the operation
//  should be retried later. It is normal for WSAEWOULDBLOCK to be reported
//  as the result from calling connect on a nonblocking SOCK_STREAM socket,
//  since some time must elapse for the connection to be established."
#define EUDAQ_ERROR_Resource_temp_unavailable WSAEWOULDBLOCK

// qute:
// Operation now in progress.
//
// 	A blocking operation is currently executing. Windows Sockets
// 	only allows a single blocking operation (per- task or thread) to
// 	be outstanding, and if any other function call is made (whether
// 	or not it references that or any other socket) the function fails
// 	with the WSAEINPROGRESS error.
//
#define EUDAQ_ERROR_Operation_progress WSAEINPROGRESS

// Interrupted function call.
//	A blocking operation was interrupted by a call to WSACancelBlockingCall.
//
#define EUDAQ_ERROR_Interrupted_function_call WSAEINTR

#define EUDAQ_ERROR_NO_DATA_RECEIVED -1
#else

#include "eudaq/TransportTCP_POSIX.inc"

// defining error code more informations under
// http://www.gnu.org/software/libc/manual/html_node/Error-Codes.html

// Redirection to EAGAIN
//
// Quate:
// 	"Resource temporarily unavailable; the call might work if you
// 	try again later. The macro EWOULDBLOCK is another name for EAGAIN;
// 	they are always the same in the GNU C Library.
//
// 	This error can happen in a few different situations:
//
// 	An operation that would block was attempted on an object
// 	that has non-blocking mode selected. Trying the same operation
// 	again will block until some external condition makes it possible
// 	to read, write, or connect (whatever the operation). You can
// 	use select to find out when the operation will be possible;
// 	see Waiting for I/O.
//
// 	Portability Note: In many older Unix systems, this condition
// 	was indicated by EWOULDBLOCK, which was a distinct error code
// 	different from EAGAIN. To make your program portable, you
// 	should check for both codes and treat them the same.
// 	A temporary resource shortage made an operation impossible.
// 	fork can return this error. It indicates that the shortage
// 	is expected to pass, so your program can try the call again
// 	later and it may succeed. It is probably a good idea to delay
// 	for a few seconds before trying it again, to allow time for
// 	other processes to release scarce resources. Such
// 	shortages are usually fairly serious and affect the whole
// 	system, so usually an interactive program should report
// 	the error to the user and return to its command loop. "
#define EUDAQ_ERROR_Resource_temp_unavailable EWOULDBLOCK

// 	An operation that cannot complete immediately was initiated
// 	on an object that has non-blocking mode selected. Some
// 	functions that must always block (such as connect; see
// 	Connecting) never return EAGAIN. Instead, they return
// 	EINPROGRESS to indicate that the operation has begun and
// 	will take some time. Attempts to manipulate the object
// 	before the call completes return EALREADY. You can use the
// 	select function to find out when the pending operation has
// 	completed; see Waiting for I/O.
#define EUDAQ_ERROR_Operation_progress EINPROGRESS

// 	Interrupted function call;
// 	an asynchronous signal occurred and
// 	prevented completion of the call.
// 	When this happens, you should try
// 	the call again.
// 	You can choose to have functions resume
// 	after a signal that is handled, rather
// 	than failing with EINTR; see Interrupted
// 	Primitives.
#define EUDAQ_ERROR_Interrupted_function_call EINTR

#define EUDAQ_ERROR_NO_DATA_RECEIVED -1

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

#include <sys/types.h>
#include <errno.h>
//#include <unistd.h>

#include <iostream>
#include <ostream>
#include <iostream>

namespace eudaq {

  const std::string TCPServer::name = "tcp";

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
/*
do_send_data handles the sending of data among components of the program via the socket system. 
It takes a socket, a char of data and a size_t length as parameters. 

This function maintains a record of how much data has been send via the socket. It loops through the following statements so long as the amount of data that has been sent is less than the length given as a parameter. 

	The function attempts to send the data via the given socket
	The result of this send is stored in an int labled result
	If the result is >1 , then it is added to sent
	If the result is <0 and the resource is only temporarily unavalible, it tries again
	If the result is =0 the program throws an error for the connection being reset
	If the result is <0 the program throws an error for sending data.

*/
    static void do_send_data(SOCKET sock, const unsigned char *data,
                             size_t len) {
      // if (len > 500000) std::cout << "Starting send" << std::endl;
      size_t sent = 0;
      do {
        int result = send(sock, reinterpret_cast<const char *>(data + sent),
                          static_cast<int>(len - sent), FLAGS);

        if (result > 0) {
          sent += result;
        } else if (result < 0 &&
                   (LastSockError() == EUDAQ_ERROR_Resource_temp_unavailable ||
                    LastSockError() == EUDAQ_ERROR_Interrupted_function_call)) {
          // continue
        } else if (result == 0) {
          EUDAQ_THROW_NOLOG("Connection reset by peer");
        } else if (result < 0) {
          EUDAQ_THROW_NOLOG(LastSockErrorString("Error sending data"));
        }
      } while (sent < len);
      // if (len > 500000) std::cout << "Done send" << std::endl;
    }

/*
do_send_packet handles the sending of packets via the do_send_data function. 
It takes a socket, a char for data, and a size_t for the length.


If the packet is smaller than 1020, The function packs the data into a buffer with signed chars and passes it to do_send_data cast as unsigned char.

Otherwise pack the data into a buffer using unsigned chars. The function passes the buffer to do_send_data and then the data to do_send_data. 
 
*/
    static void do_send_packet(SOCKET sock, const unsigned char *data,
                               size_t length) {
      // if (length > 500000) std::cout << "Starting send packet" << std::endl;
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
      // if (length > 500000) std::cout << "Done send packet" << std::endl;
    }

    //     static void send_data(SOCKET sock, uint32_t data) {
    //       std::string str;
    //       for (int i = 0; i < 4; ++i) {
    //         str += static_cast<char>(data & 0xff);
    //         data >>= 8;
    //       }
    //       send_data(sock, str);
    //     }

  } // anonymous namespace

  bool ConnectionInfoTCP::Matches(const ConnectionInfo &other) const {
    const ConnectionInfoTCP *ptr =
        dynamic_cast<const ConnectionInfoTCP *>(&other);
    // std::cout << " [Match: " << m_fd << " == " << (ptr ? to_string(ptr->m_fd)
    // : "null") << "] " << std::flush;
    if (ptr && (ptr->m_fd == m_fd))
      return true;
    return false;
  }

  void ConnectionInfoTCP::Print(std::ostream &os) const {
    ConnectionInfo::Print(os);
    os << " (" /*<< m_fd << ","*/ << m_host << ")";
  }

  void ConnectionInfoTCP::append(size_t length, const char *data) {
    m_buf += std::string(data, length);
    update_length();
    // std::cout << *this << " - append: " << m_len << ", " << m_buf.size() <<
    // std::endl;
  }

  bool ConnectionInfoTCP::havepacket() const {
    return m_buf.length() >= m_len + 4;
  }

  std::string ConnectionInfoTCP::getpacket() {
    if (!havepacket())
      EUDAQ_THROW_NOLOG("No packet available");
    std::string packet(m_buf, 4, m_len);
    m_buf.erase(0, m_len + 4);
    update_length(true);
    // std::cout << *this << " - getpacket: " << m_len << ", " << m_buf.size()
    // << std::endl;
    return packet;
  }

  void ConnectionInfoTCP::update_length(bool force) {
    // std::cout << "DBG: len=" << m_len << ", buf=" << m_buf.length();
    if (force || m_len == 0) {
      m_len = 0;
      if (m_buf.length() >= 4) {
        for (int i = 0; i < 4; ++i) {
          m_len |= to_int(m_buf[i]) << (8 * i);
        }
        //         std::cout << " (" << to_hex(m_buf[3], 2)
        //                   << " " << to_hex(m_buf[2], 2)
        //                   << " " << to_hex(m_buf[1], 2)
        //                   << " " << to_hex(m_buf[0], 2)
        //                   << ")";
      }
    }
    // std::cout << ", len=" << m_len << std::endl;
  }

  TCPServer::TCPServer(const std::string &param)
      : m_port(from_string(param, 44000)),
        m_srvsock(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)),
        m_maxfd(m_srvsock) {
    if (m_srvsock == (SOCKET)-1)
      EUDAQ_THROW_NOLOG(LastSockErrorString(
          "Failed to create socket")); //$$ check if (SOCKET)-1 is correct
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
      EUDAQ_THROW_NOLOG(LastSockErrorString("Failed to bind socket: " + param));
    }
    if (listen(m_srvsock, MAXPENDING)) {
      closesocket(m_srvsock);
      EUDAQ_THROW_NOLOG(
          LastSockErrorString("Failed to listen on socket: " + param));
    }
  }

  TCPServer::~TCPServer() {
    for (size_t i = 0; i < m_conn.size(); ++i) {
      ConnectionInfoTCP *inf =
          dynamic_cast<ConnectionInfoTCP *>(m_conn[i].get());
      if (inf && inf->IsEnabled()) {
        closesocket(inf->GetFd());
      }
    }
    closesocket(m_srvsock);
  }

  ConnectionInfoTCP &TCPServer::GetInfo(SOCKET fd) const {
    const ConnectionInfoTCP tofind(fd);
    for (size_t i = 0; i < m_conn.size(); ++i) {
      if (tofind.Matches(*m_conn[i]) && m_conn[i]->GetState() >= 0) {
        ConnectionInfoTCP *inf =
            dynamic_cast<ConnectionInfoTCP *>(m_conn[i].get());
        return *inf;
      }
    }
    EUDAQ_THROW_NOLOG("BUG: please report it");
  }

  void TCPServer::Close(const ConnectionInfo &id) {
    for (size_t i = 0; i < m_conn.size(); ++i) {
      if (id.Matches(*m_conn[i])) {
        ConnectionInfoTCP *inf =
            dynamic_cast<ConnectionInfoTCP *>(m_conn[i].get());
        if (inf && inf->IsEnabled()) {
          SOCKET fd = inf->GetFd();
          inf->Disable();
          FD_CLR(fd, &m_fdset);
          closesocket(fd);
        }
      }
    }
  }

  void TCPServer::SendPacket(const unsigned char *data, size_t len,
                             const ConnectionInfo &id, bool duringconnect) {
    // std::cout << "SendPacket to " << id << std::endl;
    for (size_t i = 0; i < m_conn.size(); ++i) {
      // std::cout << "- " << i << ": " << *m_conn[i] << std::flush;
      if (id.Matches(*m_conn[i])) {
        ConnectionInfoTCP *inf =
            dynamic_cast<ConnectionInfoTCP *>(m_conn[i].get());
        if (inf && inf->IsEnabled() && (inf->GetState() > 0 || duringconnect)) {
          // std::cout << " ok" << std::endl;
          do_send_packet(inf->GetFd(), data, len);
        } // else std::cout << " not quite" << std::endl;
      } // else std::cout << " nope" << std::endl;
    }
  }

  void TCPServer::ProcessEvents(int timeout) {
// std::cout << "DEBUG: Process..." << std::endl;
#if DEBUG_NOTIMEOUT == 0
    Time t_start = Time::Current(); /*t_curr = t_start,*/
#endif
    Time t_remain = Time(0, timeout);
    bool done = false;
    do {
      fd_set tempset;
      memcpy(&tempset, &m_fdset, sizeof(tempset));
      // std::cout << "select timeout=" << t_remain << std::endl;
      timeval timeremain = t_remain;
      int result = select(static_cast<int>(m_maxfd + 1), &tempset, NULL, NULL,
                          &timeremain);

      // std::cout << "select done" << std::endl;

      if (result == 0) {
        // std::cout << "timeout" << std::endl;
      } else if (result < 0 &&
                 LastSockError() != EUDAQ_ERROR_Interrupted_function_call) {
        std::cout << LastSockErrorString("Error in select()") << std::endl;
      } else if (result > 0) {

        if (FD_ISSET(m_srvsock, &tempset)) {
          sockaddr_in addr;
          socklen_t len = sizeof(addr);
          SOCKET peersock =
              accept(static_cast<int>(m_srvsock), (sockaddr *)&addr, &len);
          if (peersock == INVALID_SOCKET) {
            std::cout << LastSockErrorString("Error in accept()") << std::endl;
          } else {
            // std::cout << "Connect " << peersock << " from " <<
            // inet_ntoa(addr.sin_addr) << std::endl;
            FD_SET(peersock, &m_fdset);
            m_maxfd = (m_maxfd < peersock) ? peersock : m_maxfd;
            setup_socket(peersock);
            std::string host = inet_ntoa(addr.sin_addr);
            host += ":" + to_string(ntohs(addr.sin_port));
            std::shared_ptr<ConnectionInfo> ptr(
                new ConnectionInfoTCP(peersock, host));
            bool inserted = false;
            for (size_t i = 0; i < m_conn.size(); ++i) {
              if (m_conn[i]->GetState() < 0) {
                m_conn[i] = ptr;
                inserted = true;
              }
            }
            if (!inserted)
              m_conn.push_back(ptr);
            m_events.push(TransportEvent(TransportEvent::CONNECT, *ptr));
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
              ConnectionInfoTCP &m = GetInfo(j);
              m.append(result, buffer);
              while (m.havepacket()) {
                done = true;
                m_events.push(
                    TransportEvent(TransportEvent::RECEIVE, m, m.getpacket()));
              }
            } // else /*if (result == 0)*/ {
            else if (result == 0) {
              debug_transport(
                  "Server #%d, return=%d, WSAError:%d (%s) Disconnected.\n", j,
                  result, errno, strerror(errno));
              ConnectionInfoTCP &m = GetInfo(j);
              m_events.push(TransportEvent(TransportEvent::DISCONNECT, m));
              m.Disable();
              closesocket(j);
              FD_CLR(j, &m_fdset);
            } else if (result == EUDAQ_ERROR_NO_DATA_RECEIVED) {
              debug_transport(
                  "Server #%d, return=%d, WSAError:%d (%s) No Data Received.\n",
                  j, result, errno, strerror(errno));
            } else {
              debug_transport("Server #%d, return=%d, WSAError:%d (%s) \n", j,
                              result, errno, strerror(errno));
            }
          } // end if (FD_ISSET(j, &amp;tempset))
        }   // end for (j=0;...)
      }     // end else if (result > 0)

// optionally disable timeout at compile time by setting DEBUG_NOTIMEOUT to 1
#if DEBUG_NOTIMEOUT
      t_remain = Time(0, timeout);
#else
      t_remain = Time(0, timeout) + t_start - Time::Current();
#endif
    } while (!done && t_remain > Time(0));
  }

  std::string TCPServer::ConnectionString() const {
#ifdef WIN32
    const char *host = getenv("computername");
#else
    const char *host = getenv("HOSTNAME");
#endif

    if (!host)
      host = "localhost";
    // gethostname(buf, sizeof buf);
    return name + "://" + host + ":" + to_string(m_port);
  }

  TCPClient::TCPClient(const std::string &param)
      : m_server(param), m_port(44000),
        m_sock(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)),
        m_buf(ConnectionInfoTCP(m_sock, param)) {
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
    // std::cout << "Sending packet to " << id << std::endl;
    if (id.Matches(m_buf)) {
      // std::cout << " ok" << std::endl;
      do_send_packet(m_buf.GetFd(), data, len);
    }
    // std::cout << "Sent" << std::endl;
  }

  void TCPClient::ProcessEvents(int timeout) {
// std::cout << "ProcessEvents()" << std::endl;
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
#ifdef WIN32
      int result = select(static_cast<int>(m_sock + 1), &tempset, NULL, NULL,
                          &timeremain);
#else
      SOCKET result = select(static_cast<int>(m_sock + 1), &tempset, NULL, NULL,
                             &timeremain);
#endif

      //	std::cout << "Select result=" << result << std::endl;

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
          debug_transport(
              "Client, return=%d, WSAError:%d (%s) Disconnect (?)\n", result,
              errno, strerror(errno));
          donereading = true;
          EUDAQ_THROW_NOLOG(LastSockErrorString(
              "SocketClient Error (" + to_string(LastSockError()) + ")"));
        } else if (result > 0) {
          m_buf.append(result, buffer);
          while (m_buf.havepacket()) {
            m_events.push(TransportEvent(TransportEvent::RECEIVE, m_buf,
                                         m_buf.getpacket()));
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
      // std::cout << "Remaining time in ProcessEvents(): " << t_remain <<
      // (t_remain > Time(0) ? " >0" : " <0")<< std::endl;
      if (!(t_remain > Time(0))) {
        debug_transport("%s\n", "Reached Timeout in ProcessEvents().");
      }
    } while (!done && t_remain > Time(0));
    // std::cout << "done" << std::endl;
  }

  TCPClient::~TCPClient() { closesocket(m_sock); }
}
