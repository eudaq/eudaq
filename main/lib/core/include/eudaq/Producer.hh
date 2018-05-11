#ifndef EUDAQ_INCLUDED_Producer
#define EUDAQ_INCLUDED_Producer

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/FileWriter.hh"

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
  //----------DOC-MARK-----BEGINDECLEAR-----DOC-MARK----------
  class DLLEXPORT Producer : public CommandReceiver{
  public:
    Producer(const std::string &name, const std::string &runcontrol);
    void OnInitialise() override final;
    void OnConfigure() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnReset() override final;
    void OnTerminate() override final;
    void Exec() override;
    void WriteEvent(EventUP ev);


    virtual void DoInitialise(){};
    virtual void DoConfigure(){};
    virtual void DoStartRun()=0;
    virtual void DoStopRun()=0;
    virtual void DoReset()=0;
    virtual void DoTerminate()=0;

    
    //    void ReadConfigureFile(const std::string &path);
    //    void ReadInitializeFile(const std::string &path);
    void SendEvent(EventSP ev);
  private:
    bool m_exit;
    bool m_cli_run; 
    std::shared_ptr<Configuration> m_conf;
    std::shared_ptr<Configuration> m_conf_init;
    std::string m_type;
    std::string m_name;
    std::string m_fwpatt;
    std::string m_fwtype;
    uint32_t m_pdc_n;
    uint32_t m_evt_c;
    //    uint32_t m_run_number;
    FileWriterUP m_writer;

    std::map<std::string, std::unique_ptr<DataSender>> m_senders;
  };
  //----------DOC-MARK-----ENDDECLEAR-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_Producer
