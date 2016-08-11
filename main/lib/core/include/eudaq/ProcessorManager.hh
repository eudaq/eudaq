#ifndef PROCESSORMANAGER_HH
#define PROCESSORMANAGER_HH

#include<utility>
#include<queue>
#include<map>
#include<mutex>

#include "Platform.hh"
#include "Processor.hh"

namespace eudaq{

  class Processor;
  class ProcessorManager;
  
  class DLLEXPORT ProcessorManager{
  public:
    static ProcessorManager* GetInstance();
  public:
    ProcessorManager();
    ~ProcessorManager(){};
 
    PSSP MakePSSP(std::string pstype, std::string cmd = "");
    EVUP CreateEvent(std::string evtype);

    PSSP operator>>(PSSP psr);
    PSSP operator>>(std::string psr_str);
   
  private:
    std::vector<PSSP> m_pslist_root;
    std::mutex m_mtx_fifo;

  };
}

#endif
