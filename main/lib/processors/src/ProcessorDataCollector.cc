#include "ProcessorBase.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include "Processors.hh"

#include <thread>
#include <memory>
#include "eudaq/Utils.hh"



  
namespace eudaq {




class processor_data_collector :public ProcessorBase {
public:
  processor_data_collector(const std::string& listAdress);
  processor_data_collector(const std::string& listAdress, eudaq_types::outPutString outPut_connectionName);
  ~processor_data_collector();
  virtual void init();
  virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con);
  virtual void end() { m_status = waiting; };
  void DataThread();
  virtual std::string ConnectionString() const ;
private:

  struct Info {
    std::shared_ptr<ConnectionInfo> id;
    ConnectionName_t m_counter;
  };
  std::vector<Info> m_buffer;

  size_t GetInfo(const ConnectionInfo & id);



  void OnConnect(const ConnectionInfo & id);
  ConnectionName_t GetInfoCounter(const ConnectionInfo & id);

  void OnDisconnect(const ConnectionInfo & id);
  void OnReceive(const ConnectionInfo & id, std::shared_ptr<Event> ev);
  void DataHandler(TransportEvent & ev);

  std::unique_ptr<TransportServer> m_dataServer;
  enum status {
    done,
    waiting,
    runnning
  }m_status = waiting;
  std::unique_ptr<std::thread> m_thread;
  ConnectionName_t m_connectionID = random_connection();
  void handleIdentification(TransportEvent& ev);
};


Processors::processor_up Processors::dataReciver(const std::string& listAdrrs) {
  return __make_unique<processor_data_collector>(listAdrrs);

}
Processors::processor_up  Processors::dataReciver(const std::string& listAdrrs, eudaq_types::outPutString outPut_connectionName) {
    
  return  __make_unique<processor_data_collector>(listAdrrs, outPut_connectionName);

  
}

void * DataCollector_thread(void * arg) {
  processor_data_collector * dc = static_cast<processor_data_collector *>(arg);
  dc->DataThread();
  return 0;
}
processor_data_collector::processor_data_collector(const std::string& listAdress)
  : m_dataServer(
  TransportFactory::CreateServer(listAdress)
  ) {
  m_dataServer->SetCallback(TransportCallback(this, &processor_data_collector::DataHandler));

  m_thread = __make_unique<std::thread>(DataCollector_thread, this);
  
  
}

processor_data_collector::processor_data_collector(const std::string& listAdress, eudaq_types::outPutString outPut_connectionName):processor_data_collector(listAdress) {
  necessary_CONVERSION(outPut_connectionName) = m_dataServer->ConnectionString();
}

processor_data_collector::~processor_data_collector() {
  m_status = done;
  m_thread->join();
}

void processor_data_collector::init() {
  auto start = std::clock();
  while (std::clock() - start < 10000 && m_buffer.empty()) {
    mSleep(100);
  }

  if (m_buffer.empty())
  {
    EUDAQ_THROW("trying to start the data collector wihout any connected to it");
  }
  m_status = runnning;
}

ProcessorBase::ReturnParam processor_data_collector::ProcessEvent(event_sp ev, ConnectionName_ref con) {
  return processNext(std::move(ev), con);
}

void processor_data_collector::DataThread() {

  try {
    while (m_status != done) {
      m_dataServer->Process(100000);
    }
  } catch (const std::exception & e) {
    std::cout << "Error: Uncaught exception: " << e.what() << "\n" << "DataThread is dying..." << std::endl;
  } catch (...) {
    std::cout << "Error: Uncaught unrecognised exception: \n" << "DataThread is dying..." << std::endl;
  }

}

std::string processor_data_collector::ConnectionString() const {
  return m_dataServer->ConnectionString();
}

size_t processor_data_collector::GetInfo(const ConnectionInfo & id) {
  for (size_t i = 0; i < m_buffer.size(); ++i) {
    //std::cout << "Checking " << *m_buffer[i].id << " == " << id<< std::endl;
    if (m_buffer[i].id->Matches(id))
      return i;
  }
  EUDAQ_THROW("Unrecognised connection id");
}

void processor_data_collector::OnConnect(const ConnectionInfo & id) {
  Info info;
  info.id = std::shared_ptr<ConnectionInfo>(id.Clone());
  info.m_counter = random_connection();
  m_buffer.push_back(info);
}

ProcessorBase::ConnectionName_t processor_data_collector::GetInfoCounter(const ConnectionInfo & id) {
  for (size_t i = 0; i < m_buffer.size(); ++i) {
    //std::cout << "Checking " << *m_buffer[i].id << " == " << id<< std::endl;
    if (m_buffer[i].id->Matches(id))
      return m_buffer[i].m_counter;
  }
  EUDAQ_THROW("Unrecognised connection id");
}

void processor_data_collector::OnDisconnect(const ConnectionInfo & id) {
  size_t i = GetInfo(id);
  m_buffer.erase(m_buffer.begin() + i);
}

void processor_data_collector::OnReceive(const ConnectionInfo & id, std::shared_ptr<Event> ev) {
  processNext(std::move(ev), GetInfoCounter(id));
}

void processor_data_collector::DataHandler(TransportEvent & ev) {
  //std::cout << "Event: ";
  switch (ev.etype) {
  case (TransportEvent::CONNECT) :
    //std::cout << "Connect:    " << ev.id << std::endl;
    if (m_status == waiting) {
      m_dataServer->SendPacket("OK EUDAQ DATA DataCollector", ev.id, true);
    } else {
      m_dataServer->SendPacket("ERROR EUDAQ DATA Not accepting new connections", ev.id, true);
      m_dataServer->Close(ev.id);
    }
    break;
  case (TransportEvent::DISCONNECT) :
    //std::cout << "Disconnect: " << ev.id << std::endl;
    OnDisconnect(ev.id);
    break;
  case (TransportEvent::RECEIVE) :
    if (ev.id.GetState() == 0) { // waiting for identification
      handleIdentification(ev);
    } else {

      BufferSerializer ser(ev.packet.begin(), ev.packet.end());
      std::shared_ptr<Event> event(EventFactory::Create(ser));
      OnReceive(ev.id, std::move(event));

    }
    break;
  default:
    std::cout << "Unknown:    " << ev.id << std::endl;
  }
}
void handleNameAndTag(TransportEvent& ev) {
  auto splitted_packet = eudaq::split(ev.packet, " ");
  if (splitted_packet.size()<5) {
    return;
  }
  if (splitted_packet[0] != std::string("OK")) {
    return;
  }
  if (splitted_packet[1] != std::string("EUDAQ"))
  {
    return;
  }
  if (splitted_packet[2] != std::string("DATA")) {
    return;
  }
  ev.id.SetType(splitted_packet[3]);
  ev.id.SetName(splitted_packet[4]);
}
void processor_data_collector::handleIdentification(TransportEvent& ev) {
  // check packet

  handleNameAndTag(ev);
  //std::cout << "client replied, sending OK" << std::endl;
  m_dataServer->SendPacket("OK", ev.id, true);
  ev.id.SetState(1); // successfully identified
  OnConnect(ev.id);
}

}
