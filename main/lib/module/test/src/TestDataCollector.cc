#include "eudaq/DataCollector.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <thread>
#include <chrono>

class TestDataCollector : public eudaq::DataCollector {
public:
  TestDataCollector(const std::string & name, const std::string & runcontrol,
		    const std::string & listenaddress, const std::string & runnumberfile);
  void OnConnect(const eudaq::ConnectionInfo & id) override final;
  void OnConfigure(const eudaq::Configuration & param) override final;
  void OnStartRun(unsigned param) override final;
  void OnTerminate() override final;
  void OnReset() override final;
  void OnUnrecognised(const std::string & cmd, const std::string & param) override final;
  void Exec() override final;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("TestDataCollector");
private:
  bool m_done;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<TestDataCollector, const std::string&, const std::string&,
	     const std::string&, const std::string&>(TestDataCollector::m_id_factory);
}

TestDataCollector::TestDataCollector(const std::string &name, const std::string &runcontrol,
				     const std::string &listenaddress,
				     const std::string &runnumberfile)
  :eudaq::DataCollector(name, runcontrol, listenaddress, runnumberfile), m_done(false){

}

void TestDataCollector::OnConnect(const eudaq::ConnectionInfo & id){
  DataCollector::OnConnect(id);
  std::cout << "Connect:    " << id << std::endl;
}

void TestDataCollector::OnConfigure(const eudaq::Configuration & param) {
  std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
  DataCollector::OnConfigure(param);
  std::cout << "...Configured (" << param.Name() << ")" << std::endl;
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}

void TestDataCollector::OnStartRun(unsigned param) {
  DataCollector::OnStartRun(param);
  SetStatus(eudaq::Status::LVL_OK);
}

void TestDataCollector::OnTerminate() {
  std::cout << "Terminating" << std::endl;
  m_done = true;
}

void TestDataCollector::OnReset() {
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void TestDataCollector::OnUnrecognised(const std::string & cmd, const std::string & param) {
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Just testing");
}

void TestDataCollector::Exec(){
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (!m_done);
}
