#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Processor.hh"
#include <string>

#include "NiController.hh"

using namespace eudaq;
class TelProducerPS : public Processor {
public:
  TelProducerPS();
  ~TelProducerPS() override;
  void ProcessEvent(EventSPC ev) final override{};
  void ProduceEvent() final override;
  void ProcessCommand(const std::string& cmd, const std::string& arg) final override;

  static constexpr const char* m_description = "TelProducerPS";
private:
  std::shared_ptr<NiController> ni_control;
  uint8_t conf_parameters[10];
  std::string m_addr_remote;
  uint32_t m_run ;
  uint32_t m_ev ;
  bool running;
};

namespace{
  auto dummy0 = Factory<Processor>::
    Register<TelProducerPS>(cstr2hash(TelProducerPS::m_description));
}


TelProducerPS::TelProducerPS(): Processor(TelProducerPS::m_description){
  ni_control = std::make_shared<NiController>();
  m_run = 0;
  m_ev = 0;
  running = false;
}

TelProducerPS::~TelProducerPS(){
  //TODO:if not then
  if(ni_control){
    ni_control->DatatransportClientSocket_Close();
    ni_control->ConfigClientSocket_Close();
  }
}


void TelProducerPS::ProduceEvent(){
  while(1){
    size_t datalength1 = ni_control->DataTransportClientSocket_ReadLength("priv");
    std::vector<uint8_t> data_0(datalength1);
    data_0 = ni_control->DataTransportClientSocket_ReadData(datalength1);
    size_t datalength2 = ni_control->DataTransportClientSocket_ReadLength("priv");
    std::vector<uint8_t> data_1(datalength2);
    data_1 = ni_control->DataTransportClientSocket_ReadData(datalength2);
    auto rawev = RawDataEvent::MakeShared("TelRawDataEvent", GetInstanceN(),
						   m_run, m_ev++);
    rawev->AddBlock(0, data_0);
    rawev->AddBlock(1, data_1);
    if(running)
      RegisterEvent(rawev);
    if(GetProducerStopFlag())
      break;
  }
}

void TelProducerPS::ProcessCommand(const std::string& key, const std::string& val){
  switch(str2hash(key)){    
  case cstr2hash("Connect"):{
    if(!val.empty())
      m_addr_remote = val;
    if(!ni_control)
      ni_control = std::make_shared<NiController>();
    if(!m_addr_remote.empty() ){
      ni_control->ConfigClientSocket_Open(m_addr_remote, 49248);
      ni_control->DatatransportClientSocket_Open(m_addr_remote, 49250);
    }
    break;
  }
  case cstr2hash("StartRun"):{
    m_ev = 0;
    if(val.empty())
      m_run++;
    else
      m_run = std::stoul(val);
    auto rawev = RawDataEvent::MakeShared("TelRawDataEvent", GetInstanceN(),
						 m_run, m_ev++);
    rawev->SetBORE();
    rawev->SetTag("DET", "MIMOSA26");
    rawev->SetTag("MODE", "ZS2");
    RegisterEvent(rawev);
    ni_control->Start();
    break;
  }
  case cstr2hash("StopRun"):{
    ni_control->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto rawev = RawDataEvent::MakeShared("TelRawDataEvent", GetInstanceN(),
						 m_run, m_ev++);
    rawev->SetEORE();
    RegisterEvent(rawev); //TODO:: check 
    break;
  }
  case cstr2hash("NiIPaddr"):{
    m_addr_remote = val;
    break;
  }
  case cstr2hash("NiVersion"):{
    conf_parameters[0] = std::stoul(val); //TODO: endian unsafe !!!
    break;
  }
  case cstr2hash("TriggerType"):{
    conf_parameters[1] = std::stoul(val);
    break;
  }
  case cstr2hash("Det"):{
    conf_parameters[2] = std::stoul(val);
    break;
  }
  case cstr2hash("MimosaEN_1"):{
    conf_parameters[3] = std::stoul(val);
    break;
  }
  case cstr2hash("MimosaEN_2"):{
    conf_parameters[4] = std::stoul(val);
    break;
  }
  case cstr2hash("MimosaEN_3"):{
    conf_parameters[5] = std::stoul(val);
    break;
  }
  case cstr2hash("MimosaEN_4"):{
    conf_parameters[6] = std::stoul(val);
    break;
  }
  case cstr2hash("MimosaEN_5"):{
    conf_parameters[7] = std::stoul(val);
    break;
  }
  case cstr2hash("NumBoards"):{
    conf_parameters[8] = std::stoul(val);
    break;
  }
  case cstr2hash("FPGADownLoad"):{
    conf_parameters[9] = std::stoul(val);
    break;
  }
  case cstr2hash("SendConf"):{
    uint32_t ConfDataLength;
    std::vector<uint8_t> ConfDataError;
    uint8_t configur[5] = "conf";
    ni_control->ConfigClientSocket_Send(configur, sizeof(configur));
    ni_control->ConfigClientSocket_Send(conf_parameters, sizeof(conf_parameters));
    ConfDataLength = ni_control->ConfigClientSocket_ReadLength("priv");
    ConfDataError = ni_control->ConfigClientSocket_ReadData(ConfDataLength);

    bool NiConfig = false;

    if ((ConfDataError[3] & 0x1) >> 0) {
      EUDAQ_ERROR("NI crate can not be configure: ErrorReceive Config");
      NiConfig = true;
    } // ErrorReceive Config
    if ((ConfDataError[3] & 0x2) >> 1) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA open");
      NiConfig = true;
    } // Error FPGA open
    if ((ConfDataError[3] & 0x4) >> 2) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA reset");
      NiConfig = true;
    } // Error FPGA reset
    if ((ConfDataError[3] & 0x8) >> 3) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA download");
      NiConfig = true;
    } // Error FPGA download
    if ((ConfDataError[3] & 0x10) >> 4) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_0 Start");
      NiConfig = true;
    } // FIFO_0 Configure
    if ((ConfDataError[3] & 0x20) >> 5) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_1 Start");
      NiConfig = true;
    } // FIFO_0 Start
    if ((ConfDataError[3] & 0x40) >> 6) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_2 Start");
      NiConfig = true;
    } // FIFO_1 Configure
    if ((ConfDataError[3] & 0x80) >> 7) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_3 Start");
      NiConfig = true;
    } // FIFO_1 Start
    if ((ConfDataError[2] & 0x1) >> 0) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_4 Start");
      NiConfig = true;
    } // FIFO_2 Configure
    if ((ConfDataError[2] & 0x2) >> 1) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_5 Start");
      NiConfig = true;
    } // FIFO_2 Start
    if (NiConfig) {
      std::cerr << "NI crate was Configured with ERRORs "<< std::endl;
    }
    break;
  }
  default:
    std::cout<<"unkonw user cmd"<<std::endl;
  }
}

