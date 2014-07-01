#ifndef EUDAQ_INCLUDED_DataSender
#define EUDAQ_INCLUDED_DataSender

#include "eudaq/AidaPacket.hh"
#include "eudaq/Event.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/Platform.hh"
#include <string>

namespace eudaq {

  class DLLEXPORT DataSender {
    public:
      DataSender(const std::string & type, const std::string & name);
      ~DataSender();
      void Connect(const std::string & server);
      void SendEvent(const Event &);
      void SendPacket(const AidaPacket &);
    private:
      std::string m_type, m_name;
      bool m_packetreceiver;
      TransportClient * m_dataclient;
  };

}

#endif // EUDAQ_INCLUDED_DataSender
