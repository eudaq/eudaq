#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

#include "TransportServer.hh"
#include "CommandReceiver.hh"
#include "Event.hh"
#include "FileWriter.hh"
#include "Configuration.hh"
#include "Utils.hh"
#include "Platform.hh"

#include <string>
#include <vector>
#include <list>
#include <memory>


namespace eudaq {

  class DLLEXPORT DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol,
                  const std::string &listenaddress,
                  const std::string &runnumberfile = "../data/runnumber.dat");

    virtual void OnConnect(const ConnectionInfo &id);
    virtual void OnDisconnect(const ConnectionInfo &id);
    virtual void OnServer();
    virtual void OnGetRun();
    virtual void OnConfigure(const Configuration &param);
    virtual void OnPrepareRun(unsigned runnumber);
    virtual void OnStopRun();
    virtual void OnReceive(const ConnectionInfo &id, std::shared_ptr<Event> ev);
    virtual void OnCompleteEvent();
    virtual void OnStatus();
    virtual ~DataCollector();

    void DataThread();

  private:
    struct Info {
      std::shared_ptr<ConnectionInfo> id;
      std::list<EventSP> events;
    };
    
    const std::string m_runnumberfile; // path to the file containing the run number
    void DataHandler(TransportEvent &ev);
    size_t GetInfo(const ConnectionInfo &id);

    bool m_done, m_listening;
    
    std::unique_ptr<TransportServer> m_dataserver;
    std::unique_ptr<std::thread> m_thread;
    std::vector<Info> m_buffer;
    size_t m_numwaiting; ///< The number of producers with events waiting in the
                         ///buffer
    size_t m_itlu;       ///< Index of TLU in m_buffer vector, or -1 if no TLU
    unsigned m_runnumber, m_eventnumber;
    FileWriterUP m_writer;
    Configuration m_config;
    Time m_runstart;
  };
}

#endif // EUDAQ_INCLUDED_DataCollector
