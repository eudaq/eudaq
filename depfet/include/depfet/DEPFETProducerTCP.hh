#include "eudaq/Producer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DEPFETEvent.hh"
#include "depfet/tcplib.h"
#include "depfet/shmem_depfet.h"

using eudaq::to_string;
using eudaq::DEPFETEvent;

class DEPFETProducerTCP : public eudaq::Producer {
public:
  DEPFETProducerTCP(const std::string & name, const std::string & runcontrol, int listenport)
    : eudaq::Producer(name, runcontrol),
      done(false),
      m_port(listenport)
      //m_buffer(MAXDATA + DEPFET_HEADER_LENGTH)
    {
      int rc = 0;
      bool tcp_done = false;
      do {
        rc = tcp_open(m_port,"");  tcp_done = true;
        printf("RC=%d\n",rc);
        if (rc) {
          tcp_done = false;
          sleep(1);
        }
      } while (tcp_done == false);
      rc = tcp_listen();
    }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
  }
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_evt = 0;
    SendEvent(DEPFETEvent::BORE(m_run));
    SetStatus(eudaq::Status::LVL_OK, "Started");
  }
  virtual void OnStopRun() {
    SendEvent(DEPFETEvent::EORE(m_run, ++m_evt));
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  }
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }
  virtual void OnReset() {
    SetStatus(eudaq::Status::LVL_OK, "Reset");
  }
  void Process() {
    int READY = 0;
    eudaq::DEPFETEvent ev(m_run, ++m_evt);
    do {
      //evt_Builder *event_ptr = new evt_Builder;
      printf("Waiting for header \n");
      int rc=tcp_get2(&m_buffer[0], DEPFET_HEADER_LENGTH * 4);
      printf("Got header \n");
      if(rc<0) { EUDAQ_THROW(" 1 get error: rc<0  (" + to_string(rc) + ")......");}   
      else if(rc>0) {printf("Need to get more data \n");};
      //int MARKER=HEADER[1];
      int LENEVENT=m_buffer[2];
      //int xxx=m_buffer[3];
      int TriggerID=m_buffer[4];
      int Nr_Modules=m_buffer[5];
      int ModuleID=m_buffer[6];
      int TriggerID_old = 0;
      static int FIRST = 0;
      printf("received header board id=%d of %d, FIRST=%d, READY=%d\n", ModuleID, Nr_Modules, FIRST, READY);

      if (LENEVENT>MAXDATA) { 
        printf("event size > buffsize %d %d \n", LENEVENT, MAXDATA); 
        tcp_close(-1);   return;
      }
    
      rc=tcp_get2(&m_buffer[DEPFET_HEADER_LENGTH], LENEVENT);
      if(rc<0) { EUDAQ_THROW(" 2 get errorr rc<0 (" + to_string(rc) + ")......");}     
      else if(rc>0) { printf("Need to get more data=%d", rc);}

//       event_DEPFET * evtDEPFET=(event_DEPFET*) BUFFER;
//       if(!(sig_hup%2)) 
//         printf("evtDEPFET->Trigger=%d TrID=%d Mod=%d len=%d\n"
//                ,evtDEPFET->Trigger,TriggerID,ModuleID,LENEVENT);
    
      if (ModuleID==0)  { 
        //event_ptr->ptrDATA=(int*)event_ptr->DATA;  
        //event_ptr->lenDEPFET=2;
        TriggerID_old=TriggerID; 
      } else if (ModuleID>0 && TriggerID!=TriggerID_old ) {
        printf(" Error TriggerID Mod=%d Trg=%d Trg_old=%d\n"
               ,ModuleID,TriggerID,TriggerID_old);
      };

      if (FIRST==0 && ModuleID==0) { FIRST=1; }

      if ((ModuleID+1)==Nr_Modules && FIRST==1) { READY=1; printf("----- READY -----\n");}
      else if (ModuleID>Nr_Modules) {
        EUDAQ_THROW("ModuleID (" + to_string(ModuleID) + ") > Nr_Modules (" + to_string(Nr_Modules) + ")");
      }
//       event_ptr->n_Modules=Nr_Modules;
//       event_ptr->k_Modules=ModuleID;
//       event_ptr->HEADER=xxx;
//       event_ptr->Trigger=TriggerID;
//       event_ptr->READY=READY;

//       event_ptr->lenDATA[ModuleID]=LENEVENT;
//       event_ptr->lenDEPFET+=LENEVENT;
//       event_ptr->ptrMOD[ModuleID]=event_ptr->ptrDATA;

      //printf(" DATA ptr=%p \n",(void*)event_ptr->ptrDATA);

//       if (event_ptr->ptrDATA!=NULL) {
//         for(int i=0;i<LENEVENT;i++){ *(event_ptr->ptrDATA++)=BUFFER[i]; }
//       }
//       printf("Mod=%d TrID=%d \n",ModuleID,*(event_ptr->ptrMOD[ModuleID]));
//       int t2;
//       time((time_t*)&t2);
//       event_ptr->Time_mark=t2;
      printf("Adding board id=%d of %d, FIRST=%d, READY=%d\n", ModuleID, Nr_Modules, FIRST, READY);
      ev.AddBoard(ModuleID, m_buffer, sizeof m_buffer);
    } while (!READY);
    printf("Sending event \n");
    SendEvent(ev);
    printf("OK \n");
  }
  bool done;
private:
  unsigned m_port, m_run, m_evt;
  int m_buffer[MAXDATA + DEPFET_HEADER_LENGTH];
  //std::vector<int> m_buffer;
};
