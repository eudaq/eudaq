#include "eudaq/Utils.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include<sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using eudaq::to_string;


// the port client will be connecting to
#define PORT_CONFIG 	49248
#define PORT_DATATRANSF 49250
// max number of bytes we can get at once
#define MAXDATASIZE 300
#define MAXHOSTNAME 80

#define START 	0x1254
#define STOP	0x1255

class NiController {

public:
  NiController();
  void Configure(const eudaq::Configuration & conf);
  virtual ~NiController();
  void GetProduserHostInfo();
  void Start();
  void Stop();
  void TagsSetting();
  void DatatransportClientSocket_Open(const eudaq::Configuration & conf);
  void DatatransportClientSocket_Close();
  unsigned int DataTransportClientSocket_ReadLength(const char string[4]);
  std::vector<unsigned char> DataTransportClientSocket_ReadData(int datalength);

  void ConfigClientSocket_Open(const eudaq::Configuration & conf);
  void ConfigClientSocket_Close();
  void ConfigClientSocket_Send(unsigned char *text, size_t len);
  unsigned int ConfigClientSocket_ReadLength(const char string[4]);
  std::vector<unsigned char> ConfigClientSocket_ReadData(int datalength);



private:
	struct hostent *hclient, *hconfig, *hdatatransport;
	struct sockaddr_in client;
	struct sockaddr_in config;
	struct sockaddr_in datatransport;

	unsigned char conf_parameters[10];

	struct timeval tv;
	unsigned int sec;
	unsigned int microsec;

	char ThisHost[80];

	char Buffer_data[7000];
	char Buffer_length[7000];

	unsigned long data;// = INADDR_NONE;
	unsigned long data_trans_addres;// = INADDR_NONE;
	int sock_config;
	int sock_datatransport;
	int numbytes;

	 //NiIPaddr;
	unsigned TriggerType;
	unsigned Det;
	unsigned Mode;
	unsigned NiVersion;
	unsigned NumBoards;
	unsigned FPGADownload;
	unsigned MimosaID[6];
	unsigned MimosaEn[6];
	bool OneFrame;
};
