#include "DataCollector.hh"
#include "TransportFactory.hh"
#include "BufferSerializer.hh"
#include "Logger.hh"
#include "Utils.hh"
#include "Processor.hh"
#include "ExportEventPS.hh"
#include <iostream>
#include <ostream>

namespace eudaq {

  template class DLLEXPORT Factory<DataCollector>;
  template DLLEXPORT std::map<uint32_t, typename Factory<DataCollector>::UP_BASE (*)
			      (const std::string&, const std::string&)>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&>(); //TODO
  
  DataCollector::DataCollector(const std::string &name, const std::string &runcontrol)
    :CommandReceiver("DataCollector", name, runcontrol){  
    EUDAQ_DEBUG("Instantiated datacollector with name: " + name);
    m_run_n = 0;
    m_evt_c = 0;
    m_done = false;
  }

  void DataCollector::OnConfigure(const Configuration &param) {
    m_config = param;
    std::cout << "Configuring (" << m_config.Name() << ")..." << std::endl;
    try {
      std::string fwtype = m_config.Get("FileType", "native");
      std::string fwpatt = m_config.Get("FilePattern", "run$6R_tp$X");
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(fwtype), fwpatt);
      DoConfigure(m_config);
      std::cout << "...Configured (" << m_config.Name() << ")" << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + m_config.Name() + ")");
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + m_config.Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }
  
  void DataCollector::OnServer(){
    std::string listenaddress = m_config.Get("ListenAddress", "tcp://40004"); //TODO
    m_dataserver.reset(TransportFactory::CreateServer(listenaddress));
    m_dataserver->SetCallback(TransportCallback(this, &DataCollector::DataHandler));
    EUDAQ_INFO("DataCollection is listenning to " + to_string(m_dataserver->ConnectionString()));
    SetStatusTag("_SERVER", m_dataserver->ConnectionString());
  }
  
  void DataCollector::OnStartRun(uint32_t run_n){
    EUDAQ_INFO("Preparing for run " + std::to_string(run_n));
    try {
      if (!m_writer) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      if (!m_dataserver) {
        EUDAQ_THROW("You must configure before starting a run");
      }      
      m_writer->StartRun(run_n);
      DoStartRun(run_n);
      SetStatus(Status::LVL_OK);
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(run_n) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }

  void DataCollector::OnStopRun(){
    EUDAQ_INFO("End of run ");
    
  }
  
  void DataCollector::OnTerminate(){
    std::cout << "Terminating" << std::endl;
    m_done = true;
    DoTerminate();
  }
  
  void DataCollector::OnReset(){
    m_dataserver.reset();
    m_writer.reset();
    m_info_pdc.clear();
    m_run_n = 0;
    m_evt_c = 0;
    Configuration conf_empty;
    m_config = conf_empty;
    EUDAQ_INFO("DataCollector is reseted.");
  }
  
  void DataCollector::OnStatus() {
    SetStatusTag("EVENT", std::to_string(m_evt_c));
    SetStatusTag("RUN", std::to_string(m_run_n));
    if(m_writer)
      SetStatusTag("FILEBYTES", std::to_string(m_writer->FileBytes()));
  }
  
  void DataCollector::DataHandler(TransportEvent &ev) {
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA DataCollector", ev.id, true);
      break;
    case (TransportEvent::DISCONNECT):
      EUDAQ_INFO("Disconnected: " + to_string(ev.id));
      for (size_t i = 0; i < m_info_pdc.size(); ++i) {
	if (m_info_pdc[i].Matches(ev.id)){
	  m_info_pdc.erase(m_info_pdc.begin() + i);
	  return;
	}
      }
      EUDAQ_THROW("Unrecognised connection id");
      break;
    case (TransportEvent::RECEIVE):
      if (ev.id.GetState() == 0) { // waiting for identification
        do {
          size_t i0 = 0, i1 = ev.packet.find(' ');
          if (i1 == std::string::npos)
            break;
          std::string part(ev.packet, i0, i1);
          if (part != "OK")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos)
            break;
          part = std::string(ev.packet, i0, i1 - i0);
          if (part != "EUDAQ")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos)
            break;
          part = std::string(ev.packet, i0, i1 - i0);
          if (part != "DATA")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          ev.id.SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          ev.id.SetName(part);
        } while (false);
        m_dataserver->SendPacket("OK", ev.id, true);
        ev.id.SetState(1); // successfully identified
	EUDAQ_INFO("Connection from " + to_string(ev.id));
	m_info_pdc.push_back(ev.id);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	EventUP event = Factory<Event>::MakeUnique<Deserializer&>(id, ser);
	DoReceive(ev.id, std::move(event));
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }
  
  void DataCollector::WriteEvent(EventUP ev){
    try{
      ev->SetRunN(m_run_n);
      ev->SetEventN(m_evt_c);
      m_evt_c ++;
      ev->SetStreamN(m_dct_n);
      EventSP evsp(std::move(ev));
      if(m_writer)
	m_writer->WriteEvent(evsp);
      else
	EUDAQ_THROW("FileWriter is not created before writing.");
    }catch (const Exception &e) {
      std::string msg = "Exception writing to file: ";
      msg += e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }

  void DataCollector::Exec(){
    try {
      while (!m_done){
	Process();
	//TODO: sleep here is needed.
      }
    } catch (const std::exception &e) {
      std::cout <<"DataCollector::Exec() Error: Uncaught exception: " <<e.what() <<std::endl;
    } catch (...) {
      std::cout <<"DataCollector::Exec() Error: Uncaught unrecognised exception" <<std::endl;
    }
  }

}
