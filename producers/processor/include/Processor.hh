#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_

#include<set>
#include<vector>
#include<string>
#include<queue>
#include<memory>
#include<thread>
#include<mutex>
#include<atomic>
#include<condition_variable>

#include"Event.hh"
#include"Configuration.hh"
#include"ClassFactory.hh"
#include"RawDataEvent.hh"

namespace eudaq {
  class Processor;
  class ProcessorManager;

  DEFINE_FACTORY_AND_PTR(Processor, std::string);
  
  using CreatePS  = Processor* (*)(uint32_t);
  using DestroyPS = void (*)(Processor*);
  using CreateEV = Event* (*)();
  using DestroyEV = void (*)(Event*);
  
  using PSSP = std::shared_ptr<Processor>;
  using PSWP = std::weak_ptr<Processor>;
  using EVUP = std::unique_ptr<Event, std::function<void(Event*)> >;
  
  class DLLEXPORT Processor{
  public:
    enum STATE:uint32_t{
      STATE_UNCONF,
      STATE_READY,
      STATE_ERROR,
      STATE_THREAD_UNCONF,
      STATE_THREAD_READY,
      STATE_THREAD_BUSY,
      STATE_THREAD_ERROR
    };

    enum FLAG{
        FLAG_HUB_RUN=1,
	FLAG_HUB_BUSY=2,
	FLAG_PDC_RUN=4,
	FLAG_PDC_BUSY=8,
	FLAG_CSM_RUN=16,
	FLAG_CSM_BUSY=32
    };
    
    Processor(std::string pstype, uint32_t psid);
    Processor(std::string pstype, std::string cmd);

    Processor(){};
    virtual ~Processor();
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
    virtual void ProduceEvent();
    virtual void ConsumeEvent();

    bool IsHub(){return m_flag&FLAG_HUB_RUN ;};
    bool IsAsync(){return m_flag&FLAG_CSM_RUN ;};
    
    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    STATE GetState(){return m_state;};
    void HubProcessing();
    void Processing(EVUP ev);
    void AsyncProcessing(EVUP ev);
    void SyncProcessing(EVUP ev);
    virtual void ForwardEvent(EVUP ev);

    void RegisterProcessing(PSSP ps, EVUP ev);
    void RunProducerThread();
    void RunConsumerThread();
    void RunHubThread();
    
    PSWP GetPSHub(){return m_ps_hub;};
    void SetPSHub(PSWP ps); //TODO: thread safe
    void InsertEventType(uint32_t evtype);
    void EraseEventType(uint32_t evtype);

    void ProcessSysEvent(EVUP ev);
    void AddNextProcessor(PSSP ps);
    void CreateNextProcessor(std::string pstype);
    void RemoveNextProcessor(uint32_t psid);

    uint32_t GetNumUpstream(){return m_num_upstream;};
    void IncreaseNumUpstream(){m_num_upstream++;};

    void ProcessSysCmd(std::string cmd_name, std::string cmd_par);
    
    virtual PSSP operator>>(PSSP psr);

    virtual Processor& operator<<(EVUP ev);
    virtual Processor& operator<<(std::string cmd_str);
    
  private:
    std::string m_pstype;
    uint32_t m_psid;
    uint32_t m_num_upstream;
    PSWP m_ps_hub;
    
    std::set<uint32_t> m_evlist_white;
    std::vector<std::pair<PSSP, std::set<uint32_t>>> m_pslist_next;
    std::vector<std::pair<std::string, std::string>> m_cmdlist_init;
    
    
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
    std::atomic<int> m_flag;
  };
  
}

DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, eudaq::PSSP psr);
DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, std::string psr_str);
DLLEXPORT  eudaq::PSSP operator<<(eudaq::PSSP psl, std::string cmd_list);



#endif
