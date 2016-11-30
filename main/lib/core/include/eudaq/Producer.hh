#ifndef EUDAQ_INCLUDED_Producer
#define EUDAQ_INCLUDED_Producer

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataSender.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"
#include <string>

namespace eudaq {
  class Producer;
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Producer>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Producer>::UP_BASE (*)
	   (const std::string&, const std::string&)>&
  Factory<Producer>::Instance<const std::string&, const std::string&>();  
#endif
  
  /**
   * The base class from which all Producers should inherit.
   * It is both a CommandReceiver, listening to commands from RunControl,
   * and a DataSender, sending data to a DataCollector.
   */
  class DLLEXPORT Producer : public CommandReceiver{
  public:
    // static ProducerSP MakeShared(const std::string& pstype,
    // 				 std::initializer_list
    // 				 <std::pair<const std::string, const std::string>> l = {{}});
    /**
     * The constructor.
     * \param runcontrol A string containing the address of the RunControl to
     * connect to.
     */
    Producer(const std::string &name, const std::string &runcontrol);
    void OnData(const std::string &param) override;
    void Connect(const std::string & server);
    void SendEvent(const Event &);
    
    virtual void MainLoop(){};

  private:
    std::string m_name;
    std::map<std::string, std::unique_ptr<DataSender>> m_senders;
  };
}

#endif // EUDAQ_INCLUDED_Producer
