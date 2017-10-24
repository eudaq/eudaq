#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif


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


namespace eudaq {

  namespace {

    static inline void closesocket(SOCKET s) {
      close(s);
    }

    static inline int LastSockError() {
      return errno;
    }

    static inline std::string LastSockErrorString(const std::string & msg) {
      const size_t buflen = 79;
      char buf[buflen] = "";
      if(strerror_r(errno, buf, buflen));//work around warn_unused_result at GNU strerror_r
      return msg + ": " + buf;
    }

    static void setup_socket(SOCKET sock) {
      /// Set non-blocking mode
      int iof = fcntl(sock, F_GETFL, 0);
      if (iof != -1)
        fcntl(sock, F_SETFL, iof | O_NONBLOCK);

      /// Allow the socket to rebind to an address that is still shutting down
      /// Useful if the server is quickly shut down then restarted
      int one = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

      /// Periodically send packets to keep the connection open
      setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof one);

      /// Try to send any remaining data when a socket is closed
      linger ling;
      ling.l_onoff = 1; ///< Enable linger mode
      ling.l_linger = 1; ///< Linger timeout in seconds
      setsockopt(sock, SOL_SOCKET, SO_LINGER, &ling, sizeof ling);
    }

  }

}
