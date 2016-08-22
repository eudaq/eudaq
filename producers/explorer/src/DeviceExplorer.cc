#include "DeviceExplorer.hh"
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
 * Device Explorer Recived the event from a UDP/IP Serve.                                                              *
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		       *
 * The methods for configuring and getting events (Configure, ConfigureDAQ, GetSigngleEvent)                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/

//Costructor
//----------------------------------------------------------------------------------------------
DeviceExplorer::DeviceExplorer()
  : data()
  , sc()
  , cmdRdy(system(NULL))
{
  data[0] = -1;
  data[1] = -1;
  sc[0] = -1;
  sc[1] = -1;
}
//Costructor
//----------------------------------------------------------------------------------------------
DeviceExplorer::~DeviceExplorer()
{
}

//UDP server communincation
//----------------------------------------------------------------------------------------------
int DeviceExplorer::create_server_udp(int fecn, int port /* = PORT */){ //create udp serves for fecs
  int server;
  char ip[14];
  sprintf(ip,"10.0.%d.3",fecn);	//build the IPs
  if((fecn!=0 && fecn!=1) || (port!=6006 && fecn==1) || (port!=6007 && port!=6006 && fecn!=0)){
    perror("\n\n INVALID FEC ID, CAN'T CREATE SERVER\n\n");
    return -1;
  }
  if(port==DATA_PORT){
    server = data[fecn] = socket (AF_INET, SOCK_DGRAM, 0);
  }else if(port==SC_PORT){
    server = sc[fecn] = socket (AF_INET, SOCK_DGRAM, 0);
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

void DeviceExplorer::close_server_udp(int sd){
  close(sd);
}

int DeviceExplorer::get_SD(int fecn){	//get socket descriptors of fecs
  if(fecn==0) return data[0];
  else	if(fecn==1) return data[1];
  else { std::cout<<"Wrong FEC number!"<< std::endl; return -1;}
}

//------------------------------------------------------------------------------------------------
////call python scripts to steer SRS
bool DeviceExplorer::Configure(){
  if(sc[0]>=0) StopDAQ();
  if(cmdRdy){ system("../producers/explorer/srs-software/ConfigureExplorer.sh"); return true; }
  else return false;
}

bool DeviceExplorer::ConfigureDAQ(){
  if(cmdRdy){ system("../producers/explorer/srs-software/ConfigureDAQ.sh"); }
  else return false;
  cmdRdy = false;
  if (create_server_udp(0, SC_PORT) < 0) {
    perror("failed to create the slow control server");
    return false;
  }
  struct sockaddr_in dstAddr;
  bzero(&dstAddr,sizeof(dstAddr));
  dstAddr.sin_family = AF_INET;
  dstAddr.sin_port   = htons((u_short)APP_PORT);
  dstAddr.sin_addr.s_addr = inet_addr("10.0.0.2");
  connect(sc[0],(struct sockaddr *)&dstAddr,sizeof(dstAddr));
  return true;
}

bool DeviceExplorer::StopDAQ(){
  if(sc[0]>=0){
    close_server_udp(sc[0]);
    sc[0] = -1;
    cmdRdy = system(NULL);
  }
  return true;
}

bool DeviceExplorer::GetSingleEvent(){
  if (sc[0]>=0) {
    const unsigned char re[20] = { 0x80, 0x00, 0x00, 0x00,
                                   0xFF, 0xFF, 0xFF, 0xFF,
                                   0xAA, 0xBB, 0xFF, 0xFF,
                                   0x00, 0x00, 0x00, 0x16,
                                   0x00, 0x00, 0x00, 0x06
    }; // rising edge for the event request
    const unsigned char fe[20] = { 0x80, 0x00, 0x00, 0x00,
                                   0xFF, 0xFF, 0xFF, 0xFF,
                                   0xAA, 0xBB, 0xFF, 0xFF,
                                   0x00, 0x00, 0x00, 0x16,
                                   0x00, 0x00, 0x00, 0x04
    }; // falling edge for the event request
    if(send(sc[0], re, sizeof(re),0) < 0) {
      perror("failed sending the rising edge");
      return false;
    }
    if(send(sc[0], fe, sizeof(fe),0) < 0) {
      perror("failed sending the falling edge");
      return false;
    }
    return true;
  }
  else {
    perror("slow control endpoint unavailable");
    return false;
  }
}
