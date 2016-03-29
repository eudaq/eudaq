//
// Producer for the ALICE pALPIDEfs chip
// Author: Jan.Fiete.Grosse-Oetringhaus@cern.ch
//

#include "PALPIDEFSProducer.hh"

#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include <iostream>
#include <ostream>
#include <cctype>
#include <cstdlib>

#include <list>
#include <climits>

#include "config.h"

using eudaq::StringEvent;
using eudaq::RawDataEvent;

static const std::string EVENT_TYPE = "pALPIDEfsRAW";

unsigned int Bitmask(int width) {
  unsigned int tmp = 0;
  for (int i = 0; i < width; i++)
    tmp |= 1 << i;
  return tmp;
}

void ParseXML(TpAlpidefs *dut, TiXmlNode *node, int base, int rgn,
              bool readwrite) // TDAQBoard* daq_board, TiXmlNode* node, int
                              // base, int rgn, bool readwrite)
{
  // readwrite (from Chip): true = read; false = write
  for (TiXmlNode *pChild = node->FirstChild("address"); pChild != 0;
       pChild = pChild->NextSibling("address")) {
    if (pChild->Type() != TiXmlNode::TINYXML_ELEMENT)
      continue;
//         printf( "Element %d [%s] %d %d\n", pChild->Type(), pChild->Value(),
//         base, rgn);
    TiXmlElement *elem = pChild->ToElement();
    if (base == -1) {
      if (!elem->Attribute("base")) {
        std::cout << "Base attribute not found!" << std::endl;
        break;
      }
      ParseXML(dut, pChild, atoi(elem->Attribute("base")), -1, readwrite);
    } else if (rgn == -1) {
      if (!elem->Attribute("rgn")) {
        std::cout << "Rgn attribute not found!" << std::endl;
        break;
      }
      ParseXML(dut, pChild, base, atoi(elem->Attribute("rgn")), readwrite);
    } else {
      if (!elem->Attribute("sub")) {
        std::cout << "Sub attribute not found!" << std::endl;
        break;
      }
      int sub = atoi(elem->Attribute("sub"));
      int address = ((rgn << 11) + (base << 8) + sub);
      int value = 0;
//	std::cout << "region" << rgn << " " << base << " " << sub << std::endl;

      if (readwrite) {
        if (dut->ReadRegister(address, &value) != 1) {
          std::cout << "Failure to read chip address " << address << std::endl;
          continue;
        }
      }

      for (TiXmlNode *valueChild = pChild->FirstChild("value"); valueChild != 0;
           valueChild = valueChild->NextSibling("value")) {
        if (!valueChild->ToElement()->Attribute("begin")) {
          std::cout << "begin attribute not found!" << std::endl;
          break;
        }
        int begin = atoi(valueChild->ToElement()->Attribute("begin"));

        int width = 1;
        if (valueChild->ToElement()->Attribute("width")) // width attribute is
                                                         // optional
          width = atoi(valueChild->ToElement()->Attribute("width"));

        if (!valueChild->FirstChild("content") &&
            !valueChild->FirstChild("content")->FirstChild()) {
          std::cout << "content tag not found!" << std::endl;
          break;
        }
        if (readwrite) {
          int subvalue = (value >> begin) & Bitmask(width);
          char tmp[5];
          sprintf(tmp, "%X", subvalue);
          valueChild->FirstChild("content")->FirstChild()->SetValue(tmp);
        } else {
          int content = (int)strtol(
              valueChild->FirstChild("content")->FirstChild()->Value(), 0, 16);



          if (content >= (1 << width)) {
            std::cout << "value too large: " << begin << " " << width << " "
                      << content << " "
                      << valueChild->FirstChild("content")->Value()
                      << std::endl;
            break;
          }
          value += content << begin;
	}
      }
      if (!readwrite) {
//	      printf("%d %d %d: %d %d\n", base, rgn, sub, address, value);
	      if (dut->WriteRegister(address, value) != 1)
		      std::cout << "Failure to write chip address " << address << std::endl;
        int valuecompare = -1;
        if (dut->ReadRegister(address, &valuecompare) != 1)
          std::cout << "Failure to read chip address after writing chip address " << address << std::endl;
        if (address != 14 && value != valuecompare)
          std::cout << "Register read back error : write value is : " << value << " and read value is : "<< valuecompare << std::endl;

      }
    }
  }
}
// Will unmask and select one pixel per region.
// The pixels are calculated from the number of the mask stage
void DeviceReader::PrepareMaskStage(TAlpidePulseType APulseType, int AMaskStage,
                                    int steps) {
  int FirstRegion = 0;
  int LastRegion = 31;
  if (AMaskStage >= 512 * 32) {
    std::cout << "PrepareMaskStage, Warning: Mask stage too big, doing nothing"
              << std::endl;
    return;
  }
  // Mask and unselect all pixels and disable all columns, except the ones in
  // ARegion
  m_dut->SetChipMode(MODE_ALPIDE_CONFIG);
  m_dut->SetMaskAllPixels(true);
  m_dut->SetMaskAllPixels(false, PR_PULSEENABLE);
  for (int ireg = 0; ireg < 32; ++ireg) {
    m_dut->SetDisableAllColumns(ireg, true);
  }
  // now calculate pixel corresponding to mask stage
  // unmask and select this pixel in each region
  int tmp = AMaskStage + 764 * (AMaskStage / 4) + 4 * (AMaskStage / 256);
  tmp = tmp % (16 * 1024); // maximum is total number of pixels per region
  int DCol = tmp / 1024;
  int Address = tmp % 1024;
  for (int ireg = FirstRegion; ireg <= LastRegion; ireg++) {
    m_dut->SetDisableColumn(ireg, DCol, false);
    m_dut->SetMaskSinglePixel(ireg, DCol, Address, false);
    m_dut->SetInjectSinglePixel(ireg, DCol, Address, true, APulseType, false);
    for (int ipoint = 0; ipoint < steps; ++ipoint)
      ++m_data[DCol + ireg * 16][Address][ipoint];
  }
}

void DeviceReader::SetupThresholdScan(int NMaskStage, int NEvts, int ChStart,
                                      int ChStop, int ChStep,
                                      unsigned char ***Data,
                                      unsigned char *Points) {
  m_n_mask_stages = NMaskStage;
  m_n_events = NEvts;
  m_ch_start = ChStart;
  m_ch_stop = ChStop;
  m_ch_step = ChStep;
  m_data = Data;
  m_points = Points;
}

bool DeviceReader::ThresholdScan() {
  int PulseMode = 2;
  int steps = (m_ch_stop - m_ch_start) / m_ch_step;
  steps = ((m_ch_stop - m_ch_start) % m_ch_step > 0) ? steps + 1 : steps;
  std::vector<TPixHit> Hits;
  TEventHeader Header;
  for (int istage = 0; istage < m_n_mask_stages; ++istage) {
    if (!(istage % 10))
      Print(0, "Threshold scan: mask stage %d", istage);
    PrepareMaskStage(PT_ANALOGUE, istage, steps);
    m_test_setup->PrepareAnalogueInjection(m_boardid, m_ch_start, PulseMode);
    int ipoint = 0;
    // std::cout << "S-Curve scan ongoing, stage: " << istage << std::endl;
    // std::cout << "charge: ";
    for (int icharge = m_ch_start; icharge < m_ch_stop; icharge += m_ch_step) {
      Hits.clear();
      m_dut->SetDAC(DAC_ALPIDE_VPULSEL, 170 - icharge);
      // std::cout << icharge << " " << std::flush;
      for (int ievt = 0; ievt < m_n_events; ++ievt) {
        if (!m_test_setup->PulseAndReadEvent(m_boardid, PULSELENGTH_ANALOGUE,
                                             &Hits, 1, &Header)) {
          std::cout << "PulseAndReadEvent failed!" << std::endl;
          return false;
        }
      }
      for (int ihit = 0; ihit < Hits.size(); ++ihit) {
        ++m_data[Hits.at(ihit).doublecol +
                 Hits.at(ihit).region * 16][Hits.at(ihit).address][ipoint];
      }
      m_points[ipoint++] = icharge;
    }
    // std::cout << std::endl;
    m_test_setup->FinaliseAnalogueInjection(m_boardid, PulseMode);
  }
  m_dut->SetMaskAllPixels(false);
  return true;
}

DeviceReader::DeviceReader(int id, int debuglevel, TTestSetup *test_setup,
                           int boardid, TDAQBoard *daq_board, TpAlpidefs *dut)
    : m_queue_size(0), m_thread(&DeviceReader::LoopWrapper, this),
      m_stop(false), m_running(false), m_flushing(false),
      m_waiting_for_eor(false), m_threshold_scan_rqst(false),
      m_threshold_scan_result(0), m_boardid(id), m_id(id),
      m_debuglevel(debuglevel), m_test_setup(test_setup),
      m_daq_board(daq_board), m_dut(dut), m_last_trigger_id(0),
      m_queuefull_delay(100), m_max_queue_size(50 * 1024 * 1024),
      m_n_mask_stages(0), m_n_events(0), m_ch_start(0),
      m_ch_stop(0), m_ch_step(0), m_data(0x0), m_points(0x0) {
  m_last_trigger_id = m_daq_board->GetNextEventId();
  Print(0, "Starting with last event id: %lu", m_last_trigger_id);
}

void DeviceReader::Stop() {
  Print(0, "Stopping...");
  SetStopping();
  m_thread.join();
}

void DeviceReader::SetRunning(bool running) {
  Print(0, "Set running: %lu", running);
  SimpleLock lock(m_mutex);
  if (m_running && !running) {
      m_flushing = true;
    m_waiting_for_eor = true;
  }
  m_running = running;
}

void DeviceReader::StartDAQ() {
  m_daq_board->StartTrigger();
  m_daq_board->WriteBusyOverrideReg(true);
}

void DeviceReader::StopDAQ() {
  while (IsReading()) {
    eudaq::mSleep(10);
  }
  m_daq_board->WriteBusyOverrideReg(false);
  eudaq::mSleep(100);
  m_daq_board->StopTrigger();
}

SingleEvent *DeviceReader::NextEvent() {
  SimpleLock lock(m_mutex);
  if (m_queue.size() > 0)
    return m_queue.front();
  return 0;
}

void DeviceReader::DeleteNextEvent() {
  SimpleLock lock(m_mutex);
  if (m_queue.size() == 0)
    return;
  SingleEvent *ev = m_queue.front();
  m_queue.pop();
  m_queue_size -= ev->m_length;
  delete ev;
}

SingleEvent *DeviceReader::PopNextEvent() {
  SimpleLock lock(m_mutex);
  if (m_queue.size() > 0) {
    SingleEvent *ev = m_queue.front();
    m_queue.pop();
    m_queue_size -= ev->m_length;
    if (m_debuglevel > 3)
      Print(0, "Returning event. Current queue status: %lu elements. Total "
               "size: %lu",
            m_queue.size(), m_queue_size);
    return ev;
  }
  if (m_debuglevel > 3)
    Print(0, "PopNextEvent: No event available");
  return 0;
}

void DeviceReader::PrintQueueStatus() {
  SimpleLock lock(m_mutex);
  Print(0, "Queue status: %lu elements. Total size: %lu", m_queue.size(),
        m_queue_size);
}

void *DeviceReader::LoopWrapper(void *arg) {
  DeviceReader *ptr = static_cast<DeviceReader *>(arg);
  ptr->Loop();
  return 0;
}

void DeviceReader::Loop() {
  Print(0, "Loop starting...");
  while (1) {
    if (IsStopping())
      break;


    if (IsThresholdScanRqsted()) {
      {
        SimpleLock lock(m_mutex);
        m_threshold_scan_rqst = false;
        m_threshold_scan_result = 1;
      }

      if (!ThresholdScan()) {
        SimpleLock lock(m_mutex);
        m_threshold_scan_result = 2;
        Print(3, "Threshold scan failed!");
      } else {
        SimpleLock lock(m_mutex);
        m_threshold_scan_result = 3;
        Print(0, "Threshold scan finished!");
      }
    }

    if (!IsRunning() && !IsFlushing()) {
      eudaq::mSleep(20);
      continue;
    }

    if (m_debuglevel > 10)
      eudaq::mSleep(1000);

    // no event waiting

    if (IsFlushing()) {
      SimpleLock lock(m_mutex);
      m_flushing = false;
      Print(0, "Finished flushing %lu %lu", m_daq_board->GetNextEventId(),
            m_last_trigger_id);
    }

    // data taking
    const int maxDataLength =
      258 * 2 * 32 + 28; // 256 words per region, region header (2 words) per
    // region, header (20 bytes), trailer (8 bytes)
    unsigned char data_buf[maxDataLength];
    int length = -1;

    SetReading(true);
    bool readEvent = m_daq_board->ReadChipEvent(
        data_buf, &length, maxDataLength);

    SetReading(false);

    if (readEvent && length != -9) {
      if (length == 0) {
        // EOR
        if (IsFlushing()) {
          Print(0, "Flushing %lu", m_last_trigger_id);
          SimpleLock lock(m_mutex);
          m_flushing = false;
          Print(0, "EOR event received");
          m_waiting_for_eor = false;
          // gets resetted by EndOfRun
          m_last_trigger_id = m_daq_board->GetNextEventId();
        } else
          Print(
              3,
              "UNEXPECTED: 0 event received but trigger has not been stopped.");
        continue;
      }

      TEventHeader header;

      bool HeaderOK = m_daq_board->DecodeEventHeader(data_buf, &header);
      bool TrailerOK = m_daq_board->DecodeEventTrailer(data_buf + length - 8);

      if (HeaderOK && TrailerOK) {
        if (m_debuglevel > 2) {
          std::vector<TPixHit> hits;

          if (!m_dut->DecodeEvent(data_buf + 20, length - 28, &hits)) {
            std::cerr << "ERROR decoding event payload. " << std::endl;
          } else {
            m_dut->DumpHitData(hits);
          }

          m_test_setup->DumpRawData(data_buf, length);

          std::string str = "RAW payload (length %d): ";
          for (int j = 0; j < length - 28; j++) {
            char buffer[20];
            sprintf(buffer, "%02x ", data_buf[j + 20]);
            str += buffer;
          }
          str += "\n";
          Print(0, str.data(), length);
        }

        SingleEvent *ev =
          new SingleEvent(length - 28, header.EventId, header.TimeStamp);
        memcpy(ev->m_buffer, data_buf + 20, length - 28);
        // add to queue
        Push(ev);

        // DEBUG
        /*
           if (header.EventId > m_last_trigger_id*2) {
           Print(2, "Very large id %lu %lu", header.EventId, m_last_trigger_id);
           std::string str = "RAW: ";
           for (int j=0; j<length; j++) {
           char buffer[20];
           sprintf(buffer, "%02x ", data_buf[j]);
           str += buffer;
           }
           str += "\n";
           Print(0, str.data());
           }
         */
        // --DEBUG

        m_last_trigger_id = header.EventId;
      } else {
        Print(2, "ERROR decoding event. Header: %d Trailer: %d", HeaderOK,
            TrailerOK);
        char buffer[30];
        sprintf(buffer, "RAW data (length %d): ", length);
        std::string str(buffer);
        for (int j = 0; j < length; j++) {
          sprintf(buffer, "%02x ", data_buf[j]);
          str += buffer;
        }
        str += "\n";
        Print(0, str.data());
      }
      }
    }
    Print(0, "ThreadRunner stopping...");
  }



void DeviceReader::Print(int level, const char *text, uint64_t value1,
                         uint64_t value2, uint64_t value3, uint64_t value4) {
  // level:
  //   0 -> printout
  //   1 -> INFO
  //   2 -> WARN
  //   3 -> ERROR

  std::string tmp("DeviceReader %d: ");
  tmp += text;

  const int maxLength = 100000;
  char msg[maxLength];
  snprintf(msg, maxLength, tmp.data(), m_id, value1, value2, value3, value4);

  std::cout << msg << std::endl;
  if (level == 1) {
    EUDAQ_INFO(msg);
    //     SetStatus(eudaq::Status::LVL_OK, msg);
  } else if (level == 2) {
    EUDAQ_WARN(msg);
    //     SetStatus(eudaq::Status::LVL_WARN, msg);
  } else if (level == 3) {
    EUDAQ_ERROR(msg);
    //     SetStatus(eudaq::Status::LVL_ERROR, msg);
  }
}

void DeviceReader::Push(SingleEvent *ev) {
  while (QueueFull())
    eudaq::mSleep(m_queuefull_delay);
  SimpleLock lock(m_mutex);
  m_queue.push(ev);
  m_queue_size += ev->m_length;
  if (m_debuglevel > 2)
    Print(0, "Pushed events. Current queue size: %lu", m_queue.size());
}

bool DeviceReader::QueueFull() {
  SimpleLock lock(m_mutex);
  if (m_queue_size > m_max_queue_size) {
    if (m_debuglevel > 3)
      Print(0, "BUSY: %lu %lu", m_queue.size(), m_queue_size);
    return true;
  }
  return false;
}

float DeviceReader::GetTemperature() {
  return m_daq_board->GetTemperature();
}

void DeviceReader::ParseXML(TiXmlNode *node, int base, int rgn,
                            bool readwrite) {
  ::ParseXML(m_dut, node, base, rgn, readwrite);
}

// -------------------------------------------------------------------------------------------------

bool PALPIDEFSProducer::ConfigChip(int id, TpAlpidefs *dut,
                                   std::string configFile) {
  // TODO maybe add this functionality to TDAQBoard?
  char buffer[1000];
  sprintf(buffer, "Device %d: Reading chip configuration from file %s", id,
          configFile.data());
  EUDAQ_INFO(buffer);

  TiXmlDocument doc(configFile.c_str());
  if (!doc.LoadFile()) {
    std::string msg = "Unable to open file ";
    msg += configFile;
    std::cerr << msg.data() << std::endl;
    EUDAQ_ERROR(msg.data());
    SetStatus(eudaq::Status::LVL_ERROR, msg.data());
    return false;
  }
  // TODO return code?
  ParseXML(dut, doc.FirstChild("root")->ToElement(), -1, -1, false);
  return true;
}

void PALPIDEFSProducer::OnConfigure(const eudaq::Configuration &param) {
  {
    SimpleLock lock(m_mutex);
    m_configuring = true;
  }
  std::cout << "Configuring..." << std::endl;

  long wait_cnt = 0;
  while (IsRunning() || IsFlushing() || IsStopping()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still running, waiting to configure";
      std::cout << msg << std::endl;
      EUDAQ_ERROR(msg.data());
    }
  }
  EUDAQ_INFO("Configuring (" + param.Name() + ")");
  SetStatus(eudaq::Status::LVL_OK, "Configuring (" + param.Name() + ")");

  m_status_interval = param.Get("StatusInterval", -1);

  const int delay = param.Get("QueueFullDelay", 0);
  const unsigned long queue_size = param.Get("QueueSize", 0) * 1024 * 1024;

  if (param.Get("CheckTriggerIDs", 0) == 1)
    m_ignore_trigger_ids = false;

  if (param.Get("DisableRecoveryOutOfSync", 0) == 1)
    m_recover_outofsync = false;

  if (param.Get("MonitorPSU", 0) == 1)
    m_monitor_PSU = true;

  const int nDevices = param.Get("Devices", 1);
  if (m_nDevices == 0 || m_nDevices == nDevices)
    m_nDevices = nDevices;
  else {
    const char *error_msg =
        "Unsupported attempt to change the number of devices!";
    std::cout << error_msg << std::endl;
    EUDAQ_ERROR(error_msg);
    SetStatus(eudaq::Status::LVL_ERROR, error_msg);
    return;
  }

  m_full_config_v1 = param.Get("FullConfigV1", param.Get("FullConfig", ""));
  m_full_config_v2 = param.Get("FullConfigV2", "");
  m_full_config_v3 = param.Get("FullConfigV3", "");

  m_SCS_charge_start = param.Get("SCSchargeStart", 0);
  m_SCS_charge_stop = param.Get("SCSchargeStop", 50);
  if (m_SCS_charge_stop > 170) {
    m_SCS_charge_stop = 170;
    std::cout << "Maximum charge value limited to 170!" << std::endl;
  }
  m_SCS_charge_step = param.Get("SCSchargeStep", 1);
  m_SCS_n_events = param.Get("SCSnEvents", 50);
  if (m_SCS_n_events > 254) {
    m_SCS_n_events = 254;
    std::cout << "Number of events in a S-curve scan limited to 254!"
              << std::endl;
  }
  m_SCS_n_mask_stages = param.Get("SCSnMaskStages", 160);
  m_SCS_n_steps = (m_SCS_charge_stop - m_SCS_charge_start) / m_SCS_charge_step;
  m_SCS_n_steps =
      ((m_SCS_charge_stop - m_SCS_charge_start) % m_SCS_charge_step > 0)
          ? m_SCS_n_steps + 1
          : m_SCS_n_steps;

  if (!m_next_event) {
    m_next_event = new SingleEvent *[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_next_event[i] = 0x0;
    }
  }
  if (!m_chip_type)
    m_chip_type = new int[m_nDevices];
  if (!m_strobe_length)
    m_strobe_length = new int[m_nDevices];
  if (!m_strobeb_length)
    m_strobeb_length = new int[m_nDevices];
  if (!m_trigger_delay)
    m_trigger_delay = new int[m_nDevices];
  if (!m_readout_delay)
    m_readout_delay = new int[m_nDevices];
  if (!m_reader) {
    m_reader = new DeviceReader *[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_reader[i] = 0x0;
    }
  }
  if (!m_do_SCS)
    m_do_SCS = new bool[m_nDevices];
  if (!m_SCS_data) {
    m_SCS_data = new unsigned char ***[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_SCS_data[i] = 0x0;
    }
  }
  if (!m_SCS_points) {
    m_SCS_points = new unsigned char *[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_SCS_points[i] = 0x0;
    }
  }

  // Set back-bias voltage
  SetBackBiasVoltage(param);
  // Power supply monitoring
  if (m_monitor_PSU) {
    system("${SCRIPT_DIR}/meas.sh ${SCRIPT_DIR} ${LOG_DIR}/$(date "
           "+%s)-meas-tab ${LOG_DIR}/$(date +%s)-meas-log "
           "${SCRIPT_DIR}/meas-pid.txt");
  }
  // Move the linear stage
  //ControlLinearStage(param);
  ControlRotaryStages(param);

  for (int i = 0; i < m_nDevices; i++) {
    const size_t buffer_size = 100;
    char buffer[buffer_size];

    sprintf(buffer, "SCS_%d", i);
    m_do_SCS[i] = (bool)param.Get(buffer, 0);
    if (m_do_SCS[i]) {
      bool generate_arrays = false;
      if (!m_SCS_data[i])
        generate_arrays = true;
      if (generate_arrays)
        m_SCS_data[i] = new unsigned char **[512];
      if (generate_arrays)
        m_SCS_points[i] = new unsigned char[m_SCS_n_steps];
      for (int j = 0; j < 512; ++j) {
        if (generate_arrays)
          m_SCS_data[i][j] = new unsigned char *[1024];
        for (int k = 0; k < 1024; ++k) {
          if (generate_arrays)
            m_SCS_data[i][j][k] = new unsigned char[m_SCS_n_steps];
          for (int l = 0; l < m_SCS_n_steps; ++l) {
            m_SCS_data[i][j][k][l] = 255;
          }
        }
      }
      for (int j = 0; j < m_SCS_n_steps; ++j) {
        m_SCS_points[i][j] = 0;
      }
    } else {
      m_SCS_data[i] = 0x0;
      m_SCS_points[i] = 0x0;
    }
  }
  if (!InitialiseTestSetup(param))
    return;

  for (int i = 0; i < m_nDevices; i++) {
    TDAQBoard *daq_board = m_reader[i]->GetDAQBoard();
    daq_board->WriteBusyOverrideReg(false);
    }

  if (!DoSCurveScan(param))
    return;

  for (int i = 0; i < m_nDevices; i++) {
    TpAlpidefs *dut = m_reader[i]->GetDUT();
    TDAQBoard *daq_board = m_reader[i]->GetDAQBoard();
    const size_t buffer_size = 100;
    char buffer[buffer_size];

    daq_board->WriteBusyOverrideReg(false);

    // configuration
    sprintf(buffer, "Config_File_%d", i);
    std::string configFile = param.Get(buffer, "");
    if (configFile.length() > 0)
      if (!ConfigChip(i, dut, configFile))
        return;

    // noisy and broken pixels
    dut->SetMaskAllPixels(false); // unmask all pixels
    dut->ClearNoisyPixels();
    bool mask_pixels = false;
    sprintf(buffer, "Noisy_Pixel_File_%d", i);
    std::string noisyPixels = param.Get(buffer, "");
    if (noisyPixels.length() > 0) {
      sprintf(buffer, "Device %d: Reading noisy pixels from file %s", i,
              noisyPixels.data());
      EUDAQ_INFO(buffer);
      dut->ReadNoisyPixelFile(noisyPixels.data());
      mask_pixels = true;
    }
    sprintf(buffer, "Broken_Pixel_File_%d", i);
    std::string brokenPixels = param.Get(buffer, "");
    if (brokenPixels.length() > 0) {
      sprintf(buffer, "Device %d: Reading broken pixels from file %s", i,
              brokenPixels.data());
      std::cout << buffer << std::endl;
      EUDAQ_INFO(buffer);
      dut->ReadNoisyPixelFile(brokenPixels.data(), true);
      mask_pixels = true;
    }

    if (mask_pixels)
      dut->MaskNoisyPixels();

    // triggering configuration per layer
    sprintf(buffer, "StrobeLength_%d", i);
    m_strobe_length[i] = param.Get(buffer, param.Get("StrobeLength", 10));
    sprintf(buffer, "StrobeBLength_%d", i);
    m_strobeb_length[i] = param.Get(buffer, param.Get("StrobeBLength", 20));
    sprintf(buffer, "ReadoutDelay_%d", i);
    m_readout_delay[i] = param.Get(buffer, param.Get("ReadoutDelay", 10));
    sprintf(buffer, "TriggerDelay_%d", i);
    m_trigger_delay[i] = param.Get(buffer, param.Get("TriggerDelay", 75));

    // data taking configuration
    // PrepareEmptyReadout

    if (!(strcmp(dut->GetClassName(), "TpAlpidefs3"))) {
      std::cout << "This is " << dut->GetClassName() << std::endl;
      daq_board->ConfigureReadout(3, true, true);
      // buffer depth = 3, 'sampling on rising edge (changed for pALPIDE3)', packet-based mode
    }
    else daq_board->ConfigureReadout(1, false, true); //buffer depth = 1, sampling on rising edge, packet-based mode
    daq_board->ConfigureTrigger(0, m_strobe_length[i], 2, 0,
                                m_trigger_delay[i]);

    // PrepareChipReadout
    dut->PrepareReadout(m_strobeb_length[i], m_readout_delay[i],
                        MODE_ALPIDE_READOUT_B);

    if (delay > 0)
      m_reader[i]->SetQueueFullDelay(delay);
    if (queue_size > 0)
      m_reader[i]->SetMaxQueueSize(queue_size);

    std::cout << "Device " << i << " configured." << std::endl;

    eudaq::mSleep(10);
  }

  eudaq::mSleep(5000); // TODO remove this again - trying to protect from
                       // starting problems

  if (!m_configured) {
    m_configured = true;
    m_firstevent = true;
  }


  EUDAQ_INFO("Configured (" + param.Name() + ")");
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
  {
    SimpleLock lock(m_mutex);
    m_configuring = false;
  }
}

bool PALPIDEFSProducer::InitialiseTestSetup(const eudaq::Configuration &param) {
  if (!m_configured) {
    m_nDevices = param.Get("Devices", 1);

    const int delay = param.Get("QueueFullDelay", 0);
    const unsigned long queue_size = param.Get("QueueSize", 0) * 1024 * 1024;

    TConfig *config = new TConfig(TYPE_TELESCOPE, m_nDevices);
    char mybuffer[100];
    for (int idev = 0; idev < m_nDevices; idev++) {
      snprintf(mybuffer, 100, "DataPort_%d", idev);
      TDataPort dataPort = (TDataPort)param.Get(mybuffer, (int)PORT_PARALLEL);
      config->SetDataPort(idev, dataPort);
    }

    if (!m_testsetup) {
      std::cout << "Creating test setup " << std::endl;
      m_testsetup = new TTestSetup;
    }

    for (int idev = 0; idev < m_nDevices; idev++) {
      sprintf(mybuffer, "BoardAddress_%d", idev);
      int board_address = param.Get(mybuffer, -1);

      sprintf(mybuffer, "ChipType_%d", idev);
      std::string ChipType = param.Get(mybuffer, "");

      config->GetBoardConfig(idev)->GeoAdd = board_address;

      if (!ChipType.compare("PALPIDEFS1")) {
        m_chip_type[idev] = 1;
        config->GetBoardConfig(idev)->BoardType = 1;
        config->GetChipConfig(idev)->ChipType = DUT_PALPIDEFS1;
      } else if (!ChipType.compare("PALPIDEFS2")) {
        m_chip_type[idev] = 2;
        config->GetBoardConfig(idev)->BoardType = 2;
        config->GetChipConfig(idev)->ChipType = DUT_PALPIDEFS2;
      } else if (!ChipType.compare("PALPIDEFS3")) {
        m_chip_type[idev] = 3;
        config->GetBoardConfig(idev)->BoardType = 2;
        config->GetBoardConfig(idev)->EnableDDR = false;
        config->GetChipConfig(idev)->ChipType = DUT_PALPIDEFS3;
      }
    }

    std::cout << "Searching for DAQ boards " << std::endl;
    m_testsetup->FindDAQBoards(config);
    std::cout << "Found " << m_testsetup->GetNDAQBoards() << " DAQ boards."
              << std::endl;

    if (m_testsetup->GetNDAQBoards() < m_nDevices) {
      char msg[100];
      sprintf(msg, "Not enough devices connected. Configuration requires %d "
                   "devices, but only %d present.",
              m_nDevices, m_testsetup->GetNDAQBoards());
      std::cerr << msg << std::endl;
      EUDAQ_ERROR(msg);
      SetStatus(eudaq::Status::LVL_ERROR, msg);
      return false;
    }

    m_testsetup->AddDUTs(config);
    for (int i = 0; i < m_nDevices; i++) {
      TpAlpidefs *dut = 0;
      TDAQBoard *daq_board = 0;
      const size_t buffer_size = 100;
      char buffer[buffer_size];

      if (!m_configured) {
        sprintf(buffer, "BoardAddress_%d", i);
        int board_address = param.Get(buffer, i);

        // find board
        int board_no = -1;
        for (int j = 0; j < m_testsetup->GetNDAQBoards(); j++) {
          if (m_testsetup->GetDAQBoard(j)->GetBoardAddress() == board_address) {
            board_no = j;
            break;
          }
        }

        if (board_no == -1) {
          char msg[100];
          sprintf(msg, "Device with board address %d not found", board_address);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
          SetStatus(eudaq::Status::LVL_ERROR, msg);
          return false;
        }
        std::cout << "Enabling device " << board_no << std::endl;
        dut = (TpAlpidefs *)m_testsetup->GetDUT(board_no);
        daq_board = m_testsetup->GetDAQBoard(board_no);
        int overflow = 0x0;
        if (!m_testsetup->PowerOnBoard(board_no, overflow)) {
          char msg[100];
          sprintf(msg,
                  "Powering device with board address %d failed, overflow=0x%x",
                  board_address, overflow);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
          SetStatus(eudaq::Status::LVL_ERROR, msg);
          return false;
        }

        if (!m_testsetup->InitialiseChip(board_no, overflow)) {
          char msg[100];
          sprintf(
              msg,
              "Initialising device with board address %d failed, overflow=0x%x",
              board_address, overflow);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
          SetStatus(eudaq::Status::LVL_ERROR, msg);
          return false;
        }

        std::cout << "Device " << i << " with board address " << board_address
                  << " (delay " << delay << " - queue size " << queue_size
                  << ") powered." << std::endl;

        m_reader[i] = new DeviceReader(i, m_debuglevel, m_testsetup, board_no,
                                       daq_board, dut);
        if (m_next_event[i])
          delete m_next_event[i];
        m_next_event[i] = 0;
      } else {
        std::cout
            << "Already initialized and powered. Doing only reconfiguration..."
            << std::endl;
      }
    }
  }
  return true;
}

bool PALPIDEFSProducer::PowerOffTestSetup() {
  std::cout << "Powering off test setup" << std::endl;
  for (int i = 0; i < m_nDevices; i++) {
    if (m_reader[i]) {
      TDAQBoard *daq_board = m_reader[i]->GetDAQBoard();
      m_reader[i]->Stop();
      delete m_reader[i];
      m_reader[i] = 0x0;
      // power of the DAQboard
      std::vector<SFieldReg> ADCConfigReg0 =
          daq_board
              ->GetADCConfigReg0(); // Get Descriptor Register ADCConfigReg0
      daq_board->SetPowerOnSequence(1, 0, 0, 0);
      daq_board->SendADCControlReg(ADCConfigReg0, 1,
                                   0); // register 0, self shutdown = 1, off = 0
      daq_board->ResetBoardFPGA(10);
      daq_board->ResetBoardFX3(10);
    }
  }
  if (m_testsetup) {
    struct libusb_context *context = m_testsetup->GetContext();
    delete m_testsetup;
    m_testsetup = 0x0;
    libusb_exit(context);
    m_configured = false;
  }
  eudaq::mSleep(5000);
  system("${SCRIPT_DIR}/fx3/program.sh");
  return true;
}

bool PALPIDEFSProducer::DoSCurveScan(const eudaq::Configuration &param) {
  bool result = true;
  bool doScan = false;
  for (int i = 0; i < m_nDevices; i++) {
    if (m_do_SCS[i])
      doScan = true;
  }
  if (!doScan)
    return true;

  for (int i = 0; i < m_nDevices; i++) {
    if (!m_do_SCS[i])
      continue;
    m_reader[i]->SetupThresholdScan(
        m_SCS_n_mask_stages, m_SCS_n_events, m_SCS_charge_start,
        m_SCS_charge_stop, m_SCS_charge_step, m_SCS_data[i], m_SCS_points[i]);
    m_reader[i]->RequestThresholdScan();
  }

  for (int i = 0; i < m_nDevices; i++) {
    if (!m_do_SCS[i])
      continue;

    while (m_reader[i]->GetThresholdScanState() <= 1)
      eudaq::mSleep(500);

    if (m_reader[i]->GetThresholdScanState() != 3) {
      const size_t buffer_size = 100;
      char buffer[buffer_size];
      snprintf(buffer, buffer_size, "S-Curve scan of DUT %d failed!", i);
      std::cout << buffer << std::endl;
      EUDAQ_ERROR(buffer);
      SetStatus(eudaq::Status::LVL_ERROR, buffer);
      result = false;
    }
  }
  if (!PowerOffTestSetup()) {
    char buffer[] = "Powering off the DAQ boards failed!";
    std::cout << buffer << std::endl;
    EUDAQ_ERROR(buffer);
    SetStatus(eudaq::Status::LVL_ERROR, buffer);
    result = false;
  }
  if (!InitialiseTestSetup(param)) {
    char buffer[] = "Reinitialising the DAQ boards failed!";
    std::cout << buffer << std::endl;
    EUDAQ_ERROR(buffer);
    SetStatus(eudaq::Status::LVL_ERROR, buffer);
    result = false;
  }
  return result;
}

void PALPIDEFSProducer::SetBackBiasVoltage(const eudaq::Configuration &param) {
  m_back_bias_voltage = param.Get("BackBiasVoltage", -1.);
  if (m_back_bias_voltage >= 0.) {
    std::cout << "Setting back-bias voltage..." << std::endl;
    system("if [ -f ${SCRIPT_DIR}/meas-pid.txt ]; then kill -2 $(cat "
           "${SCRIPT_DIR}/meas-pid.txt); fi");
    const size_t buffer_size = 100;
    char buffer[buffer_size];
    snprintf(buffer, buffer_size, "${SCRIPT_DIR}/change_back_bias.py %f",
             m_back_bias_voltage);
    if (system(buffer) != 0) {
      const char *error_msg = "Failed to configure the back-bias voltage";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetStatus(eudaq::Status::LVL_ERROR, error_msg);
      m_back_bias_voltage = -2.;
    } else {
      std::cout << "Back-bias voltage set!" << std::endl;
    }
  }
}

void PALPIDEFSProducer::ControlRotaryStages(const eudaq::Configuration &param) {
#ifndef SIMULATION
  // rotary stage
  // step size: 1 step = 4.091 urad

  m_dut_angle1 = param.Get("DUTangle1", -1.);
  std::cout << "angle 1: " << m_dut_angle1 << " deg" << std::endl;

  int nsteps1 = (int)(m_dut_angle1/180.*3.14159/(4.091/1e6));
  std::cout << "   nsteps1: " << nsteps1 << std::endl;

  m_dut_angle2 = param.Get("DUTangle2", -1.);
  std::cout << "angle 2: " << m_dut_angle2 << " deg" << std::endl;

  int nsteps2 = (int)(m_dut_angle2/180.*3.14159/(4.091/1e6));
  std::cout << "   nsteps2: " << nsteps2 << std::endl;

  if (m_dut_angle1 >= 0. && m_dut_angle2 >= 0.) {
    std::cout << "Rotating DUT..." << std::endl;
    bool move_failed = false;
    if (system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 0") == 0) { // init of stage 1 (only one stage sufficient)
      // rotate both stages to home position
      //system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 1")
      //system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 2 1")

      // rotate stages
      const size_t buffer_size = 100;
      char buffer[buffer_size];
      // rotate stage 1
      std::cout << "   stage 1..." << std::endl;
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 3 %i", nsteps1);
      if (system(buffer) != 0)
        move_failed = true;
      // rotate stage 2
      std::cout << "   stage 2..." << std::endl;
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 2 3 %i", nsteps2);
      if (system(buffer) != 0)
        move_failed = true;

    } else
      move_failed = true;

    if (move_failed) {
      const char *error_msg = "Failed to rotate one of the stages";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetStatus(eudaq::Status::LVL_ERROR, error_msg);
      m_dut_angle1 = -2.;
      m_dut_angle2 = -2.;
    } else {
      std::cout << "DUT in position!" << std::endl;
    }
  }
#endif
}

void PALPIDEFSProducer::ControlLinearStage(const eudaq::Configuration &param) {
  // linear stage
  m_dut_pos = param.Get("DUTposition", -1.);
  if (m_dut_pos >= 0.) {
    std::cout << "Moving DUT to position..." << std::endl;
    bool move_failed = false;
    if (system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 0") == 0) {
      const size_t buffer_size = 100;
      char buffer[buffer_size];
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 5 %f", m_dut_pos);
      if (system(buffer) != 0)
        move_failed = true;
    } else
      move_failed = true;
    if (move_failed) {
      const char *error_msg = "Failed to move the linear stage";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetStatus(eudaq::Status::LVL_ERROR, error_msg);
      m_dut_pos = -2.;
    } else {
      std::cout << "DUT in position!" << std::endl;
    }
  }
}

void PALPIDEFSProducer::OnStartRun(unsigned param) {
  long wait_cnt = 0;
  while (IsConfiguring()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still configuring, waiting to run";
      std::cout << msg << std::endl;
      EUDAQ_ERROR(msg.data());
    }
  }
  m_run = param;
  m_ev = 0;

  // the queues should be empty at this stage, if not flush them
  PrintQueueStatus();
  bool queues_empty = true;
  for (int i = 0; i < m_nDevices; i++) {
    while (m_reader[i]->GetQueueLength() > 0) {
      m_reader[i]->DeleteNextEvent();
      queues_empty = false;
    }
  }
  if (!queues_empty) {
    EUDAQ_WARN( "Queues not empty on SOR");
  }
  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
  bore.SetTag("Devices", m_nDevices);
  bore.SetTag("DataVersion", 2);

  // driver version
  bore.SetTag("Driver_GITVersion", m_testsetup->GetGITVersion());
  bore.SetTag("EUDAQ_GITVersion", PACKAGE_VERSION);

  std::vector<char> SCS_data_block;
  std::vector<char> SCS_points_block;

  // read configuration, dump to XML string
  for (int i = 0; i < m_nDevices; i++) {
    std::string configstr;

    if (m_chip_type[i] == 3) configstr = m_full_config_v3;
    else if (m_chip_type[i] == 2) configstr = m_full_config_v2;

    else configstr = m_full_config_v1;
    TiXmlDocument doc(configstr.c_str());
    if (!doc.LoadFile()) {
      std::string msg = "Failed to load config file: ";

      if (m_chip_type[i] == 3) msg += m_full_config_v3;
      else msg += (m_chip_type[i] == 2) ? m_full_config_v2 : m_full_config_v1;
      std::cerr << msg.data() << std::endl;
      EUDAQ_ERROR(msg.data());
      SetStatus(eudaq::Status::LVL_ERROR, msg.data());
      return;
    }
	std::cout << "PARSEXML READTRUE" << std::endl;
    m_reader[i]->ParseXML(doc.FirstChild("root")->ToElement(), -1, -1, true);

    std::string configStr;
    configStr << doc;

    char tmp[100];
    sprintf(tmp, "Config_%d", i);
    bore.SetTag(tmp, configStr);

    // store masked pixels
    const std::vector<TPixHit> pixels = m_reader[i]->GetDUT()->GetNoisyPixels();

    std::string pixelStr;
    for (int j = 0; j < pixels.size(); j++) {
      char buffer[50];
      sprintf(buffer, "%d %d %d\n", pixels.at(j).region, pixels.at(j).doublecol,
              pixels.at(j).address);
      pixelStr += buffer;
    }

    sprintf(tmp, "MaskedPixels_%d", i);
    bore.SetTag(tmp, pixelStr);

    // firmware version
    sprintf(tmp, "FirmwareVersion_%d", i);
    bore.SetTag(tmp, m_reader[i]->GetDAQBoard()->GetFirmwareName());

    sprintf(tmp, "ChipType_%d", i);
    bore.SetTag(tmp, m_chip_type[i]);
    // readout / triggering settings
    sprintf(tmp, "StrobeLength_%d", i);
    bore.SetTag(tmp, m_strobe_length[i]);
    sprintf(tmp, "StrobeBLength_%d", i);
    bore.SetTag(tmp, m_strobeb_length[i]);
    sprintf(tmp, "ReadoutDelay_%d", i);
    bore.SetTag(tmp, m_readout_delay[i]);
    sprintf(tmp, "TriggerDelay_%d", i);
    bore.SetTag(tmp, m_trigger_delay[i]);

    // S-curve scan data
    SCS_data_block.clear();
    SCS_data_block.reserve(512 * 1024 * m_SCS_n_steps);
    SCS_points_block.clear();
    SCS_points_block.reserve(m_SCS_n_steps);
    sprintf(tmp, "SCS_%d", i);
    if (m_do_SCS[i]) {
      for (int j = 0; j < 512; ++j) {
        for (int k = 0; k < 1024; ++k) {
          for (int l = 0; l < m_SCS_n_steps; ++l) {
            SCS_data_block.push_back(m_SCS_data[i][j][k][l]);
          }
        }
      }
      for (int j = 0; j < m_SCS_n_steps; ++j) {
        SCS_points_block.push_back(m_SCS_points[i][j]);
      }
      bore.SetTag(tmp, (int)1);
    } else {
      bore.SetTag(tmp, (int)0);
    }
    bore.AddBlock(2 * i, SCS_data_block);
    bore.AddBlock(2 * i + 1, SCS_points_block);
  }

  // general S-curve scan configuration
  bore.SetTag("SCSchargeStart", m_SCS_charge_start);
  bore.SetTag("SCSchargeStop", m_SCS_charge_stop);
  bore.SetTag("SCSchargeStep", m_SCS_charge_step);
  bore.SetTag("SCSnEvents", m_SCS_n_events);
  bore.SetTag("SCSnMaskStages", m_SCS_n_mask_stages);

  // back-bias voltage
  bore.SetTag("BackBiasVoltage", m_back_bias_voltage);
  bore.SetTag("DUTposition", m_dut_pos);

  //Configuration is done, Read DAC Values and send to log

  for (int i = 0; i < m_nDevices; i++) {
    TDAQBoard *daq_board = m_reader[i]->GetDAQBoard();
    std::cout << "Reading Device:" << i << " ADCs" <<  std::endl;
    daq_board->ReadAllADCs();
  }

  SendEvent(bore);

  {
    SimpleLock lock(m_mutex);
    m_running = true;
  }
  for (int i = 0; i < m_nDevices; i++) {
    m_reader[i]->SetRunning(true);
    m_reader[i]->StartDAQ();
  }

  SetStatus(eudaq::Status::LVL_OK, "Running");
  EUDAQ_INFO("Running");
}

void PALPIDEFSProducer::OnStopRun() {
  std::cout << "Stop Run" << std::endl;
  {
    SimpleLock lock(m_mutex);
    m_running = false;
    m_stopping = true;
  }
  for (int i = 0; i < m_nDevices; i++) { // stop the event polling loop
    m_reader[i]->SetRunning(false);
  }
  for (int i = 0; i < m_nDevices;
       i++) { // wait until all read transactions are done
    while (m_reader[i]->IsReading())
      eudaq::mSleep(20);

  }
  for (int i = 0; i < m_nDevices; i++) { // stop the DAQ board
    m_reader[i]->StopDAQ();
  }
  {
    SimpleLock lock(m_mutex);
    m_flush = true;
  }

  long wait_cnt = 0;
  eudaq::mSleep(100);


  while (IsFlushing() || IsStopping()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still flushing...";
      std::cout << msg << std::endl;
      SetStatus(eudaq::Status::LVL_WARN, msg.data());
    }
  }
  SetStatus(eudaq::Status::LVL_OK, "Run Stopped");
}

void PALPIDEFSProducer::OnTerminate() {
  std::cout << "Terminate" << std::endl;
  PowerOffTestSetup();
  std::cout << "All boards powered off" << std::endl;
  SimpleLock lock(m_mutex);
  m_done = true;
}

void PALPIDEFSProducer::OnReset() {
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void PALPIDEFSProducer::OnUnrecognised(const std::string &cmd,
                                       const std::string &param) {
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0)
    std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Unrecognised");
}

void PALPIDEFSProducer::Loop() {
  unsigned long count = 0;
  time_t last_status = time(0);
  do {
    eudaq::mSleep(20);

    if (!IsRunning()) {
      if (IsFlushing()) {
        if (IsStopping()) {
          EUDAQ_INFO("SendEOR");
          SendEOR();
        }
        EUDAQ_INFO("Flushing");
        // check if any producer is waiting for EOR
        bool waiting_for_eor = false;
        for (int i = 0; i < m_nDevices; i++) {
          if (m_reader[i]->IsWaitingForEOR()) {
            waiting_for_eor = true;
             EUDAQ_INFO("WAITING FOR EOR");
           }
        }
        if (!waiting_for_eor) {
          // write out last events
          eudaq::mSleep(1000);
          while (BuildEvent() > 0) {
          }
          PrintQueueStatus();
          m_flush = false ;
          continue;
        }
      } else
        continue;
    }
    // build events
    while (IsRunning()) {
      int events_built = BuildEvent();
      count += events_built;

      if (events_built == 0) {
        if (m_status_interval > 0 &&
            time(0) - last_status > m_status_interval) {
          if (IsRunning())
            SendStatusEvent();
          PrintQueueStatus();
          last_status = time(0);
        }
        break;
      }

      if (count % 20000 == 0)
       std::cout << "Sending event " << count << std::endl;
    }
  } while (!IsDone());
}

int PALPIDEFSProducer::BuildEvent() {
  if (m_debuglevel > 3)
    std::cout << "BuildEvent..." << std::endl;

  if (m_debuglevel > 10)
    eudaq::mSleep(2000);

  // fill next event and check if all event fragments are there
  // NOTE a partial _last_ event is lost in this way (can be improved based on
  // flush)
  uint64_t trigger_id = ULONG_MAX;
  uint64_t timestamp = ULONG_MAX;
  for (int i = 0; i < m_nDevices; i++) {
    if (m_next_event[i] == 0)
      m_next_event[i] = m_reader[i]->PopNextEvent();
    if (m_next_event[i] == 0)
      return 0;
    if (trigger_id > m_next_event[i]->m_trigger_id)
      trigger_id = m_next_event[i]->m_trigger_id;
    if (timestamp > m_next_event[i]->m_timestamp_corrected)
      timestamp = m_next_event[i]->m_timestamp_corrected;
    if (m_debuglevel > 2)
      std::cout << "Fragment " << i << " with trigger id "
                << m_next_event[i]->m_trigger_id << std::endl;
  }

  if (m_debuglevel > 2)
    std::cout << "Sending event with trigger id " << trigger_id << std::endl;

  bool *layer_selected = new bool[m_nDevices];
  for (int i = 0; i < m_nDevices; i++)
    layer_selected[i] = false;

  // select by trigger id
  for (int i = 0; i < m_nDevices; i++) {
    if (m_ignore_trigger_ids || m_next_event[i]->m_trigger_id == trigger_id)
      layer_selected[i] = true;
  }

  // time stamp check & recovery
  bool timestamp_error = false;
  for (int i = 0; i < m_nDevices; i++) {
    if (!layer_selected[i])
      continue;

    SingleEvent *single_ev = m_next_event[i];

    if (timestamp != 0 &&
        (float)single_ev->m_timestamp_corrected / timestamp > 1.01 &&
        single_ev->m_timestamp_corrected - timestamp >= 20) {
      char msg[200];
      sprintf(msg, "Event %d. Out of sync: Timestamp of current event "
                   "(device %d) is %llu while smallest is %llu.",
              m_ev, i, single_ev->m_timestamp_corrected, timestamp);
      std::string str(msg);

      if (m_firstevent) {
        // the different layers may not have booted their FPGA at the same
        // time, so the timestamp of the first event can be different. All
        // subsequent ones should be identical.
        str += " This is the first event after startup - so it might be OK.";
      } else {
        timestamp_error = true;

        if (m_recover_outofsync) {
          layer_selected[i] = false;
          str += " Excluding layer.";
        }
      }

      std::cerr << str << std::endl;
      if (m_firstevent) {
        EUDAQ_INFO(str);
      } else {
        EUDAQ_WARN(str);
        SetStatus(eudaq::Status::LVL_WARN, str);
      }
    }
  }

  if (timestamp_error && m_recover_outofsync) {
    // recovery needs at least one more event in planes in question
    for (int i = 0; i < m_nDevices; i++) {
      if (!layer_selected[i])
        continue;
      if (!m_reader[i]->NextEvent()) {
        delete[] layer_selected;
        return 0;
      }
    }
  }

  // send event with trigger id trigger_id
  // send all layers in one block
  unsigned long total_size = 0;
  for (int i = 0; i < m_nDevices; i++) {
    if (layer_selected[i]) {
      total_size += 2 + sizeof(uint16_t);
      total_size += 2 * sizeof(uint64_t);
      total_size += m_next_event[i]->m_length;
    }
  }

  char *buffer = new char[total_size];
  unsigned long pos = 0;
  for (int i = 0; i < m_nDevices; i++) {
    if (layer_selected[i]) {
      buffer[pos++] = 0xff;
      buffer[pos++] = i;

      SingleEvent *single_ev = m_next_event[i];

      // data length
      uint16_t length = sizeof(uint64_t) * 2 + single_ev->m_length;
      memcpy(buffer + pos, &length, sizeof(uint16_t));
      pos += sizeof(uint16_t);

      // event id and timestamp per layer
      memcpy(buffer + pos, &(single_ev->m_trigger_id), sizeof(uint64_t));
      pos += sizeof(uint64_t);
      memcpy(buffer + pos, &(single_ev->m_timestamp), sizeof(uint64_t));
      pos += sizeof(uint64_t);

      memcpy(buffer + pos, single_ev->m_buffer, single_ev->m_length);
      pos += single_ev->m_length;
    }
  }

  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.AddBlock(0, buffer, total_size);
  if (IsRunning())
    SendEvent(ev);
  delete[] buffer;
  m_firstevent = false;

  // clean up
  for (int i = 0; i < m_nDevices; i++) {
    if (layer_selected[i]) {
      delete m_next_event[i];
      m_next_event[i] = 0;
    }
  }

  if (timestamp_error && m_recover_outofsync) {
    printf("Event %d. Trying to recover from out of sync error by adding "
           "%llu to next event in layer ",
           m_ev - 1, timestamp);
    for (int i = 0; i < m_nDevices; i++)
      if (layer_selected[i])
        printf("%d (%llu), ", i,
               m_reader[i]->NextEvent()->m_timestamp_corrected);

    printf("\n");

    for (int i = 0; i < m_nDevices; i++) {
      if (layer_selected[i]) {
        m_next_event[i] = m_reader[i]->PopNextEvent();
        m_next_event[i]->m_timestamp_corrected += timestamp;
      }
    }
  }

  delete[] layer_selected;

  return 1;
}

void PALPIDEFSProducer::SendEOR() {
  char msg[100];
  snprintf(msg, 100, "Sending EOR, Run %d, Event %d", m_run, m_ev);
  std::cout << msg << std::endl;
  EUDAQ_INFO(msg);
  SendEvent(RawDataEvent::EORE(EVENT_TYPE, m_run, m_ev++));
  SimpleLock lock(m_mutex);
  m_stopping = false;
}

void PALPIDEFSProducer::SendStatusEvent() {
  std::cout << "Sending status event" << std::endl;
  // temperature readout
  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.SetTag("pALPIDEfs_Type", 1);
  for (int i = 0; i < m_nDevices; i++) {
    float temp = m_reader[i]->GetTemperature();
    ev.AddBlock(i, &temp, sizeof(float));
  }
  if (IsRunning())
    SendEvent(ev);
}

void PALPIDEFSProducer::PrintQueueStatus() {
  for (int i = 0; i < m_nDevices; i++)
    m_reader[i]->PrintQueueStatus();
}

// -------------------------------------------------------------------------------------------------

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Producer", "1.0", "The pALPIDEfs Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "pALPIDEfs", "string",
                                  "The name of this Producer");
  eudaq::Option<int> debug_level(op, "d", "debug-level", 0, "int",
                                 "Debug level for the producer");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    PALPIDEFSProducer producer(name.Value(), rctrl.Value(),
                               debug_level.Value());
    producer.Loop();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
