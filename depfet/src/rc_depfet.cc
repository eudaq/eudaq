#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include "depfet/rc_depfet.hh"


void process_msg (char *buf, char **tk, int *ntk, int max_tok );
int field_cmd(char *msg);
int TCP_CONNECT(char *HOST, int PORT);
int STREQ(char*s1,const char*s2) { if (strncasecmp(s1,s2,strlen(s2))) return 0; else return 1; }

//-------------------------------------------------------------
//             Global VARs
//-------------------------------------------------------------
struct HOST  cmd_host;
int cmd_socket=0;

//----------------------------------------------------------------------
pthread_t  rc_thread;
int run_control (struct HOST* host);
static void *run_control_thread (void* arg) {
  struct HOST *host = (struct HOST*) arg;
  run_control (host);
  return 0;
}
/*=====================================================================*/

//-------------------------------------------------------------------   
//             SET value for hosts/ports  
//-------------------------------------------------------------------

int set_host(char *host, int port) {
    
  if(host==NULL) 
    strncpy(cmd_host.NAME,"localhost",LHOST);
  else
    strncpy(cmd_host.NAME,host,strlen(host));
  if (!port) 
    cmd_host.PORT=32767;
  else
    cmd_host.PORT=port;

  printf(" CMD_HOST=%s PORT=%d\n",cmd_host.NAME,cmd_host.PORT); 

  cmd_host.sock=0;
  printf("Start run_control client thread \n");
  int ret = pthread_create (&rc_thread, 0, run_control_thread, &cmd_host);
  if (ret) {
    perror ("pthread_create");
  }
/*
  void *retval;
  if (pthread_join (rc_thread, &retval))
  perror ("pthread_join");
*/

  return 0;
}
//-------------------------------------------------------------
//static void *run_control (void* arg) {
int run_control (struct HOST* host) {

  char *Buffer;
  int Result, Bytes=1024;

  //-------------------------------------------------------------------
  //           TCP   connection   init
  //-------------------------------------------------------------------
  printf("run_control()::try to connect to  host=%s port=%d\n",host->NAME,host->PORT);
  cmd_socket=TCP_CONNECT(host->NAME, host->PORT);
  host->sock=cmd_socket;
  printf("run_control():: Connected to cmd host sock=%d  !!!\n",cmd_socket);
  //return 0;
  //-------------------------------------------------------------------
  //           TCP   connection  loop   
  //-------------------------------------------------------------------
  Buffer = (char *)malloc(Bytes);
  if (Buffer == 0x0)
    {
      printf("run_control()::could not malloc buffer of byte length %d\n",Bytes);
      exit (102);
    }
  while (1)
    {   memset(Buffer, 0, Bytes);  
      //printf("run_control():: new recv...sock=%d\n",cmd_socket);
      if ((Result=recv(cmd_socket, (void *)Buffer, Bytes, 0)) < 0)
        {  perror ("run_control()::Can't recv");   break;
        }  else if (Result == 0) { printf ("EOF\n"); 
        close(cmd_socket);
        cmd_socket=TCP_CONNECT(host->NAME, host->PORT); 
        host->sock=cmd_socket;
      }
	
      printf("%s",Buffer);
      //printf("run_control():: get string = %s\n",Buffer);
      //for (int i=0;i<Result;i++) { printf(" %02x ",Buffer[i]); };

      if (!strncmp(Buffer,"CMD_EVB_TEST",7))  { printf("EVB run_control():: \n");  }
      else { if (field_cmd(Buffer)<0) break; }
    }
  close(cmd_socket);
  printf("run_control():: EXIT\n");
  return 0;
}

//------------------------------------------
int TCP_CONNECT(char *HOST, int PORT) {
  int Socket,ret;
  struct hostent *hp;
  static char hostname[128];
  struct sockaddr_in Addr;
  char INFO[]="RCM INFO EVB IDLE ONLINE";
  Addr.sin_family = AF_INET;
  Addr.sin_port = htons(PORT);
 
  strcpy(hostname,HOST);
  printf("TCP_CONNECT:: go find out about the desired host machine:%s \n",hostname);
  if ((hp = gethostbyname(hostname)) == 0) {
    perror("gethostbyname");
    return -1;
  };
  Addr.sin_addr.s_addr=((struct in_addr *)(hp->h_addr))->s_addr;
  /*  if (inet_aton(HOST, &Addr.sin_addr) == 0x0) {
      printf("could not get ip address for %s\n", HOST); exit (101);
      }  */

  if ((Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf ("TCP_CONNECT():: Can't open datagram socket");  exit (106);
  }
  while(connect(Socket, (struct sockaddr *)&Addr, sizeof(Addr)) < 0) {
    printf ("TCP_CONNECT()::Can't connect to %s %d  wait 10 sec ...\n",HOST,PORT);
    sleep(10);
  } 
  printf ("TCP_CONNECT()::Connected to host=%s port=%d socket=%d INFO:: %s\n",HOST,PORT,Socket,INFO);
  if ((ret=send(Socket, (void *)INFO, strlen(INFO), 0))!=(int)strlen(INFO)) {
    printf("Error Send INFO ret=%d of (%d)\n",ret,(int)strlen(INFO)); perror("send to RC");
  }    
  return Socket;
}
//------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------

void process_msg (char *buf, char **tk, int *ntk,int MAX_TOKENS) {
  int n = 0, len;
  char *ptr;
  *ntk=0;
  ptr=buf;
  len=strlen(buf);
  for (int i=0;i<len;i++) { 
    loop:
    if (ptr[i]=='\n' || ptr[i]=='\r'|| ptr[i]=='\t') ptr[i]=0x20;
    if (i>0 && len>=i && ptr[i]==0x20 && ptr[i-1]==0x20) { 
      memcpy(&ptr[i-1],&ptr[i],len-i); ptr[len-1]=0; len--; 
      goto loop;
    }
  };  if (ptr[len-1]==0x20) ptr[len-1]=0;

  len=strlen(buf); 
  for ( n = 0 ; n < MAX_TOKENS ; n++ ) {
    tk[n] = NULL;
    tk[n] = strsep(&buf, " \t\n\r");
    if ( tk[n] == NULL ) break;
  }
  *ntk = n;
}

/********************************************************************************/
/**          f i e l d _ c m d _ 2                                             **/
/********************************************************************************/
int field_cmd(char *msg) {
  int ntk,ret;
#define MAX_TOKENS 10
  char *tk[MAX_TOKENS];
  char ss[256],/*command[256],*/sndstr[1024];

  memset(ss, 0, sizeof(ss));        /*    get tokens   */
  strncpy(ss, msg,sizeof(ss));
  process_msg(ss, tk, &ntk,MAX_TOKENS);
  if (ntk < 2) {
    printf ("rc_depfet():: Error: wrong message format (%d tokens)\n", ntk);
    return 1;
  }
  /*------------------------------------------------------------------------------------------*/
  /*  ** If it CMD prefixed command ?  */
  if (!STREQ(tk[0],"CMD")) {
    //printf("EVB::rc_client():: Unknown command ignored: %s \n",msg);
    return 1; /* Unknown command ignored */ 
  }   
  /*------------------------------------------------------------------------------------------*/
  if(STREQ(tk[1],"HELP")) {  
    //printf(" >>>>>>>>>>>>>>>>>>>>> RCM HELP  <<<<<<<<<<<<<<<<<<<\n");
    //printf("=================> sizeof(sndstr)=%d\n",sizeof(sndstr));
    memset(sndstr, 0, sizeof(sndstr));
    sprintf(sndstr                 ,"----------->  RC CDAQ  <--------------\n");
    sprintf(&sndstr[strlen(sndstr)]," CMD EVB FILE [on/off] [format] \n");

    ret=send(cmd_socket, (void *)sndstr, strlen(sndstr), 0);
  }
  /*------------------------------------------------------------------------------------------*/
  if(STREQ(tk[1],"SETUP") || STREQ(tk[1],"INIT")) {  /* this is ACTIVATE */
    //printf("field_cmd_2():: RCM SETUP\n");
  }
  /*------------------------------------------------------------------------------------------*/
  if(STREQ(tk[1],"EXIT")) {  /* this is EXIT */
    printf("EVB::rc_client():: %s\n",msg);
    //return -1;
  }

  /*------------------------------------------------------------------------------------------*/
  return  0;
}

//==================================================================================================
//                        SEND  Commands
//==================================================================================================



int cmd_send(const std::string & command) {
  int ret;

  while (cmd_socket<=0) { printf("cmd_send():: Wait for connection sock=%d\n",cmd_socket); sleep(1);}

  if ((ret=send(cmd_socket, (void *)command.c_str(),command.length(), 0))!=(int)command.length()) {
    printf("Error Send INFO ret=%d of (%d)\n",ret,(int)command.length()); perror("send to RC");
  }    
  printf("cmd_send():: cmd=%s",command.c_str());

  printf("to do: at  this point : wait for replay and analize \n");

  return 0;
}
