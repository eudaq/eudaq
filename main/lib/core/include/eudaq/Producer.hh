#ifndef EUDAQ_INCLUDED_Producer
#define EUDAQ_INCLUDED_Producer

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <string>

namespace eudaq {
  class Producer;
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Producer>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Producer>::UP_BASE (*)
	   (const std::string&, const std::string&)>&
  Factory<Producer>::Instance<const std::string&, const std::string&>();  
#endif

  using ProducerSP = Factory<Producer>::SP_BASE;
  
  /**
   * The base class from which all Producers should inherit.
   * It is both a CommandReceiver, listening to commands from RunControl,
   * and a DataSender, sending data to a DataCollector.
   */
  //----------DOC-MARK-----BEGINDECLEAR-----DOC-MARK----------
  class DLLEXPORT Producer : public CommandReceiver{
  public:
    Producer(const std::string &name, const std::string &runcontrol);

    virtual void DoInitialise(){};
    virtual void DoConfigure(){};
    virtual void DoStartRun(){};
    virtual void DoStopRun(){};
    virtual void DoReset(){};
    virtual void DoTerminate(){};
    virtual void DoStatus(){};
    
    void SendEvent(EventSP ev);
    static ProducerSP Make(const std::string &code_name, const std::string &run_name,
			   const std::string &runcontrol);

  private:
    void OnInitialise() override final;
    void OnConfigure() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnReset() override final;
    void OnTerminate() override final;
    void OnStatus() override;
  
  protected:
    uint32_t m_evt_c;
  private:
    uint32_t m_pdc_n;
    std::mutex m_mtx_sender;
    std::map<std::string, std::shared_ptr<DataSender>> m_senders;
  };
  //----------DOC-MARK-----ENDDECLEAR-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_Producer
