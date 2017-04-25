//#define JF_WIN 1

  #include  <stdio.h>
  #include <string.h>
#ifdef JF_WIN
  #include <windows.h>
  #include <winsock.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <errno.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <signal.h>
  #include <stdlib.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <sys/wait.h>
  #include <time.h>

#endif


#define PACKSIZE    65535
#define DEBUG   2
#define T_WAIT  10

// REPLACE with your server machine name
//#define HOST        "atlas-konf-156"
//#define HOST        "silab18"
//#define HOST        "137.138.65.180"
//#define HOST        "137.138.65.34"
//#define HOST        "131.220.165.146"
//#define HOST        "localhost"
//#define PORT        20248
#define DIRSIZE     8192
#define SDDIRSIZE     16384



int tcp_event_get(char *HOST, unsigned int *DATA, int *lenDATA, int *Nr_Mod ,int *ModuleID, unsigned int *Trig);
int tcp_event_snd(unsigned int *DATA, int lenDATA ,int n, int i, unsigned int evtHDR, unsigned int Trig);
int tcp_event_host(char *host, int port);


