#include "NiController.hh"
#include "eudaq/Logger.hh"

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
#define NI_CLOSE_SOCKET(x) close(x)
#else

#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#define NI_CLOSE_SOCKET(x) closesocket(x)
#endif


void NiController::ConfigClientSocket_Open(const std::string& addr, uint16_t port) {
  /*** Network configuration for NI, NAME and INET ADDRESS ***/
  // convert string in config into IPv4 address
  hostent *host = gethostbyname(addr.c_str());
  if (!host) {
    perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
           "address: ");	  
    EUDAQ_THROW("ConfSocket: Bad NiIPaddr value in config file: must be legal "
                "IPv4 address!");
  }
  memcpy((char *)&m_config.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  if ((m_sock_config = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("ConfSocket Error: socket()");	  
    EUDAQ_THROW("ConfSocket: Error creating the TCP socket  ");
  } else
    printf("----TCP/NI crate: SOCKET is OK...\n");

  printf("----TCP/NI crate INET ADDRESS is: %s \n", inet_ntoa(m_config.sin_addr));
  printf("----TCP/NI crate INET PORT is: %i \n", port);

  m_config.sin_family = AF_INET;
  m_config.sin_port = htons(port);
  memset(&(m_config.sin_zero), '\0', 8);
  if (connect(m_sock_config, (struct sockaddr *)&m_config,
              sizeof(struct sockaddr)) == -1) {
    perror("ConfSocket Error: connect()");
    EUDAQ_THROW("ConfSocket: National Instruments crate doesn't appear to be running");
  } else
    printf("----TCP/NI crate The CONNECT is OK...\n");
}

void NiController::ConfigClientSocket_Send(const std::string &msg) {  
  int re = send(m_sock_config, msg.data(), msg.length()+1, 0); //include the null end
  if(re==-1){
    perror("Server-send() error lol!");
    EUDAQ_THROW("Server-send() error");
  }

}

void NiController::ConfigClientSocket_Send(const std::vector<unsigned char> &bin) {  
  int re = send(m_sock_config, (char *)(bin.data()), bin.size(), 0);
  if(re==-1){
    perror("Server-send() error lol!");
    EUDAQ_THROW("Server-send() error");
  }
}

void NiController::ConfigClientSocket_Close() {
  NI_CLOSE_SOCKET(m_sock_config);
}

bool NiController::DataTransportClientSocket_Select(){
  fd_set rfds;
  timeval tv;
  int retval;
  FD_ZERO(&rfds);
  FD_SET(m_sock_datatransport, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  retval = select(static_cast<int>(m_sock_datatransport + 1), &rfds, NULL, NULL, &tv);
  if (retval == -1){
    return false;
  }
  else if (retval){
    //printf("Data is available now.\n");
    /* FD_ISSET(0, &rfds) will be true. */
    return true;
  }
  else
    return false;
}

bool NiController::ConfigClientSocket_Select(){
  fd_set rfds;
  timeval tv;
  int retval;
  FD_ZERO(&rfds);
  FD_SET(m_sock_config, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  retval = select(static_cast<int>(m_sock_config + 1), &rfds, NULL, NULL, &tv);
  if (retval == -1){
    return false;
  }
  else if (retval){
    //printf("Data is available now.\n");
    /* FD_ISSET(0, &rfds) will be true. */
    return true;
  }
  else
    return false;
}

unsigned int
NiController::ConfigClientSocket_ReadLength() {
  unsigned int datalengthTmp;
  unsigned int datalength;
  int i;
  bool dbg = false;
  int numbytes;
  if ((numbytes = recv(m_sock_config, m_buffer_length, 2, 0)) == -1) {
    perror("recv()");
    EUDAQ_THROW("DataTransportSocket: Read length error ");
  } else {
    datalengthTmp = 0;
    datalengthTmp = 0xFF & m_buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & m_buffer_length[1];
    datalength = datalengthTmp;
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
  int numbytes;
  while (read_bytes_left > 0) {
    if ((numbytes = recv(m_sock_config, m_buffer_data, read_bytes_left, 0)) == -1) {
      perror("recv()");	    
      EUDAQ_THROW("|==ConfigClientSocket_ReadLength==| Read data error ");
    } else {
      if (dbg)
        printf("|==ConfigClientSocket_ReadLength==|    numbytes=%u \n",
               static_cast<uint32_t>(numbytes));
      read_bytes_left = read_bytes_left - numbytes;
      for (int k = 0; k < numbytes; k++) {
        ConfigData[stored_bytes] = m_buffer_data[k];
        stored_bytes++;
      }
      i = 0;
      if (dbg) {
        while ((int)i < numbytes) {
          printf(" 0x%x \n", 0xFF & m_buffer_data[i]);
          i++;
        }
      }
    }
  }
  if (dbg)
    printf("\n");
  return ConfigData;
}



void NiController::DatatransportClientSocket_Open(const std::string& addr, uint16_t port){
  hostent *host = gethostbyname(addr.c_str());
  if (!host) {
    perror("ConfSocket: Bad NiIPaddr value in config file: must be legal IPv4 "
           "address: ");
    EUDAQ_THROW("ConfSocket: Bad NiIPaddr value in config file: must be legal "
                "IPv4 address!");
  }
  memcpy((char *)&m_datatransport.sin_addr.s_addr, host->h_addr_list[0],
         host->h_length);

  if ((m_sock_datatransport = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("DataTransportSocket Error: socket()");	  
    EUDAQ_THROW("DataTransportSocket: Error creating socket ");
  } else
    printf("----TCP/NI crate DATA TRANSPORT: The SOCKET is OK...\n");

  printf("----TCP/NI crate DATA TRANSPORT INET ADDRESS is: %s \n",
         inet_ntoa(m_datatransport.sin_addr));
  printf("----TCP/NI crate DATA TRANSPORT INET PORT is: %i \n", port);

  m_datatransport.sin_family = AF_INET;
  m_datatransport.sin_port = htons(port);
  memset(&(m_datatransport.sin_zero), '\0', 8);
  if (connect(m_sock_datatransport, (struct sockaddr *)&m_datatransport,
              sizeof(struct sockaddr)) == -1) {
    perror("DataTransportSocket: connect()");
    EUDAQ_THROW("DataTransportSocket: National Instruments crate doesn't "
                "appear to be running  ");
  } else
    printf("----TCP/NI crate DATA TRANSPORT: The CONNECT executed OK...\n");
}

unsigned int
NiController::DataTransportClientSocket_ReadLength() {
  unsigned int datalengthTmp;
  unsigned int datalength;
  int numbytes;
  if ((numbytes = recv(m_sock_datatransport, m_buffer_length, 2, 0)) == -1) {
    perror("recv()");
    EUDAQ_THROW("DataTransportSocket: Read length error ");
  } else {
    datalengthTmp = 0;
    datalengthTmp = 0xFF & m_buffer_length[0];
    datalengthTmp <<= 8;
    datalengthTmp += 0xFF & m_buffer_length[1];
    datalength = datalengthTmp;

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
  int numbytes;
  while (read_bytes_left > 0) {
    if ((numbytes = recv(m_sock_datatransport, m_buffer_data, read_bytes_left, 0)) == -1) {
      perror("recv()");
      EUDAQ_THROW("DataTransportSocket: Read data error ");
    } else {
      if (dbg)
        printf("|==DataTransportClientSocket_ReadData==|    numbytes=%u \n",
               static_cast<uint32_t>(numbytes));
      read_bytes_left = read_bytes_left - numbytes;
      for (int k = 0; k < numbytes; k++) {
        mimosa_data[stored_bytes] = m_buffer_data[k];
        stored_bytes++;
      }
      i = 0;
      if (dbg) {
        while ((int)i < numbytes) {
          printf(" 0x%x%x", 0xFF & m_buffer_data[i], 0xFF & m_buffer_data[i + 1]);
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
  NI_CLOSE_SOCKET(m_sock_datatransport);
}
