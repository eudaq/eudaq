#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
//#include "eudaq/TLU2Packet.hh"
#include "tlu/caliceahcalbifController.hh"
#include <iostream>
#include <ostream>
#include <vector>
//#include <chrono>
//#include <cctype>
//#include <memory>

//typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class caliceahcalbifProducer: public eudaq::Producer {
   public:

      caliceahcalbifProducer(const std::string & runcontrol) :
            eudaq::Producer("caliceahcalbif", runcontrol), m_tlu(nullptr), m_run(-1), readout_delay(100), TLUJustStopped(false), TLUStarted(false), done(false), _acq_start_cycle(0), _acq_start_ts(0), _bxidLengthNs(4000), _dumpRaw(0), _firstBxidOffsetBins(
                  13000), _firstStutterCycle(-1), _firstTrigger(true), _last_readout_time(std::time(NULL)), _ReadoutCycle(0), _ROC_started(false), _trigger_id(-1), _triggersInCycle(0), _WaitAfterStopSeconds(0), _writeRaw(false), _writerawfilename_timestamp(
                  true), _dumpCycleInfoLevel(1), _dumpSummary(true), _dumpTriggerInfoLevel(0) {
      }

      void MainLoop() {
         std::cout << "Main loop!" << std::endl;

         _last_readout_time = std::time(NULL);
//         eudaq::RawDataEvent CycleEvent; //

         _ROC_started = false;
         _firstTrigger = true;
         do {
            if (!m_tlu) {
               eudaq::mSleep(50);
               continue;
            }
            bool JustStopped = TLUJustStopped;
            if (JustStopped) {
               //m_tlu->Stop();
               eudaq::mSleep(10);
            }
            if (TLUStarted || JustStopped) {
               //	std::cout << "... " << TLUStarted << " - " << JustStopped << std::endl;
               // eudaq::mSleep(readout_delay);
               while (true) {               //loop until all data from BIF is processed
                  if (!FetchBifDataWasSuccessfull()) continue;
                  auto controller_queue_size = m_tlu->GetEventData()->size(); //number of 64-bit words in the local data vector
                  if (m_tlu->GetEventData()->size()) {
                     ProcessQueuedBifData();
                  } else {
                     eudaq::mSleep(10); //save CPU resources
                     break;
                  }
               }
            } else {
               eudaq::mSleep(50);
            }
            if (JustStopped) {
               // 	m_tlu->Update(timestamps);
//               std::cout << "waiting: " << std::difftime(std::time(NULL), last_readout_time) << std::endl;
               if (std::difftime(std::time(NULL), _last_readout_time) < _WaitAfterStopSeconds) continue; //wait for safe time after last packet arrived
               SendEvent(eudaq::RawDataEvent::EORE("CaliceObject", m_run, ++_ReadoutCycle));
               TLUJustStopped = false;
            }
         } while (!done);
      }


    // This gets called whenever the DAQ is initialised
    virtual void OnInitialise(const eudaq::Configuration & init) {
      try {
	SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "BIF Initisialize " + init.Name() + ")");
      } 
      catch (...) {
        // Message as cout in the terminal of your producer
        std::cout << "Unknown exception" << std::endl;
        // Message to the LogCollector
        EUDAQ_ERROR("Error Message to the LogCollector from caliceahcalbif_Producer");
        // Otherwise, the State is set to ERROR
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
      }
    }

      virtual void OnConfigure(const eudaq::Configuration & param) {
        SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Start Conf");

	if (m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)

         _dumpRaw = param.Get("DumpRawOutput", 0);
         /*DumpRawOutput = 0
          DumpCycleInfo = 1
          DumpTriggerInfo = 1
          DumpSummary = 1*/
         _dumpCycleInfoLevel = param.Get("DumpCycleInfoLevel", 1);
         _dumpTriggerInfoLevel = param.Get("DumpTriggerInfoLevel", 1);
         _dumpSummary = param.Get("DumpSummary", 1);

         _writeRaw = param.Get("WriteRawOutput", 0);
         _rawFilename = param.Get("RawFileName", "run%d.raw");
         _writerawfilename_timestamp = param.Get("WriteRawFileNameTimestamp", 0);
         _firstBxidOffsetBins = param.Get("FirstBxidOffsetBins", 0);
         _bxidLengthNs = param.Get("BxidLengthNs", 4000);
         _WaitAfterStopSeconds = param.Get("WaitAfterStopSeconds", 1.0);

         try {
            std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
            if (m_tlu) m_tlu = nullptr;
            //	int errorhandler = param.Get("ErrorHandler", 2);

            m_tlu = std::unique_ptr<caliceahcalbifController>(new caliceahcalbifController(param.Get("ConnectionFile", "file:///bif_connections.xml"), param.Get("DeviceName", "minitlu_bif")));

            std::cout << "Firmware version " << std::hex << m_tlu->GetFirmwareVersion() << std::dec << std::endl;
            std::cout << "Firmware version " << std::hex << m_tlu->GetFirmwareVersion() << std::dec << std::endl;
            std::cout << "Firmware version " << std::hex << m_tlu->GetFirmwareVersion() << std::dec << std::endl;

            m_tlu->SetCheckConfig(param.Get("CheckConfig", 1));

            readout_delay = param.Get("ReadoutDelay", 100);
//            m_tlu->AllTriggerVeto();
            m_tlu->InitializeI2C(param.Get("I2C_DAC_Addr", 0x1f), param.Get("I2C_ID_Addr", 0x50));
            if (param.Get("UseIntDACValues", 1)) {
               m_tlu->SetDACValue(0, param.Get("DACIntThreshold0", 0x4100));
               m_tlu->SetDACValue(1, param.Get("DACIntThreshold1", 0x4100));
               m_tlu->SetDACValue(2, param.Get("DACIntThreshold2", 0x4100));
               m_tlu->SetDACValue(3, param.Get("DACIntThreshold3", 0x4100));
            } else {
               m_tlu->SetThresholdValue(0, param.Get("DACThreshold0", 1.3));
               m_tlu->SetThresholdValue(1, param.Get("DACThreshold1", 1.3));
               m_tlu->SetThresholdValue(2, param.Get("DACThreshold2", 1.3));
               m_tlu->SetThresholdValue(3, param.Get("DACThreshold3", 1.3));
            }
            if (param.Get("resetClocks", 0)) {
               m_tlu->ResetBoard();
            }
            if (param.Get("resetSerdes", 1)) {
               m_tlu->ResetSerdes();
            }
            m_tlu->ConfigureInternalTriggerInterval(param.Get("InternalTriggerInterval", 42));
            m_tlu->SetTriggerMask(param.Get("TriggerMask", 0x0));
            m_tlu->SetDUTMask(param.Get("DUTMask", 0x1));
            m_tlu->SetEnableRecordData(param.Get("EnableRecordData", 0x0));
            // write DUT mask (not implemented)
            // write DUT style (not implemented)

            // by dhaas
//				eudaq::mSleep(1000);

// 	m_tlu->Update(timestamps);
            std::cout << "...Configured (" << param.Name() << ")" << std::endl;
            EUDAQ_INFO("Configured (" + param.Name() + ")");
	    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");

         } catch (const std::exception & e) {
            printf("Caught exception: %s\n", e.what());
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
         } catch (...) {
            printf("Unknown exception\n");
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
         }
      }

      virtual void OpenRawFile(unsigned param, bool _writerawfilename_timestamp) {

         //	read the local time and save into the string myString
         time_t ltime;
         struct tm *Tm;
         ltime = time(NULL);
         Tm = localtime(&ltime);
         char file_timestamp[25];
         std::string myString;
         if (_writerawfilename_timestamp == 1) {
            sprintf(file_timestamp, "__%02dp%02dp%02d__%02dp%02dp%02d.raw", Tm->tm_mday, Tm->tm_mon + 1, Tm->tm_year + 1900, Tm->tm_hour, Tm->tm_min, Tm->tm_sec);
            myString.assign(file_timestamp, 26);
         } else
            myString = ".raw";

         std::string _rawFilenameTimeStamp;
         //if chosen like this, add the local time to the filename
         _rawFilenameTimeStamp = _rawFilename + myString;
         char _rawFilename[256];
         sprintf(_rawFilename, _rawFilenameTimeStamp.c_str(), (int) param);

         _rawFile.open(_rawFilename);
      }

      virtual void OnStartRun(unsigned param) {
        // SetStatus(eudaq::Status::LVL_OK, "Wait");
         _ReadoutCycle = -1;
         _stats= {0,0,0,0};
         // raw file open
         if (_writeRaw) OpenRawFile(param, _writerawfilename_timestamp);

         if (!m_tlu) {
	   SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "caliceahcalbif connection not present!");

            return;
         }
         try {
            m_run = param;
            std::cout << std::dec << "Start Run: " << param << std::endl;
//            TLUEvent ev(TLUEvent::BORE(m_run));
            eudaq::RawDataEvent infoevent(eudaq::RawDataEvent::BORE("CaliceObject", m_run));
            std::string s = "EUDAQConfigBIF";
            infoevent.AddBlock(0, s.c_str(), s.length());
            infoevent.SetTimeStampToNow();
            infoevent.SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
            infoevent.SetTag("BoardID", to_string(m_tlu->GetBoardID()));
//            ev.SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
//            ev.SetTag("BoardID", to_string(m_tlu->GetBoardID()));
// 	ev.SetTag("ReadoutDelay", to_string(readout_delay));
// 	ev.SetTag("TriggerInterval", to_string(trigger_interval));
// 	ev.SetTag("DutMask", "0x" + to_hex(dut_mask));
// 	ev.SetTag("AndMask", "0x" + to_hex(and_mask));
// 	ev.SetTag("OrMask", "0x" + to_hex(or_mask));
// 	ev.SetTag("VetoMask", "0x" + to_hex(veto_mask));
// 	//      SendEvent(TLUEvent::BORE(m_run).SetTag("Interval",trigger_interval).SetTag("DUT",dut_mask));
// 	ev.SetTag("TimestampZero", to_string(m_tlu->TimestampZero()));
//SendEvent(TLUEvent::BORE(m_run).SetTag("DUTIntf",42));
            eudaq::mSleep(500);               // temporarily, to fix startup with EUDRB
            SendEvent(infoevent);
// 	if (timestamp_per_run)
// 	  m_tlu->ResetTimestamp();
            eudaq::mSleep(500);
// 	m_tlu->ResetTriggerCounter();
// 	if (timestamp_per_run)
// 	  m_tlu->ResetTimestamp();
// 	m_tlu->ResetScalers();
// 	m_tlu->Update(timestamps);
// 	m_tlu->Start();
            try {
               m_tlu->ResetCounters();
               std::cout << "Words in FIFO before start " << m_tlu->GetEventFifoFillLevel() << ". Clearing." << std::endl;
               m_tlu->CheckEventFIFO();
               m_tlu->ReadEventFIFO();
               m_tlu->ClearEventFIFO(); // software side
               m_tlu->ResetEventFIFO(); // hardware side
               m_tlu->NoneTriggerVeto();
            } catch (std::exception& e) {
               std::cout << "startup did not went correct. Problem with BIF communication " << e.what();
               EUDAQ_ERROR("startup did not went correct. BIF data for whole run might miss triggers!");
            }
            TLUStarted = true;

	    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");

         } catch (const std::exception & e) {
            printf("Caught exception: %s\n", e.what());
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error");
         } catch (...) {
            printf("Unknown exception\n");
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Start Error");
         }
 
         
         std::cout << "Done starting" << std::endl;
      }

      virtual void OnStopRun() {
         try {
            std::cout << "Stop Run" << std::endl;
            TLUStarted = false;
            TLUJustStopped = true;
            while (TLUJustStopped) {
               eudaq::mSleep(100);
            }
            uint64_t duration = _stats.runStopTS - _stats.runStartTS;
            if (_stats.runStartTS > _stats.runStopTS) duration = (~duration) + 1; //convert negative
            float trigs_per_roc = ((float) _stats.triggers) / _ReadoutCycle;
            float duration_sec = timestamp_resolution_ns * 1E-9 * (1 + duration << 5);
            float roc_per_sec = ((float) _ReadoutCycle + 1.0) / duration_sec;
            float onTime_sec = timestamp_resolution_ns * 1E-9 * (1 + _stats.onTime << 5);
            float onTime_percent = ((float_t) _stats.onTime) / duration;
            if (_dumpSummary) {
               std::cout << std::dec << std::endl;
               std::cout << "================================================================" << std::endl;
               std::cout << "== Run statistics" << std::endl;
               std::cout << "================================================================" << std::endl;
               std::cout << "Cycles: " << _ReadoutCycle;
//            std::cout << "\tduration: " << duration;
               std::cout << "\tduration: " << duration_sec << " s " << std::endl;
               std::cout << "Ontime: " << onTime_sec << " s ";
               std::cout << "\tontime: " << onTime_percent * 100 << "%" << std::endl;
               std::cout << "avgROCLength: " << 1000. * onTime_sec / (_ReadoutCycle) << " ms ";
               std::cout << "\tavgROC/s: " << roc_per_sec;
               std::cout << std::endl;
               std::cout << "triggers: " << _stats.triggers << " ";
               std::cout << "\tavgTrig/s: " << _stats.triggers / onTime_sec << " ";
               std::cout << "\tavgTrig/roc: " << trigs_per_roc << std::endl;
               std::cout << "================================================================" << std::endl;
            }
            m_tlu->AllTriggerVeto();

	    if (m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
	      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");

         } catch (const std::exception & e) {
            printf("Caught exception: %s\n", e.what());
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
         } catch (...) {
            printf("Unknown exception\n");
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stopp Error");
         }
 
         sleep(1);
         if (_writeRaw) _rawFile.close();

      }

      virtual void OnTerminate() {
         std::cout << "Terminate (press enter)" << std::endl;
         done = true;
//			eudaq::mSleep(1000);
      }

      virtual void OnReset() {
         try {
            std::cout << "Reset" << std::endl;
	    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopp Error");
	    // 	m_tlu->Stop(); // stop
	    // 	m_tlu->Update(false); // empty events
         } catch (const std::exception & e) {
            printf("Caught exception: %s\n", e.what());
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Reset Error");
         } catch (...) {
            printf("Unknown exception\n");
	    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Reset Error");
         }
      }

      virtual void OnStatus() {
         m_connectionstate.SetTag("TRIG", to_string(_stats.triggers)); //to_string(_ReadoutCycle));
//       if (m_tlu) {
               //	m_status.SetTag("TIMESTAMP", );//to_string(Timestamp2Seconds(m_tlu->GetTimestamp())));
               // 	m_status.SetTag("LASTTIME", );//to_string(Timestamp2Seconds(lasttime)));
         m_connectionstate.SetTag("PARTICLES", to_string(_stats.triggers));
// 	m_status.SetTag("STATUS", m_tlu->GetStatusString());
// 	for (int i = 0; i < 4; ++i) {
// 	  m_status.SetTag("SCALER" + to_string(i), to_string(m_tlu->GetScaler(i)));
// 	}
//       }
//std::cout << "Status " << m_status << std::endl;
      }

      virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
         std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
         if (param.length() > 0) std::cout << " (" << param << ")";
         std::cout << std::endl;
	 SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unrecognised command");
      }

   private:
      bool FetchBifDataWasSuccessfull() {
         try {
            m_tlu->CheckEventFIFO();
         } catch (std::exception& e) {
            std::cout << "error when checking eventfifo. " << e.what() << std::endl;
            char errorMessage[200];
            sprintf(errorMessage, "error when checking BIF eventfifo in cycle %d. Recovering", _ReadoutCycle);
            EUDAQ_WARN(errorMessage);
            return false;
         } catch (...) {
            std::cout << "unrecognized error when checking eventfifo. " << std::endl;
            EUDAQ_ERROR("unrecognized error when checking eventfifo");
            return false;
         }
         uint32_t bif_fifo_size = m_tlu->GetNFifo();
         try {
            if (bif_fifo_size) {
               m_tlu->ReadRawFifo(bif_fifo_size);
               _last_readout_time = std::time(NULL);
            } else {
               //std::cout << "~";
               eudaq::mSleep(readout_delay);
            }
         } catch (std::exception& e) {
            std::cout << "error when reading eventfifo. " << e.what() << std::endl;
            char errorMessage[200];
            sprintf(errorMessage, "error when reading BIF eventfifo in cycle %d. Recovering", _ReadoutCycle);
            EUDAQ_ERROR(errorMessage);
            return false;
         } catch (...) {
            std::cout << "unrecognized error when reading eventfifo. " << std::endl;
            EUDAQ_ERROR("unrecognized error when reading eventfifo");
            return false;
         }
         return true;
      }

      void ProcessQueuedBifData() {
         bool event_complete; //previous event in fifo was complete.
         event_complete = true;
         uint64_t word1, word2;

         /* Packet structure from BIF:
          *
          * for shutter on/off only 1 word:
          * ----------------------------------------------------------------
          * | 4b evttype | 12b counter| 48b Timestamp (25ns)               |
          * ----------------------------------------------------------------
          *
          * For triggers:
          * word1 (64 bits)
          * ----------------------------------------------------------------
          * | 4b evttype | 12b itrig | 48b Timestamp (25ns)                |
          * ----------------------------------------------------------------
          * word2 (64 bits)
          * ----------------------------------------------------------------
          * | 8b SC0 | 8b SC1 | 8b SC2 | 8b SC3 | 32b evtnumber            |
          * ----------------------------------------------------------------
          *
          * evttype: 0=internal trigger, 1=external trigger, 2=shutter off, 3=shutter on
          * SC0..3 are fine timestamps (5 bits), but shifted by value 8
          * */

         for (auto it = m_tlu->GetEventData()->begin(); it != m_tlu->GetEventData()->end(); it++) { //iterate the local buffer
            event_complete = false;
            word1 = *(it);
            uint64_t timestamp = word1 & 0x0000FFFFFFFFFFFF; //take only first 48 bits
            uint16_t inputs = (word1 >> 48) & 0x0fff;
            uint32_t evtNumber;
            uint8_t fineStamps[4] = { 0, 0, 0, 0 };
            uint32_t cycleLengthBxids;
            switch (word1 >> 60) {
               case 0x00: //internal trigger
                  _triggersInCycle++;
//                  std::cout << "i";
                  break;
               case 0x01: //external trigger
                  _triggersInCycle++;
//                  std::cout << ".";
                  break;
               case 0x03: //shutter on
                  if (_ROC_started) {
                     std::cout << std::dec << "start of readout cycle received without previous stop. Readout cycle: " << _ReadoutCycle << std::endl;
                     char errorMessage[200];
                     sprintf(errorMessage, "Another BIF Start-of-Readout-Cycle received without previous stop in Run %d, ROC %d.", m_run, _ReadoutCycle);
                     EUDAQ_WARN(errorMessage);
                     unsigned int arrived_cycle_modulo = ((inputs - _firstStutterCycle) & 0x0FFF);
                     unsigned int expected_cycle_modulo = ((_ReadoutCycle + 1) & 0x0FFF);
                     uint16_t difference = (arrived_cycle_modulo - expected_cycle_modulo) & 0x0FFF;
                     if (difference == 0) { //If the Readout cycle logically continues in serie
                        std::cout << "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run " << m_run << ", ROC " << _ReadoutCycle << std::endl;
                        char errorMessage[200];
                        sprintf(errorMessage, "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run %d, ROC %d.", m_run, _ReadoutCycle);
                        EUDAQ_WARN(errorMessage);
                        WriteOutEudaqEvent();
                     } else if ((difference >= 1) && (difference <= 500)) { //Don't compensate too big event loss
                        WriteOutEudaqEvent();
                     } else if (difference == 0x0FFF) {
                        _ReadoutCycle--; //probably receiving same readoutcycledata twice. Lets fix it only if it is the same packet,
                     }
                  }
                  _ROC_started = true;
                  _triggersInCycle = 0;
                  _acq_start_ts = timestamp;
                  //                              std::cout << "<";
                  _acq_start_cycle = inputs;
                  if (_ReadoutCycle == -1) {
                     _firstTrigger = true;
                     _firstStutterCycle = _acq_start_cycle;
                     _stats.runStartTS = _acq_start_ts;
                     _ReadoutCycle = 0;
                  } else {
                     _ReadoutCycle++;
                  }
                  if (((_acq_start_cycle - _firstStutterCycle) & 0x0FFF) != (_ReadoutCycle & 0x0FFF)) {
                     unsigned int arrived_cycle_modulo = ((_acq_start_cycle - _firstStutterCycle) & 0x0FFF);
                     unsigned int expected_cycle_modulo = (_ReadoutCycle & 0x0FFF);
                     std::cout << std::hex << "BIF cycle not in sequence! _ReadoutCycle: " << _ReadoutCycle << "\t expected_cycle_modulo:" << expected_cycle_modulo;
                     std::cout << "\tarrived modulo: " << arrived_cycle_modulo << std::dec << std::endl;
                     char errorMessage[200];
                     sprintf(errorMessage, "BIF Cycle not in sequence in Run %d! expected: %04X arrived: %04X. Possible data loss", m_run, expected_cycle_modulo, arrived_cycle_modulo);
                     EUDAQ_WARN(errorMessage);
                     uint16_t difference = (arrived_cycle_modulo - expected_cycle_modulo) & 0x0FFF;
                     //                                 if (difference==0x0FFF) differnece=-1;//differnece is too big for correction
                     while (difference) { //write out anything (or empty events) if the data is coming from wrong readout cycle
                        std::cout << "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run " << m_run << ", ROC " << _ReadoutCycle << std::endl;
                        char errorMessage[200];
                        sprintf(errorMessage, "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run %d, ROC %d.", m_run, _ReadoutCycle);
                        EUDAQ_WARN(errorMessage);
                        WriteOutEudaqEvent();
                        _ReadoutCycle++;
                        difference--;
                     }
                     //_ReadoutCycle += difference;
                  }
                  event_complete = true;
                  trigger_push_back(_cycleData, 0x03000000, (uint32_t) _ReadoutCycle, _acq_start_ts << 5);
                  //trigger_push_back(_cycleData, 0x03000000, _acq_start_cycle, _acq_start_ts << 5);
                  break;
               case 0x02: //shutter off
                  //                              std::cout << ">" << std::endl;
                  if (!_ROC_started) {
                     std::cout << "end of readout cycle received without previous start. Readout cycle: " << _ReadoutCycle << std::endl;
                     char errorMessage[200];
                     sprintf(errorMessage, "BIF End-of-Readout-Cycle received without previous start in Run %d ROC %d.Possible data loss", m_run, _ReadoutCycle);
                     EUDAQ_WARN(errorMessage);
                  }
                  _ROC_started = false;
                  _stats.triggers += _triggersInCycle;
                  _stats.runStopTS = timestamp;
                  if (timestamp < _acq_start_ts) {
                     _stats.onTime += ~(timestamp - _acq_start_ts) + 1;                  //convert unsigned negative
                  } else {
                     _stats.onTime += timestamp - _acq_start_ts;
                  }

                  if (_acq_start_cycle != inputs) {
                     char errorMessage[200];
                     sprintf(errorMessage, "BIF Cycle end mismatch in run %d! start: %04X stop:%04X. Possible data loss", m_run, _acq_start_cycle, inputs);
                     EUDAQ_WARN(errorMessage);
                  }
                  cycleLengthBxids = (timestamp_resolution_ns * ((timestamp - _acq_start_ts) << 5) - _firstBxidOffsetBins) / _bxidLengthNs;
                  switch (_dumpCycleInfoLevel) {
                     case 2:
                        std::cout << std::dec << "ROC: " << (int) _ReadoutCycle;
                        std::cout << "\tLength[BXings]: " << (int) cycleLengthBxids;
                        std::cout << "\tTrigs: " << (int) _triggersInCycle;
                        std::cout << "\tlen: " << 32 * (0.00000078125) * (timestamp - _acq_start_ts) << " ms";
                        std::cout << std::endl;
                        break;
                     case 1:
                        std::cout << "@" << std::flush;
                        break;
                     default:
                        break;
                  }
                  //                              std::cout << "\tcycle: " << (int) acq_start_cycle
                  event_complete = true;
                  {
                     trigger_push_back(_cycleData, 0x02000000, (uint32_t) _ReadoutCycle, _acq_start_ts << 5);
                     //trigger_push_back(_cycleData, 0x02000000, inputs, _acq_start_ts << 5);
                     WriteOutEudaqEvent();
                  }
                  break;
               case 0x04: //edge falling
               case 0x05: //edge rising
               case 0x06: //spill off
               case 0x07: //spill on
                  EUDAQ_ERROR("wrong TLU packet received. Data might be corrupted");
                  event_complete = true;
                  break;
               default:
                  break;
            }
            if (_writeRaw) _rawFile.write((char*) &word1, sizeof(word1));
            //                      printf("raw64: %016lX\n", word1);
            if (event_complete) continue;
            it++; //treat next word as 2nd word of the packet
            if (it == m_tlu->GetEventData()->end()) break;
            word2 = *(it);
            evtNumber = word2 & 0x00000000FFFFFFFF;
            if (evtNumber != ++_trigger_id) {
               _trigger_id = evtNumber;
               if (_firstTrigger) {
                  _firstTrigger = false;
               } else {
                  std::cout << "trigger not in sequence in ROC " << _ReadoutCycle << std::endl;
                  char errorMessage[200];
                  sprintf(errorMessage, "BIF trigger not in sequence in ROC %d. Possible data loss", _ReadoutCycle);
                  EUDAQ_WARN(errorMessage);
               }
            }
            fineStamps[0] = ((word2 >> 56) + 0x18) & 0x1F;
            fineStamps[1] = ((word2 >> 48) + 0x18) & 0x1F;
            fineStamps[2] = ((word2 >> 40) + 0x18) & 0x1F;
            fineStamps[3] = ((word2 >> 32) + 0x18) & 0x1F;
            switch (word1 >> 60) {
               case 0x00: //internal trigger
                  switch (_dumpTriggerInfoLevel) {
                     case 2:
                        std::cout << std::hex << "trigger TS: " << (timestamp << 5) << "\tinputs: " << inputs;
                        std::cout << std::dec << "\teventNo: " << (word2 & 0x00000000FFFFFFFF) << std::endl;
                        std::cout << std::hex << "ts0: " << (uint32_t) fineStamps[0];
                        std::cout << std::hex << "\tts1: " << (uint32_t) fineStamps[1];
                        std::cout << std::hex << "\tts2: " << (uint32_t) fineStamps[2];
                        std::cout << std::hex << "\tts3: " << (uint32_t) fineStamps[3];
                        std::cout << std::dec << std::endl;
                        break;
                     case 1:
                        std::cout << "." << std::flush;
                        break;
                     default:
                        break;
                  }
                  trigger_push_back(_cycleData, 0, (uint32_t) evtNumber, (uint32_t) ((timestamp << 5) | fineStamps[0]));
                  break;
               case 0x01: //external trigger
                  for (uint16_t input = 0; input < 4; ++input) {
                     if ((inputs >> 8) & (1 << input)) {
                        switch (_dumpTriggerInfoLevel) {
                           case 2:
                              std::cout << "Trigger from input: " << input << "\tTS: ";
                              std::cout << std::hex << ((timestamp << 5) | fineStamps[input]);
                              std::cout << std::hex << "\tfineTS: " << (uint32_t) fineStamps[input];
                              std::cout << std::dec << "\teventNo: " << (word2 & 0x00000000FFFFFFFF);
                              std::cout << std::dec << std::endl;
                              break;
                           case 1:
                              std::cout << "," << std::flush;
                              break;
                           default:
                              break;
                        }
                        uint32_t type = 0x01000000 | (input << 16);
                        uint64_t Timestamp = (timestamp << 5) | fineStamps[input];
                        trigger_push_back(_cycleData, type, evtNumber, Timestamp);
                     }
                  }
                  break;
               default:
                  break;
            }

            if (_writeRaw) _rawFile.write((char*) &word2, sizeof(word2));
            event_complete = true;
         } //for eventdata
         m_tlu->ClearEventFIFO();
         if (!event_complete) m_tlu->GetEventData()->push_back(word1); //data in the fifo ends in the middle of the packet. Let's put back the word1. It will be completed in the next iteration
      }

      void trigger_push_back(std::vector<uint32_t> &cycleData, const uint32_t type, const uint32_t evtNumber, const uint64_t Timestamp) {
         cycleData.push_back(type);
         cycleData.push_back((uint32_t) (evtNumber));
         cycleData.push_back((uint32_t) (Timestamp));
         cycleData.push_back((uint32_t) ((Timestamp >> 32)));
      }

      void WriteOutEudaqEvent()
      {
         eudaq::RawDataEvent CycleEvent("CaliceObject", m_run, _ReadoutCycle);
         //                     CycleEvent.SetTag("PARTICLES", eudaq::to_string(_triggersInCycle));
         //                     CycleEvent.SetTag("TRIG", eudaq::to_string(_triggersInCycle));
         std::string s = "EUDAQDataBIF";
         CycleEvent.AddBlock(0, s.c_str(), s.length());
         s = "i:Type,i:EventCnt,i:TS_Low,i:TS_High";
         CycleEvent.AddBlock(1, s.c_str(), s.length());
         unsigned int times[2];
         struct timeval tv;
         ::gettimeofday(&tv, NULL);
         times[0] = tv.tv_sec;
         times[1] = tv.tv_usec;
         CycleEvent.AddBlock(2, times, sizeof(times));
         CycleEvent.AddBlock(3);
         CycleEvent.AddBlock(4);
         CycleEvent.AddBlock(5);
         CycleEvent.AddBlock(6, _cycleData);
         eudaq::DataSender::SendEvent(CycleEvent);
         _cycleData.clear();
      }

      unsigned m_run;
      //      unsigned m_ev;
//      unsigned trigger_interval;
//      unsigned dut_mask;
//      unsigned veto_mask;
//      unsigned and_mask;
//      unsigned or_mask;
//      uint32_t strobe_period, strobe_width;
//      unsigned enable_dut_veto;
//      unsigned trig_rollover;
      unsigned readout_delay;
      //      bool timestamps;
//      bool timestamp_per_run;
      bool done;
      bool TLUStarted;
      bool TLUJustStopped;
      std::unique_ptr<caliceahcalbifController> m_tlu;

// debug output
      double _WaitAfterStopSeconds;
      bool _dumpRaw; //print events to screen
      int _dumpCycleInfoLevel; // print Readout cycle summary
      int _dumpTriggerInfoLevel; // print detailed info about each trigger
      bool _dumpSummary; // print summary after the end of run
      bool _writeRaw; //write to separate file
      std::string _rawFilename;
      bool _writerawfilename_timestamp; //whether to append timestamp to the raw file
      int _ReadoutCycle; //current ReadoutCycle in the run
      uint64_t _acq_start_ts; //timestamp of last start acquisition (shutter on)
      uint32_t _acq_start_cycle; //raw cycle number of the acq start. Direct copy from the packet (no correction for starting the run from cycle 0
      uint32_t _trigger_id; //last trigger raw event number

      std::vector<uint32_t> _cycleData; //data, that will be sent to eudaq rawEvent
      bool _ROC_started; //true when start acquisition was preceeding
      unsigned int _triggersInCycle;
      int _firstStutterCycle; //first shutter cycle, which is defined as cycle0
      bool _firstTrigger; //true if the next trigger will be the first in the cycle
      uint64_t _firstBxidOffsetBins; //when the bxid0 in AHCAL starts. In 0.78125 ns tics.
      uint32_t _bxidLengthNs; //How long is the BxID in AHCAL. Typically 4000 ns in TB mode
      const double timestamp_resolution_ns = 0.78125;
      struct {
            uint64_t runStartTS;
            uint64_t runStopTS;
            uint64_t onTime; //sum of all acquisition cycle lengths
            uint32_t triggers; //total count of triggers
      } _stats;

      std::time_t _last_readout_time; //last time when there was any data from BIF

      std::ofstream _rawFile;
}
;

int main(int /*argc*/, const char ** argv) {
   eudaq::OptionParser op("EUDAQ AHCAL BIF Producer", "1.0", "The Producer task for the BIF (based on mini-tlu)");
   eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
   eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
   eudaq::Option<std::string> op_trace(op, "t", "tracefile", "", "filename", "Log file for tracing USB access");
   try {
      op.Parse(argv);
      EUDAQ_LOG_LEVEL(level.Value());
      //if (op_trace.Value() != "") {
      //	setusbtracefile(op_trace.Value());
      //}
      caliceahcalbifProducer producer(rctrl.Value());
      producer.MainLoop();
      std::cout << "Quitting" << std::endl;
      eudaq::mSleep(300);
   } catch (...) {
      return op.HandleMainException();
   }
   return 0;
}

