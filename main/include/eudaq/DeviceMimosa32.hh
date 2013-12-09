#include <stdio.h>
#include <stdlib.h>
#include <string.h> // string function definitions

#ifdef WIN32

#include <winsock2.h>
#include <WS2tcpip.h>
#include <io.h>

#else

#include <unistd.h> // UNIX standard function definitions
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h> // POSIX terminal control definitionss
#include <arpa/inet.h>

#endif

#include <sys/types.h>
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <time.h>   // time calls
#include <math.h>
class DeviceMimosa32
{
private:
        struct sockaddr_in fClientAddr; /* indirizzo del client che si collega al server*/	
	int fNumChip;   
 // Network  
        int fSD; //Socket Descriptor
	char * fIp;  // Indirizzo IP
	int fPort;   // Porta
//Dati	
//	char fCDH[32];      // cdh 8 word da 4 byte
//	char fDH[16];       // Data Header
//        char fData[2560];   // Data
//	char fAllData[2608]; // Tutti i dati cdh + dh + data
	
public:
	DeviceMimosa32();               // Costruttore.
	virtual ~DeviceMimosa32();      // Distruttore.
	int  create_client_tcp(int port,char *ip);        // Creazione del socket client per la comunicazione con il dispositivo.
	int  connect_client_tcp(int sd); // Connessione del socket client con il dispositivo.
	void close_client_tcp(int sd);  // Chiusura del socket client.
	int  read_file_cfg();            // Lettura del file di configurazione.
        int  readout_event(int num_word_event, FILE *fp, char *fCDH, char *fDH, char *fData);// Lettura dei dati dal dispositivo.
	void print_data(char *data, int pos_end, FILE *fp); // Stampa dei dati su file e a video.
	int  get_num_word_event(); // Numero di word in un evento.
	int  from_carachter_to_word(char *buffer, char *word);
	int  send_instruction(char *word);
	int  cmdStartRun();
	int  cmdStopRun();
	int  get_num_chip();
	int  get_port();
	void get_ip(char *ip);
	
};
