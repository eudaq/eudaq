#ifndef EUDAQ_INCLUDED_TransportFactory
#define EUDAQ_INCLUDED_TransportFactory

#include "eudaq/TransportClient.hh"
#include "eudaq/TransportServer.hh"

namespace eudaq {

  /** Stores the name and factory methods for a type of Transport.
   */
  struct TransportInfo {
    typedef TransportServer * (*ServerFactory)(const std::string &);
    typedef TransportClient * (*ClientFactory)(const std::string &);
    std::string name;
    ServerFactory serverfactory;
    ClientFactory clientfactory;
  };

  /** Creates Transport instances from a name without needing to know the concrete type at compile time.
   */
  class TransportFactory {
  public:
    static TransportServer * CreateServer(const std::string & name);
    static TransportClient * CreateClient(const std::string & name);

    static void Register(const TransportInfo & info);
  };

  /** A utility template class for registering a Transport with the TransportFactory.
   */
  template <typename T_Server, typename T_Client>
  struct RegisterTransport {
    RegisterTransport(const std::string & name) {
      TransportInfo info = { name, &serverfactory, &clientfactory };
      TransportFactory::Register(info);
    }
    static TransportServer * serverfactory(const std::string & param) {
      return new T_Server(param);
    }
    static TransportClient * clientfactory(const std::string & param) {
      return new T_Client(param);
    }
  };

}

#endif // EUDAQ_INCLUDED_TransportFactory
