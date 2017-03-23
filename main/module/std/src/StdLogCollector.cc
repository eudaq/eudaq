#include "eudaq/LogCollector.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <thread>
#include <chrono>
namespace eudaq{

  class StdLogCollector : public eudaq::LogCollector {
  public:
    StdLogCollector(const std::string &runcontrol,
		     const std::string &listenaddress,
		     const std::string &directory,
		     int loglevel);
    void DoConnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void DoDisconnect(std::shared_ptr<const ConnectionInfo> id) override final;
    void DoReceive(const eudaq::LogMessage & ev) override final;
    void DoTerminate() override final;
    static const uint32_t m_id_factory = eudaq::cstr2hash("StdLogCollector");
  private:
    int m_loglevel;
  };

  namespace{
    auto dummy0 = eudaq::Factory<eudaq::LogCollector>::
      Register<StdLogCollector, const std::string&, const std::string&,
	       const std::string&, const int&>(StdLogCollector::m_id_factory);
  }

  StdLogCollector::StdLogCollector(const std::string & runcontrol,
				     const std::string & listenaddress,
				     const std::string & directory,
				     int loglevel)
    :eudaq::LogCollector(runcontrol, listenaddress, directory),
     m_loglevel(loglevel){
  }

  void StdLogCollector::DoConnect(std::shared_ptr<const ConnectionInfo> id){
    std::cout << "Connect:    " << *id << std::endl;
  }

  void StdLogCollector::DoDisconnect(std::shared_ptr<const ConnectionInfo> id){
    std::cout << "Disconnect: " << *id << std::endl;
  }
  
  void StdLogCollector::DoReceive(const eudaq::LogMessage & ev){
    std::cout<<"Log message: "<< ev <<std::endl;
    if (ev.GetLevel() >= m_loglevel)
      std::cout << ev << std::endl;
  }

  void StdLogCollector::DoTerminate(){
    std::cout << "Terminating" << std::endl;
  }

}
