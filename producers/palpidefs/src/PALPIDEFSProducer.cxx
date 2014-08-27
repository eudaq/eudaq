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
#include <iostream>
#include <ostream>
#include <cctype>
#include <cstdlib>

#include <list>
#include <climits>

// #define SIMULATION // don't initialize, send dummy events
#define LOADSIMULATION // if set simulate continous stream, if not set simulate SPS conditions

using eudaq::StringEvent;
using eudaq::RawDataEvent;

static const std::string EVENT_TYPE = "pALPIDEfsRAW";

unsigned int Bitmask(int width)
{
  unsigned int tmp = 0;
  for (int i=0; i<width; i++)
    tmp |= 1 << i;
  return tmp;
}

int16_t GetChipWord (unsigned char *Data)
{
    int16_t       Data16 = 0;
    unsigned char LSByte, MSByte;

    LSByte = Data[0];
    MSByte = Data[1];

    Data16 += ((int16_t)MSByte) << 8;
    Data16 += LSByte;

    return Data16;
}


void ParseXML(TDAQBoard* daq_board, TiXmlNode* node, int base, int rgn, bool readwrite)
{
  // readwrite (from Chip): true = read; false = write
  
  for (TiXmlNode* pChild = node->FirstChild("address"); pChild != 0; pChild = pChild->NextSibling("address")) 
  {
    if (pChild->Type() != TiXmlNode::TINYXML_ELEMENT)
      continue;
//     printf( "Element %d [%s] %d %d\n", pChild->Type(), pChild->Value(), base, rgn);
    TiXmlElement* elem = pChild->ToElement();
    if (base == -1) {
      if (!elem->Attribute("base")) {
	std::cout << "Base attribute not found!" << std::endl;
	break;
      }
      ParseXML(daq_board, pChild, atoi(elem->Attribute("base")), -1, readwrite);
    }
    else if (rgn == -1) {
      if (!elem->Attribute("rgn")) {
	std::cout << "Rgn attribute not found!" << std::endl;
	break;
      }
      ParseXML(daq_board, pChild, base, atoi(elem->Attribute("rgn")), readwrite);
    }
    else {
      if (!elem->Attribute("sub")) {
	std::cout << "Sub attribute not found!" << std::endl;
	break;
      }
      int sub = atoi(elem->Attribute("sub"));
      int address = ((rgn << 11) + (base << 8) + sub);

      int value = 0;
      
      if (readwrite) {
	if (daq_board->ReadChipRegister(address, &value) != 1) {
	  std::cout << "Failure to read chip address " << address << std::endl;
	  continue;
	}
      }
      
      for (TiXmlNode* valueChild = pChild->FirstChild("value"); valueChild != 0; valueChild = valueChild->NextSibling("value")) 
      {
	if (!valueChild->ToElement()->Attribute("begin")) {
	  std::cout << "begin attribute not found!" << std::endl;
	  break;
	}
	int begin = atoi(valueChild->ToElement()->Attribute("begin"));
	
	int width = 1;
	if (valueChild->ToElement()->Attribute("width")) // width attribute is optional
	  width = atoi(valueChild->ToElement()->Attribute("width"));
	
	if (!valueChild->FirstChild("content") && !valueChild->FirstChild("content")->FirstChild()) {
	  std::cout << "content tag not found!" << std::endl;
	  break;
	}
	
	if (readwrite) {
	  int subvalue = (value >> begin) & Bitmask(width);
	  char tmp[5];
	  sprintf(tmp, "%X", subvalue);
	  valueChild->FirstChild("content")->FirstChild()->SetValue(tmp);
	} else { 
	  int content = (int) strtol(valueChild->FirstChild("content")->FirstChild()->Value(), 0, 16);
	  
	  if (content >= (1 << width)) {
	    std::cout << "value too large: " << begin << " " << width << " " << content << " " << valueChild->FirstChild("content")->Value() << std::endl;
	    break;
	  }
	  
	  value += content << begin;
	}
      }
      
      if (!readwrite) {
//       printf("%d %d %d: %d %d\n", base, rgn, sub, address, value);
      printf("Writing chip register %d %d\n", address, value);
	if (daq_board->WriteChipRegister(address, value) != 1)
	  std::cout << "Failure to write chip address " << address << std::endl;
      }
    }
  }  
}

DeviceReader::DeviceReader(int id, int debuglevel, TDAQBoard* daq_board, TpAlpidefs* dut) : 
  m_queue_size(0), 
  m_stop(false), 
  m_running(false), 
  m_flushing(false),
  m_id(id), 
  m_debuglevel(debuglevel), 
  m_daq_board(daq_board),
  m_dut(dut),
  m_last_trigger_id(0),
  m_queuefull_delay(100),
  m_max_queue_size(50*1024*1024),
  m_high_rate_mode(false)
{ 
  m_thread.start(DeviceReader::LoopWrapper, this);
#ifndef SIMULATION
  m_last_trigger_id = m_daq_board->GetNextEventId();
  Print(0, "Starting with last event id: %lu", m_last_trigger_id);
#endif
  // packet based readout variables
  // global variables needed since information can be distribitued over multiple readouts
  m_count_word_header = 0;
  m_count_word_data = 0;
  m_last_word=0;
  m_last_word0=0;
  m_last_word1=0;
  m_lastEventID = 0;
  m_size=0;
  m_HeaderOK = m_EventOK = m_isTrailer = m_isData = m_endHeader=false;
  m_isHeader = m_eventIDOK=true;

}
    
void DeviceReader::Stop() 
{
  Print(0, "Stopping...");
  SetStopping();
  m_thread.join();
  Disconnect();
}

void DeviceReader::SetRunning(bool running) 
{
  Print(0, "Set running: %lu", running);
  SimpleLock lock(m_mutex);
  if (m_running && !running)
    m_flushing = true;
  m_running = running;
  
#ifndef SIMULATION  
  if (m_running)
    m_daq_board->StartTrigger();
  else
    m_daq_board->StopTrigger();
#endif
}

SingleEvent* DeviceReader::NextEvent() 
{
  SimpleLock lock(m_mutex);
  if (m_queue.size() > 0)
    return m_queue.front();
  return 0;
}

void DeviceReader::DeleteNextEvent() 
{
  SimpleLock lock(m_mutex);
  if (m_queue.size() == 0)
    return;
  SingleEvent* ev = m_queue.front();
  m_queue.pop();
  m_queue_size -= ev->m_length;
  delete ev;
}

SingleEvent* DeviceReader::PopNextEvent() 
{
  SimpleLock lock(m_mutex);
  if (m_queue.size() > 0) {
    SingleEvent* ev = m_queue.front();
    m_queue.pop();
    m_queue_size -= ev->m_length;
    if (m_debuglevel > 3)
      Print(0, "Returning event. Current queue status: %lu elements. Total size: %lu", m_queue.size(), m_queue_size);
    return ev;
  }
  if (m_debuglevel > 3)
    Print(0, "PopNextEvent: No event available");
  return 0;
}

void DeviceReader::PrintQueueStatus() 
{
  SimpleLock lock(m_mutex);
  Print(0, "Queue status: %lu elements. Total size: %lu", m_queue.size(), m_queue_size);
}

void* DeviceReader::LoopWrapper(void* arg) 
{
  DeviceReader* ptr = static_cast<DeviceReader*>(arg);
  ptr->Loop();
  return 0;
}
  
void DeviceReader::Loop() 
{
  Print(0, "Loop starting...");
  while (1) {
    
    if (IsStopping())
      break;
    
//     eudaq::mSleep(10);

    if (!IsRunning() && !IsFlushing()) {
      eudaq::mSleep(20); 
      continue;
    }
    
    if (m_debuglevel > 10)
      eudaq::mSleep(1000);
    
#ifdef SIMULATION
#ifdef LOADSIMULATION	  
    // simulate constantly events to test max throughput
      
    // process data
    m_last_trigger_id++;

    // simulation for missing data in some layers
    if (m_debuglevel > 10) {
      if (m_id == 2 && m_last_trigger_id % 2 == 0)
	continue;
      if (m_id == 3 && m_last_trigger_id % 3 == 0)
	continue;
    }
    
    int length = 80;
    SingleEvent* ev = new SingleEvent(length, m_last_trigger_id, 0);
    for (int i = 0; i < length; ++i) {
      ev->m_buffer[i] = 0; //(char) std::rand();
    }
    
    // add to queue
    Push(ev);
    
    // achieve 25 Hz
    eudaq::mSleep(1000 / 25);
    
#else
    // simulate 100 kHz for 2.4s spill, every 20 seconds
    
    int eventsPerSpill = 240000;
    if (m_debuglevel > 10)
      eventsPerSpill = 10;
    for (int i=0; i<eventsPerSpill; i++) {
      const int kPixelsHit = 1;
      int length = 64 + kPixelsHit*2;
      SingleEvent* ev = new SingleEvent(length, m_last_trigger_id++);
      // empty headers
      int pos = 0;
      for (int j = 0; j < 32; ++j) {
	// "test" pixels are put into region m_id, all others are empty
	ev->m_buffer[pos++] = (j == m_id) ? kPixelsHit : 0; // length of data
	ev->m_buffer[pos++] = (j << 3); // region number (5 MSB)
	if (j == m_id) {
	  short pixeladdr = m_last_trigger_id % (1 << 14); // 14 bits: 10 LSB for pixel id, 4 HSB for double column address
	  char clustering = m_id % 4;
	  pixeladdr |= clustering << 14;

	  ev->m_buffer[pos++] = (char) (pixeladdr & 0xff);
	  ev->m_buffer[pos++] = (char) (pixeladdr >> 8);
	}
      }

      if (m_debuglevel > 10) {
	std::string str;
	for (int j=0; j<length; j++) {
	  char buffer[20];
	  sprintf(buffer, "%x ", ev->m_buffer[j]);
	  str += buffer;
	}
	str += "\n";
	Print(0, str.data());
      }
    
      // add to queue
      Push(ev);
      
      if (i % 100 == 0)
	eudaq::mSleep(1);
    }
    Print(0, "One spill simulated.");
    
    eudaq::mSleep(20000 - 2400);
#endif
#else

    // Not needed for packet based!
    bool event_waiting = true;
    if (!m_high_rate_mode && m_daq_board->GetNextEventId() <= m_last_trigger_id+1)
        event_waiting = false;
    
    if (!event_waiting)
    {
//       std::cout << "No event " << m_daq_board->GetNextEventId() << " " << m_last_trigger_id << std::endl;
        // no event waiting
        /*
        // flushing moved at the end of the loop. correct?
        if (IsFlushing()) {
            SimpleLock lock(m_mutex);
            m_flushing = false;
            Print(0, "Finished flushing %lu %lu", m_daq_board->GetNextEventId(), m_last_trigger_id);
        }
        */
        eudaq::mSleep(1);
        continue;
    }

    // data taking
    const int maxDataLength = 1024; // 512*32 + 64; // TODO check
    unsigned char data_buf[maxDataLength];
    int Length;
    std::vector<TPixHit> Hits;

    int count_byte=0;
    char buffer[20];
    float byte_packet;
    int j;

    if (m_daq_board->ReadChipEvent(data_buf, &Length, maxDataLength)) {
        count_byte =0;
        while (count_byte < Length){
            // Last Word of buffer
            if (count_byte==0 && m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte)==0xabfeabfe && m_isHeader==false && m_endHeader==false) { 
                m_Trailer[0] = m_last_word;
                m_Trailer[1] = m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte) ;
                count_byte+=4;
                if(m_debuglevel > 2) {
                    m_EventOK = m_dut->DecodeEvent(m_Data,m_count_word_data, &Hits);
                    if(m_EventOK)
                        std::cerr << "ERROR decoding event payload. " << std::endl;
                    else
                        m_dut->DumpHitData(Hits);
                }
                SingleEvent* ev = new SingleEvent(m_count_word_data, m_header.EventId, m_header.TimeStamp);
                memcpy(ev->m_buffer, m_Data, m_count_word_data);
                Push(ev); // add to queue
                m_last_trigger_id = m_header.EventId;
                m_TrailerOK = m_daq_board->DecodeEventTrailerNew(m_Trailer,&m_trailer);
                m_count_word_data=0;
                m_isData = false;
                m_isTrailer = false;
                m_isHeader = true;
                m_lastEventID = m_header.EventId;
                m_size += m_trailer.EventSize;
            }
            if (count_byte==0 && m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte) != 0xabfeabfe && m_isHeader==false && m_endHeader==false){
                m_Data[m_count_word_data]=m_last_word;
//                m_Data[m_count_word_data]= m_last_word0;
                m_count_word_data++;
//                m_Data[m_count_word_data]= m_last_word1;
//                m_count_word_data++;
            } 
            // Read Header
            if (m_endHeader == true) m_endHeader = false;
            if (m_isHeader == true){
                m_Header[m_count_word_header] = m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte);
                m_count_word_header++;
                count_byte+=4;
                if ( m_count_word_header == 5){
                    m_HeaderOK = m_daq_board->DecodeEventHeaderNew(m_Header, &m_header);
                    m_count_word_header = 0;
                    m_isHeader = false;
                    m_endHeader= true;
                }
            }
            if (count_byte + 4 < Length){
                if (m_isHeader==false){
                    if (m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte +4) != 0xabfeabfe ) {m_isData = true;m_endHeader=false;}
                    else {m_isData=false; m_isTrailer=true;m_endHeader=false;}
                }
                //Read Data
                if ( m_isData == true ){   
                    m_Data[m_count_word_data] = data_buf[count_byte];
                    m_Data[m_count_word_data+1] = data_buf[count_byte+1];
                    m_count_word_data+=2;
                    count_byte+=2;
                }
                // Read Trailer
                if(m_isTrailer == true){
                    m_Trailer[0]= m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte);
                    count_byte+=4;
                    m_Trailer[1]= m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte);
                    count_byte+=4;
                    if(m_debuglevel > 2) {
                        m_EventOK = m_dut->DecodeEvent(m_Data,m_count_word_data, &Hits);
                        if(m_EventOK)
                            std::cerr << "ERROR decoding event payload. " << std::endl;
                        else
                            m_dut->DumpHitData(Hits);
                    }
                    SingleEvent* ev = new SingleEvent(m_count_word_data, m_header.EventId, m_header.TimeStamp);
                    memcpy(ev->m_buffer, m_Data, m_count_word_data);
                    // add to queue
                    Push(ev);
                    m_last_trigger_id = m_header.EventId;

                    m_count_word_data=0;
                    m_TrailerOK = m_daq_board->DecodeEventTrailerNew(m_Trailer,&m_trailer);
                    m_size += m_trailer.EventSize;
                    m_isData = false;
                    m_isTrailer = false;
                    m_isHeader = true;
                    // TODO:
                    // OUT OF SYNC is currently checked via timestamps; consider substituting it with the following code
                    /*
                    // Check if the event is corrupted
                    if (m_trigger_id > 0) {
                        if (header.EventId > lastEventID + 1) {
                            std::cout << "It seems that an event was missed. Last id: " << lastEventID << " Current id: " << header.EventId << std::endl;
                            eventIDOK = false;
                        }
                    }
                    */
                    m_lastEventID = m_header.EventId;
                    if (m_debuglevel > 2 || !m_HeaderOK || !m_TrailerOK || !m_eventIDOK) {
                        Print(2, "ERROR decoding event. Header: %d Trailer: %d", m_HeaderOK, m_TrailerOK);
                        std::cerr << "DEBUG/ERROR in data stream: Header: " << m_HeaderOK << " Event: " << m_EventOK << " Trailer: " << m_TrailerOK<<"ID: "<< m_eventIDOK<<std::endl;
                        std::string str = "RAW dump: ";
                        for (j=0; j<Length; j++) {
                            sprintf(buffer, "%02x ", data_buf[j]);
                            str += buffer;
                        }
                        str += "\n";
                        std::cerr << str.data() << std::endl;
                    }
                    if (m_eventIDOK == false) m_eventIDOK = true;
                }
            }
            else{
                if (m_isHeader == false && m_endHeader == false){
                    m_last_word=m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte);
                    m_last_word0=GetChipWord(data_buf + count_byte);
                    m_last_word1=GetChipWord(data_buf + count_byte + 2);
                    count_byte+=4;
                }
            }
            if (IsFlushing() && m_daq_board->GetIntFromBinaryStringReversed(4, data_buf + count_byte) == 0xebfeebfe){
                count_byte += 4;
                Print(0, "Flushing %lu", m_last_trigger_id);
                SimpleLock lock(m_mutex);
                m_flushing = false;
                Print(0, "Sending EndOfRun word");
                m_daq_board->EndOfRun();
		
		// gets resetted by EndOfRun
		m_last_trigger_id = m_daq_board->GetNextEventId();

                m_count_word_header = 0;
                m_count_word_data = 0;
                m_last_word=0;
                m_last_word0=0;
                m_last_word1=0;
                m_lastEventID = 0;
                m_size=0;
                m_HeaderOK = m_EventOK = m_isTrailer = m_isData = m_endHeader=false;
                m_isHeader = m_eventIDOK=true;
            }
        }
    }
#endif
  }
  Print(0, "ThreadRunner stopping...");
}
    
void DeviceReader::Print(int level, const char* text, uint64_t value1, uint64_t value2, uint64_t value3, uint64_t value4) 
{
  // level:
  //   0 -> printout
  //   1 -> INFO
  //   2 -> WARN
  //   3 -> ERROR
  
  std::string tmp("DeviceReader %d: ");
  tmp += text;
  
  char msg[1000];
  sprintf(msg, tmp.data(), m_id, value1, value2, value3, value4);
  
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

bool DeviceReader::Disconnect() 
{
  // TODO move poweroff here
  Print(0, "Disconnecting device...");
}

void DeviceReader::Push(SingleEvent* ev) 
{
  while (QueueFull())
    eudaq::mSleep(m_queuefull_delay);
  SimpleLock lock(m_mutex);
  m_queue.push(ev);
  m_queue_size += ev->m_length;
  if (m_debuglevel > 2)
    Print(0, "Pushed events. Current queue size: %lu", m_queue.size());
}

bool DeviceReader::QueueFull() 
{
  SimpleLock lock(m_mutex);
  if (m_queue_size > m_max_queue_size) {
    if (m_debuglevel > 3)
      Print(0, "BUSY: %lu %lu", m_queue.size(), m_queue_size);
    return true;
  }
  return false;
}

float DeviceReader::GetTemperature()
{
#ifndef SIMULATION
    return m_daq_board->GetTemperature();
#else
  return 0;
#endif
}

void DeviceReader::ParseXML(TiXmlNode* node, int base, int rgn, bool readwrite)
{
  ::ParseXML(m_daq_board, node, base, rgn, readwrite);
}

// -------------------------------------------------------------------------------------------------

void PALPIDEFSProducer::OnConfigure(const eudaq::Configuration & param) 
{
  std::cout << "Configuring..." << std::endl;
  
  if (!m_configured) {
    m_nDevices = param.Get("Devices", 1);
    m_status_interval = param.Get("StatusInterval", -1);
  
    const int delay = param.Get("QueueFullDelay", 0);
    const unsigned long queue_size = param.Get("QueueSize", 0) * 1024 * 1024;
    
    const int StrobeLength =  param.Get("StrobeLength",  10);
    const int StrobeBLength = param.Get("StrobeBLength", 20);
    const int ReadoutDelay =  param.Get("ReadoutDelay",  10);
    const int TriggerDelay =  param.Get("TriggerDelay",  75);
    
    if (param.Get("CheckTriggerIDs", 0) == 1)
      m_ignore_trigger_ids = false;
    
    const bool high_rate_mode = (param.Get("HighRateMode", 0) == 1);
    
    m_full_config = param.Get("FullConfig", "");
    
  #ifndef SIMULATION
    if (!m_testsetup) {
      std::cout << "Creating test setup " << std::endl;
      m_testsetup = new TTestSetup;
    }
    
    std::cout << "Searching for DAQ boards " << std::endl;
    m_testsetup->FindDAQBoards();
    std::cout << "Found " << m_testsetup->GetNDAQBoards() << " DAQ boards." << std::endl;
    
    if (m_testsetup->GetNDAQBoards() < m_nDevices) {
      char msg[100];
      sprintf(msg, "Not enough devices connected. Configuration requires %d devices, but only %d present.", m_nDevices, m_testsetup->GetNDAQBoards());
      std::cerr << msg << std::endl;
      EUDAQ_ERROR(msg);
      SetStatus(eudaq::Status::LVL_ERROR, msg);
      return;
    }
    
    for (int i=0; i<m_nDevices; i++)
      m_testsetup->AddDUTs(DUT_PALPIDEFS);
  #endif

    m_next_event = new SingleEvent*[m_nDevices];
    m_reader = new DeviceReader*[m_nDevices];
    for (int i=0; i<m_nDevices; i++) {
      char buffer[100];
      sprintf(buffer, "BoardAddress_%d", i);
      int board_address = param.Get(buffer, i);

  #ifndef SIMULATION
      // find board
      int board_no = -1;
      for (int j=0; j<m_testsetup->GetNDAQBoards(); j++) {
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
	return;
      }
      
      std::cout << "Enabling device " << board_no << std::endl;
      TpAlpidefs* dut = (TpAlpidefs *) m_testsetup->GetDUT(board_no);
      TDAQBoard* daq_board = m_testsetup->GetDAQBoard(board_no);
      m_testsetup->PowerOnBoard(board_no);
      m_testsetup->InitialiseChip(board_no);

      // configuration
      sprintf(buffer, "Config_File_%d", i);
      std::string configFile = param.Get(buffer, "");
      if (configFile.length() > 0) {
	// TODO maybe add this functionality to TDAQBoard?
	std::cout << "Running chip configuration..." << std::endl;
	
	TiXmlDocument doc(configFile.c_str());
	if (!doc.LoadFile()) {
	  std::string msg = "Unable to open file ";
	  msg += configFile;
	  std::cerr << msg.data() << std::endl;
	  EUDAQ_ERROR(msg.data());
	  SetStatus(eudaq::Status::LVL_ERROR, msg.data());
	  return;
	}
      
	// TODO return code?
	ParseXML(daq_board, doc.FirstChild("root")->ToElement(), -1, -1, false);
      }
      
      // noisy pixels
      dut->SetMaskAllPixels(false); //unmask all pixels
      sprintf(buffer, "Noisy_Pixel_File_%d", i);
      std::string noisyPixels = param.Get(buffer, "");
      if (noisyPixels.length() > 0) {
	std::cout << "Setting noisy pixel file..." << std::endl;
	dut->ReadNoisyPixelFile(noisyPixels.data());
	dut->MaskNoisyPixels();
      }
      
      // data taking configuration

      // PrepareEmptyReadout
      daq_board->ConfigureReadout (1, false);       // buffer depth = 1, sampling on rising edge
      daq_board->ConfigureTrigger (0, StrobeLength, 2, 0, TriggerDelay);
      
      // PrepareChipReadout
      dut->SetChipMode(MODE_ALPIDE_CONFIG);
      dut->SetReadoutDelay     (ReadoutDelay);
      dut->SetEnableClustering (false);
      dut->SetStrobeTiming     (StrobeBLength);
      dut->SetEnableOutputDrivers(true, true);
      dut->SetChipMode         (MODE_ALPIDE_READOUT_B);
  #else
      TpAlpidefs* dut = 0;
      TDAQBoard* daq_board = 0;
  #endif

      m_reader[i] = new DeviceReader(i, m_debuglevel, daq_board, dut);
      if (delay > 0)
	m_reader[i]->SetQueueFullDelay(delay);
      if (queue_size > 0)
	m_reader[i]->SetMaxQueueSize(queue_size);
      m_reader[i]->SetHighRateMode(high_rate_mode);
      m_next_event[i] = 0;
      
      std::cout << "Device " << i << " with board address " << board_address << " (delay " << delay << " - queue size " << queue_size << ") intialized." << std::endl;

      eudaq::mSleep(10);
    }
    
    m_configured = true;
  } else {
    // reconfigure
    
    std::cout << "Already initialized and powered. Doing only reconfiguration..." << std::endl;
    
    for (int i=0; i<m_nDevices; i++) {
      // only configuration
#ifndef SIMULATION
      char buffer[100];
      sprintf(buffer, "Config_File_%d", i);
      std::string configFile = param.Get(buffer, "");
      if (configFile.length() > 0) {
	// TODO maybe add this functionality to TDAQBoard?
	std::cout << "Running chip configuration..." << std::endl;
	
	TiXmlDocument doc(configFile.c_str());
	if (!doc.LoadFile()) {
	  std::string msg = "Unable to open file ";
	  msg += configFile;
	  std::cerr << msg.data() << std::endl;
	  EUDAQ_ERROR(msg.data());
	  SetStatus(eudaq::Status::LVL_ERROR, msg.data());
	  return;
	}
      
	// TODO return code?
	ParseXML(m_reader[i]->GetDAQBoard(), doc.FirstChild("root")->ToElement(), -1, -1, false);
      }
#endif
    }
  }
  
  EUDAQ_INFO("Configured (" + param.Name() + ")");
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}
    
void PALPIDEFSProducer::OnStartRun(unsigned param) 
{
  std::cout << "Start Run: " << param << std::endl;

  m_run = param;
  m_ev = 0;
  
  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
  bore.SetTag("Devices", m_nDevices);
  bore.SetTag("DataVersion", 1);
  
  // read configuration, dump to XML string
  for (int i=0; i<m_nDevices; i++) {
    TiXmlDocument doc(m_full_config.c_str());
    if (!doc.LoadFile()) {
      std::string msg = "Failed to load config file: ";
      msg += m_full_config;
      std::cerr << msg.data() << std::endl;
      EUDAQ_ERROR(msg.data());
      SetStatus(eudaq::Status::LVL_ERROR, msg.data());
      return;
    }
    
#ifndef SIMULATION  
    m_reader[i]->ParseXML(doc.FirstChild("root")->ToElement(), -1, -1, true);
#endif
    
    std::string configStr;
    configStr << doc;
    
//     std::cout << configStr << std::endl;
    
    char tmp[100];
    sprintf(tmp, "Config_%d", i);
    bore.SetTag(tmp, configStr);

    // TODO noisy pixels
  }
  
  SendEvent(bore);

  {
    SimpleLock lock(m_mutex);
    m_running = true;
  }
  for (int i=0; i<m_nDevices; i++)
    m_reader[i]->SetRunning(true);

  SetStatus(eudaq::Status::LVL_OK, "Running");
}
    
void PALPIDEFSProducer::OnStopRun() 
{
  std::cout << "Stop Run" << std::endl;
  for (int i=0; i<m_nDevices; i++)
    m_reader[i]->SetRunning(false);
  
  SimpleLock lock(m_mutex);
  m_running = false;
  m_flush = true;
}

void PALPIDEFSProducer::OnTerminate() 
{
  std::cout << "Terminate" << std::endl;
  for (int i=0; i<m_nDevices; i++) {
    if (m_reader[i] != 0)
      m_reader[i]->Stop();
    // PowerOff Boards (TODO move to DeviceReader)
    m_testsetup->PowerOffBoard(i);
  }
  // TODO does not power off the boards selected by the jumpers
  std::cout << "All boards powered off" << std::endl;
  SimpleLock lock(m_mutex);
  m_done = true;
}

void PALPIDEFSProducer::OnReset() 
{
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void PALPIDEFSProducer::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Unrecognised");
}

void PALPIDEFSProducer::Loop() 
{
    unsigned long count = 0;
    time_t last_status = time(0);
    do {
        eudaq::mSleep(20);
    
        if (!IsRunning())
        {
            if (IsFlushing())
                SendEOR();
            continue;
        }
    
        // build events
        while (1) {
            int events_built = BuildEvent();
            count += events_built;
        
            if (events_built == 0) {
                if (m_status_interval > 0 && time(0) - last_status > m_status_interval) {
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
    
int PALPIDEFSProducer::BuildEvent() 
{
  if (m_debuglevel > 3)
    std::cout << "BuildEvent..." << std::endl;

  if (m_debuglevel > 10)
    eudaq::mSleep(2000);
  
  // fill next event and check if all event fragments are there
  // NOTE a partial _last_ event is lost in this way (can be improved based on flush)
  uint64_t trigger_id = ULONG_MAX;
  uint64_t timestamp = ULONG_MAX;
  for (int i=0; i<m_nDevices; i++) {
    if (m_next_event[i] == 0)
      m_next_event[i] = m_reader[i]->PopNextEvent();
    if (m_next_event[i] == 0)
      return 0;
    if (trigger_id > m_next_event[i]->m_trigger_id)
      trigger_id = m_next_event[i]->m_trigger_id;
    if (timestamp > m_next_event[i]->m_timestamp)
      timestamp = m_next_event[i]->m_timestamp;
    if (m_debuglevel > 2)
      std::cout << "Fragment " << i << " with trigger id " << m_next_event[i]->m_trigger_id << std::endl;
  }
  
  if (m_debuglevel > 2)
    std::cout << "Sending event with trigger id " << trigger_id << std::endl;
  
  // send event with trigger id trigger_id
  // send all layers in one block
  unsigned long total_size = m_nDevices * (2 + 2 * sizeof(uint64_t));
  for (int i=0; i<m_nDevices; i++) {
    SingleEvent* single_ev = m_next_event[i];
    if (m_ignore_trigger_ids || single_ev->m_trigger_id == trigger_id)
      total_size += single_ev->m_length;
  }
  
  char* buffer = new char[total_size+m_nDevices*2];
  unsigned long pos = 0;
  for (int i=0; i<m_nDevices; i++) {
    buffer[pos++] = 0xff;
    buffer[pos++] = i;
    
    // TODO add length of data here
    
    SingleEvent* single_ev = m_next_event[i];

    // select by timestamp
    if (timestamp != 0 && (float) single_ev->m_timestamp / timestamp > 1.5) {
      char msg[100];
      sprintf(msg, "Out of sync: Timestamp of current event (device %d) is %lu while smallest is %lu.", i,  single_ev->m_timestamp, timestamp);
      std::cerr << msg << std::endl;
      EUDAQ_WARN(msg);
      SetStatus(eudaq::Status::LVL_WARN, msg);
    }
      
    // select by trigger id
    if (m_ignore_trigger_ids || single_ev->m_trigger_id == trigger_id)
    {
      // event id and timestamp per layer
      memcpy(buffer+pos, &(single_ev->m_trigger_id), sizeof(uint64_t));
      pos += sizeof(uint64_t);
      memcpy(buffer+pos, &(single_ev->m_timestamp), sizeof(uint64_t));
      pos += sizeof(uint64_t);
    
      memcpy(buffer + pos, single_ev->m_buffer, single_ev->m_length);
      pos += single_ev->m_length;
    }
  }
  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.AddBlock(0, buffer, total_size);
  SendEvent(ev);
  delete buffer;
  
  // clean up
  for (int i=0; i<m_nDevices; i++) {
    if (m_ignore_trigger_ids || m_next_event[i]->m_trigger_id == trigger_id) 
    {
      delete m_next_event[i];
      m_next_event[i] = 0;
    }
  }
  
  return 1;
}

void PALPIDEFSProducer::SendEOR() 
{
  std::cout << "Sending EOR" << std::endl;
  SendEvent(RawDataEvent::EORE(EVENT_TYPE, m_run, m_ev++));
  SimpleLock lock(m_mutex);
  m_flush = false;
}

void PALPIDEFSProducer::SendStatusEvent() 
{
  std::cout << "Sending status event" << std::endl;
  // temperature readout
  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.SetTag("pALPIDEfs_Type", 1);
  for (int i=0; i<m_nDevices; i++) {
    float temp = m_reader[i]->GetTemperature();
    ev.AddBlock(i, &temp, sizeof(float));
  }
  SendEvent(ev);
}

void PALPIDEFSProducer::PrintQueueStatus() 
{
  for (int i=0; i<m_nDevices; i++)
    m_reader[i]->PrintQueueStatus();
}

// -------------------------------------------------------------------------------------------------

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Producer", "1.0", "The pALPIDEfs Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "pALPIDEfs", "string",
      "The name of this Producer");
  eudaq::Option<int> debug_level(op, "d", "debug-level", 0, "int",
      "Debug level for the producer");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    PALPIDEFSProducer producer(name.Value(), rctrl.Value(), debug_level.Value());
    producer.Loop();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
