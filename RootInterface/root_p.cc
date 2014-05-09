#include "ROOTProducer.h"
#include "TSystem.h"

class root_p{
  RQ_OBJECT("root_p")
  root_p(const char * name);
  void connect2runcontrol(const char * runControl);

  void MainLoop(){

    bool invector[8]={1,1,0,0,0,0,1,1};
    	while (!done) {
        if (!getStarted())
        {
         gSystem->Sleep(20);
          // Then restart the loop
          continue;
        }



        m_sct->createNewEvent();

        
        m_sct->setTimeStamp2Now();
        m_sct->AddPlane2Event(0,8,invector,invector);
        m_sct->sendEvent();
      }
  }


  bool getDone(){
    bool done=true;
    getDone(&done);
    return done;
  }
  void getDone(bool* Done){    
      EmitVA("getDone(bool*)",1,done);
  }

  bool getStarted(){
    bool started=false;
    getStarted(&started);
    return started;
  }
  void getStarted(bool* started){
      EmitVA("getStarted(bool*)",1,started);

  }

   ROOTProducer* m_sct;
   bool done;



};