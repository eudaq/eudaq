#ifndef EUDAQ_INCLUDED_DataReceiver
#define EUDAQ_INCLUDED_DataReceiver

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
#include <future>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>

namespace eudaq {
  class DataReceiver;
  
  using DataReceiverSP = Factory<DataReceiver>::SP_BASE;

  class DLLEXPORT DataReceiver{
  public:
    DataReceiver();
    virtual ~DataReceiver();
    virtual void OnConnect(ConnectionSPC id);
    virtual void OnDisconnect(ConnectionSPC id);
    virtual void OnReceive(ConnectionSPC id, EventSP ev);
    std::string Listen(const std::string &addr);
    void StopListen();//TODO: remove this method later
  private:
    void DataHandler(TransportEvent &ev);
    bool Deamon();
    bool AsyncReceiving();
    bool AsyncForwarding();
    
  private:
    std::unique_ptr<TransportServer> m_dataserver;
    std::string m_last_addr;
    std::vector<ConnectionSP> m_vt_con;
    bool m_is_destructing;
    bool m_is_listening;
    bool m_is_async_rcv_return;
    std::future<bool> m_fut_async_rcv;
    std::future<bool> m_fut_async_fwd;
    std::future<bool> m_fut_deamon;
    std::mutex m_mx_qu_ev;
    std::mutex m_mx_deamon;
    std::queue<std::pair<EventSP, ConnectionSPC>> m_qu_ev;
    std::condition_variable m_cv_not_empty;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_DataReceiver
