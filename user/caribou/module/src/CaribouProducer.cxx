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
#include <thread>

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
  unsigned m_ev;

  caribouDeviceMgr* manager_;
  caribouDevice* device_;
  std::string name_;

  std::mutex device_mutex_;
  LogLevel level_;
  bool m_exit_of_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CaribouProducer, const std::string&, const std::string&>(CaribouProducer::m_id_factory);
}

CaribouProducer::CaribouProducer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_ev(0), m_exit_of_run(false), name_(name) {
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
  m_exit_of_run = true;

  // Delete all devices:
  std::lock_guard<std::mutex> lock{device_mutex_};
  manager_->clearDevices();
}

void CaribouProducer::DoInitialise() {
  std::cout << "Initialising CaribouProducer" << std::endl;
  auto ini = GetInitConfiguration();

  auto level = ini->Get("log_level", "INFO");
  try {
    LogLevel log_level = Log::getLevelFromString(level);
    Log::setReportingLevel(log_level);
    LOG(INFO) << "Set verbosity level to \"" << std::string(level) << "\"";
  } catch(std::invalid_argument& e) {
    LOG(ERROR) << "Invalid verbosity level \"" << std::string(level) << "\", ignoring.";
  }

  level_ = Log::getReportingLevel();

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

  std::lock_guard<std::mutex> lock{device_mutex_};
  size_t device_id = manager_->addDevice(name_, config);
  EUDAQ_INFO("Manager returned device ID " + std::to_string(device_id) + ", fetching device...");
  device_ = manager_->getDevice(device_id);
}

// This gets called whenever the DAQ is configured
void CaribouProducer::DoConfigure() {
  auto config = GetConfiguration();
  std::cout << "Configuring CaribouProducer: " << config->Name() << std::endl;
  config->Print(std::cout);

  std::lock_guard<std::mutex> lock{device_mutex_};
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

  // Start the DAQ
  std::lock_guard<std::mutex> lock{device_mutex_};
  device_->daqStart();

  std::cout << "Started run." << std::endl;
  m_exit_of_run = false;
}

void CaribouProducer::DoStopRun() {

  std::cout << "Stopping run..." << std::endl;

  // Set a flag to signal to the polling loop that the run is over
  m_exit_of_run = true;

  // Stop the DAQ
  std::lock_guard<std::mutex> lock{device_mutex_};
  device_->daqStop();
  std::cout << "Stopped run." << std::endl;
}

void CaribouProducer::RunLoop() {

  Log::setReportingLevel(level_);

  std::cout << "Starting run loop..." << std::endl;
  std::lock_guard<std::mutex> lock{device_mutex_};

  while(!m_exit_of_run) {
    try {
      // Create new event
      auto event = eudaq::Event::MakeUnique("CariouRawDataEvent");
      // Use trigger N to store frame counter
      event->SetTriggerN(m_ev);

      // pearydata data;
      std::vector<uint32_t> data;
      try {
        device_->command("triggerPatternGenerator", "1");
        // Read the data:
        data = device_->getRawData();
      } catch(caribou::DataException& e) {
        // Retrieval failed, retry once more before aborting:
        EUDAQ_WARN(std::string(e.what()) + ", skipping frame.");
        device_->timestampsPatternGenerator(); // in case of readout error, clear timestamp fifo before going to next event
        continue;
      }

      std::vector<uint64_t> timestamps = device_->timestampsPatternGenerator();

      // Add timestamps to the event
      event->AddBlock(0, timestamps);

      // Add data to the event
      event->AddBlock(1, data);

      // Send the event to the Data Collector
      SendEvent(std::move(event));

      LOG(INFO) << m_ev << " | " << data.size() << " data words, time: " << caribou::listVector(timestamps);

      // Now increment the event number
      m_ev++;
    } catch(caribou::DataException& e) {
      device_->timestampsPatternGenerator(); // in case of readout error, clear timestamp fifo before going to next event
      continue;
    } catch(caribou::caribouException& e) {
      EUDAQ_ERROR(e.what());
      break;
    }
  }

  std::cout << "Exiting run loop." << std::endl;
}
