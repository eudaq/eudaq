#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_

#include<set>
#include<vector>
#include<string>
#include<memory>
#include<thread>
#include<mutex>
#include<atomic>
#include<condition_variable> 

#include"Event.hh"
#include"ProcessorManager.hh"
#include"Configuration.hh"
#include"ClassFactory.hh"

namespace eudaq {
  class Processor;
  
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
    
    Processor(std::string pstype, uint32_t psid);
    Processor(){};
    virtual ~Processor() {};
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProduceEvent();

    void operator()();
    
    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    STATE GetState(){return m_state;};
    void Processing(EVUP ev);
    void AsyncProcessing(EVUP ev);
    void SyncProcessing(EVUP ev);
    void ForwardEvent(EVUP ev);

    void RegisterProcessing(PSSP ps, EVUP ev);
    void ProducerThread(){ProduceEvent();};
    void CustomerThread();//TODO: operator ()
    void HubThread();//
    void Adapted();
    
    PSSP GetPSHub(){return m_ps_hub;};
    void SetPSHub(PSSP ps); //TODO: thread safe
    std::set<std::string> UpdateEventWhiteList();
    void ProcessSysEvent(EVUP ev);
    void AddNextProcessor(PSSP ps);
    void CreateNextProcessor(std::string pstype, uint32_t psid);
    void RemoveNextProcessor(uint32_t psid);

    uint32_t GetNumUpstream(){return m_num_upstream;};
    void IncreaseNumUpstream(){m_num_upstream++;};

    // void SetPSHub(PSSP ps){m_ps_hub = ps;}; //TODO: thread safe
  private:
    std::string m_pstype;
    uint32_t m_psid;
    uint32_t m_num_upstream;
    std::shared_ptr<Processor> m_ps_hub;
    
    std::set<std::string> m_evlist_white;
    std::vector<PSSP > m_pslist_next;
    std::queue<EVUP> m_fifo_events;
    std::queue<std::pair<PSSP, EVUP> > m_fifo_pcs;
    std::mutex m_mtx_fifo;
    std::mutex m_mtx_pcs;
    std::mutex m_mtx_state;
    std::mutex m_mtx_list;
    std::condition_variable m_cv;
    std::condition_variable m_cv_pcs;
    std::atomic<STATE> m_state;
    std::map<std::string, std::pair<CreateEV, DestroyEV> > m_evlist_cache;
  };
  
}


// DLLEXPORT  PSSP operator>>(Processor* psl, PSSP psr);
DLLEXPORT  eudaq::PSSP operator>>(eudaq::ProcessorManager* pm, eudaq::PSSP psr);
DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, eudaq::PSSP psr);
DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, std::string psr_str);


#endif
