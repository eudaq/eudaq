#include "eudaq/TransportClient.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include "ProcessorBase.hh"
#include "Processors.hh"
#include "Processor_inspector.hh"


#include <memory>
// this is a macro to give throw the error from the function itself and not from a helper function 
#define CHECK_RECIVED_PACKET(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("Invalid response from DataCollector server. Expected: " + std::string(expectedString) + "  recieved: " +packet[position])
  
namespace eudaq {



class processor_data_sender :public Processor_Inspector {
public:
  processor_data_sender(const std::string& serverAdress, const std::string & type = "", const std::string & name = "");
  
  virtual void init() { m_status = runnning; };
  virtual ReturnParam inspectEvent(const Event &ev, ConnectionName_ref con);
  virtual void end() { m_status = waiting; };

private:

  std::unique_ptr<TransportClient> m_dataclient;
  enum status {
    done,
    waiting,
    runnning
  }m_status = waiting;
  std::string m_type, m_name;
};

processor_data_sender::processor_data_sender(const std::string& serverAdress, const std::string & type, const std::string & name) :m_type(type), m_name(name) {
  m_dataclient = std::unique_ptr<TransportClient>(TransportFactory::CreateClient(serverAdress));

  std::string packet;
  if (!m_dataclient->ReceivePacket(&packet, 1000000)) {
    EUDAQ_THROW("No response from DataCollector server");
  }
  auto splitted_packet = eudaq::split(packet, " ");
  if (splitted_packet.size()<4){
    EUDAQ_THROW("Invalid response from DataCollector server");
  }
  std::string expectedString = "OK";
  size_t position=0;
  CHECK_RECIVED_PACKET(splitted_packet, 0, "OK");
  CHECK_RECIVED_PACKET(splitted_packet, 1, "EUDAQ");
  CHECK_RECIVED_PACKET(splitted_packet, 2, "DATA");
  CHECK_RECIVED_PACKET(splitted_packet, 3, "DataCollector");

  m_dataclient->SendPacket("OK EUDAQ DATA " + m_type + " " + m_name);
  packet = "";
  if (!m_dataclient->ReceivePacket(&packet, 1000000)) {
    EUDAQ_THROW("No response from DataCollector server");
  }
  auto split2 = eudaq::split(packet, " ");
  if (split2.empty()) {
    EUDAQ_THROW("Invalid response from DataCollector server");
  }
  CHECK_RECIVED_PACKET(split2, 0, "OK");
}

ProcessorBase::ReturnParam processor_data_sender::inspectEvent(const Event &ev, ConnectionName_ref con) {
  if (!m_dataclient) {
    EUDAQ_THROW("Transport not connected error");
  }

  BufferSerializer ser;
  ev.Serialize(ser);
  m_dataclient->SendPacket(ser);
  return sucess;
}


Processors::processor_i_up Processors::dataSender(const std::string& serverAdress, const std::string& type_, const std::string& name_) {
  return __make_unique<processor_data_sender>(serverAdress, type_, name_);
}

}
