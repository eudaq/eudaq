#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

#include <pthread.h>
#include <string>
#include <vector>
#include <list>

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/counted_ptr.hh"

namespace eudaq {

  /** Implements the functionality of the File Writer application.
   *
   */
  class DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string & runcontrol,
                  const std::string & listenaddress);

    virtual void OnConnect(const ConnectionInfo & id);
    virtual void OnDisconnect(const ConnectionInfo & id);
    virtual void OnServer();
    virtual void OnGetRun();
    virtual void OnConfigure(const Configuration & param);
    virtual void OnPrepareRun(unsigned runnumber);
    virtual void OnStopRun();
    virtual void OnReceive(const ConnectionInfo & id, counted_ptr<Event> ev);
    virtual void OnCompleteEvent();
    virtual void OnStatus();
    virtual ~DataCollector();

    void DataThread();
  private:
    struct Info {
      counted_ptr<ConnectionInfo> id;
      std::list<counted_ptr<Event> > events;
    };

    void DataHandler(TransportEvent & ev);
    size_t GetInfo(const ConnectionInfo & id);

    bool m_done, m_listening;
    TransportServer * m_dataserver; ///< Transport for receiving data packets
    pthread_t m_thread;
    pthread_attr_t m_threadattr;
    std::vector<Info> m_buffer;
    size_t m_numwaiting; ///< The number of producers with events waiting in the buffer
    size_t m_itlu; ///< Index of TLU in m_buffer vector, or -1 if no TLU
    unsigned m_runnumber, m_eventnumber;
    counted_ptr<FileWriter> m_writer;
    Configuration m_config;
    Time m_runstart;
  };

}

#endif // EUDAQ_INCLUDED_DataCollector
