#include "depfet/TCPclient.h"
static char HOST[128];
static int  PORT=20248;
#define TCP_CLIENT_LIB 1
//-------------------------------------------------------------------------------------------------------
int tcp_event_host(char *host, int port) {    
  strncpy(HOST,host,128);
  PORT=port;
  printf("==>> TCPclient:: set HOST=%s PORT=%d \n",HOST,PORT);
  return 0;
}
//---------------------------------------------------------------------------------------------------------------
//                      S E N D  
//---------------------------------------------------------------------------------------------------------------
int tcp_event_snd( unsigned int *DATA, int lenDATA,int n,int k, unsigned int evtHDR, unsigned int TriggerID )
{
  static char hostname[100];
  static int  sd;
  static struct sockaddr_in pin;
  struct hostent *hp;
  static int TCP_FLAG,HEADER[10];
  int nleft,nsent;
  static unsigned int time0,time1;
  char* snd;
#ifdef JF_WIN
  WSADATA wsaData;
#endif
// 0=initial -1 -no connection -2 error send 
  if (PORT==0) {  //-- set HOST and PORT to default: localhost:20248
    strncpy(HOST,"localhost",128);
    PORT=20248;
    printf("==> TCPclient:: set DEFAULT!! HOST=%s PORT=%d \n",HOST,PORT);
  }
  if(TCP_FLAG==0) { 

    TCP_FLAG=-2;       
#ifdef JF_WIN	
    if(WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
      printf("WSA konnte nicht initialisiert werden. \n");
    time0=timeGetTime()/1000;
#else
    time((time_t*)&time0);
#endif
    strcpy(hostname,HOST);
    printf("TCPclient:: go find out about the desired host machine \n");
    if ((hp = gethostbyname(hostname)) == 0) {
      perror("gethostbyname");
      return -1;
    }
    printf( "IP=%u.%u.%u.%u \n",
            (unsigned char) hp->h_addr_list[0][0], (unsigned char) hp->h_addr_list[0][1],
            (unsigned char) hp->h_addr_list[0][2], (unsigned char) hp->h_addr_list[0][3]);

    //-------- fill in the socket structure with host information
    memset(&pin, 0, sizeof(pin));
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
    pin.sin_port = htons(PORT);
    //-------- grab an Internet domain socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      return -1;
    }
    printf("TCPclient:: try to connect to %s port=%d\n",hostname,PORT);
    //------- connect to PORT on HOST
    if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
      perror("connect");
      return -1;
    }
    printf("TCPclient:: CONNECTED to %s  port=%d  local_sock=%d port=%d\n",hostname,PORT,sd,ntohs(pin.sin_port));
    TCP_FLAG=1;  //---  server OK, send DATA 
  } else if (TCP_FLAG==-1) { //--- no server (error connect)
#ifdef JF_WIN
    time1=timeGetTime()/1000;;
#else
    time((time_t*)&time1);  //---- timer for retry
#endif
    printf("TCPclient:: t1=%d  t0=%d t1-t0=%d \n",time1,time0,time1-time0);
    if ((time1-time0) > T_WAIT ) { time0=time1;
      printf("TCPclient:: %d RE-try to connect to %s\n",time1-time0,hostname);
      TCP_FLAG=0; 
    };
    return -1;
  } else if (TCP_FLAG==-2) { //--- server disappeared  (error send)
    printf("TCPclient:: close socket... %s\n",hostname);
#ifdef JF_WIN
    closesocket(sd);
    time0=timeGetTime()/1000;;
#else
    close(sd); 
    time((time_t*)&time0);
#endif
    TCP_FLAG=-1; 
    return -1;
  };
   
  //--------------------------------------------------
  //------------   ENDIF TCP_FLAG  -------------------
  //--------------------------------------------------

  HEADER[0]=0x5;  //---  buffered for evb  
  HEADER[1]=0xAABBCCDD;
  HEADER[2]=lenDATA;
  HEADER[3]=evtHDR;
  HEADER[4]=TriggerID;
  HEADER[5]=n;
  HEADER[6]=k;

#ifdef JF_DEBUG
  printf("send HEADER size=%d\n",sizeof(HEADER));
#endif
   
  if (send(sd, (char*) HEADER, sizeof(HEADER), 0) == -1) {
    perror("send"); TCP_FLAG=-2;
    return -1; 
  }
#ifdef JF_DEBUG
  printf("send DATA lenDATA=%d bytes (%d words)\n",lenDATA*4,lenDATA);
#endif
   
  nleft=lenDATA*4;
  snd=(char*)DATA;
  while(nleft>0){
    if(DEBUG>3) printf("try to send = %d of %d\n",PACKSIZE,nleft);
    if (nleft<PACKSIZE) nsent=send(sd,snd, nleft, 0);
    else                nsent=send(sd,snd, PACKSIZE, 0);
    //printf("sent DATA\n");
    if (nsent <=0) { perror("send"); TCP_FLAG=-2; return -1;}
    nleft-=nsent;
    snd+=nsent;
  }
  return nleft; 
}
//---------------------------------------------------------------------------------------------------------------
//                      G E T 
//---------------------------------------------------------------------------------------------------------------
int tcp_event_get(char *HOST2, unsigned int *DATA, int *lenDATA, int *Nr_Modules ,int *ModuleID, unsigned int *TriggerID )
{
  static char hostname[100];
  static int  sd;
  static struct sockaddr_in pin;
  struct hostent *hp;
  static int TCP_FLAG,HEADER[10];
  int nleft,nread;
  static unsigned int time0,time1;
  char  *rcv;
  static unsigned int MARKER,REQUEST,REQUESTED;
  int evtTrigID, evtModID, evtSize;


#ifdef JF_WIN
  WSADATA wsaData;
#endif
// 0=initial -1 -no connection -2 error send 
  if(TCP_FLAG==0) { 

    TCP_FLAG=-2;       
#ifdef JF_WIN	
    if(WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
      printf("WSA konnte nicht initialisiert werden. \n");
    time0=timeGetTime()/1000;
#else
    time((time_t*)&time0);
#endif
    strcpy(hostname,HOST2);
    printf("TCPclient:: go find out about the desired host machine \n");
    if ((hp = gethostbyname(hostname)) == 0) {
      perror("gethostbyname");
      return -1;
    }
    printf( "IP=%u.%u.%u.%u \n",
            (unsigned char) hp->h_addr_list[0][0], (unsigned char) hp->h_addr_list[0][1],
            (unsigned char) hp->h_addr_list[0][2], (unsigned char) hp->h_addr_list[0][3]);

    //-------- fill in the socket structure with host information
    memset(&pin, 0, sizeof(pin));
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
    pin.sin_port = htons(PORT);
    //-------- grab an Internet domain socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      return -1;
    }
    printf("TCPclient:: try to connect to %s port=%d\n",hostname,PORT);
    //------- connect to PORT on HOST
    if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
      perror("connect");
      return -1;
    }
    printf("TCPclient:: CONNECTED to %s  port=%d  local_sock=%d\n",hostname,PORT,sd);
    TCP_FLAG=1;  //---  server OK, send DATA 
  } else if (TCP_FLAG==-1) { //--- no server (error connect)
#ifdef JF_WIN
    time1=timeGetTime()/1000;;
#else
    time((time_t*)&time1);  //---- timer for retry
#endif
    printf("TCPclient:: t1=%d  t0=%d t1-t0=%d wait %d sec...\n",time1,time0,time1-time0,T_WAIT-(time1-time0));
    if ((time1-time0) > T_WAIT ) { time0=time1;
      printf("TCPclient:: %d RE-try to connect to %s\n",time1-time0,hostname);
      TCP_FLAG=0; 
    };
    return -1;
  } else if (TCP_FLAG==-2) { //--- server disappeared  (error send)
    printf("TCPclient:: close socket... %s\n",hostname);
#ifdef JF_WIN
    closesocket(sd);
    time0=timeGetTime()/1000;;
#else
    close(sd); 
    time((time_t*)&time0);
#endif
    REQUESTED=0;
    TCP_FLAG=-1; 
    return -1;
  };
   
  //--------------------------------------------------
  //------------   ENDIF TCP_FLAG  -------------------
  //--------------------------------------------------

  REQUEST=*Nr_Modules;
  //printf("REQUEST=%d REQUESTED=%d\n",REQUEST,REQUESTED);
   
  if (REQUESTED==0 && REQUEST>0x1000) {
    if (REQUEST==0x1002) {
      HEADER[0]=0x2;  //-- request for 1 event --
      REQUESTED=0;
    } else if (REQUEST==0x1003) {
      HEADER[0]=0x3;  //-- request continuous events --
      REQUESTED=1;
    } else if (REQUEST==0x1013) {
      HEADER[0]=0x13;  //-- request continuous exclusive events --
      REQUESTED=1;
    } else {
      HEADER[0]=0x2;  //-- request for 1 event default --
      REQUESTED=0;
    } 
       
    HEADER[1]=0xAABBCCDD;
    HEADER[2]=*lenDATA;
    HEADER[3]=0;
    HEADER[4]=0;
    HEADER[5]=*Nr_Modules;
    HEADER[6]=*ModuleID;
       
    //printf("ask DATA max lenDATA=%d\n",*lenDATA);
       
    if (send(sd, (char*) HEADER, sizeof(HEADER), 0) == -1) {
      perror("send"); TCP_FLAG=-2;
      return -1; 
    }
  }
  //-----------------------------------------------------

  //-- recv HEADER ---

  nread=0;        
  nleft=sizeof(HEADER);
  rcv=(char*)HEADER;
  while(nleft>0){
    nread=recv(sd,rcv,nleft, 0);
    if (nread <=0) {perror("recv_header"); TCP_FLAG=-2; return -1;}
    nleft-=nread;
    rcv+=nread;
  }
  MARKER= HEADER[1];
  *lenDATA=HEADER[2];
  *TriggerID=HEADER[4];
  *Nr_Modules=HEADER[5];
  *ModuleID=HEADER[6];
   
  //printf("HEADER OK=%#X, Nr_MOD=%d, ModID=%d, lenDATA=%d\n",MARKER,*Nr_Modules,*ModuleID,*lenDATA);
   
  //-- recv DATA ---
   
  nleft=*lenDATA*4;
  rcv=(char*)DATA;
  while(nleft>0){
    nread=recv(sd,rcv, nleft, 0);
    if (nread <=0) { perror("recv_data"); TCP_FLAG=-2; return -1;}
    nleft-=nread;
    rcv+=nread;
    //printf("DATA recv=%d  nleft=%d\n",nread,nleft);
  }

  evtTrigID=DATA[1];
  evtModID=(DATA[0]>>24)&0xf;
  evtSize=DATA[0]&0xfffff;

  printf("OK, TCP recv=%d  nleft=%d Trg=%d(%d) Mod=%d siz=%d\n",nread,nleft,evtTrigID,*TriggerID,evtModID,evtSize);

  return 0; 
}
//============================================================================

