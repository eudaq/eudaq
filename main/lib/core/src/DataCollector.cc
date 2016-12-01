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
			      (const std::string&, const std::string&,
			       const std::string&, const std::string& )>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&,
				   const std::string&, const std::string&>();
  
  DataCollector::DataCollector(const std::string &name,
                               const std::string &runcontrol,
                               const std::string &listenaddress,
                               const std::string &runnumberfile)
      : CommandReceiver("DataCollector", name, runcontrol, false),
        m_runnumberfile(runnumberfile), m_done(false),
        m_runnumber(ReadFromFile(runnumberfile, 0U)), m_eventnumber(0),
        m_runstart(0) {
    
    m_dataserver.reset(TransportFactory::CreateServer(listenaddress));
    m_dataserver->SetCallback(TransportCallback(this, &DataCollector::DataHandler));
    EUDAQ_DEBUG("Instantiated datacollector with name: " + name);
    m_thread =std::thread(&DataCollector::DataThread, this);
    EUDAQ_DEBUG("Listen address=" + to_string(m_dataserver->ConnectionString()));
    CommandReceiver::StartThread();
  }

  DataCollector::~DataCollector() {
    m_done = true;
    if(m_thread.joinable())
      m_thread.join();
    if(m_thread_writer.joinable())
      m_thread_writer.join();
  }

  void DataCollector::OnServer() {
    m_status.SetTag("_SERVER", m_dataserver->ConnectionString());
  }

  void DataCollector::OnConnect(const ConnectionInfo &id) {
    EUDAQ_INFO("Connection from " + to_string(id));
    m_info_pdc.push_back(id);    
  }

  void DataCollector::OnDisconnect(const ConnectionInfo &id) {
    EUDAQ_INFO("Disconnected: " + to_string(id));
    for (size_t i = 0; i < m_info_pdc.size(); ++i) {
      if (m_info_pdc[i].Matches(id)){
	m_info_pdc.erase(m_info_pdc.begin() + i);
	return;
      }
    }
    EUDAQ_THROW("Unrecognised connection id");
  }

  void DataCollector::OnConfigure(const Configuration &param) {
    Configuration m_config = param;
    std::string fwtype = m_config.Get("FileType", "native");
    std::string fwpatt = m_config.Get("FilePattern", "run$6R_tp$X");

    std::map<uint32_t, ProcessorSP> ps_col;
    m_ps_input.reset();
    m_ps_output = Processor::MakeShared("ExportEventPS");
    std::string config_key = "PS_CHAIN";
    std::string config_val;
    uint32_t n = 0;
    while(1){
      config_val = m_config.Get(config_key+std::to_string(n), "");
      if(config_val.empty())
	break;
      std::cout<<config_key+std::to_string(n)<<"="<<config_val<<std::endl;

      std::stringstream ss(config_val);
      std::string item;
      std::vector<std::string> elems;
      while (std::getline(ss, item, ':')) {
	item=trim(item);
	elems.push_back(item);
      }
      if(elems[2]=="CREATE"){
	uint32_t ps_n = std::stoul(elems[1]);
	ps_col[ps_n] = Processor::MakeShared(elems[3], {{"SYS:PSID", std::to_string(ps_n)}});
      }
      else if(elems[2]=="CMD"){
	uint32_t ps_n = std::stoul(elems[1]);
	std::string cmd = elems[3];
	for(uint32_t i = 4; i< elems.size(); i++)
	  cmd = cmd + ":" + elems[i];
	std::cout<< "cmd ="<<cmd<<"\n";
	ps_col[ps_n]<<cmd;
      }
      else if(elems[2]=="PIPE"){
	uint32_t ps_n = std::stoul(elems[1]);
	uint32_t ps_m = std::stoul(elems.back());
	std::stringstream evss(elems[3]);
	std::string evtype;
	while (std::getline(evss, evtype, '+')) {
	  evtype=trim(evtype);
	  ps_col[ps_n]+evtype;
	}
	ps_col[ps_n]>>ps_col[ps_m];
      }
      else if(elems[2]=="INPUT"){
	uint32_t ps_n = std::stoul(elems[1]);
	m_ps_input = ps_col[ps_n];
      }
      else if(elems[2]=="OUTPUT"){
	uint32_t ps_n = std::stoul(elems[1]);
	ps_col[ps_n]>>m_ps_output;
      }
      n++;
    }
    if(!m_ps_input){ //defualt
      m_ps_input = Processor::MakeShared("SyncByEventNumberPS");
      m_ps_input >> m_ps_output;
    }
    
    if(!m_writer){
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(fwtype), fwpatt);
    }
  }


  void DataCollector::OnStartRun(uint32_t run_n ){
    EUDAQ_INFO("Preparing for run " + std::to_string(run_n));
    m_runstart = Time::Current();
    try {
      if (!m_writer) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      
      m_writer->StartRun(run_n);
      WriteToFile(m_runnumberfile, run_n);
      m_runnumber = run_n;
      
      m_done_writer = false;
      m_thread_writer = std::thread(&DataCollector::WriterThread, this);
      m_ps_input<<"STREAM:SIZE="+std::to_string(m_info_pdc.size());
      SetStatus(Status::LVL_OK);
    } catch (const Exception &e) {
      std::string msg =
	"Error preparing for run " + std::to_string(run_n) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }    
  }

  
  void DataCollector::OnStopRun() {
    EUDAQ_INFO("End of run " + to_string(m_runnumber));
    m_done_writer = true;
    if(m_thread_writer.joinable())
      m_thread_writer.join();
    m_ps_input<<"STREAM:CLEAR";
    m_ps_output<<"EVENT:CLEAR";
  }

  void DataCollector::OnReceive(const ConnectionInfo &id, EventSP ev) {
    if(!m_ps_input)
      return;
    if(ev->IsBORE())
      m_ps_input<<"STREAM::ADD="+std::to_string(ev->GetStreamN());
    m_ps_input<<=ev;
    return;
  }

  void DataCollector::OnStatus() {
    m_status.SetTag("EVENT", std::to_string(m_eventnumber));
    m_status.SetTag("RUN", std::to_string(m_runnumber));
    if (m_writer.get())
      m_status.SetTag("FILEBYTES", std::to_string(m_writer->FileBytes()));
  }


  void DataCollector::DataHandler(TransportEvent &ev) {
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA DataCollector", ev.id, true);
      break;
    case (TransportEvent::DISCONNECT):
      OnDisconnect(ev.id);
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
        OnConnect(ev.id);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	EventSP event = Factory<Event>::Create<Deserializer&>(id, ser);
	OnReceive(ev.id, event);
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void DataCollector::DataThread() {
    try {
      while (!m_done) {
        m_dataserver->Process(100000);
      }
    } catch (const std::exception &e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n"
                << "DataThread is dying..." << std::endl;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "DataThread is dying..." << std::endl;
    }
  }

  void DataCollector::WriterThread() {
    try {
      EventSPC sync_ev;
      auto ps_export = std::dynamic_pointer_cast<ExportEventPS>(m_ps_output);
      while (!m_done && !m_done_writer){
	while(sync_ev = ps_export->GetEvent()){
	  m_writer->WriteEvent(sync_ev);
	  m_eventnumber = sync_ev->GetEventN();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } catch (const Exception &e) {
      std::string msg = "Exception writing to file: ";
      msg += e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }
}
