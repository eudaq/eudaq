#include <iostream>
#include <ostream>
#include <thread>
#include <memory>
#include "eudaq/DataCollector.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/JSONimpl.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/EventSynchronisationBase.hh"


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
    m_thread(),  m_runnumber( ReadFromFile(runnumberfile, 0U)), m_eventnumber(0), m_runstart(0), m_packetNumberLastPacket( 0 )
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

  }

  void DataCollector::OnDisconnect(const ConnectionInfo & id) {
    EUDAQ_INFO("Disconnected: " + to_string(id));
    size_t i = GetInfo(id);
    m_buffer.erase(m_buffer.begin() + i);
    // if (during run) THROW
  }

  void DataCollector::OnConfigure(const Configuration & param) {
    m_config = param;
    m_writer = FileWriterFactory::Create(m_config.Get("FileType", FileWriterFactory::getDefaultType()));
      
      // std::unique_ptr<eudaq::AidaFileWriter>(AidaFileWriterFactory::Create(m_config.Get("FileType", "")));
    m_writer->SetFilePattern(m_config.Get("FilePattern", ""));
    m_expected_data_streams = m_config.Get("expected_data_streams", m_buffer.size());
    m_nameOfSyncAlgorithm = m_config.Get("SyncAlgorithm", "DetectorEvents");
    m_sync = eudaq::EventSyncFactory::create(m_nameOfSyncAlgorithm, m_config);
  }

  void DataCollector::OnPrepareRun(unsigned runnumber) {
    //if (runnumber == m_runnumber && m_ser.get()) return false;
    EUDAQ_INFO("Preparing for run " + to_string(runnumber));
    m_runstart = Time::Current();
    try {
      if (!m_writer) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      m_writer->StartRun(runnumber);
      WriteToFile(m_runnumberfile, runnumber);
      m_runnumber = runnumber;
      m_eventnumber = 0;
            
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
    while (!m_sync->OutputIsEmpty())
    {
      std::cout << "waiting for Outputqueue " << std::endl;
      eudaq::mSleep(20);
    }
    m_sync.reset();
    m_sync = eudaq::EventSyncFactory::create(m_nameOfSyncAlgorithm, m_config);
    std::cout << "stopping " << std::endl;
  }





  void DataCollector::OnReceive(const ConnectionInfo & id, std::shared_ptr<Event> ev ) {
    //std::cout << "Received Event from " << id << ": " << *ev << std::endl;
    if (!m_sync||!m_writer)
    {
      EUDAQ_WARN("data collector: not ready ");
      return;
    }
    if (!m_sync->pushEvent(ev, GetInfo(id))){
      EUDAQ_WARN("no more data from producer: " + id.GetName()); 

    }
    if (ev->IsBORE())
    {
      ++m_numOfBoreEvents;
      if (m_numOfBoreEvents == m_expected_data_streams)
      {
        //all bore events seen yet 
        // since the merging of bore events happens only when the sync base has
        //seen a real event in this data stream one has to skip this event.
        // it will be processed in the when the next events comes in. it is not lost just waiting in the queue

        return;
      }
    }


    if (m_numOfBoreEvents < m_expected_data_streams)
    {
      //not all bore events seen yet 
      return;
    }

    if (m_numOfBoreEvents>m_expected_data_streams)
    {
      EUDAQ_WARN("to many bore events ");
    }

    std::shared_ptr<Event> dev;
    while(m_sync->getNextEvent(dev)&&m_writer){
      if (dev->IsBORE()) {
        dev->SetTag("STARTTIME", m_runstart.Formatted());
        dev->SetTag("CONFIG", to_string(m_config));
      }
      if (dev->IsEORE()) {
        dev->SetTag("STOPTIME", Time::Current().Formatted());
        EUDAQ_INFO("Run " + to_string(dev->GetRunNumber()) + ", EORE = " + to_string(dev->GetEventNumber()));
      }

      m_writer->WriteBaseEvent(*dev);
      
      if (m_eventnumber <= 10 || m_eventnumber % 100 == 0) {
        std::cout << "Complete Event: " << m_runnumber << "." << m_eventnumber << std::endl;
      }
      ++m_eventnumber;

    }
      
  }

  void DataCollector::OnStatus() {
    std::string evt;
    if (m_eventnumber > 0)
      evt = to_string(m_eventnumber - 1);
    m_status.SetTag("EVENT", evt);
    m_status.SetTag("RUN", to_string(m_runnumber));
    if (m_writer.get())
      m_status.SetTag("FILEBYTES", to_string(m_writer->FileBytes()));

    if (m_sync)
    {
      m_status.SetTag("SYNC", m_nameOfSyncAlgorithm);
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




}
