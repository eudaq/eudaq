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
    
    void InitProcessorPlugins();
    void RegisterProcessorType(std::string pstype, CreatePS c, DestroyPS d);
    void RegisterEventType(std::string evtype, CreateEV c, DestroyEV d);
 
    PSSP MakePSSP(std::string pstype, std::string cmd = "");
    EVUP CreateEvent(std::string evtype);

    PSSP operator>>(PSSP psr);
    PSSP operator>>(std::string psr_str);
   
  private:
    std::vector<PSSP> m_pslist_root;
    std::map<std::string, std::pair<CreateEV, DestroyEV> > m_evlist;
    std::map<std::string, std::pair<CreatePS, DestroyPS> > m_pslist;  //std::pair<Processor * (*)(), void(*)(Processor *)>
    std::mutex m_mtx_fifo;

  };

}

#endif
