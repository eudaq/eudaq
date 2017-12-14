#ifndef EUDAQ_INCLUDED_DataSender
#define EUDAQ_INCLUDED_DataSender

#include "eudaq/Platform.hh"
#include <string>

namespace eudaq {

  class TransportClient;
  class Event;
  class AidaPacket;

  class DLLEXPORT DataSender {
  public:
    DataSender(const std::string &type, const std::string &name);
    ~DataSender();
    void Connect(const std::string &server);
    void SendEvent(const Event &);
    void SendPacket(const AidaPacket &);

  private:
    std::string m_type, m_name;
    TransportClient *m_dataclient;
  };
}

#endif // EUDAQ_INCLUDED_DataSender
