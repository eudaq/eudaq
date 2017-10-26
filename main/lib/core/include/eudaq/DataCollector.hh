#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector
#include "eudaq/CommandReceiver.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/DataReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <atomic>

namespace eudaq {
  class DataCollector;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<DataCollector>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<DataCollector>::UP_BASE (*)
				     (const std::string&, const std::string&)>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&>();
#endif

  using DataCollectorSP = Factory<DataCollector>::SP_BASE;
  
  //----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
  class DLLEXPORT DataCollector : public CommandReceiver, public DataReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol);
    ~DataCollector() override;
    virtual void DoInitialise();
    virtual void DoConfigure();
    virtual void DoStartRun();
    virtual void DoStopRun();
    virtual void DoReset();
    virtual void DoTerminate();
    virtual void DoStatus();
    virtual void DoConnect(ConnectionSPC id);
    virtual void DoDisconnect(ConnectionSPC id);
    virtual void DoReceive(ConnectionSPC id, EventSP ev);
    void WriteEvent(EventSP ev);
    void SetServerAddress(const std::string &addr);
    static DataCollectorSP Make(const std::string &code_name,
				const std::string &run_name,
				const std::string &runcontrol);
  private:
    void OnInitialise() override final;
    void OnConfigure() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnReset() override final;
    void OnTerminate() override final;
    void OnStatus() override final;
    void OnConnect(ConnectionSPC id) override final;
    void OnDisconnect(ConnectionSPC id) override final;
    void OnReceive(ConnectionSPC id, EventSP ev) override final;
  private:
    std::string m_data_addr;
    FileWriterUP m_writer;
    std::unique_ptr<DataReceiver> m_receiver;
    std::map<std::string, std::unique_ptr<DataSender>> m_senders;
    std::string m_fwpatt;
    std::string m_fwtype;
    uint32_t m_dct_n;
    uint32_t m_evt_c;
    ConfigurationSPC m_conf;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_DataCollector
