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
				     (const std::string&, const std::string&,
				      const std::string&, const std::string& )>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&,
				   const std::string&, const std::string&>();
#endif
  
  class DLLEXPORT DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol,
                  const std::string &listenaddress,
                  const std::string &runnumberfile);
    ~DataCollector() override;
    void OnServer() override;
    void OnConfigure(const Configuration &param) override;
    void OnStartRun(uint32_t) override;
    void OnStopRun() override;
    void OnStatus() override;
    virtual void OnConnect(const ConnectionInfo &id);
    virtual void OnDisconnect(const ConnectionInfo &id);
    virtual void OnReceive(const ConnectionInfo &id, EventSP ev);
  private:
    void DataThread();
    void WriterThread();
    void DataHandler(TransportEvent &ev);
    Time m_runstart;	
    const std::string m_runnumberfile;
    std::atomic_bool m_done;
    std::atomic_bool m_done_writer;
    std::atomic<uint32_t> m_runnumber;
    std::atomic<uint32_t> m_eventnumber;
    std::unique_ptr<TransportServer> m_dataserver;
    ProcessorSP m_ps_input;
    ProcessorSP m_ps_output;
    FileWriterUP m_writer;
    std::vector<ConnectionInfo> m_info_pdc;
    std::thread m_thread;
    std::thread m_thread_writer;
  };
}

#endif // EUDAQ_INCLUDED_DataCollector
