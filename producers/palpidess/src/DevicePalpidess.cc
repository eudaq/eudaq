#include "DevicePalpidess.hh"
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
#include <arpa/inet.h>
#include <iostream>
#include <fstream>

/***********************************************************************************************************************
 *                                                                                                                     *
 * Device Palpidess Recived the event from a UDP/IP Serve.                                                              *
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		       *
 * The methods for configuring and getting events (Configure, ConfigureDAQ, GetSigngleEvent)                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/

using namespace std;
//Costructor
//----------------------------------------------------------------------------------------------
DevicePalpidess::DevicePalpidess()
  : data(-1)
  , sc(-1)
  , cmdRdy(system(NULL))
{
}
//Costructor
//----------------------------------------------------------------------------------------------
DevicePalpidess::~DevicePalpidess()
{
}

//UDP server communincation
//----------------------------------------------------------------------------------------------
int DevicePalpidess::create_server_udp(int port /* = PORT */){ //create udp serves for fecs
  int server = -1;
  const char ip[] = "10.0.0.3";
  if(port==DATA_PORT){
    server = data = socket (AF_INET, SOCK_DGRAM, 0);
  }else if(port==SC_PORT){
    server = sc = socket (AF_INET, SOCK_DGRAM, 0);
  }else{
    server = -1;
  }

  if (server < 0){
    perror("\n\n ERROR CREATING SOCKET: \n\n");
  }
  else printf ("\n\nCreated socket UDP .................... OK\n\n");

  struct sockaddr_in srvAddr;
  bzero(&srvAddr,sizeof(srvAddr));
  srvAddr.sin_family = AF_INET;
  srvAddr.sin_port = htons((u_short)port);
  srvAddr.sin_addr.s_addr = inet_addr(ip);

  if(bind(server,(struct sockaddr *)&srvAddr,sizeof(srvAddr))<0) 	//bind adress
    perror ( " ERROR BINDING: " );
  else
    printf ("Bind Socket ........................... OK\n\n");

  return server;
}

void DevicePalpidess::close_server_udp(int sd){
  close(sd);
}

int DevicePalpidess::get_SD(){	//get socket descriptors of fecs
  return data;
}

//------------------------------------------------------------------------------------------------
////call python scripts to steer SRS
bool DevicePalpidess::Configure(std::string arg){
  if(sc>=0) StopDAQ();
  std::string cmd = "../producers/palpidess/srs-software/ConfigurePalpidess.sh" + arg;
  if(cmdRdy){ system(cmd.c_str()); return true; }
  else return false;
}

bool DevicePalpidess::ConfigureDAQ(){
  if(cmdRdy){ system("../producers/palpidess/srs-software/ConfigureDAQ.sh"); }
  else return false;
  cmdRdy = false;
  if (create_server_udp(SC_PORT) < 0) {
    perror("failed to create the slow control server");
    return false;
  }
  struct sockaddr_in dstAddr;
  bzero(&dstAddr,sizeof(dstAddr));
  dstAddr.sin_family = AF_INET;
  dstAddr.sin_port   = htons((u_short)APP_PORT);
  dstAddr.sin_addr.s_addr = inet_addr("10.0.0.2");
  connect(sc,(struct sockaddr *)&dstAddr,sizeof(dstAddr));
  return true;
}

bool DevicePalpidess::StopDAQ(){
  if(sc>=0){
    close_server_udp(sc);
    sc = -1;
    cmdRdy = system(NULL);
  }
  return true;
}

bool DevicePalpidess::GetSingleEvent(){
  if (sc>=0) {
    const unsigned char re[20] = { 0x80, 0x00, 0x00, 0x00,
                                   0xFF, 0xFF, 0xFF, 0xFF,
                                   0xAA, 0xBB, 0xFF, 0xFF,
                                   0x00, 0x00, 0x00, 0x16,
                                   0x00, 0x00, 0x00, 0x06
    }; // rising edge for the event request
    if(send(data, re, sizeof(re),0) < 0) {
      perror("failed sending the rising edge");
      return false;
    }
    return true;
  }
  else {
    perror("slow control endpoint unavailable");
    return false;
  }
}
