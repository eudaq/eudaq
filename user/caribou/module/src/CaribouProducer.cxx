#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include "device/DeviceManager.hpp"
#include "utils/configuration.hpp"
#include "utils/log.hpp"

#include <vector>
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

  DeviceManager* manager_;
  Device* device_{nullptr};
  Device* secondary_device_{nullptr};
  std::string name_;

  std::mutex device_mutex_;
  LogLevel level_;
  bool m_exit_of_run;

  size_t number_of_subevents_{0};

  std::string adc_signal_;
  uint64_t adc_freq_;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CaribouProducer, const std::string&, const std::string&>(CaribouProducer::m_id_factory);
}

CaribouProducer::CaribouProducer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_ev(0), m_exit_of_run(false) {
  // Add cout as the default logging stream
  Log::addStream(std::cout);

  // Separate the device name from the device identifier, the latter is only relevant for EUDAQ:
  auto const pos = name.find_last_of('_');
  if(pos == std::string::npos) {
    name_ = name;
    LOG(INFO) << "Instantiated CaribouProducer for device \"" << name << "\"";
  } else {
    name_ = name.substr(0, pos);
    const auto identifier = name.substr(pos + 1);
    LOG(INFO) << "Instantiated CaribouProducer for device \"" << name << "\" with identifier \"" << identifier << "\"";
  }

  // Create new Peary device manager
  manager_ = new DeviceManager();
}

CaribouProducer::~CaribouProducer() {
    delete manager_;
}

void CaribouProducer::DoReset() {
  LOG(WARNING) << "Resetting CaribouProducer";
  m_exit_of_run = true;

  // Delete all devices:
  std::lock_guard<std::mutex> lock{device_mutex_};
  manager_->clearDevices();
  device_ = nullptr;
  secondary_device_ = nullptr;
}

void CaribouProducer::DoInitialise() {
  LOG(INFO) << "Initialising CaribouProducer";
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

  // Add secondary device if it is configured:
  if(ini->Has("secondary_device")) {
    std::string secondary = ini->Get("secondary_device", std::string());
    if(std::find(sections.begin(), sections.end(), secondary) != sections.end()) {
      config.SetSection(secondary);
    }
    size_t device_id2 = manager_->addDevice(secondary, config);
    EUDAQ_INFO("Manager returned device ID " + std::to_string(device_id2) + ", fetching secondary device...");
    secondary_device_ = manager_->getDevice(device_id2);
  }
}

// This gets called whenever the DAQ is configured
void CaribouProducer::DoConfigure() {
  auto config = GetConfiguration();
  LOG(INFO) << "Configuring CaribouProducer: " << config->Name();

  std::lock_guard<std::mutex> lock{device_mutex_};
  EUDAQ_INFO("Configuring device " + device_->getName());

  // Switch on the device power:
  device_->powerOn();

  if(secondary_device_ != nullptr) {
    secondary_device_->powerOn();
  }

  // Wait for power to stabilize and for the TLU clock to be present
  eudaq::mSleep(1000);

  // Configure the device
  device_->configure();

  if(secondary_device_ != nullptr) {
    secondary_device_->configure();
  }
  // Set additional registers from the configuration:

  if(config->Has("register_key") || config->Has("register_value")) {
    auto key = config->Get("register_key", "");
    auto value = config->Get("register_value", 0);
    device_->setRegister(key, value);
    EUDAQ_USER("Setting " + key + " = " + std::to_string(value));
  }

  // Select which ADC signal to regularly fetch:
  adc_signal_ = config->Get("adc_signal", "");
  adc_freq_ = config->Get("adc_frequency", 1000);

  if(!adc_signal_.empty()) {
    // Try it out directly to catch misconfiugration
    auto adc_value = device_->getADC(adc_signal_);
    EUDAQ_USER("Will probe ADC signal \"" + adc_signal_ + "\" every " + std::to_string(adc_freq_) + " events");
  }

  // Allow to stack multiple sub-events
  number_of_subevents_ = config->Get("number_of_subevents", 0);
  EUDAQ_USER("Will stack " + std::to_string(number_of_subevents_) + " subevents before sending event");

  LOG(STATUS) << "CaribouProducer configured. Ready to start run.";
}

void CaribouProducer::DoStartRun() {
  m_ev = 0;

  LOG(INFO) << "Starting run...";

  // Start the DAQ
  std::lock_guard<std::mutex> lock{device_mutex_};

  // Sending initial Begin-of-run event, just containing tags with detector information:
  // Create new event
  auto event = eudaq::Event::MakeUnique("Caribou" + name_ + "Event");
  event->SetBORE();
  event->SetTag("software",  device_->getVersion());
  event->SetTag("firmware",  device_->getFirmwareVersion());
  event->SetTag("timestamp", LOGTIME);

  auto registers = device_->getRegisters();
  for(const auto& reg : registers) {
    event->SetTag(reg.first, reg.second);
  }

  // Send the event to the Data Collector
  SendEvent(std::move(event));

  // Start DAQ:
  device_->daqStart();

  LOG(INFO) << "Started run.";
  m_exit_of_run = false;
}

void CaribouProducer::DoStopRun() {

  LOG(INFO) << "Draining buffers...";
  eudaq::mSleep(500);
  LOG(INFO) << "Stopping run...";

  // Set a flag to signal to the polling loop that the run is over
  m_exit_of_run = true;

  // Stop the DAQ
  std::lock_guard<std::mutex> lock{device_mutex_};
  device_->daqStop();
  LOG(INFO) << "Stopped run.";
}

void CaribouProducer::RunLoop() {

  Log::setReportingLevel(level_);

  LOG(INFO) << "Starting run loop...";
  std::lock_guard<std::mutex> lock{device_mutex_};

  std::vector<eudaq::EventSPC> data_buffer;
  while(!m_exit_of_run) {
    try {
      // Retrieve data from the device:
      auto data = device_->getRawData();

      if(!data.empty()) {
        // Create new event
        auto event = eudaq::Event::MakeUnique("Caribou" + name_ + "Event");
        // Set event ID
        event->SetEventN(m_ev);
        // Add data to the event
        event->AddBlock(0, data);

        // Query ADC if wanted:
        if(m_ev%adc_freq_ == 0) {
          if(!adc_signal_.empty()) {
            auto adc_value = device_->getADC(adc_signal_);
            LOG(DEBUG) << "Reading ADC: " << adc_value << "V";
            EUDAQ_USER("ADC reading: " + adc_signal_ + " =  " + std::to_string(adc_value));
            event->SetTag(adc_signal_, adc_value);
          }
        }

        if(number_of_subevents_ == 0) {
          // We do not want to generate sub-events - send the event directly off to the Data Collector
          SendEvent(std::move(event));
        } else {
          // We are still buffering sub-events, buffer not filled yet:
          data_buffer.push_back(std::move(event));
        }
      }

      // Now increment the event number
      m_ev++;

      // Buffer of sub-events is full, let's ship this off to the Data Collector
      if(!data_buffer.empty() && data_buffer.size() == number_of_subevents_) {
        auto evup = eudaq::Event::MakeUnique("Caribou" + name_ + "Event");
        for(auto& subevt : data_buffer) {
          evup->AddSubEvent(subevt);
        }
        SendEvent(std::move(evup));
        data_buffer.clear();
      }

      LOG_PROGRESS(STATUS, "status") << "Frame " << m_ev;
    } catch(caribou::NoDataAvailable&) {
        continue;
    } catch(caribou::DataException& e) {
      // Retrieval failed, retry once more before aborting:
      EUDAQ_WARN(std::string(e.what()) + ", skipping data packet");
      continue;
    } catch(caribou::caribouException& e) {
      EUDAQ_ERROR(e.what());
      break;
    }
  }

  // Send remaining pixel data:
  if(!data_buffer.empty()) {
    LOG(INFO) << "Sending remaining " << data_buffer.size() << " events from data buffer";
    auto evup = eudaq::Event::MakeUnique("Caribou" + name_ + "Event");
    for(auto& subevt : data_buffer) {
      evup->AddSubEvent(subevt);
    }
    SendEvent(std::move(evup));
    data_buffer.clear();
  }

  LOG(INFO) << "Exiting run loop.";
}
