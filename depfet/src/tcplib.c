#include "depfet/tcplib.h"
struct sockaddr_in sinn;
struct sockaddr_in pin;
struct hostent *hp=0;
static int	sd=0, sd_current=0;
int sig_int=0, sig_hup=0, sig_alarm=0,sig_pipe=0;
/*====================================================================*/
/*             TCP open                                               */
/*====================================================================*/
int tcp_open(int PORT,char* hostname){
  int rc;
  printf(" tcp_open:: PORT=%d HOST=%s \n",PORT,hostname);
  if (PORT<0) { PORT=-PORT; rc=open_remote(PORT,hostname); }
  else        {             rc=open_local(PORT);  }
  return rc;
}
/*--------------------------------------------------------------------*/
int open_local(int PORT){                      /*--- LISTEN !!! -----*/
  
  printf(" open_local:: PORT=%d \n",PORT);
  /* get an internet domain socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return -1;
  }
  /* complete the socket structure */
  memset(&sinn, 0, sizeof(sinn));
  sinn.sin_family = AF_INET;
  sinn.sin_addr.s_addr = INADDR_ANY;
  sinn.sin_port = htons(PORT);
  
  /* bind the socket to the port number */
  if (bind(sd, (struct sockaddr *) &sinn, sizeof(sinn)) == -1) {
    perror("bind");
    return -1;
  }
 printf("qqqq\n");
  return 0;
}
/*--------------------------------------------------------------------*/
int open_remote(int PORT,char* hostname){   /*----- CONNECT  !!!  ----*/
  if(!sig_hup) printf(" go find out about the desired host machine \n");
  if ((hp = gethostbyname(hostname)) == 0) {
    if(!sig_hup) perror("gethostbyname");
    return -1;
  }
  if(!sig_hup) printf(" name=%s \n",hp->h_name);
  if(!sig_hup) printf(" alias=%s \n",(char*)hp->h_aliases);
  if(!sig_hup) printf(" adr_type=0x%x \n",hp->h_addrtype);
  if(!sig_hup) printf(" adr_len=%d \n",hp->h_length);
  if(!sig_hup) printf(" address=%x \n",(unsigned int)hp->h_addr);
  
  if(!sig_hup) printf(" fill in the socket structure with host information \n");
  
  /* fill in the socket structure with host information */
  memset(&pin, 0, sizeof(pin));
  
  if(!sig_hup) printf(" after memset info \n");
  
  pin.sin_family = AF_INET;
  pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
  pin.sin_port = htons(PORT);

  if(!sig_hup) printf(" grab an Internet domain socket\n ");
  
  if ((sd_current = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
    if(!sig_hup) perror("socket");
     return -1;
  }
  
  if(!sig_hup) printf(" get socket option \n ");
  
  
  if(!sig_hup) printf(" try to connect \n");
  /* connect to PORT on HOST */
  if (connect(sd_current,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
    if(!sig_hup) perror("connect");
    return -1;
  }
  return 0;
}
/*==================================================================*/
/*               LISTEN TCP SOCKET                                  */
/*==================================================================*/
int tcp_listen(void){
  static int 	 addrlen;
  printf("Waiting for replay\n");
  /* show that we are willing to listen */
  if (listen(sd, 5) == -1) {
    perror("listen");
    return -1;
  }
  /* wait for a client to talk to us */
  if ((sd_current = accept(sd, (struct sockaddr *)  &pin, &addrlen)) == -1) {
    perror("accept");
    return -1;
  }
  printf("listen:: Connection from %s \n",inet_ntoa(pin.sin_addr));

  return 0;
}
/*====================================================================*/
/*             TCP send    data                                       */
/*====================================================================*/
int tcp_send(int *DATA,int lenDATA){
  int cc;
  /*-----------------------------------------------------*/
  if ((cc=send(sd_current, DATA,lenDATA, 0)) == -1) {
    if(!sig_hup) perror("send DATA");
    return -1;
  }
  return 0;
}
/*====================================================================*/
/*             TCP receive data                                       */
/*====================================================================*/
int tcp_get(int *DATA,int lenDATA){
  int nleft,nread=0;        
  char *rcv;
      /*-----------------------------------------------------*/
      nleft=lenDATA;
      rcv=(char*)DATA;
      while(nleft>0){
	nread=recv(sd_current,rcv,nleft, 0);
	if (nread <=0) {perror("recv"); return -1;}
	nleft-=nread;
	rcv+=nread;
      }
      return 0; 
}
/*====================================================================*/
int tcp_get2(int *DATA,int lenDATA){
  int nleft,nread=0;        
  char *rcv;
      nleft=lenDATA;
      rcv=(char*)DATA;
      while(nleft>0){
	nread=recv(sd_current,rcv,nleft, 0);
	if (nread <=0) {perror("recv"); return -1;}
	nleft-=nread;
	rcv+=nread;
      }
      return nleft; 
}

/*====================================================================*/
/*             TCP close                                              */
/*====================================================================*/
int tcp_close(int PORT){
    close(sd_current);  
    printf("TCPLIB::close_port sd_current=%d PORT_FLG=%d\n",sd_current,PORT);
    if (PORT==0) { 
	close(sd); 
	printf("TCPLIB::close_port sd=%d PORT_FLG=%d\n",sd,PORT); 
    }
    return 0;
}
/*-----------------------------*/
int tcp_close_old(){
  close(sd);
  printf("TCPLIB::close_old port=%d\n",sd);
  return 0;
}
/*====================================================================*/
/* fork2() - like fork, but the new process is immediately orphaned
 *           (won't leave a zombie when it exits)
 * Returns 1 to the parent, not any meaningful pid.
 * The parent cannot wait() for the new process (it's unrelated).
 */

/* This version assumes that you *haven't* caught or ignored SIGCHLD. */
/* If you have, then you should just be using fork() instead anyway.  */

int fork2()
{
    pid_t pid;
    int status;

    if (!(pid = fork()))
    {
        switch (fork())
        {
          case 0:  return 0;
          case -1: _exit(errno);    /* assumes all errnos are <256 */
          default: _exit(0);
        }
    }

    if (pid < 0 || waitpid(pid,&status,0) < 0)
      return -1;

    if (WIFEXITED(status))
      if (WEXITSTATUS(status) == 0)
        return 1;
      else
        errno = WEXITSTATUS(status);
    else
      errno = EINTR;  /* well, sort of :-) */
    return -1;
}
/*====================================================================*/

