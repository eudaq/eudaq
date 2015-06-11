#ifndef DEVICEEXPLORER_HH
#define DEVICEEXPLORER_HH
#include <string>

//Costants
#define MTU_UDP_IP 9000 // Size buffer
#define DATA_PORT 6006  // Port UDP server used for data taking
#define SC_PORT 6007    // Port UDP server used for slow control
#define APP_PORT 6039

/***********************************************************************************************************************
 *                                                                                                                     *
 * Device Palpidess Received the event from a UDP/IP Server.                                                           *
 * In this class is implemented:                                                                                       *
 * - Methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		                   *
 * - Methods for configuring and getting events (Configure, ConfigureDAQ, GetSingleEvent)                              *
 *                                                                                                                     *
 ***********************************************************************************************************************/
class DevicePalpidess
{
private:
  int data;		//socket descriptors used for data taking
  int sc;
  bool cmdRdy;

public:
  DevicePalpidess();              // Costrutctor.
  virtual ~DevicePalpidess();     // Destructor.

  int create_server_udp(int port = DATA_PORT);
  void close_server_udp(int sd);
  int get_SD();

  bool Configure(std::string arg);
  bool ConfigureDAQ();
  bool StopDAQ();
  bool GetSingleEvent();
};
#endif
