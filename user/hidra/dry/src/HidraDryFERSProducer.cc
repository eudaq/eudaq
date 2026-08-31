#include "../include/HidraDryFERSProducer.hh"
#include "HidraUtils.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <limits>
#include <cstdint>
#include <iterator>  // std::ostream_iterator
#include <algorithm> // std::copy, std::min, std::max

#define FILE_HEADER_SIZE 25
#define EVENT_HEADER_SIZE 27
#define MAX_EVENT_SIZE 795 // for Ack Mode 03: EVENT_HEADER_SIZE+12*64

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<HidraDryFERSProducer, const std::string&, const std::string&>(
    HidraDryFERSProducer::m_id_factory);
}

HidraDryFERSProducer::HidraDryFERSProducer(const std::string& name, const std::string& runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_exit_of_run(false) {}

void HidraDryFERSProducer::DoInitialise() {
  auto ini = GetInitConfiguration();
}

void HidraDryFERSProducer::DoConfigure() {
  auto conf = GetConfiguration();
  EUDAQ_LOG_LEVEL((int)(conf->Get("HIDRA_MUTE_DEBUG", 0)));
  m_data_in_path = conf->Get("DATA_IN_PATH", "infile.dat");
  EUDAQ_INFO("Using FERS raw data file " + m_data_in_path);
  m_event_spacing_us = 1000 * (int)conf->Get("REPLAY_EVENT_SPACING_MS", -1);
  std::string inforeplay = m_event_spacing_us < 0 ? "automatic" : std::to_string(m_event_spacing_us) + " us";
  EUDAQ_INFO("Replay rate set to " + inforeplay);
  ReadFileInfo();
}

void HidraDryFERSProducer::DoStartRun() {
  // ReadFileInfo() reopens the file and reads the FILE_HEADER_SIZE-byte header, leaving the get pointer at the first
  // event block — exactly the position the first run uses, where Mainloop starts reading. DoStopRun() closes the file,
  // so without this a second start (without a re-configure) would read from a closed stream and replay nothing.
  // (This also drops the old re-run seek to FILE_HEADER_SIZE+1, which was one byte past the first event.)
  ReadFileInfo();

  m_eudaq_run_number = GetRunNumber();
  auto bore = eudaq::Event::MakeUnique("DryFERS");
  bore->SetBORE();
  bore->SetRunN(m_eudaq_run_number);
  bore->SetTag("FileRunNumber", std::to_string(m_file_run_number));
  bore->SetTag("Producer", "HidraDryFERSProducer");
  EUDAQ_INFO("Starting HidraDryFERSProducer run");
  EUDAQ_INFO("Sending Dry FERS BORE " + hidra::utils::GetEventInfo(bore.get()));
  SendEvent(std::move(bore));

  m_exit_of_run = false;
  m_thd_run = std::thread(&HidraDryFERSProducer::Mainloop, this);
}

void HidraDryFERSProducer::DoStopRun() {
  auto eore = eudaq::Event::MakeUnique("DryFERS");
  eore->SetEORE();
  eore->SetRunN(m_eudaq_run_number);
  eore->SetTag("FileRunNumber", std::to_string(m_file_run_number));
  EUDAQ_INFO("Sending Dry FERS EORE " + hidra::utils::GetEventInfo(eore.get()));
  m_exit_of_run = true;
  EUDAQ_INFO("Exiting HidraDryFERSProducer Run");
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }
  m_ifile.close();
}

void HidraDryFERSProducer::DoReset() {
  m_exit_of_run = true;
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }

  m_ifile.close();
  m_thd_run = std::thread();
  m_exit_of_run = false;
}

void HidraDryFERSProducer::DoTerminate() {
  m_exit_of_run = true;
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }
}

// Open the replay file (closing it first if already open), read the FILE_HEADER_SIZE-byte header (which also yields the
// file run number) and leave the get pointer at the first event block. Called both at configure and at the start of
// every run, so each run replays from the same known-good position.
void HidraDryFERSProducer::ReadFileInfo() {
  if (m_ifile.is_open()) {
    m_ifile.close();
  }

  m_ifile.open(m_data_in_path, std::ios::binary | std::ios::ate);
  if (!m_ifile.is_open()) {
    EUDAQ_THROW("input data file (" + m_data_in_path + ") can not open for reading");
  }

  m_ifile_size = (uint64_t)m_ifile.tellg();
  EUDAQ_INFO("File size: " + std::to_string(m_ifile_size));
  m_ifile.seekg(0, std::ios::beg);
  m_bytes_read = 0;

  // file header
  uint8_t header_size = FILE_HEADER_SIZE; // 25 bytes
  std::vector<uint8_t> file_header(header_size);
  m_ifile.read(reinterpret_cast<char*>(file_header.data()), header_size);
  m_bytes_read += m_ifile.gcount();
  memcpy(&m_file_run_number, &file_header[7], 2);
  EUDAQ_WARN("Run number from file is " + std::to_string(m_file_run_number));
  eudaq::mSleep(2000);
}

void HidraDryFERSProducer::sleepUntilNext(uint64_t last_evt, uint64_t current_evt, uint64_t last_real) {
  if (last_evt == 0) {
    return; // it means this is the first event
  }
  if (last_real < 999) {
    EUDAQ_ERROR("Meaningless timestamp of the last event sent (" + std::to_string(last_real) + ")");
    return;
  }
  if (m_event_spacing_us == 0) {
    return;
  }
  if (m_event_spacing_us > 0) {
    uint64_t n = hidra::utils::getTimeus();
    if ((n > last_real) && (n - last_real) < m_event_spacing_us) {
      std::this_thread::sleep_for(std::chrono::microseconds(m_event_spacing_us - (n - last_real)));
      return;
    }
  } else { // m_event_spacing_us < 0

    if (current_evt < last_evt) {
      EUDAQ_ERROR("Current event timestamp " + std::to_string(current_evt) + " < last " + std::to_string(last_evt));
      return;
    }

    uint64_t n = hidra::utils::getTimeus();

    // EUDAQ_DEBUG("sleepUtils "+std::to_string(last_evt)+" "+std::to_string(current_evt)+"
    // "+std::to_string(last_real)+" "+std::to_string(n));

    if ((n - last_real) < (current_evt - last_evt)) {
      int usec = (int)((current_evt - last_evt) - (n - last_real));
      if (usec > 500000) {
        EUDAQ_DEBUG("Sleeping " + std::to_string(usec / 1000) + " ms");
      }
      std::this_thread::sleep_for(std::chrono::microseconds(usec));
    }

    return;
  }
}

void HidraDryFERSProducer::Mainloop() {
  // auto tp_start_run = std::chrono::steady_clock::now();

  EUDAQ_INFO("Entering Mainloop");

  while (!m_exit_of_run) {

    uint64_t current_trigger_id = std::numeric_limits<uint64_t>::max();
    uint64_t current_timestamp = 0;

    // std::unique_ptr<eudaq::Event> current_evt;
    eudaq::EventUP current_evt;
    bool have_open_evt = false;
    uint64_t min_timestamp = std::numeric_limits<uint64_t>::max();
    uint64_t max_timestamp = 0;

    uint64_t evt_time_last_sent = 0;
    uint64_t real_time_last_sent = 0;

    uint64_t iblock = 0;
    uint64_t ievt = 0;

    uint64_t event_size = 0;

    for (;; iblock++) {

      if (m_exit_of_run == true) {
        break;
      }

      auto ev = eudaq::Event::MakeUnique("FERSEvent");

      std::vector<uint8_t> eventsize_b(2);

      if (!m_ifile.read(reinterpret_cast<char*>(eventsize_b.data()), eventsize_b.size())) {
        m_exit_of_run = true;
        EUDAQ_INFO("File finished. Block counter: " + std::to_string(iblock));
        continue; // should be equivalent to break here
      }
      m_bytes_read += m_ifile.gcount();

      uint16_t block_event_size;
      memcpy(&block_event_size, &eventsize_b[0], 2);

      if (block_event_size < 27 || block_event_size > MAX_EVENT_SIZE) {
        EUDAQ_ERROR("Block " + std::to_string(iblock) + ", inconsistent event size " +
                    std::to_string(block_event_size));
        m_exit_of_run = true; // cannot decode anymore if the event size is not correct
        continue;
      } else {
        // EUDAQ_DEBUG("Block "+std::to_string(iblock)+" size: "+std::to_string(block_event_size));
      }

      std::vector<uint8_t> eventblock(block_event_size - 2);

      m_ifile.read(reinterpret_cast<char*>(eventblock.data()), eventblock.size());
      m_bytes_read += m_ifile.gcount();

      uint64_t trigger_id = 0;
      memcpy(&trigger_id, &eventblock[9], 8);

      double d_trigger_timestamp;
      memcpy(&d_trigger_timestamp, &eventblock[1], 8);
      // trigger_timestamp = *static_cast<double*>&eventblock[1]
      uint64_t trigger_timestamp = static_cast<uint64_t>(d_trigger_timestamp * 1000);

      uint8_t board_id;
      memcpy(&board_id, &eventblock[0], 1);

      if (!have_open_evt) {
        EUDAQ_INFO("Creating the first event with trigID " + std::to_string(trigger_id));

        current_evt = eudaq::Event::MakeUnique("FERSEvent");
        current_trigger_id = trigger_id;
        current_timestamp = trigger_timestamp;
        current_evt->SetTriggerN(current_trigger_id);
        current_evt->SetEventN(current_trigger_id);
        current_evt->SetRunN(m_file_run_number);

        have_open_evt = true;
      }

      if (have_open_evt && trigger_id > current_trigger_id) {

        if ((long long)trigger_timestamp - current_timestamp < 300000){
          HIDRA_ERROR("New trigger {} is only {} is away from previous trigger {}. Less than 300us --> Skipping block",
          trigger_id, ((long long)trigger_timestamp - current_timestamp)/1000., current_trigger_id);
          continue;
        }

        // prepare sending
        current_evt->SetTag("detectorDataSize", std::to_string(event_size));
        current_evt->SetTimestamp(min_timestamp, max_timestamp, true);
        current_evt->SetTag("nativeTimestampBegin", std::to_string(min_timestamp));
        current_evt->SetTag("endianness", "dummy");
        sleepUntilNext(evt_time_last_sent / 1000, current_evt->GetTimestampBegin() / 1000, real_time_last_sent);
        // send
        EUDAQ_INFO("Sending DryFERS evt " + std::to_string(ievt) + " (at block " + std::to_string(iblock) + ")-- " +
                   hidra::utils::GetEventInfo(current_evt.get()));
        evt_time_last_sent = current_evt->GetTimestampBegin();
        real_time_last_sent = hidra::utils::getTimeus();
        
        SendEvent(std::move(current_evt));
        event_size = 0;
        ievt++;

        current_evt = eudaq::Event::MakeUnique("FERSEvent");
        current_trigger_id = trigger_id;
        min_timestamp = std::numeric_limits<uint64_t>::max();
        max_timestamp = 0;
        current_evt->SetTriggerN(current_trigger_id);
        current_evt->SetEventN(current_trigger_id);
        current_evt->SetRunN(m_file_run_number);
      }

      if (have_open_evt && trigger_id < current_trigger_id) {
        EUDAQ_ERROR("Old trigger id detected: " + std::to_string(trigger_id) + ", most recent is " +
                    std::to_string(current_trigger_id) + ". Skipping block " + std::to_string(iblock) + " in evt " +
                    std::to_string(ievt) + " byte " + std::to_string(m_bytes_read));
        continue; // skip this block
      }

      min_timestamp = std::min(min_timestamp, trigger_timestamp);
      max_timestamp = std::max(max_timestamp, trigger_timestamp);
      current_evt->AddBlock(current_evt->GetNumBlock(), eventblock);
      event_size += eventblock.size();

    } // end of iblock loop

    if (have_open_evt && current_evt) {
      // prepare sending
      current_evt->SetTag("detectorDataSize", std::to_string(event_size));
      current_evt->SetTimestamp(min_timestamp, max_timestamp, true);
      current_evt->SetTag("nativeTimestampBegin", std::to_string(min_timestamp));
      current_evt->SetTag("endianness", "dummy");
      sleepUntilNext(evt_time_last_sent / 1000, current_evt->GetTimestampBegin() / 1000, real_time_last_sent);
      // send
      EUDAQ_INFO("Sending DryFFRS evt " + std::to_string(ievt) + " (at block " + std::to_string(iblock) + ")-- " +
                 hidra::utils::GetEventInfo(current_evt.get()));
      SendEvent(std::move(current_evt));
      event_size = 0;
      ievt++;
    }

    m_exit_of_run = true;
  }
  EUDAQ_INFO("EXITING DRYFESR MAINLOOP THREAD");
}
