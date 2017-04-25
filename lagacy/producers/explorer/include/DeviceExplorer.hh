#ifndef DEVICEEXPLORER_HH
#define DEVICEEXPLORER_HH
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // string function definitions
#include <unistd.h> // UNIX standard function definitions
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <termios.h> // POSIX terminal control definitionss
#include <time.h>   // time calls
#include <math.h>

//Costants
#define MTU_UDP_IP 9000 // Size buffer
#define DATA_PORT 6006  // Port UDP server used for data taking
#define SC_PORT 6007    // Port UDP server used for slow control
#define APP_PORT 6039

/***********************************************************************************************************************
 *                                                                                                                     *
 * Device Explorer Recived the event from a UDP/IP Server.                                                             *
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		       *
 * The methods for configuring and getting events (Configure, ConfigureDAQ, GetSigngleEvent)                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/
class DeviceExplorer
{
private:
  int data[2];		//socket descriptors used for data taking
  int sc[2];
  bool cmdRdy;

public:
  DeviceExplorer();               // Costrutctor.
  virtual ~DeviceExplorer();      // Destructor.

  int create_server_udp(int fecn, int port = DATA_PORT);
  void close_server_udp(int sd);
  int get_SD(int fecn);

  bool Configure();
  bool ConfigureDAQ();
  bool StopDAQ();
  bool GetSingleEvent();
};
#endif
