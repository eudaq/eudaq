#include <iostream>
#include <ostream>
#include <thread>
#include "eudaq/DataCollector.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/JSONimpl.hh"

#include "config.h"

namespace eudaq {

  namespace {

    void * DataCollector_thread(void * arg) {
      DataCollector * dc = static_cast<DataCollector *>(arg);
      dc->DataThread();
      return 0;
    }

  } // anonymous namespace

  DataCollector::DataCollector(const std::string & name, const std::string & runcontrol, const std::string & listenaddress, const std::string & runnumberfile) :
    CommandReceiver("DataCollector", name, runcontrol, false),
    m_runnumberfile(runnumberfile), m_name( name ), m_done(false), m_listening(true),
    m_dataserver(TransportFactory::CreateServer(listenaddress)),
    m_thread(), m_numwaiting(0), m_itlu((size_t) -1), m_runnumber( ReadFromFile(runnumberfile, 0U)), m_eventnumber(0), m_runstart(0), m_packetNumberLastPacket( 0 )
  {
      m_dataserver->SetCallback(TransportCallback(this, &DataCollector::DataHandler));
      EUDAQ_DEBUG("Instantiated datacollector with name: " + name);
      m_thread=std::unique_ptr<std::thread>(new std::thread(DataCollector_thread,this));
      EUDAQ_DEBUG("Listen address=" + to_string(m_dataserver->ConnectionString()));
      CommandReceiver::StartThread();
  }

  DataCollector::~DataCollector() {
    m_done = true;
    m_thread->join();
    delete m_dataserver;
  }

  void DataCollector::OnServer() {
    m_status.SetTag("_SERVER", m_dataserver->ConnectionString());
  }

  void DataCollector::OnGetRun() {
    //std::cout << "Sending run number: " << m_runnumber << std::endl;
    m_status.SetTag("_RUN", to_string(m_runnumber));
  }

  void DataCollector::OnConnect(const ConnectionInfo & id) {
    EUDAQ_INFO("Connection from " + to_string(id));
    Info info;
    info.id = std::shared_ptr<ConnectionInfo>(id.Clone());
    m_buffer.push_back(info);
    if (id.GetType() == "Producer" && id.GetName() == "TLU") {
      m_itlu = m_buffer.size() - 1;
    }
  }

  void DataCollector::OnDisconnect(const ConnectionInfo & id) {
    EUDAQ_INFO("Disconnected: " + to_string(id));
    size_t i = GetInfo(id);
    if (i == m_itlu) {
      m_itlu = (size_t) -1;
    } else if (i < m_itlu) {
      --m_itlu;
    }
    m_buffer.erase(m_buffer.begin() + i);
    // if (during run) THROW
  }

  void DataCollector::OnConfigure(const Configuration & param) {
    m_config = param;
    m_writer =  std::unique_ptr<eudaq::AidaFileWriter>(AidaFileWriterFactory::Create(m_config.Get("FileType", "")));
    m_writer->SetFilePattern(m_config.Get("FilePattern", ""));
  }

  void DataCollector::OnPrepareRun(unsigned runnumber) {
    //if (runnumber == m_runnumber && m_ser.get()) return false;
    EUDAQ_INFO("Preparing for run " + to_string(runnumber));
    m_runstart = Time::Current();
    try {
      if (!m_writer) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      m_writer->StartRun( m_name, runnumber, buildJsonConfigHeader( runnumber ) );
      WriteToFile(m_runnumberfile, runnumber);
      m_runnumber = runnumber;
      m_eventnumber = 0;

      for (size_t i = 0; i < m_buffer.size(); ++i) {
        if (m_buffer[i].events.size() > 0) {
          EUDAQ_WARN("Buffer " + to_string(*m_buffer[i].id) + " has " + to_string(m_buffer[i].events.size()) + " events remaining.");
          m_buffer[i].events.clear();
        }
      }
      m_numwaiting = 0;

      SetStatus(Status::LVL_OK);
    } catch (const Exception & e) {
      std::string msg = "Error preparing for run " + to_string(runnumber) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }

  void DataCollector::OnStopRun() {
    EUDAQ_INFO("End of run " + to_string(m_runnumber));
    // Leave the file open, more events could still arrive
    //m_ser = counted_ptr<FileSerializer>();
  }

  void DataCollector::OnReceive(const ConnectionInfo & id, std::shared_ptr<AidaPacket> packet ) {
	  WritePacket( packet );
  }



  void DataCollector::OnReceive(const ConnectionInfo & id, std::shared_ptr<Event> ev ) {
    //std::cout << "Received Event from " << id << ": " << *ev << std::endl;

    Info & inf = m_buffer[GetInfo(id)];
    inf.events.push_back(ev);
    bool tmp = false;
    if (inf.events.size() == 1) {
      m_numwaiting++;
      if (m_numwaiting == m_buffer.size()) {
        tmp = true;
      }
    }
    //std::cout << "Waiting buffers: " << m_numwaiting << " out of " << m_buffer.size() << std::endl;

    if (tmp)
      OnCompleteEvent();
  }

  void DataCollector::OnStatus() {
    std::string evt;
    if (m_eventnumber > 0)
      evt = to_string(m_eventnumber - 1);
    m_status.SetTag("EVENT", evt);
    m_status.SetTag("RUN", to_string(m_runnumber));
    if (m_writer.get())
      m_status.SetTag("FILEBYTES", to_string(m_writer->FileBytes()));
  }


  void DataCollector::OnCompleteEvent() {
    bool more = true;
    while (more) {
      if (m_eventnumber < 10 || m_eventnumber % 100 == 0) {
        std::cout << "Complete Event: " << m_runnumber << "." << m_eventnumber << std::endl;
      }
      unsigned n_run = m_runnumber, n_ev = m_eventnumber;
      uint64_t n_ts = (uint64_t) -1;
      if (m_itlu != (size_t) -1) {
        TLUEvent * ev = static_cast<TLUEvent *>(m_buffer[m_itlu].events.front().get());
        n_run = ev->GetRunNumber();
        n_ev = ev->GetEventNumber();
        n_ts = ev->GetTimestamp();
      }
      DetectorEvent ev(n_run, n_ev, n_ts);
      unsigned tluev = 0;
      for (size_t i = 0; i < m_buffer.size(); ++i) {
        if (m_buffer[i].events.front()->GetRunNumber() != m_runnumber) {
          EUDAQ_ERROR("Run number mismatch in event " + to_string(ev.GetEventNumber()));
        }
        if ((m_buffer[i].events.front()->GetEventNumber() != m_eventnumber) && (m_buffer[i].events.front()->GetEventNumber() != m_eventnumber - 1)) {
          if (ev.GetEventNumber() % 1000 == 0) {
            // dhaas: added if-statement to filter out TLU event number 0, in case of bad clocking out
            if (m_buffer[i].events.front()->GetEventNumber() != 0)
              EUDAQ_WARN("Event number mismatch > 2 in event " + to_string(ev.GetEventNumber()) + " " + to_string(m_buffer[i].events.front()->GetEventNumber()) + " " + to_string(m_eventnumber));
            if (m_buffer[i].events.front()->GetEventNumber() == 0)
              EUDAQ_WARN("Event number mismatch > 2 in event " + to_string(ev.GetEventNumber()));
          }
        }
        ev.AddEvent(m_buffer[i].events.front());
        m_buffer[i].events.pop_front();
        if (m_buffer[i].events.size() == 0) {
          m_numwaiting--;
          more = false;
        }
      }
      if (ev.IsBORE()) {
        ev.SetTag("STARTTIME", m_runstart.Formatted());
        ev.SetTag("CONFIG", to_string(m_config));
      }
      if (ev.IsEORE()) {
        ev.SetTag("STOPTIME", Time::Current().Formatted());
        EUDAQ_INFO("Run " + to_string(ev.GetRunNumber()) + ", EORE = " + to_string(ev.GetEventNumber()));
      }
      WriteEvent( ev );
    }
  }


  void DataCollector::WriteEvent( const DetectorEvent & ev ) {
	  auto packet = std::shared_ptr<AidaPacket>( new EventPacket( ev ) );
	  packet->SetPacketNumber( ++m_packetNumberLastPacket );
	  WritePacket( packet );
	  // std::cout << ev << std::endl;
	  ++m_eventnumber;
  }


  void DataCollector::WritePacket( std::shared_ptr<AidaPacket> packet ) {
	  if (m_writer.get()) {
		  try {
			  m_packetNumberLastPacket = packet->GetPacketNumber();
			  m_writer->WritePacket( packet );
		  } catch(const Exception & e) {
			  std::string msg = "Exception writing to file: ";
			  msg += e.what();
			  EUDAQ_ERROR(msg);
			  SetStatus(Status::LVL_ERROR, msg);
		  }
	  } else {
		  EUDAQ_ERROR("Event received before start of run");
	  }
  }



  size_t DataCollector::GetInfo(const ConnectionInfo & id) {
    for (size_t i = 0; i < m_buffer.size(); ++i) {
      //std::cout << "Checking " << *m_buffer[i].id << " == " << id<< std::endl;
      if (m_buffer[i].id->Matches(id))
        return i;
    }
    EUDAQ_THROW("Unrecognised connection id");
  }

  void DataCollector::DataHandler(TransportEvent & ev) {
    //std::cout << "Event: ";
    switch (ev.etype) {
      case (TransportEvent::CONNECT):
        //std::cout << "Connect:    " << ev.id << std::endl;
        if (m_listening) {
          m_dataserver->SendPacket("OK EUDAQ DATA DataCollector", ev.id, true);
        } else {
          m_dataserver->SendPacket("ERROR EUDAQ DATA Not accepting new connections", ev.id, true);
          m_dataserver->Close(ev.id);
        }
        break;
      case (TransportEvent::DISCONNECT):
        //std::cout << "Disconnect: " << ev.id << std::endl;
        OnDisconnect(ev.id);
        break;
      case (TransportEvent::RECEIVE):
        if (ev.id.GetState() == 0) { // waiting for identification
          // check packet
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
          //std::cout << "client replied, sending OK" << std::endl;
          m_dataserver->SendPacket("OK", ev.id, true);
          ev.id.SetState(1); // successfully identified
          OnConnect(ev.id);
        } else {
        	if ( ev.packet.find( AidaPacket::identifier().string ) == 0 ) {
        		BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        		std::shared_ptr<AidaPacket> packet( PacketFactory::Create( ser ) );
        		OnReceive( ev.id, packet );
        		// this is a packet
        	} else {
        		//std::cout << "Receive: " << ev.id << " " << ev.packet.size() << std::endl;
        		//for (size_t i = 0; i < 8 && i < ev.packet.size(); ++i) {
        		//    std::cout << to_hex(ev.packet[i], 2) << ' ';
        		//}
        		//std::cout << ")" << std::endl;
        		BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        		//std::cout << "Deserializing" << std::endl;
        		std::shared_ptr<Event> event(EventFactory::Create(ser));
        		//std::cout << "Done" << std::endl;
        		OnReceive(ev.id, event );
        		//std::cout << "End" << std::endl;
        	}
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
    } catch (const std::exception & e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n" << "DataThread is dying..." << std::endl;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n" << "DataThread is dying..." << std::endl;
    }
  }


  std::shared_ptr<JSON> DataCollector::buildJsonConfigHeader( unsigned int runnumber ) {
	  auto J = JSON::Create();

	  jsoncons::json& header = JSONimpl::get( J.get() );
	  header["runnumber"] = runnumber;
	  header["package_name"] = PACKAGE_NAME;
	  header["package_version"] = PACKAGE_VERSION;
	  header["date"] = Time::Current().Formatted();
	  header["schema"] = AidaPacket::getCurrentSchema();

	  header["sources"] = jsoncons::json::an_array;
	  for ( auto info : m_buffer ) {
		  jsoncons::json src;
		  src["type"] = info.id->GetType();
		  src["name"] = info.id->GetName();
		  header["sources"].add( src );
	  }

	  return J;
  }

}
