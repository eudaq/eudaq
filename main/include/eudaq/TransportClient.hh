#ifndef EUDAQ_INCLUDED_TransportClient
#define EUDAQ_INCLUDED_TransportClient

#include "eudaq/TransportBase.hh"
#include <string>

namespace eudaq {

  class TransportClient : public TransportBase {
    public:
      //virtual void SendPacket(const std::string & packet) = 0;
      //virtual bool ReceivePacket(std::string * packet, int timeout = -1) = 0;

      virtual ~TransportClient();
  };

}

#endif // EUDAQ_INCLUDED_TransportClient
