#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include "devicemgr.hpp"
#include "configuration.hpp"
#include "log.hpp"

#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <signal.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <ctime>
#endif

using namespace caribou;

class CaribouProducer : public eudaq::Producer {
public:
  CaribouProducer(const std::string name, const std::string &runcontrol);
  ~CaribouProducer() override;
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  // void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CaribouProducer");
private:
  bool m_running;

  unsigned m_ev;

  caribouDeviceMgr* manager_;
  caribouDevice* device_;
  std::string name_;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CaribouProducer, const std::string&, const std::string&>(CaribouProducer::m_id_factory);
}

CaribouProducer::CaribouProducer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_ev(0), m_running(false), name_(name) {
  std::cout << "Instantiated CaribouProducer for device \"" << name << "\"" << std::endl;

  // Add cout as the default logging stream
  Log::addStream(std::cout);

  // Create new Peary device manager
  manager_ = new caribouDeviceMgr();
}

CaribouProducer::~CaribouProducer() {
    delete manager_;
}

void CaribouProducer::DoReset() {
  std::cout << "Resetting CaribouProducer" << std::endl;

  // Delete all devices:
  manager_->clearDevices();
}

void CaribouProducer::DoInitialise() {
  std::cout << "Initialising CaribouProducer" << std::endl;
  auto ini = GetInitConfiguration();

  auto level = ini->Get("log_level", "INFO");
  try {
    LogLevel log_level = Log::getLevelFromString(level);
    Log::setReportingLevel(log_level);
  } catch(std::invalid_argument& e) {
    LOG(ERROR) << "Invalid verbosity level \"" << std::string(level) << "\", ignoring.";
  }

  // Open configuration file and create object:
  caribou::Configuration config;
  auto confname = ini->Get("config_file", "");
  std::ifstream file(confname);
  EUDAQ_INFO("Attempting to use initial device configuration \"" + confname + "\"");
  if(!file.is_open()) {
    LOG(ERROR) << "No configuration file provided.";
    EUDAQ_ERROR("No Caribou configuration file provided.");
  } else {
    config = caribou::Configuration(file);
  }

  // Select section from the configuration file relevant for this device:
  auto sections = config.GetSections();
  if(std::find(sections.begin(), sections.end(), name_) != sections.end()) {
    config.SetSection(name_);
  }

  size_t device_id = manager_->addDevice(name_, config);
  EUDAQ_INFO("Manager returned device ID " + std::to_string(device_id) + ", fetching device...");
  device_ = manager_->getDevice(device_id);
}

// This gets called whenever the DAQ is configured
void CaribouProducer::DoConfigure() {
  auto config = GetConfiguration();
  std::cout << "Configuring CaribouProducer: " << config->Name() << std::endl;
  config->Print(std::cout);

  EUDAQ_INFO("Configuring device " + device_->getName());

  // Switch on the device power:
  device_->powerOn();

  // Wait for power to stabilize:
  eudaq::mSleep(10);

  try {
    // Configure the device
    device_->configure();
  } catch(const CommunicationError& e) {
    EUDAQ_ERROR(e.what());
  }

  std::cout << std::endl;
  std::cout << "CaribouProducer configured. Ready to start run. " << std::endl;
  std::cout << std::endl;
}

void CaribouProducer::DoStartRun() {
  m_ev = 0;

  std::cout << "Starting run..." << std::endl;

  // Stop the DAQ
  device_->daqStart();

  std::cout << "Started run." << std::endl;
  m_running = true;
}

void CaribouProducer::DoStopRun() {

  std::cout << "Stopping run..." << std::endl;

  // Set a flag to signal to the polling loop that the run is over
  m_running = false;

  eudaq::mSleep(10);

  // Stop the DAQ
  device_->daqStop();

  std::cout << "Stopped run." << std::endl;
}

void CaribouProducer::RunLoop() {

  std::cout << "Starting run loop..." << std::endl;

  unsigned int m_ev_next_update=0;
  while(1) {
    if(!m_running){
      break;
    }

	    // // Current event
	    // auto evup = eudaq::Event::MakeUnique("Timepix3RawDataEvent");
	    // evup->SetTriggerN(m_ev);
      //
	    // // and add it to the event
	    // evup->AddBlock( 0, bufferTrg );
	    // // Add buffer to block
	    // evup->AddBlock( 1, bufferPix );
	    // // Send the event to the Data Collector
	    // SendEvent(std::move(evup));

	    // Now increment the event number
	    m_ev++;
  }

  std::cout << "Exiting run loop." << std::endl;
}
