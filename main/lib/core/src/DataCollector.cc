#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <ostream>
#include <ctime>
#include <iomanip>
namespace eudaq {
  template class DLLEXPORT Factory<DataCollector>;
  template DLLEXPORT std::map<uint32_t, typename Factory<DataCollector>::UP_BASE (*)
			      (const std::string&, const std::string&)>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&>(); //TODO
  
  DataCollector::DataCollector(const std::string &name, const std::string &runcontrol)
    :CommandReceiver("DataCollector", name, runcontrol){
    m_dct_n= str2hash(GetFullName());
    m_evt_c = 0;
  }

  DataCollector::~DataCollector(){  
  }

  void DataCollector::DoInitialise(){
  }
  
  void DataCollector::DoConfigure(){
  }
  
  void DataCollector::DoStartRun(){
  }
  
  void DataCollector::DoStopRun(){
  }
  
  void DataCollector::DoReset(){
  }
  
  void DataCollector::DoTerminate(){
  }
  
  void DataCollector::DoStatus(){
  }

  void DataCollector::DoConnect(ConnectionSPC id){
  }
  
  void DataCollector::DoDisconnect(ConnectionSPC id){
  }
  
  void DataCollector::DoReceive(ConnectionSPC id, EventSP ev){
  }

  void DataCollector::SetServerAddress(const std::string &addr){
    m_data_addr = addr;
  }
  
  void DataCollector::OnInitialise(){
    EUDAQ_INFO(GetFullName() + " is to be initialised...");
    auto conf = GetConfiguration();
    try{
      m_data_addr = Listen(m_data_addr);
      SetStatusTag("_SERVER", m_data_addr);
      StopListen();
      DoInitialise();
      CommandReceiver::OnInitialise();
    }catch (const Exception &e) {
      std::string msg = "Error when init by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
  
  void DataCollector::OnConfigure(){
    EUDAQ_INFO(GetFullName() + " is to be configured...");
    auto conf = GetConfiguration();
    try {
      SetStatus(Status::STATE_UNCONF, "Configuring");
      m_fwtype = conf->Get("EUDAQ_FW", "native");
      m_fwpatt = conf->Get("EUDAQ_FW_PATTERN", "$12D_run$6R$X");
      m_dct_n = conf->Get("EUDAQ_ID", m_dct_n);
      DoConfigure();
      CommandReceiver::OnConfigure();
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
  
  void DataCollector::OnStartRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be started...");
    try {
      m_data_addr = Listen(m_data_addr);
      SetStatusTag("_SERVER", m_data_addr);
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(m_fwtype), m_fwpatt);
      m_evt_c = 0;

      std::string mn_str = GetConfiguration()->Get("EUDAQ_MN", "");
      std::vector<std::string> col_mn_name = split(mn_str, ";,", true);
      std::string cur_backup = GetConfiguration()->GetCurrentSectionName();
      GetConfiguration()->SetSection("");
      for(auto &mn_name: col_mn_name){
	std::string mn_addr =  GetConfiguration()->Get("Monitor."+mn_name, "");
	if(!mn_addr.empty()){
	  m_senders[mn_addr]
	    = std::unique_ptr<DataSender>(new DataSender("DataCollector", GetName()));
	  m_senders[mn_addr]->Connect(mn_addr);
	}
      }
      GetConfiguration()->SetSection(cur_backup);
      DoStartRun();
      CommandReceiver::OnStartRun();
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void DataCollector::OnStopRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be stopped...");
    try {
      DoStopRun();
      m_senders.clear();
      StopListen();
      CommandReceiver::OnStopRun();
    } catch (const Exception &e) {
      std::string msg = "Error stopping for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void DataCollector::OnReset(){
    EUDAQ_INFO(GetFullName() + " is to be reset...");
    try{
      DoReset();
      m_senders.clear();
      StopListen();
      CommandReceiver::OnReset();
    } catch (const std::exception &e) {
      EUDAQ_THROW( std::string("DataCollector Reset:: Caught exception: ") + e.what() );
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      EUDAQ_THROW("DataCollector Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void DataCollector::OnTerminate(){
    EUDAQ_INFO(GetFullName() + " is to be terminated...");
    DoTerminate();
    CommandReceiver::OnTerminate();
  }
    
  void DataCollector::OnStatus(){
    SetStatusTag("EventN", std::to_string(m_evt_c));
    DoStatus();
    if(m_writer && m_writer->FileBytes()){
      SetStatusTag("FILEBYTES", std::to_string(m_writer->FileBytes()));
    }
  }

  void DataCollector::OnConnect(ConnectionSPC id){
    DoConnect(id);
  }
    
  void DataCollector::OnDisconnect(ConnectionSPC id){
    DoDisconnect(id);
  }
    
  void DataCollector::OnReceive(ConnectionSPC id, EventSP ev){
    DoReceive(id, ev);
  }  
    
  void DataCollector::WriteEvent(EventSP ev){
    try{
      if(ev->IsBORE()){
	if(GetConfiguration())
	  ev->SetTag("EUDAQ_CONFIG", to_string(*GetConfiguration()));
	if(GetInitConfiguration())
	  ev->SetTag("EUDAQ_CONFIG_INIT", to_string(*GetInitConfiguration()));
      }
      
      ev->SetRunN(GetRunNumber());
      ev->SetEventN(m_evt_c);
      SetStatusTag("EventN", std::to_string(m_evt_c));
      m_evt_c ++;
      ev->SetStreamN(m_dct_n);
      if(m_writer)
	m_writer->WriteEvent(ev);
      else
	EUDAQ_THROW("FileWriter is not created before writing.");
      for(auto &e: m_senders){
	if(e.second)
	  e.second->SendEvent(ev);
	else
	  EUDAQ_THROW("DataCollector::WriterEvent, using a null pointer of DataSender");
      }
    }catch (const Exception &e) {
      std::string msg = "Exception writing to file: ";
      msg += e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  DataCollectorSP DataCollector::Make(const std::string &code_name,
				      const std::string &run_name,
				      const std::string &runcontrol){
    return Factory<DataCollector>::
      MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(code_name), run_name, runcontrol);
  }
}
