#ifndef EUDAQ_INCLUDED_SLOWPRODUCER
#define EUDAQ_INCLUDED_SLOWPRODUCER

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/Platform.hh"
#include <string>

namespace eudaq {
    /**
     * The base class from which all SlowProducers should inherit.
     * It is both a CommandReceiver, listening to commands from RunControl,
     * and a DataSender, sending data to a DataCollector.
     * SlowProducer can generate less events then simple producer and data
     * collector doesn't have to wait an event from SlowProducer to complete
     * receiving data from producers.
     */
    class DLLEXPORT SlowProducer : public DataSender, public CommandReceiver {
    public:
        /**
         * The constructor.
         * \param runcontrol A string containing the address of the RunControl to
         * connect to.
         */
        SlowProducer(const std::string &name, const std::string &runcontrol);
        virtual ~SlowProducer() {}

        virtual void OnData(const std::string &param);

      private:
    };

}
#endif // EUDAQ_INCLUDED_SLOWPRODUCER
