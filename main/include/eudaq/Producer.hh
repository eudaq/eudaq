#ifndef EUDAQ_INCLUDED_Producer
#define EUDAQ_INCLUDED_Producer

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include <string>

namespace eudaq {

  /**
   * The base class from which all Producers should inherit.
   * It is both a CommandReceiver, listening to commands from RunControl,
   * and a DataSender, sending data to a DataCollector.
   */
  class Producer : public CommandReceiver, public DataSender {
    public:
      /**
       * The constructor.
       * \param runcontrol A string containing the address of the RunControl to connect to.
       */
      Producer(const std::string & name, const std::string & runcontrol);
      virtual ~Producer() {}

      virtual void OnData(const std::string & param);
    private:
  };

}

#endif // EUDAQ_INCLUDED_Producer
