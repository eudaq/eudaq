#include "NiController.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/types.h>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define EUDAQ_BCOPY(source, dest, NSize) bcopy(source, dest, NSize)
#define EUDAQ_Sleep(x) sleep(x)
#define EUDAQ_CLOSE_SOCKET(x) close(x)
#else

#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#define EUDAQ_Sleep(x) Sleep(x)
#define EUDAQ_BCOPY(source, dest, NSize) memmove(dest, source, NSize)

#define EUDAQ_CLOSE_SOCKET(x) closesocket(x)

#endif

int EUDAQ_SEND(SOCKET s, unsigned char *buf, int len, int flags) {

#ifdef WIN32
  // on windows this function takes a signed char ...
  // in prinziple I always distrust reinterpreter_cast but in this case it
  // should be save
  return send(s, reinterpret_cast<const char *>(buf), len, flags);

#else
  return send(s, buf, len, flags);
#endif
}

unsigned char start[5] = "star";
unsigned char stop[5] = "stop";

NiController::NiController() {
  // NI_IP = "192.76.172.199";
}
void NiController::Configure(const eudaq::Configuration & /*param*/) {
  // NiIPaddr = param.Get("NiIPaddr", "");
}
void NiController::TagsSetting() {
  // ev.SetTag("DET", 12);
}
void NiController::GetProduserHostInfo() {
  /*** get Producer information, NAME and INET ADDRESS ***/
  gethostname(ThisHost, MAXHOSTNAME);
  printf("----TCP/Producer running at host NAME: %s\n", ThisHost);
  hclient = gethostbyname(ThisHost);
  if (hclient != 0) {
    EUDAQ_BCOPY(hclient->h_addr, &(client.sin_addr), hclient->h_length);
    printf("----TCP/Producer INET ADDRESS is: %s \n",
           inet_ntoa(client.sin_addr));
  } else {
    printf("----TCP/Producer -- Warning! -- failed at executing "
           "gethostbyname() for: %s. Check your /etc/hosts list \n",
           ThisHost);
  }
}
void NiController::Start() { ConfigClientSocket_Send(start, sizeof(start)); }
void NiController::Stop() { ConfigClientSocket_Send(stop, sizeof(stop)); }

void NiController::ConfigClientSocket_Open(const eudaq::Configuration &param) {
  /*** Network configuration for NI, NAME and INET ADDRESS ***/

  std::string m_server;
  m_server = param.Get("NiIPaddr", "");

  std::string m_config_socket_port;
  m_config_socket_port = param.Get("NiConfigSocketPort", "49248");

  // convert string in config into IPv4 address
  hostent *host = gethostbyname(m_server.c_str());
  if (!host) {
    EUDAQ_ERROR("ConfSocket: Bad NiIPaddr value in config file: must be legal "
                "IPv4 address!");
    perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
           "address: ");
  }
  memcpy((char *)&config.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  if ((sock_config = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    EUDAQ_ERROR("ConfSocket: Error creating the TCP socket  ");
    perror("ConfSocket Error: socket()");
    exit(1);
  } else
    printf("----TCP/NI crate: SOCKET is OK...\n");

  printf("----TCP/NI crate INET ADDRESS is: %s \n", inet_ntoa(config.sin_addr));
  printf("----TCP/NI crate INET PORT is: %s \n", m_config_socket_port.c_str());

  config.sin_family = AF_INET;
  int i_auto = std::stoi(m_config_socket_port, nullptr, 10);
  config.sin_port = htons(i_auto);
  memset(&(config.sin_zero), '\0', 8);
  if (connect(sock_config, (struct sockaddr *)&config,
              sizeof(struct sockaddr)) == -1) {
    EUDAQ_ERROR("ConfSocket: National Instruments crate doesn't appear to be "
                "running  ");
    perror("ConfSocket Error: connect()");
    EUDAQ_Sleep(60);
    exit(1);
  } else
    printf("----TCP/NI crate The CONNECT is OK...\n");
}
void NiController::ConfigClientSocket_Send(unsigned char *text, size_t len) {
  bool dbg = false;
  if (dbg)
    printf("size=%zu", len);

  if (EUDAQ_SEND(sock_config, text, len, 0) == -1)
    perror("Server-send() error lol!");
}
void NiController::ConfigClientSocket_Close() {

  EUDAQ_CLOSE_SOCKET(sock_config);
}
unsigned int
NiController::ConfigClientSocket_ReadLength(const char * /*string[4]*/) {
  unsigned int datalengthTmp;
  unsigned int datalength;
  int i;
  bool dbg = false;
  if ((numbytes = recv(sock_config, Buffer_length, 2, 0)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: Read length error ");
    perror("recv()");
    exit(1);
  } else {
    if (dbg)
      printf("|==ConfigClientSocket_ReadLength ==|    numbytes=%u \n",
             static_cast<uint32_t>(numbytes));
    i = 0;
    if (dbg) {
      while (i < numbytes) {
        printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i + 1]);
        i = i + 2;
      }
    }
    datalengthTmp = 0;
    datalengthTmp = 0xFF & Buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & Buffer_length[1];
    datalength = datalengthTmp;

    if (dbg)
      printf(" data= %d", datalength);
    if (dbg)
      printf("\n");
  }
  return datalength;
}
std::vector<unsigned char>
NiController::ConfigClientSocket_ReadData(int datalength) {
  std::vector<unsigned char> ConfigData(datalength);
  unsigned int stored_bytes;
  unsigned int read_bytes_left;
  unsigned int i;
  bool dbg = false;

  stored_bytes = 0;
  read_bytes_left = datalength;
  while (read_bytes_left > 0) {
    if ((numbytes = recv(sock_config, Buffer_data, read_bytes_left, 0)) == -1) {
      EUDAQ_ERROR("|==ConfigClientSocket_ReadLength==| Read data error ");
      perror("recv()");
      exit(1);
    } else {
      if (dbg)
        printf("|==ConfigClientSocket_ReadLength==|    numbytes=%u \n",
               static_cast<uint32_t>(numbytes));
      read_bytes_left = read_bytes_left - numbytes;
      for (int k = 0; k < numbytes; k++) {
        ConfigData[stored_bytes] = Buffer_data[k];
        stored_bytes++;
      }
      i = 0;
      if (dbg) {
        while ((int)i < numbytes) {
          printf(" 0x%x \n", 0xFF & Buffer_data[i]);
          i++;
        }
      }
    }
  }
  if (dbg)
    printf("\n");
  return ConfigData;
}

void NiController::DatatransportClientSocket_Open(
    const eudaq::Configuration &param) {
  /*** Creation for the data transmit socket, NAME and INET ADDRESS ***/
  std::string m_server;
  m_server = param.Get("NiIPaddr", "");

  std::string m_data_transport_socket_port;
  m_data_transport_socket_port =
      param.Get("NiDataTransportSocketPort", "49250");

  // convert string in config into IPv4 address
  hostent *host = gethostbyname(m_server.c_str());
  if (!host) {
    EUDAQ_ERROR("ConfSocket: Bad NiIPaddr value in config file: must be legal "
                "IPv4 address!");
    perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
           "address: ");
  }
  memcpy((char *)&datatransport.sin_addr.s_addr, host->h_addr_list[0],
         host->h_length);

  if ((sock_datatransport = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: Error creating socket ");
    perror("DataTransportSocket Error: socket()");
    exit(1);
  } else
    printf("----TCP/NI crate DATA TRANSPORT: The SOCKET is OK...\n");

  printf("----TCP/NI crate DATA TRANSPORT INET ADDRESS is: %s \n",
         inet_ntoa(datatransport.sin_addr));
  printf("----TCP/NI crate DATA TRANSPORT INET PORT is: %s \n",
         m_data_transport_socket_port.c_str());

  datatransport.sin_family = AF_INET;
  int i_auto = std::stoi(m_data_transport_socket_port, nullptr, 10);
  datatransport.sin_port = htons(i_auto);
  memset(&(datatransport.sin_zero), '\0', 8);
  if (connect(sock_datatransport, (struct sockaddr *)&datatransport,
              sizeof(struct sockaddr)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: National Instruments crate doesn't "
                "appear to be running  ");
    perror("DataTransportSocket: connect()");
    EUDAQ_Sleep(60);
    exit(1);
  } else
    printf("----TCP/NI crate DATA TRANSPORT: The CONNECT executed OK...\n");
}
unsigned int
NiController::DataTransportClientSocket_ReadLength(const char * /*string[4]*/) {
  unsigned int datalengthTmp;
  unsigned int datalength;
  int i;
  bool dbg = false;
  if ((numbytes = recv(sock_datatransport, Buffer_length, 2, 0)) == -1) {
    EUDAQ_ERROR("DataTransportSocket: Read length error ");
    perror("recv()");
    exit(1);
  } else {
    if (dbg)
      printf("|==DataTransportClientSocket_ReadLength ==|    numbytes=%u \n",
             static_cast<uint32_t>(numbytes));
    i = 0;
    if (dbg) {
      while (i < numbytes) {
        printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i + 1]);
        i = i + 2;
      }
    }
    datalengthTmp = 0;
    datalengthTmp = 0xFF & Buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & Buffer_length[1];
    datalength = datalengthTmp;

    if (dbg)
      printf(" data= %d", datalength);
    if (dbg)
      printf("\n");
  }
  return datalength;
}
std::vector<unsigned char>
NiController::DataTransportClientSocket_ReadData(int datalength) {

  std::vector<unsigned char> mimosa_data(datalength);
  unsigned int stored_bytes;
  unsigned int read_bytes_left;
  unsigned int i;
  bool dbg = false;

  stored_bytes = 0;
  read_bytes_left = datalength;
  while (read_bytes_left > 0) {
    if ((numbytes =
             recv(sock_datatransport, Buffer_data, read_bytes_left, 0)) == -1) {
      EUDAQ_ERROR("DataTransportSocket: Read data error ");
      perror("recv()");
      exit(1);
    } else {
      if (dbg)
        printf("|==DataTransportClientSocket_ReadData==|    numbytes=%u \n",
               static_cast<uint32_t>(numbytes));
      read_bytes_left = read_bytes_left - numbytes;
      for (int k = 0; k < numbytes; k++) {
        mimosa_data[stored_bytes] = Buffer_data[k];
        stored_bytes++;
      }
      i = 0;
      if (dbg) {
        while ((int)i < numbytes) {
          printf(" 0x%x%x", 0xFF & Buffer_data[i], 0xFF & Buffer_data[i + 1]);
          i = i + 2;
        }
      }
    }
  }
  if (dbg)
    printf("\n");
  return mimosa_data;
}
void NiController::DatatransportClientSocket_Close() {
  EUDAQ_CLOSE_SOCKET(sock_datatransport);
}
NiController::~NiController() {
  //
}
