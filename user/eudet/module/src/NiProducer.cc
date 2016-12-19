#include "NiController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <stdio.h>
#include <stdlib.h>
#include <memory>


class NiProducer : public eudaq::Producer {
public:
  NiProducer(const std::string name, const std::string &runcontrol);
  ~NiProducer();
  void DoConfigure(const eudaq::Configuration &param) override final;
  void DoStartRun(unsigned param) override final;
  void DoStopRun() override final;
  void DoReset() override final {};
  void DoTerminate() override final;

  void DataLoop();
private:
  bool m_running;
  bool m_configured;
  std::thread m_thd_data;
  std::shared_ptr<NiController> ni_control;
  
  unsigned int datalength1;
  unsigned int datalength2;
  unsigned int ConfDataLength;
  std::vector<uint8_t> ConfDataError;

  unsigned TriggerType;
  unsigned Det;
  unsigned Mode;
  unsigned NiVersion;
  unsigned NumBoards;
  unsigned FPGADownload;
  unsigned MimosaID[6];
  unsigned MimosaEn[6];
  bool OneFrame;
  bool NiConfig;
  
  unsigned char conf_parameters[10];
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<NiProducer, const std::string&, const std::string&>(eudaq::cstr2hash("NiProducer"));
}


NiProducer::NiProducer(const std::string name, const std::string &runcontrol)
  : eudaq::Producer(name, runcontrol){
  m_running = false;
  m_configured = false;
  std::cout << "NI Producer was started successful " << std::endl;
}

NiProducer::~NiProducer(){
  m_running = false;
  if(m_thd_data.joinable()){
    m_thd_data.join();
  }
}

void NiProducer::DataLoop(){
  while(m_running){
    datalength1 = ni_control->DataTransportClientSocket_ReadLength("priv");
    std::vector<unsigned char> mimosa_data_0(datalength1);
    mimosa_data_0 =
      ni_control->DataTransportClientSocket_ReadData(datalength1);
     
    datalength2 = ni_control->DataTransportClientSocket_ReadLength("priv");
    std::vector<unsigned char> mimosa_data_1(datalength2);
    mimosa_data_1 =
      ni_control->DataTransportClientSocket_ReadData(datalength2);

    auto evup = eudaq::RawDataEvent::MakeUnique("TelRawDataEvent");
    auto ev = dynamic_cast<eudaq::RawDataEvent*>(evup.get());
    ev->AddBlock(0, mimosa_data_0);
    ev->AddBlock(1, mimosa_data_1);
    SendEvent(std::move(evup));
  };
}

void NiProducer::DoConfigure(const eudaq::Configuration &param) {

  unsigned char configur[5] = "conf";

  if (!m_configured) {
    ni_control = std::make_shared<NiController>();
    ni_control->GetProduserHostInfo();
    std::string addr = param.Get("NiIPaddr", "localhost");
    uint16_t port = param.Get("NiConfigSocketPort", 49248);
    ni_control->ConfigClientSocket_Open(addr, port);
    addr = param.Get("NiIPaddr", "localhost");
    port = param.Get("NiDataTransportSocketPort", 49250);
    ni_control->DatatransportClientSocket_Open(addr, port);
    std::cout << " " << std::endl;
    m_configured = true;
  }

  TriggerType = param.Get("TriggerType", 255);
  Det = param.Get("Det", 255);
  Mode = param.Get("Mode", 255);
  NiVersion = param.Get("NiVersion", 255);
  NumBoards = param.Get("NumBoards", 255);
  FPGADownload = param.Get("FPGADownload", 1);
  for (unsigned char i = 0; i < 6; i++) {
    MimosaID[i] = param.Get("MimosaID_" + std::to_string(i + 1), 255);
    MimosaEn[i] = param.Get("MimosaEn_" + std::to_string(i + 1), 255);
  }
  OneFrame = param.Get("OneFrame", 255);

  std::cout << "Configuring ...(" << param.Name() << ")" << std::endl;

  conf_parameters[0] = NiVersion;
  conf_parameters[1] = TriggerType;
  conf_parameters[2] = Det;
  conf_parameters[3] = MimosaEn[1];
  conf_parameters[4] = MimosaEn[2];
  conf_parameters[5] = MimosaEn[3];
  conf_parameters[6] = MimosaEn[4];
  conf_parameters[7] = MimosaEn[5];
  conf_parameters[8] = NumBoards;
  conf_parameters[9] = FPGADownload;

  ni_control->ConfigClientSocket_Send(configur, sizeof(configur));
  ni_control->ConfigClientSocket_Send(conf_parameters,
				      sizeof(conf_parameters));

  ConfDataLength = ni_control->ConfigClientSocket_ReadLength("priv");
  ConfDataError = ni_control->ConfigClientSocket_ReadData(ConfDataLength);

  NiConfig = false;

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
    EUDAQ_THROW("NiProducer was Configured with ERRORs "+ param.Name());
  }
}

void NiProducer::DoStartRun(unsigned param) {
  auto ev = eudaq::RawDataEvent::MakeUnique("TelRawDataEvent");
  ev->SetBORE();
  ev->SetTag("DET", "MIMOSA26");
  ev->SetTag("MODE", "ZS2");
  ev->SetTag("BOARDS", NumBoards);
  for (unsigned char i = 0; i < 6; i++)
    ev->SetTag("ID" + std::to_string(i), std::to_string(MimosaID[i]));
  for (unsigned char i = 0; i < 6; i++)
    ev->SetTag("MIMOSA_EN" + std::to_string(i), std::to_string(MimosaEn[i]));
  SendEvent(std::move(ev));
  eudaq::mSleep(500);
  ni_control->Start();
  m_running = true;
  if(!m_thd_data.joinable())
    m_thd_data = std::thread(&NiProducer::DataLoop, this);
  else
    EUDAQ_THROW("NiProducer::OnStartRun(): Last run is not stopped.");
}

void NiProducer::DoStopRun() {
  ni_control->Stop();
  eudaq::mSleep(5000);
  m_running = false;
  if(m_thd_data.joinable())
    m_thd_data.join();
  auto ev = eudaq::RawDataEvent::MakeUnique("TelRawDataEvent");
  ev->SetEORE();
  SendEvent(std::move(ev));
}

void NiProducer::DoTerminate() {
  std::cout << "Terminate (press enter)" << std::endl;
  m_running = false;
  if(m_thd_data.joinable()){
    m_thd_data.join();
  }
  ni_control->DatatransportClientSocket_Close();
  ni_control->ConfigClientSocket_Close();
  eudaq::mSleep(1000);
}
