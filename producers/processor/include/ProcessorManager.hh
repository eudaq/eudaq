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
    void RegisterProcessing(PSSP ps, EVUP ev);
    
    void EventLoop();

    PSSP CreateProcessor(std::string pstype, uint32_t psid);
    PSSP MakePSSP(std::string pstype, std::string cmd = "");
    EVUP CreateEvent(std::string evtype);

    PSSP operator>>(PSSP psr);
    
  private:
    
    
  private:
    std::map<std::string, std::pair<CreateEV, DestroyEV> > m_evlist;
    std::map<std::string, std::pair<CreatePS, DestroyPS> > m_pslist;  //std::pair<Processor * (*)(), void(*)(Processor *)>
    std::queue<std::pair<PSSP, EVUP> > m_fifo_events;
    std::mutex m_mtx_fifo;

  };
}

#endif
