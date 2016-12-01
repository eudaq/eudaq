#include "eudaq/LogCollector.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <thread>
#include <chrono>

class TestLogCollector : public eudaq::LogCollector {
  public:
  TestLogCollector(const std::string &runcontrol,
		   const std::string &listenaddress,
		   const std::string &directory,
		   int loglevel);
  void OnConnect(const eudaq::ConnectionInfo & id) override final;
  void OnDisconnect(const eudaq::ConnectionInfo & id) override final;
  void OnReceive(const eudaq::LogMessage & ev) override final;
  void OnConfigure(const eudaq::Configuration & param) override final;
  void OnStartRun(unsigned param) final;
  void OnStopRun()override final;
  void OnTerminate() override final;
  void OnReset() override final;
  void OnStatus() override final;
  void OnUnrecognised(const std::string & cmd, const std::string & param) override final;
  void Exec() override final;

  static const uint32_t m_id_factory = eudaq::cstr2hash("TestLogCollector");
private:
  int m_loglevel;
  bool m_done;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::LogCollector>::
    Register<TestLogCollector, const std::string&, const std::string&,
	     const std::string&, const int&>(TestLogCollector::m_id_factory);
}


TestLogCollector::TestLogCollector(const std::string & runcontrol,
				   const std::string & listenaddress,
				   const std::string & directory,
				   int loglevel)
  :eudaq::LogCollector(runcontrol, listenaddress, directory),
  m_loglevel(loglevel), m_done(false){

}

void TestLogCollector::OnConnect(const eudaq::ConnectionInfo & id){
  std::cout << "Connect:    " << id << std::endl;
}

void TestLogCollector::OnDisconnect(const eudaq::ConnectionInfo & id){
  std::cout << "Disconnect: " << id << std::endl;
}

void TestLogCollector::OnReceive(const eudaq::LogMessage & ev){
  if (ev.GetLevel() >= m_loglevel) std::cout << ev << std::endl;
}

void TestLogCollector::OnConfigure(const eudaq::Configuration & param){
  std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
  LogCollector::OnConfigure(param);
  std::cout << "...Configured (" << param.Name() << ")" << std::endl;
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}

void TestLogCollector::OnStartRun(unsigned param) {
  LogCollector::OnStartRun(param);
  SetStatus(eudaq::Status::LVL_OK, "Running");
}

void TestLogCollector::OnStopRun(){
  std::cout << "Stop Run" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void TestLogCollector::OnTerminate(){
  SetStatus(eudaq::Status::LVL_OK, "LC Terminating");
  std::cout << "Terminating" << std::endl;
  m_done = true;
}

void TestLogCollector::OnReset(){
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void TestLogCollector::OnStatus(){
}

void TestLogCollector::OnUnrecognised(const std::string & cmd, const std::string & param){
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_ERROR, "Just testing");
}

void TestLogCollector::Exec(){
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (!m_done);
}
