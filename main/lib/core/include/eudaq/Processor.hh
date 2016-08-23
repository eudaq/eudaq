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
#include"RawDataEvent.hh"

#include"Factory.hh"

namespace eudaq {
  class Processor;

#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT
  Factory<Processor>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>&
  Factory<Processor>::Instance<std::string&>();
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>&
  Factory<Processor>::Instance<std::string&&>();
#endif
  

  using ProcessorUP = Factory<Processor>::UP_BASE;
  using ProcessorSP = Factory<Processor>::SP_BASE;
  using ProcessorWP = Factory<Processor>::WP_BASE;

  
  using PSSP = ProcessorSP;
  using PSWP = ProcessorWP;
  using EVUP = EventUP;
  
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

    static PSSP MakePSSP(std::string pstype, std::string cmd);
    
    Processor(std::string pstype, std::string cmd);
    
    virtual ~Processor();
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
    virtual void ProduceEvent();
    void ConsumeEvent();
    
    void HubProcessing();
    void Processing(EVUP ev);
    void AsyncProcessing(EVUP ev);
    void SyncProcessing(EVUP ev);
    void ForwardEvent(EVUP ev);
    void RegisterProcessing(PSSP ps, EVUP ev);

    void ProcessSysCmd(std::string cmd_name, std::string cmd_par);
    void ProcessSysEvent(EVUP ev);
    
    void RunProducerThread();
    void RunConsumerThread();
    void RunHubThread();
    
    void InsertEventType(uint32_t evtype);
    void EraseEventType(uint32_t evtype);


    virtual void AddNextProcessor(PSSP ps);
    void AddUpstream(PSWP ps);
    void UpdatePSHub(PSWP ps);
    void SetThisPtr(PSWP ps){m_ps_this = ps;};
    
    PSSP operator>>(PSSP psr);
    PSSP operator>>(std::string stream_str);

    Processor& operator<<(EVUP ev);
    Processor& operator<<(std::string cmd_str);

    bool IsHub(){return m_flag&FLAG_HUB_RUN ;};
    bool IsAsync(){return m_flag&FLAG_CSM_RUN ;};
    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    STATE GetState(){return m_state;};
    PSSP GetNextPSSP(Processor *p);

    
  private:
    std::string m_pstype;
    uint32_t m_psid;
    PSWP m_ps_hub;
    PSWP m_ps_this;
    std::vector<PSWP> m_ps_upstr;
    std::vector<std::pair<PSSP, std::set<uint32_t>>> m_pslist_next;
    
    std::set<uint32_t> m_evlist_white;
    std::vector<std::pair<std::string, std::string>> m_cmdlist_init;    
    
    std::queue<EVUP> m_fifo_events; //fifo async
    std::queue<std::pair<PSSP, EVUP> > m_fifo_pcs; //fifo hub
    std::mutex m_mtx_fifo;
    std::mutex m_mtx_pcs;
    std::mutex m_mtx_state;
    std::mutex m_mtx_list;
    std::condition_variable m_cv;
    std::condition_variable m_cv_pcs;
    std::atomic<STATE> m_state;
    std::atomic<int> m_flag;
  };


  // class ProcessorBatch: public Processor{
  // public:
  //   virtual void AddNextProcessor(PSSP ps);

  // private:
  //   std::vector<PSSP> m_ps_root;
  // };

}

DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, eudaq::PSSP psr);
DLLEXPORT  eudaq::PSSP operator>>(eudaq::PSSP psl, std::string psr_str);
DLLEXPORT  eudaq::PSSP operator<<(eudaq::PSSP psl, std::string cmd_list);


#endif
