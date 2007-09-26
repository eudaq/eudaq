#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
/*=====================================================================*/
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;
extern int sig_int, sig_hup, sig_alarm, sig_pipe;

#define PORT_OLD 		20248
             /* REPLACE with your server machine name*/
#define HOST_OLD        "silab18"
#define ADDR_OLD        " "

#define DIRSIZE     8192


typedef struct {
  char zerro[4];
  char header[4];
  int  chan;
  int  lenbuf;
  int  databuf[65000];
} 
tcpbuf;


int tcp_open(int PORT,char* HOSTNAME);
int open_remote(int PORT,char* HOSTNAME);
int open_local(int PORT);
int tcp_listen(void);
int tcp_close(int PORT);
int tcp_close_old();
int tcp_send(int *DATA,int lenDATA);
int read_event(FILE *fp0,int *Time1,int *Time2,int *ADC1,int *ADC2);
int tcp_get(int *DATA,int lenDATA);
int tcp_get2(int *DATA,int lenDATA);
int fork2();

#ifdef __cplusplus
}
#endif
