#ifndef NICONTROLLER_HH
#define NICONTROLLER_HH

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <string>
#include <vector>

#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/time.h>
typedef int SOCKET;
#endif

class NiController {
public:
  void DatatransportClientSocket_Open(const std::string& addr, uint16_t port);
  void DatatransportClientSocket_Close();
  bool DataTransportClientSocket_Select();
  unsigned int DataTransportClientSocket_ReadLength();
  std::vector<unsigned char> DataTransportClientSocket_ReadData(int datalength);
  void ConfigClientSocket_Open(const std::string& addr, uint16_t port);
  void ConfigClientSocket_Close();
  bool ConfigClientSocket_Select();
  void ConfigClientSocket_Send(const std::string& msg);
  void ConfigClientSocket_Send(const std::vector<unsigned char> &bin);
  unsigned int ConfigClientSocket_ReadLength();
  std::vector<unsigned char> ConfigClientSocket_ReadData(int datalength);

private:
  sockaddr_in m_config;
  sockaddr_in m_datatransport;
  SOCKET m_sock_config;
  SOCKET m_sock_datatransport;
  char m_buffer_data[7000];
  char m_buffer_length[7000];
};

#endif
