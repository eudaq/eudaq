#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <time.h>

#define SEM_ID	0xDF1	
#define SHM_ID	0xDF2
#define PERMS	00666	

#define MAXDATA 128000
#define MAXMOD  6

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{   unsigned int counter;
    unsigned int Time_mark;
    int READY;
    int n_Modules;
    int k_Modules;
    int lenDATA[MAXMOD]; 
    int lenDEPFET;
    int *ptrMOD[MAXMOD];
    int *ptrDATA;
/*--- here DEPFET event  ----*/
    unsigned int HEADER;
    unsigned int Trigger; 
    unsigned int DATA[MAXMOD*MAXDATA];
}
evt_Builder;

typedef struct
{ 
    unsigned int HEADER;
    unsigned int Trigger; 
    unsigned int DATA[MAXDATA];
}
event_DEPFET;

int shmem_depfet(int *EVENT, int *lenEVENT);

#ifdef __cplusplus
}
#endif
