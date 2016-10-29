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
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)()>&
  Factory<Processor>::Instance<>();
#endif
  
  using ProcessorUP = Factory<Processor>::UP_BASE;
  using ProcessorSP = Factory<Processor>::SP_BASE;
  using ProcessorWP = Factory<Processor>::WP_BASE;
  
  class DLLEXPORT Processor: public std::enable_shared_from_this<Processor>{
  public:
    static ProcessorSP MakeShared(const std::string& pstype,
				  std::initializer_list
				  <std::pair<const std::string, const std::string>> l = {{}});
    
    Processor(const std::string& dsp);
    Processor(Processor&) = delete;
    Processor() = delete;
    virtual ~Processor();
    virtual void ProcessEvent(EventSPC ev);
    virtual void ProduceEvent(){};
    virtual void ProcessCommand(const std::string& cmd, const std::string& arg){};

    void ForwardEvent(EventSPC ev);
    void RegisterEvent(EventSPC ev);

    uint32_t GetInstanceN()const {return m_instance_n;};
    std::string GetDescription()const {return m_description;};
    void Print(std::ostream &os, uint32_t offset=0) const;
    
    ProcessorSP operator>>(ProcessorSP psr);
    ProcessorSP operator<<(const std::string& cmd);

    ProcessorSP operator+(const std::string& evtype);
    ProcessorSP operator-(const std::string& evtype);
    ProcessorSP operator<<=(EventSPC ev);

    //TODO: to be removed
    uint32_t GetID()const {return GetInstanceN();};
    
  private:
    void RegisterProcessing(ProcessorSP ps, EventSPC ev);
    void Processing(EventSPC ev);
    void BufferEvent(EventSPC ev);
    void ConsumeEvent();
    void HubProcessing(); //relay
    void ProcessSysCommand(const std::string& cmd, const std::string& arg);
    void AddDownstream(ProcessorSP ps);
    void UpdateHub(ProcessorWP ps);
    
  private:
    std::string m_description;
    uint32_t m_instance_n;
    ProcessorWP m_ps_hub;
    std::vector<ProcessorWP> m_ps_upstream;
    std::vector<std::pair<ProcessorSP, std::set<uint32_t>>> m_ps_downstream;
    
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
    
    std::mutex m_mtx_config;
    std::atomic_bool m_hub_force;
    
    std::set<uint32_t> m_evlist_white; //TODO: for processor input
    

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
