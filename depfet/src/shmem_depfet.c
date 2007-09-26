/*=====================================================================*/
/*          TCP/IP server (read information from shared memory         */
/*          Author: Julia Furletova  (julia@mail.desy.de)             */
/*          Created:  21-sept-2007                                     */
/*=====================================================================*/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "depfet/shmem_depfet.h"
/*=====================================================================*/
int shmem_depfet(int *EVENT, int *lenEVENT) {
    static evt_Builder *event_ptr;
    static int	semid,shmid,FIRST;
    int *ptrDEPFET,i;
    time_t t2;
    unsigned int TriggerID=0, ModuleID,Nr_Modules,xxx;
    static int BUFFER[MAXMOD][MAXDATA],READY;
    event_DEPFET *evtDEPFET;
    if(FIRST==0) { 
	FIRST=-1;
/*-----------------------------------------------------------*/
	if ( (semid=semget( SEM_ID, 1, 0) ) < 0 ) {
	    printf("shmem_DEPFET: can not get semaphore \n"); return -1;
	}
	if ( (shmid=shmget(SHM_ID,sizeof(evt_Builder), 0))<0) {
	    printf("shmem_DEPFET: shared memory attach error\n");return -1;
	}
	
	if ( (event_ptr=(evt_Builder*) shmat (shmid, 0, 0)) <0 ) {
	    printf("shmem_DEPFET: shared memory attach error\n");return -1;
	}
	semctl(semid, 0, SETVAL, 0);  
/*-----------------------------------------------------------*/
	FIRST=1;
    }    
	/*------ shmem access (keep short)*/
	while ( semctl (semid, 0, GETVAL, 0) ) usleep(100);
	semctl (semid, 0, SETVAL, 1); 
	READY=event_ptr->READY;
	if(READY) {
	    Nr_Modules=event_ptr->n_Modules;
	    ModuleID=event_ptr->k_Modules;
	    xxx=event_ptr->HEADER;
	    TriggerID=event_ptr->Trigger;
            *lenEVENT=event_ptr->lenDEPFET;
	    ptrDEPFET=&(event_ptr->HEADER);
	    for (i=0;i<event_ptr->lenDEPFET;i++)  {
		EVENT[i]=*(ptrDEPFET+i);
	    }
	    t2=event_ptr->Time_mark;
	    event_ptr->READY=0;
	    for (i=0;i<Nr_Modules;i++) {
	    }
	}
	semctl(semid, 0, SETVAL, 0);      usleep(100);
	/*------- end shmem access  */
	if(READY) {
	    evtDEPFET=(event_DEPFET*) BUFFER;
	    printf("evtDEPFET->Trigger=%d TrID=%d Mod=%d len=%d\n"
		   ,evtDEPFET->Trigger,TriggerID,ModuleID,*lenEVENT);
	    return TriggerID;
	};
	return 0;
}

