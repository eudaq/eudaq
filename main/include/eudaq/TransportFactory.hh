#ifndef EUDAQ_INCLUDED_TransportFactory
#define EUDAQ_INCLUDED_TransportFactory

#include "eudaq/TransportClient.hh"
#include "eudaq/TransportServer.hh"

namespace eudaq {

  /** Creates Transport instances from a name without needing to know the concrete type at compile time.
   */
  class TransportFactory {
  public:
    static TransportServer * CreateServer(const std::string & name);
    static TransportClient * CreateClient(const std::string & name);

    struct TransportInfo;
    static void Register(const TransportInfo & info);
  };

}

#endif // EUDAQ_INCLUDED_TransportFactory
