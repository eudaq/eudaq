#ifndef EUDAQ_INCLUDED_Producer
#define EUDAQ_INCLUDED_Producer

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"
#include "eudaq/Event.hh"
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
  
  /**
   * The base class from which all Producers should inherit.
   * It is both a CommandReceiver, listening to commands from RunControl,
   * and a DataSender, sending data to a DataCollector.
   */
  class DLLEXPORT Producer : public CommandReceiver{
  public:
    /**
     * The constructor.
     * \param runcontrol A string containing the address of the RunControl to
     * connect to.
     */
    Producer(const std::string &name, const std::string &runcontrol);
    void OnConfigure(const Configuration &conf) override final;
    void OnStartRun(uint32_t run_n) override final;
    void OnStopRun() override final;
    void OnReset() override final{}; //TODO: reset member variable
    void OnTerminate() override final;
    void OnServer() override final{};
    void OnData(const std::string &param) override final;
    void Exec() override; //TODO: mark it final to report derived class which has Exec override.

    virtual void DoConfigure(const Configuration &conf) = 0;
    virtual void DoStartRun(uint32_t run_n) = 0;
    virtual void DoStopRun() = 0;
    virtual void DoReset() = 0;
    virtual void DoTerminate() = 0;
    
    void SendEvent(EventUP ev);
  private:
    uint32_t m_pdc_n;
    uint32_t m_run_n;
    uint32_t m_evt_c;
    std::map<std::string, std::unique_ptr<DataSender>> m_senders;
  };
}

#endif // EUDAQ_INCLUDED_Producer
