#include "depfet/shmem_depfet.h"
#include <unistd.h>

/*=====================================================================*/
int main(int argc,char ** argv)  { 
    int rc, lenEVT;
    int EVTBUF[MAXMOD*MAXDATA]; //---  about 170000 for 5 modules
    while(1){
	rc=shmem_depfet(EVTBUF,&lenEVT);

	if      (rc<0) printf(" Error shmem rc=%d\n",rc);
	else if (rc>0) printf(" get EVENT len=%d Trig=%d %d %d %d\n"
                ,lenEVT,EVTBUF[1],EVTBUF[3],EVTBUF[2+32780+1],EVTBUF[2+32780*2+1]);        
	else     usleep(100); 
    }
    return 0;
}

