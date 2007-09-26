/*=====================================================================*/
/*          TCP/IP server (read information from TCP and stored it     */
/*             to the shared memory                                    */
/*          Author: Julia Furletova                                    */
/*               (julia@mail.desy.de, furletova@physik.uni-bonn.de)    */
/*          Created:  13-sept-2007                                     */
/*          Modified:                                                  */
/*          Modification:                                              */
/*=====================================================================*/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "depfet/shmem_depfet.h"
#include "depfet/tcplib.h"
/*=====================================================================*/

/*----------------------TCP--------------------------------------------*/
int PORT=0;
tcpbuf MBUFF;
#define TCPBUFF ((char *)&MBUFF)
#define TCPBUFF4 ((int *)&MBUFF)
#define  DEV_N 7

#define  MAX_RATEAV 10   /* 2 sec average => 2 sec : 0.2 sec = 10 rates*/

#define MAX_MxRMS 600 /* MAXIMUM 2 min average for RMSmean */
#define MAX_RATEAVRMS 300

int rc=0;

/*=====================================================================*/
/*             f u n c t i o n s                                       */
/*=====================================================================*/
int run_child();
int fork2();
int usage(char* name);
static unsigned long  swap4(unsigned long x);

void ctrl_c(int m) { sig_int=1;
   printf("\n CTRL-C pressed...\n"); 
}
void falarm(int m) { sig_alarm=1; printf(" *** ALARM *** "); }
void hungup(int m) { sig_hup++;   printf("\n Signal HUP received \n\r");}
void fpipe(int m)  { sig_pipe=1;   printf("\n Signal PIPE received \n\r");}
void sys_err(char *msg)
{ puts(msg);
  exit(1);
}
/*=====================================================================*/
static evt_Builder *event_ptr=0;
static int TCP_FLAG=0;
static int	semid=0;
static int 	shmid=0;
/*=====================================================================*/
/* closeall() - close all FDs >= a specified value */

void closeall(int fd)
{
  int fdlimit = sysconf(_SC_OPEN_MAX);

  while (fd < fdlimit)
  close(fd++);
}
/*=====================================================================*/
int main(int argc,char ** argv)
{
  int rc;
  pid_t pid;
  /*--------------------------------------*/
  if(argc < 3)  usage(argv[0]);
  if(!strcmp(argv[1], "-p"))   
    sscanf(argv[2],"%d",&PORT);
  else
    usage(argv[0]);
  printf("PORT=%d\n",PORT);
  /*--------------------------------------*/
  signal(SIGALRM,falarm);
  signal(SIGHUP,hungup);
  signal(SIGPIPE,fpipe);
  /*--------------------------------------*/
  /*--------------------------------------*/
  printf("Create shmem ID=%d\n",SHM_ID);
  if ( (semid=semget( SEM_ID, 1, PERMS | IPC_CREAT ) ) < 0 )
    sys_err("shmem server: can not create semaphore");
  semctl(semid, 0, SETVAL, 0);

  if ( (shmid=shmget(SHM_ID,sizeof(evt_Builder), PERMS | IPC_CREAT ))<0)
     printf("shmem server: can't create shared memory segment \n");
  else if ( (shmid=shmget(SHM_ID,sizeof(evt_Builder),0))<0)
    sys_err("shmem server: can't FIND shared memory segment ");

  if ( (event_ptr=(evt_Builder*) shmat (shmid, 0, 0)) <0 )
    sys_err("shmem server: shared memory attach error");


/*-----------------------------------------------------------*/

  while (TCP_FLAG==0) {
    rc=tcp_open(PORT,"");  TCP_FLAG=1;
    if(rc){ 
      tcp_close(0); TCP_FLAG=0;
      sleep(1); 
    }
  }
  while(!sig_int) {
    printf(" while_loop:: PORT=%d ",PORT);
    rc=tcp_listen();
    pid=fork2();
    if (pid==0) { tcp_close_old(); run_child(); exit(0); }
    tcp_close(-1);
  }
  /*-----------------------------------------------------------*/
  printf(" Close TCP ports \n");
  tcp_close(0);
  shmdt ((char*)event_ptr);
  exit(0);
}
/*-----------------------------------------------------------*/
/*====================================================================*/
int run_child() {
  int i,t2;
  struct timeval tv1,tv2;
  struct timezone tz1,tz2;
  static int BUFFER[MAXDATA],HEADER[10];
  static int icc=0, READY=0;
  int LENEVENT, rc;
  unsigned int TriggerID=0,TriggerID_old=0, ModuleID=0,Nr_Modules,xxx;
  unsigned int MARKER,FIRST=0;
  event_DEPFET *evtDEPFET;

  event_ptr->ptrDATA=NULL;

  while(!sig_int){
    
    if (TCP_FLAG==0)  {     
        printf(" EXIT PORT=%d ",PORT);
     return 1;
    }

    /*--------- get TCP data ------------*/
    icc++;
    rc=tcp_get2(HEADER,sizeof(HEADER));
    if(rc<0) { printf(" 1 get error: rc<0  (%d)......\n",rc);return 1;}   
    else if(rc>0) {printf("Need to get more data \n");};
    
    MARKER=HEADER[1];
    LENEVENT=HEADER[2];
    xxx=HEADER[3];
    TriggerID=HEADER[4];
    Nr_Modules=HEADER[5];
    ModuleID=HEADER[6];
    if(LENEVENT>sizeof(BUFFER)) { 
	printf("event size > buffsize %d %d \n", LENEVENT,sizeof(BUFFER) ); 
	tcp_close(-1);	 return -1;
    }
    
    rc=tcp_get2(BUFFER,LENEVENT);
    if(rc<0) { printf(" 2 get errorr rc<0 (%d)......\n",rc); return 1;}     
    else if(rc>0) { printf("Need to get more data=%d \n",rc); return 1;};

    evtDEPFET=(event_DEPFET*) BUFFER;
    if(!(sig_hup%2)) 
	printf("evtDEPFET->Trigger=%d TrID=%d Mod=%d len=%d\n"
	       ,evtDEPFET->Trigger,TriggerID,ModuleID,LENEVENT);
    
    if (ModuleID==0)  { 
	event_ptr->ptrDATA=(int*)event_ptr->DATA;  
	event_ptr->lenDEPFET=2;
	TriggerID_old=TriggerID; 
    } else if (ModuleID>0 && evtDEPFET->Trigger!=TriggerID_old ) {
	printf(" Error TriggerID Mod=%d Trg=%d Trg_old=%d\n"
                      ,ModuleID,evtDEPFET->Trigger,TriggerID_old);
    };

    if (FIRST==0 && ModuleID==0) { FIRST=1; }

    if ((ModuleID+1)==Nr_Modules && FIRST==1) { READY=1; printf("----- READY -----\n");}
    else if (ModuleID>Nr_Modules) {
	printf("ModuleID > Nr_Modules %d %d\n",ModuleID,Nr_Modules);
        return -1;
    }

    /*---- Copy recv Buffer to SHMEM -----*/
    while ( semctl (semid, 0, GETVAL, 0) ) usleep(100);
    semctl (semid, 0, SETVAL, 1);
    
    event_ptr->n_Modules=Nr_Modules;
    event_ptr->k_Modules=ModuleID;
    event_ptr->HEADER=xxx;
    event_ptr->Trigger=TriggerID;
    event_ptr->READY=READY;

    event_ptr->lenDATA[ModuleID]=LENEVENT;
    event_ptr->lenDEPFET+=LENEVENT;
    event_ptr->ptrMOD[ModuleID]=event_ptr->ptrDATA;

    printf(" DATA ptr=%p \n",event_ptr->ptrDATA);

    if (event_ptr->ptrDATA!=NULL) {
	for(i=0;i<LENEVENT;i++){ *(event_ptr->ptrDATA++)=BUFFER[i]; }
    }
    printf("Mod=%d TrID=%d \n",ModuleID,*(event_ptr->ptrMOD[ModuleID]));

    time((time_t*)&t2);
    event_ptr->Time_mark=t2;
    semctl(semid, 0, SETVAL, 0);
    /*---- end of SHMEM fill  -----*/

    if (READY && !sig_hup)   printf("Event is READY .. wait.. \n");
    while (READY) {
	while ( semctl (semid, 0, GETVAL, 0) ) usleep(100);
	semctl (semid, 0, SETVAL, 1);
	READY=event_ptr->READY;
	semctl(semid, 0, SETVAL, 0);
    }   

  }   return 0;
}
/*====================================================================*/
static unsigned long  swap4(unsigned long x)
{
        union {
                unsigned long i;
                char c[4];
        } wc, wc1;
 
        wc.i = x;
        wc1.c[0] = wc.c[3];
        wc1.c[1] = wc.c[2];
        wc1.c[2] = wc.c[1];
        wc1.c[3] = wc.c[0];
 
        return(wc1.i);
}   

/*====================================================================*/
int usage(char* name)
{
  printf("Usage: %s -p port \n", name);
  exit(1);
}
/*-------------------------------------------------------------------------*/
/* End. */









