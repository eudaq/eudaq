#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include "devicemgr.hpp"
#include "configuration.hpp"
#include "log.hpp"

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

  caribouDeviceMgr* manager_;
  caribouDevice* device_;
  std::string name_;

  std::mutex device_mutex_;
  LogLevel level_;
  bool m_exit_of_run;

  bool drop_empty_frames_{}, drop_before_t0_{};
  bool t0_seen_{};
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CaribouProducer, const std::string&, const std::string&>(CaribouProducer::m_id_factory);
}

CaribouProducer::CaribouProducer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_ev(0), m_exit_of_run(false), name_(name) {
  // Add cout as the default logging stream
  Log::addStream(std::cout);

  LOG(INFO) << "Instantiated CaribouProducer for device \"" << name << "\"";

  // Create new Peary device manager
  manager_ = new caribouDeviceMgr();
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
}

// This gets called whenever the DAQ is configured
void CaribouProducer::DoConfigure() {
  auto config = GetConfiguration();
  LOG(INFO) << "Configuring CaribouProducer: " << config->Name();

  drop_empty_frames_ = config->Get("drop_empty_frames", false);
  drop_before_t0_ = config->Get("drop_before_t0", false);

  std::lock_guard<std::mutex> lock{device_mutex_};
  EUDAQ_INFO("Configuring device " + device_->getName());

  // Switch on the device power:
  device_->powerOn();

  // Wait for power to stabilize and for the TLU clock to be present
  eudaq::mSleep(1000);

  // Configure the device
  device_->configure();

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

  int empty_frames = 0, total_words = 0;
  uint64_t last_shutter_open = 0;

  while(!m_exit_of_run) {
    try {
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


      std::stringstream times;
      bool shutterOpen = false;
      uint64_t shutter_open = 0;
      for(const auto& timestamp : timestamps) {
        if((timestamp >> 48) == 3) {
          times << ", frame: " << (timestamp & 0xffffffffffff);
          last_shutter_open = shutter_open;
          shutter_open = (timestamp & 0xffffffffffff);
          shutterOpen = true;
        } else if((timestamp >> 48) == 1 && shutterOpen == true) {
          times << " -- " << (timestamp & 0xffffffffffff);
          shutterOpen = false;
        }
      }

      // Check if there was a T0:
      if(!t0_seen_) {
        // Last shtter open had higher timestamp than this one:
        t0_seen_ = (last_shutter_open > shutter_open);
      }

      if(data.size() > 12) {
        LOG(DEBUG) << m_ev << " | " << data.size() << " data words" << times.str();
        total_words += data.size();
      } else {
        empty_frames++;
      }

      if(!drop_empty_frames_ || data.size() > 12) {
        if(!drop_before_t0_ || t0_seen_) {
          // Create new event
          auto event = eudaq::Event::MakeUnique("Caribou" + name_ + "Event");
          // Use trigger N to store frame counter
          event->SetTriggerN(m_ev);
          // Add timestamps to the event
          event->AddBlock(0, timestamps);
          // Add data to the event
          event->AddBlock(1, data);
          // Send the event to the Data Collector
          SendEvent(std::move(event));
        }
      }

      // Now increment the event number
      m_ev++;

      LOG_PROGRESS(STATUS, "status") << "Frame " << m_ev << " empty: " << empty_frames << " with pixels: " << (m_ev - empty_frames);
    } catch(caribou::DataException& e) {
      device_->timestampsPatternGenerator(); // in case of readout error, clear timestamp fifo before going to next event
      continue;
    } catch(caribou::caribouException& e) {
      EUDAQ_ERROR(e.what());
      break;
    }
  }

  LOG(INFO) << "Exiting run loop.";
}
