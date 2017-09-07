#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/DataSender.hh"
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

  //----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
  class DLLEXPORT DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol);
    void OnInitialise() override final;
    void OnConfigure() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnReset() override final;
    void OnTerminate() override final;
    void OnStatus() override final;
    void Exec() override;

    //running in commandreceiver thread
    virtual void DoInitialise(){};
    virtual void DoConfigure(){};
    virtual void DoStartRun(){};
    virtual void DoStopRun(){};
    virtual void DoReset(){};
    virtual void DoTerminate(){};

    //running in dataserver thread
    virtual void DoConnect(ConnectionSPC id) {}
    virtual void DoDisconnect(ConnectionSPC id) {}
    virtual void DoReceive(ConnectionSPC id, EventSP ev){};
    
    void WriteEvent(EventSP ev);
    void SetServerAddress(const std::string &addr){m_data_addr = addr;};
    void StartDataCollector();
    void CloseDataCollector();
    bool IsActiveDataCollector(){return m_thd_server.joinable();}
  private:
    void DataHandler(TransportEvent &ev);
    void DataThread();

  private:
    std::thread m_thd_server;
    bool m_exit;
    std::unique_ptr<TransportServer> m_dataserver;
    std::string m_data_addr;
    FileWriterUP m_writer;
    std::map<std::string, std::unique_ptr<DataSender>> m_senders;
    std::string m_fwpatt;
    std::string m_fwtype;
    std::vector<std::shared_ptr<ConnectionInfo>> m_info_pdc;
    uint32_t m_dct_n;
    uint32_t m_evt_c;
    std::unique_ptr<const Configuration> m_conf;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_DataCollector
