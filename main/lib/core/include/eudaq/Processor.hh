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
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)()>&
  Factory<Processor>::Instance<>();
#endif
  

  using ProcessorUP = Factory<Processor>::UP_BASE;
  using ProcessorSP = Factory<Processor>::SP_BASE;
  using ProcessorWP = Factory<Processor>::WP_BASE;

  
  using PSSP = ProcessorSP;
  using PSWP = ProcessorWP;
  
  class DLLEXPORT Processor: public std::enable_shared_from_this<Processor>{
  public:

    static ProcessorSP MakeShared(std::string pstype, std::string cmd);
    
    Processor(std::string pstype, std::string cmd);
    
    virtual ~Processor();
    virtual void ProcessUserEvent(EventSPC ev);
    virtual void ProcessUserCmd(const std::string cmd_name, const std::string cmd_par)final{};
    virtual void ProduceUserEvent()final{};

    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){};
    virtual void ProduceEvent(){};

    void Processing(EventSPC ev);
    void ForwardEvent(EventSPC ev);
    
    void RunProducerThread();
    void RunConsumerThread();
    void RunHubThread(); //delilver //TODO remove it, 
    
    void AddNextProcessor(ProcessorSP ps);
    void AddUpstream(ProcessorWP ps);
    void UpdatePSHub(ProcessorWP ps);

    void ProcessCmd(const std::string& cmd_list);

    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    void Print(std::ostream &os, uint32_t offset=0) const;
    
    ProcessorSP operator>>(ProcessorSP psr);
    ProcessorSP operator>>(const std::string& stream_str);
    ProcessorSP operator<<(const std::string& cmd_str);

    ProcessorSP operator+(const std::string& evtype);
    ProcessorSP operator-(const std::string& evtype);
    ProcessorSP operator<<=(EventSPC ev);

  private:
    void RegisterProcessing(ProcessorSP ps, EventSPC ev);
    void ConsumeEvent();
    void HubProcessing();
    void AsyncProcessing(EventSPC ev);
    void SyncProcessing(EventSPC ev);
    void ProcessSysEvent(EventSPC ev);
    void ProcessSysCmd(const std::string& cmd_name, const std::string& cmd_par);
    
  private:
    std::string m_pstype;
    uint32_t m_psid;
    ProcessorWP m_ps_hub;
    std::vector<ProcessorWP> m_ps_upstr;
    std::vector<std::pair<ProcessorSP, std::set<uint32_t>>> m_pslist_next;
        
    std::deque<EventSPC> m_que_csm;
    std::deque<std::pair<ProcessorSP, EventSPC> > m_que_hub;
    std::mutex m_mtx_csm;
    std::mutex m_mtx_hub;
    std::condition_variable m_cv_csm;
    std::condition_variable m_cv_hub;
    std::thread m_th_csm;
    std::thread m_th_hub;
    std::atomic_bool m_csm_go_stop;
    std::atomic_bool m_hub_go_stop;

    
    std::set<uint32_t> m_evlist_white; //TODO: for processor input
    std::set<uint32_t> m_evlist_black; //TODO: for processor output
    std::vector<std::pair<std::string, std::string>> m_cmdlist_init;
    
    std::mutex m_mtx_list;

    std::thread m_th_pdc;
  };

  template <typename T>
  ProcessorSP operator>>(ProcessorSP psl, T&& t){
    return *psl>>std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator<<(ProcessorSP psl, T&& t){
    return *psl<<std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator+(ProcessorSP psl, T&& t){
    return (*psl)+std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator-(ProcessorSP psl, T&& t){
    return (*psl)-std::forward<T>(t);
  }
  
  template <typename T>
  ProcessorSP operator<<=(ProcessorSP psl, T&& t){
    return (*psl)<<=std::forward<T>(t);
  }
  
}


#endif
