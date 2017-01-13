#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Processor.hh"
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
  
  class DLLEXPORT DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol);
    void OnConfigure() override final;
    void OnServer() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnTerminate() override final;
    void OnStatus() override final;
    void OnData(const std::string &param) override final{};
    void Exec() override;

    //running in commandreceiver thread
    virtual void DoConfigure(){};
    virtual void DoStartRun(){};
    virtual void DoStopRun(){};
    virtual void DoTerminate(){};

    //running in dataserver thread
    virtual void DoConnect(const ConnectionInfo & /*id*/) {}
    virtual void DoDisconnect(const ConnectionInfo & /*id*/) {}
    virtual void DoReceive(const ConnectionInfo &id, EventUP ev) = 0;

    void WriteEvent(EventUP ev);
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
    FileWriterUP m_writer;
    std::vector<ConnectionInfo> m_info_pdc;
    uint32_t m_dct_n;
    uint32_t m_evt_c;
    std::unique_ptr<const Configuration> m_conf;
  };
}

#endif // EUDAQ_INCLUDED_DataCollector
