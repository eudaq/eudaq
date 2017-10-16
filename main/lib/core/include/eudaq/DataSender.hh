#ifndef EUDAQ_INCLUDED_DataSender
#define EUDAQ_INCLUDED_DataSender


#include "eudaq/Platform.hh"
#include "eudaq/Event.hh"
#include <string>
#include <future>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eudaq {

class TransportClient;

  class DLLEXPORT DataSender {
  public:
      DataSender(const std::string & type, const std::string & name);
      ~DataSender();
      void Connect(const std::string & server);
      void SendEvent(EventSPC ev);
  private:
      bool AsyncSending();
      std::string m_type, m_name;
      std::unique_ptr<TransportClient> m_dataclient;
      uint64_t m_packetCounter;
      std::future<bool> m_fut_async;
      bool m_is_connected;
      std::mutex m_mx_qu_ev; 
      std::queue<EventSPC> m_qu_ev;
      std::condition_variable m_cv_not_empty;
  };

}

#endif // EUDAQ_INCLUDED_DataSender
