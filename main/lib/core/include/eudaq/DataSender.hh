#ifndef EUDAQ_INCLUDED_DataSender
#define EUDAQ_INCLUDED_DataSender


#include "eudaq/Platform.hh"
#include "eudaq/Event.hh"
#include <string>

namespace eudaq {

class TransportClient;

  class DLLEXPORT DataSender {
    public:
      DataSender(const std::string & type, const std::string & name);
      void Connect(const std::string & server);
      void SendEvent(EventSPC ev);
    private:
      std::string m_type, m_name;
      std::unique_ptr<TransportClient> m_dataclient;
      uint64_t m_packetCounter;
  };

}

#endif // EUDAQ_INCLUDED_DataSender
