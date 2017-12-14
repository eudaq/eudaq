#ifndef EUDAQ_INCLUDED_Controller
#define EUDAQ_INCLUDED_Controller

#include "eudaq/CommandReceiver.hh"
#include "eudaq/Platform.hh"
#include <string>

namespace eudaq {

  /**
   * The base class from which all Controllers should inherit.
   * It is solely a CommandReceiver, listening to commands from RunControl.
   */
  class DLLEXPORT Controller : public CommandReceiver {
  public:
    /**
     * The constructor.
     * \param runcontrol A string containing the address of the RunControl to
     * connect to.
     */
    Controller(const std::string &name, const std::string &runcontrol);
    virtual ~Controller() {}

  private:
  };
}

#endif // EUDAQ_INCLUDED_Controller
