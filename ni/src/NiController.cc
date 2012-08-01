#include "NiController.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

unsigned char start[5] = "star";
unsigned char stop[5] = "stop";


NiController::NiController() {
	//NI_IP = "192.76.172.199";
}
void NiController::Configure(const eudaq::Configuration & param) {
	//NiIPaddr = param.Get("NiIPaddr", "");

}
void NiController::TagsSetting(){
	//ev.SetTag("DET", 12);
}
void NiController::GetProduserHostInfo(){
	/*** get Producer information, NAME and INET ADDRESS ***/
	gethostname(ThisHost, MAXHOSTNAME);
	printf("----TCP/Producer running at host NAME: %s\n", ThisHost);
	hclient = gethostbyname(ThisHost);       
        if( hclient != 0 )
        {
	  bcopy ( hclient->h_addr, &(client.sin_addr), hclient->h_length);
  	  printf("----TCP/Producer INET ADDRESS is: %s \n", inet_ntoa(client.sin_addr));
        }
        else
        {
	  printf("----TCP/Producer -- Warning! -- failed attempting to gethostbyname() for: %s. Check your /etc/hosts list \n", ThisHost );        
        }
}
void NiController::Start(){
	ConfigClientSocket_Send(start, sizeof(start) );
}
void NiController::Stop(){
	ConfigClientSocket_Send(stop, sizeof(stop) );
}

void NiController::ConfigClientSocket_Open(const eudaq::Configuration & param){
	/*** Network configuration for NI, NAME and INET ADDRESS ***/

	std::string m_server;
	m_server = param.Get("NiIPaddr", "");
	data= inet_addr(m_server.c_str());

	hconfig = gethostbyaddr(&data, 4, AF_INET);
	if ( hconfig == NULL) {
		EUDAQ_ERROR("Config. Socket: get HOST error  " );
		perror("Config. Socket: gethostbyname()");
		exit(1);
	} else{
		bcopy ( hconfig->h_addr, &(config.sin_addr), hconfig->h_length);
		printf("----TCP/NI crate INET ADDRESS is: %s \n", inet_ntoa(config.sin_addr));
		printf("----TCP/NI crate INET PORT is: %d \n", PORT_CONFIG );
	}
	if ((sock_config = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		EUDAQ_ERROR("ConfSocket: Create socket error  " );
		perror("Config. Socket: socket()");
		exit(1);
	} else
		printf("----TCP/NI crate The SOCKET is OK...\n");

	config.sin_family = AF_INET;
	config.sin_port = htons(PORT_CONFIG);
	memset(&(config.sin_zero), '\0', 8);
	if (connect(sock_config, (struct sockaddr *) &config, sizeof(struct sockaddr)) == -1) {
		EUDAQ_ERROR("ConfSocket: National Instruments crate doesn't running  " );
		perror("Config. Socket: connect()");
		sleep(5);
		exit(1);
	} else
		printf("----TCP/NI crate The CONNECT is OK...\n");
}
void NiController::ConfigClientSocket_Send(unsigned char *text, size_t len){
	bool dbg = false;
	if (dbg) printf("size=%d ", len);
	if (send(sock_config, text, len, 0) == -1) 	perror("Server-send() error lol!");
}
void NiController::ConfigClientSocket_Close(){
	close(sock_config);
}
unsigned int NiController::ConfigClientSocket_ReadLength(const char string[4]){
	unsigned int datalengthTmp;
	unsigned int datalength;
	int i;
	bool dbg =false;
	if ((numbytes = recv(sock_config, Buffer_length, 2, 0)) == -1) {
		EUDAQ_ERROR("DataTransportSocket: Read length error " );
		perror("recv()");
		exit(1);
	}
	else {
		if (dbg)printf("|==ConfigClientSocket_ReadLength ==|    numbytes=%d \n", numbytes);
		i=0;
		if (dbg){
			while (i<numbytes){
				printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i+1]);
				i=i+2;
			}
		}
		datalengthTmp = 0;
		datalengthTmp = 0xFF & Buffer_length[0];
		datalengthTmp <<= 8;
		datalengthTmp += 0xFF & Buffer_length[1];
		datalength = datalengthTmp;

		if (dbg) printf(" data= %d", datalength);
		if (dbg) printf("\n");
	}
	return datalength;
}
std::vector<unsigned char> NiController::ConfigClientSocket_ReadData(int datalength){
	std::vector<unsigned char> ConfigData(datalength);
	unsigned int stored_bytes;
	unsigned int read_bytes_left;
	unsigned int i;
	bool dbg =false;

	stored_bytes = 0;
	read_bytes_left = datalength;
	while (read_bytes_left > 0 ) {
		if ((numbytes = recv(sock_config, Buffer_data, read_bytes_left, 0)) == -1) {
			EUDAQ_ERROR("|==ConfigClientSocket_ReadLength==| Read data error " );
			perror("recv()");
			exit(1);
		}
		else {
			if (dbg) printf("|==ConfigClientSocket_ReadLength==|    numbytes=%d \n", numbytes);
			read_bytes_left = read_bytes_left - numbytes;
			for (int k=0; k< numbytes; k++){
				ConfigData[stored_bytes] = Buffer_data[k];
				stored_bytes++;
			}
			i=0;
			if (dbg){
				while (i<numbytes){
					printf(" 0x%x \n", 0xFF & Buffer_data[i]);
					i++;
				}
			}
		}
	}
	if (dbg) printf("\n");
	return ConfigData;
}

void NiController::DatatransportClientSocket_Open(const eudaq::Configuration & param){
	/*** Creation for the data transmit socket, NAME and INET ADDRESS ***/
	std::string m_server;
	m_server = param.Get("NiIPaddr", "");
	data_trans_addres= inet_addr(m_server.c_str());
	hdatatransport = gethostbyaddr(&data_trans_addres, 4, AF_INET);
	if ( hdatatransport == NULL) {
		EUDAQ_ERROR("DataTransportSocket: get HOST error " );
		perror("DataTransportSocket: gethostbyname()");
		exit(1);
	} else{
		bcopy ( hdatatransport->h_addr, &(datatransport.sin_addr), hdatatransport->h_length);
		printf("----TCP/NI crate DATA TRANSPORT INET ADDRESS is: %s \n", inet_ntoa(datatransport.sin_addr));
		printf("----TCP/NI crate DATA TRANSPORT INET PORT is: %d \n", PORT_DATATRANSF );
	}
	if ((sock_datatransport = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		EUDAQ_ERROR("DataTransportSocket: create socket error " );
		perror("DataTransportSocket: socket()");
		exit(1);
	} else
		printf("----TCP/NI crate DATA TRANSPORT: The SOCKET is OK...\n");
	//sleep (5);
	datatransport.sin_family = AF_INET;
	datatransport.sin_port = htons(PORT_DATATRANSF);
	memset(&(datatransport.sin_zero), '\0', 8);
	if (connect(sock_datatransport, (struct sockaddr *) &datatransport, sizeof(struct sockaddr)) == -1) {
		EUDAQ_ERROR("DataTransportSocket: National Instruments crate doesn't running  " );
		perror("DataTransportSocket: connect()");
		exit(1);
	} else
		printf("----TCP/NI crate DATA TRANSPORT: The CONNECT is OK...\n");
}
unsigned int NiController::DataTransportClientSocket_ReadLength(const char string[4]) {
	unsigned int datalengthTmp;
	unsigned int datalength;
	int i;
	bool dbg =false;
	if ((numbytes = recv(sock_datatransport, Buffer_length, 2, 0)) == -1) {
		EUDAQ_ERROR("DataTransportSocket: Read length error " );
		perror("recv()");
		exit(1);
	}
	else {
		if (dbg)printf("|==DataTransportClientSocket_ReadLength ==|    numbytes=%d \n", numbytes);
		i=0;
		if (dbg){
			while (i<numbytes){
				printf(" 0x%x%x", 0xFF & Buffer_length[i], 0xFF & Buffer_length[i+1]);
				i=i+2;
			}
		}
		datalengthTmp = 0;
		datalengthTmp = 0xFF & Buffer_length[0];
		datalengthTmp <<= 8;
		datalengthTmp += 0xFF & Buffer_length[1];
		datalength = datalengthTmp;

		if (dbg) printf(" data= %d", datalength);
		if (dbg) printf("\n");
	}
	return datalength;
}
std::vector<unsigned char> NiController::DataTransportClientSocket_ReadData(int datalength) {

	std::vector<unsigned char> mimosa_data(datalength);
	unsigned int stored_bytes;
	unsigned int read_bytes_left;
	unsigned int i;
	bool dbg =false;

	stored_bytes = 0;
	read_bytes_left = datalength;
	while (read_bytes_left > 0 ) {
		if ((numbytes = recv(sock_datatransport, Buffer_data, read_bytes_left, 0)) == -1) {
			EUDAQ_ERROR("DataTransportSocket: Read data error " );
			perror("recv()");
			exit(1);
		}
		else {
			if (dbg) printf("|==DataTransportClientSocket_ReadData==|    numbytes=%d \n", numbytes);
			read_bytes_left = read_bytes_left - numbytes;
			for (int k=0; k< numbytes; k++){
				mimosa_data[stored_bytes] = Buffer_data[k];
				stored_bytes++;
			}
			i=0;
			if (dbg){
				while (i<numbytes){
					printf(" 0x%x%x", 0xFF & Buffer_data[i], 0xFF & Buffer_data[i+1]);
					i=i+2;
				}
			}
		}
	}
	if (dbg) printf("\n");
	return mimosa_data;
}
void NiController::DatatransportClientSocket_Close(){
	close(sock_datatransport);
}
NiController::~NiController() {
	//
}
