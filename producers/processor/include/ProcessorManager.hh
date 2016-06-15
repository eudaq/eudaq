#ifndef PROCESSORMANAGER_HH
#define PROCESSORMANAGER_HH

#include<utility>
#include<queue>
#include<map>
#include<mutex>

#include "Platform.hh"
#include "Event.hh"
#include "RawDataEvent.hh"

namespace eudaq{

  class Processor;
  class ProcessorManager;

  typedef Processor* (*CreatePS)(uint32_t);
  typedef void (*DestroyPS)(Processor*);
  typedef RawDataEvent* (*CreateEV)();
  typedef void (*DestroyEV)(Event*);

  // using PSSP = std::shared_ptr<Processor, DestroyPS>;
  // using EVUP = std::unique_ptr<Event, DestroyEV>;
  // typedef std::shared_ptr<Processor> PSSP;
  // typedef std::unique_ptr<Event> EVUP;
  using PSSP = std::shared_ptr<Processor>;
  using EVUP = std::unique_ptr<RawDataEvent>;
  
  class ProcessorManager{
  public:
    static std::shared_ptr<ProcessorManager> GetInstance();
  public:
    ProcessorManager();
    ~ProcessorManager(){};
    
    void InitProcessorPlugins();
    void RegisterProcessorType(std::string pstype, CreatePS c, DestroyPS d);
    void RegisterEventType(std::string evtype, CreateEV c, DestroyEV d);
    void RegisterProcessing(PSSP ps, EVUP ev);
    void RegisterProcessing(uint32_t psid, EVUP ev);
    
    void EventLoop();

    PSSP CreateProcessor(std::string pstype, uint32_t psid);
    EVUP CreateEvent(std::string evtype);

  private:
    
    
  private:
    std::map<uint32_t, PSSP > m_pslist_instance;
    std::map<std::string, std::pair<CreateEV, DestroyEV> > m_evlist;
    std::map<std::string, std::pair<CreatePS, DestroyPS> > m_pslist;  //std::pair<Processor * (*)(), void(*)(Processor *)>
    std::queue<std::pair<PSSP, EVUP> > m_fifo_events;
    std::mutex m_mtx_fifo;

  };
}

#endif
