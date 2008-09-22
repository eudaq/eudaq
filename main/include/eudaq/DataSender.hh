#ifndef EUDAQ_INCLUDED_DataSender
#define EUDAQ_INCLUDED_DataSender

#include "eudaq/Event.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/Serializer.hh"
#include <string>

namespace eudaq {

  class DataSender {
  public:
    DataSender(const std::string & type, const std::string & name);
    ~DataSender();
    void Connect(const std::string & server);
    void SendEvent(const Event &);
  private:
    std::string m_type, m_name;
    TransportClient * m_dataclient;
  };

}

#endif // EUDAQ_INCLUDED_DataSender
