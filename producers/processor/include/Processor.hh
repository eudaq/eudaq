#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_

#include<vector>
#include<string>
#include<thread>
#include<mutex>
#include<atomic>
#include<condition_variable> 

#include"Event.hh"
#include"ProcessorManager.hh"
#include"Configuration.hh"
#include"ClassFactory.hh"

namespace eudaq {

  class DLLEXPORT Processor{
  public:
    enum STATE{
      STATE_UNCONF,
      STATE_READY,
      STATE_ERROR,
      STATE_THREAD_UNCONF,
      STATE_THREAD_READY,
      STATE_THREAD_BUSY,
      STATE_THREAD_ERROR
    };
    
    
    Processor(std::string pstype, uint32_t psid, uint32_t psid_mother);
    Processor(){};
    virtual ~Processor() {};
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProcessCmdEvent(EVUP ev);
    virtual void ProduceEvent();

    void operator()();
    
    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    STATE GetState(){return m_state;};
    void Processing(EVUP ev);
    void AsyncProcessing(EVUP ev);
    void SyncProcessing(EVUP ev);
    void ForwardEvent(EVUP ev);

    void RootThread();
  private:
    void ProcessSysEvent(EVUP ev);
    void AddNextProcessor(PSSP ps);
    void CreateNextProcessor(std::string pstype, uint32_t psid);
    void RemoveNextProcessor(uint32_t psid);
    void AddAuxProcessor(uint32_t psid);
  private:
    std::string m_pstype;
    uint32_t m_psid;
    uint32_t m_psid_mother;
    std::vector<std::string> m_typelist_event;
    std::vector<PSSP > m_pslist_next;
    std::vector<uint32_t > m_pslist_aux;
    std::queue<EVUP> m_fifo_events;
    std::mutex m_mtx_fifo;
    std::mutex m_mtx_state;
    std::mutex m_mtx_list;
    std::condition_variable m_cv;
    std::atomic<STATE> m_state;

    std::map<std::string, std::pair<CreateEV, DestroyEV> > m_evlist_cache;
  };

}
#endif
