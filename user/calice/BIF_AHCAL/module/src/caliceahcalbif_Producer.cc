#include "eudaq/Producer.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "tlu/caliceahcalbifController.hh"
#include <iostream>
#include <ostream>
#include <vector>
//#include <memory> //unique_ptr

using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class caliceahcalbifProducer: public eudaq::Producer {
   public:
      caliceahcalbifProducer(const std::string name, const std::string &runcontrol);
      //      void OnStatus() override final;
      //void OnUnrecognised(const std::string & cmd, const std::string & param) override;

      void DoInitialise() override final;
      void DoConfigure() override final;
      void DoStartRun() override final;
      void DoStopRun() override final;
      void DoTerminate() override final;
      void DoReset() override final;
      void RunLoop() override final;
      //void Exec() override final;

      void MainLoop();
      void OpenRawFile(unsigned param, bool _writerawfilename_timestamp);

      static const uint32_t m_id_factory = eudaq::cstr2hash("caliceahcalbifProducer");
      private:
      bool FetchBifDataWasSuccessfull();
      void ProcessQueuedBifData();
      void trigger_push_back(std::vector<uint32_t> &cycleData, const uint32_t type,
            const uint32_t evtNumber, const uint64_t Timestamp);
      void buildEudaqEvents(std::deque<eudaq::EventUP> & deqEvent);
      void sendallevents(std::deque<eudaq::EventUP> & deqEvent, int minimumsize);
      void buildEudaqEventsROC(std::deque<eudaq::EventUP>& deqEvent);
      void buildEudaqEventsTriggers(std::deque<eudaq::EventUP>& deqEvent);
      void buildEudaqEventsBxid(std::deque<eudaq::EventUP>& deqEvent);
      enum class EventBuildingMode {
         ROC, TRIGGERS, BXID
      };
      enum class EventNumbering {
         TRIGGERNUMBER, TIMESTAMP
      };

      caliceahcalbifProducer::EventBuildingMode _eventBuildingMode;
      caliceahcalbifProducer::EventNumbering _eventNumberingPreference;

      unsigned m_run;
      unsigned readout_delay;
      bool done;
      bool _TLUStarted;
      bool _TLUJustStopped;
      std::unique_ptr<caliceahcalbifController> m_tlu;
      bool _BORESent;

      // debug output
      double _WaitAfterStopSeconds;
      bool _dumpRaw; //print events to screen
      int _dumpCycleInfoLevel; // print Readout cycle summary
      int _dumpTriggerInfoLevel; // print detailed info about each trigger
      bool _dumpSummary; // print summary after the end of run
      bool _writeRaw; //write to separate file
      std::string _rawFilename;
      bool _writerawfilename_timestamp; //whether to append timestamp to the raw file
      bool _word1WrittenPreviously; //word was already stored to raw file in previous processing
      int _ReadoutCycle; //current ReadoutCycle in the run
      int _countRocFrom; //from which ReadOutCycle number should be counted. By default (_countRocFrom==0)the first ROC=0. This number will be added to ReadOutCycles_counted_from_0 and will make the ROC in the data
      uint64_t _lastTriggerTime; //Timestamp of last trigger. Used for filtering double trigger event
      int _consecutiveTriggerIgnorePeriod;
      uint64_t _acq_start_ts; //timestamp of last start acquisition (shutter on)
      uint64_t _acq_stop_ts; // timestamp of the stop of the acquisition
      uint32_t _acq_start_cycle; //raw cycle number of the acq start. Direct copy from the packet (no correction for starting the run from cycle 0
      uint32_t _trigger_id; //last trigger raw event number

      std::vector<uint32_t> _cycleData; //data, that will be sent to eudaq rawEvent
      bool _ROC_started; //true when start acquisition was preceeding
      unsigned int _triggersInCycle;
      int _firstStutterCycle; //first shutter cycle, which is defined as cycle0
      bool _firstTrigger; //true if the next trigger will be the first in the cycle
      int _firstTriggerNumber;
      uint64_t _firstBxidOffsetBins; //when the bxid0 in AHCAL starts. In 0.78125 ns tics.
      uint32_t _bxidLengthNs; //How long is the BxID in AHCAL. Typically 4000 ns in TB mode
      int _bxidLengthBins;
      const double timestamp_resolution_ns = 0.78125;
      struct {
            uint64_t runStartTS;
            uint64_t runStopTS;
            uint64_t onTime; //sum of all acquisition cycle lengths
            uint32_t triggers; //total count of triggers
      } _stats;
      std::time_t _last_readout_time; //last time when there was any data from BIF
      std::ofstream _rawFile;

      std::string _redirectedInputFileName; //file name, which will be used to receive data from (instead of real minitlu)
      std::ifstream _ifs;
      std::vector<uint64_t> _redirectedInputData;

      std::deque<eudaq::EventUP> _deqEvent; //queue of the EUDAQ raw events
      std::mutex _deqEventAccessMutex; //lock for accessing the _deqEvent
};

namespace {
   auto dummy0 = eudaq::Factory<eudaq::Producer>::
         Register<caliceahcalbifProducer, const std::string&, const std::string&>(caliceahcalbifProducer::m_id_factory);
}

caliceahcalbifProducer::caliceahcalbifProducer(const std::string name, const std::string &runcontrol) :
      eudaq::Producer(name, runcontrol), m_tlu(nullptr), m_run(-1), readout_delay(100),
            _TLUJustStopped(false), _TLUStarted(false), done(false), _acq_start_cycle(0), _acq_start_ts(0),
            _bxidLengthNs(4000), _dumpRaw(0), _firstBxidOffsetBins(13000), _firstStutterCycle(-1),
            _firstTrigger(true), _firstTriggerNumber(0), _last_readout_time(std::time(NULL)),
            _ReadoutCycle(0), _ROC_started(false), _consecutiveTriggerIgnorePeriod(0), _lastTriggerTime(0LLU),
            _trigger_id(-1), _triggersInCycle(0), _WaitAfterStopSeconds(0), _writeRaw(false),
            _writerawfilename_timestamp(true), _dumpCycleInfoLevel(1), _dumpSummary(true), _dumpTriggerInfoLevel(0),
            _BORESent(false), _word1WrittenPreviously(false), _eventBuildingMode(caliceahcalbifProducer::EventBuildingMode::ROC),
            _eventNumberingPreference(caliceahcalbifProducer::EventNumbering::TIMESTAMP) {

}

//void caliceahcalbifProducer::Exec() {
//   StartCommandReceiver();
//   while (IsActiveCommandReceiver()) {
//      MainLoop();
//      std::this_thread::sleep_for(std::chrono::milliseconds(500));
//   }
//}

void caliceahcalbifProducer::RunLoop() {
   std::cout << "Main loop!" << std::endl;
   _last_readout_time = std::time(NULL);
   //         eudaq::RawDataEvent CycleEvent; //
   _firstTrigger = true;
   bool justStopped = false;
   bool done = false;
   do {
      if ((!m_tlu) && (!_redirectedInputFileName.length())) {
         EUDAQ_ERROR_STREAMOUT("No connection to TLU or file", std::cout, std::cerr);
         eudaq::mSleep(500);
         continue;
      }
      justStopped = _TLUJustStopped; //copy  this variable, so that it doesnt change within the main loop
      if (_TLUStarted || justStopped) {
         if (_redirectedInputFileName.length()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000)); //otherwise we might be too early with data sending, even before ahcal connects to the datacollector
         }
         while (true) {               //loop until all data from BIF is processed
            bool FetchResult = FetchBifDataWasSuccessfull();
            auto controller_queue_size = _redirectedInputFileName.length() ? _redirectedInputData.size() : m_tlu->GetEventData()->size(); //number of 64-bit words in the local data vector
            if (controller_queue_size) {
               //std::cout << "size: " << controller_queue_size << std::endl;//DEBUG
               ProcessQueuedBifData();
            } else {
               eudaq::mSleep(10); //save CPU resources
               break;
            }
            sendallevents(_deqEvent, 1);
            SetStatusTag("TRIG", to_string(_stats.triggers)); //to_string(_ReadoutCycle));
            SetStatusTag("ROC", to_string(_ReadoutCycle)); //to_string(_ReadoutCycle));
         }
      } else {
         eudaq::mSleep(50);
      }
      if (justStopped) {
         if (std::difftime(std::time(NULL), _last_readout_time) < _WaitAfterStopSeconds) continue; //wait for safe time after last packet arrived
         sendallevents(_deqEvent, 0);
         _ReadoutCycle++;
         // SendEvent(eudaq::RawDataEvent::EORE("CaliceObject", m_run, ++_ReadoutCycle));
         _TLUJustStopped = false;
         done = true;
      }
   } while (!done);
}

void caliceahcalbifProducer::DoInitialise() {
   //nothing to do for BIF.
}

void caliceahcalbifProducer::DoConfigure() {
   const eudaq::Configuration &param = *GetConfiguration();
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
   _bxidLengthBins = static_cast<int>(((double) _bxidLengthNs) / timestamp_resolution_ns);
   std::cout << "DEBUG bxid length in bins: " << _bxidLengthBins << std::endl;
   _WaitAfterStopSeconds = param.Get("WaitAfterStopSeconds", 1.0);
   _redirectedInputFileName = param.Get("RedirectInputFromFile", "");

   std::string eventBuildingMode = param.Get("EventBuildingMode", "ROC");
   if (!eventBuildingMode.compare("ROC")) _eventBuildingMode = caliceahcalbifProducer::EventBuildingMode::ROC;
   if (!eventBuildingMode.compare("TRIGGERS")) _eventBuildingMode = caliceahcalbifProducer::EventBuildingMode::TRIGGERS;
   if (!eventBuildingMode.compare("BXID")) _eventBuildingMode = caliceahcalbifProducer::EventBuildingMode::BXID;
   std::cout << "Creating events in \"" << eventBuildingMode << "\" mode" << std::endl;

   std::string eventNumberingMode = param.Get("EventNumberingPreference", "TRIGGERID");
   if (!eventNumberingMode.compare("TRIGGERNUMBER")) _eventNumberingPreference = caliceahcalbifProducer::EventNumbering::TRIGGERNUMBER;
   if (!eventNumberingMode.compare("TIMESTAMP")) _eventNumberingPreference = caliceahcalbifProducer::EventNumbering::TIMESTAMP;
   std::cout << "Preferring event numbering type: \"" << eventNumberingMode << "\"" << std::endl;
   _consecutiveTriggerIgnorePeriod = param.Get("ConsecutiveTriggerIgnorePeriod", 0);
   _countRocFrom = param.Get("CountRocFrom", 0);
   std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
   if (m_tlu) m_tlu = nullptr;
   if (!_redirectedInputFileName.length()) {
      //	int errorhandler = param.Get("ErrorHandler", 2);

      m_tlu = std::unique_ptr<caliceahcalbifController>(
            new caliceahcalbifController(param.Get("ConnectionFile", "file:///bif_connections.xml"), param.Get("DeviceName", "minitlu_bif")));

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
   } else {
      _redirectedInputData.reserve(1024);
   }
   std::cout << "...Configured (" << param.Name() << ")" << std::endl;
}

void caliceahcalbifProducer::OpenRawFile(unsigned param, bool _writerawfilename_timestamp) {
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

void caliceahcalbifProducer::DoStartRun() {
   std::cout << "Starting Run" << std::endl;
   _deqEvent.clear();
   _ReadoutCycle = -1;
   _stats= {0,0,0,0};
   _BORESent = false;
   _firstTriggerNumber = 0;
   // raw file open
   if (_writeRaw) OpenRawFile(GetRunNumber(), _writerawfilename_timestamp);

   if ((!m_tlu) && (!_redirectedInputFileName.length())) {
      std::cout << "caliceahcalbif connection not present!" << std::endl;
      EUDAQ_ERROR("caliceahcalbif connection not present!");
      return;
   }
   m_run = GetRunNumber();
   std::cout << std::dec << "Start Run: " << m_run << std::endl;
   // eudaq::RawDataEvent infoevent(eudaq::RawDataEvent::BORE("CaliceObject", m_run));
   if (_redirectedInputFileName.length()) {
      _ifs.open(_redirectedInputFileName, std::ifstream::in | std::ifstream::binary);
      std::cout << "file " << _redirectedInputFileName << " open with result " << _ifs.rdstate() << std::endl;
   } else {
      //auto ev = eudaq::RawDataEvent::MakeUnique("CaliceObject");
      //ev->SetBORE();
      //std::string s = "EUDAQConfigBIF";
      //auto infoevent = dynamic_cast<eudaq::RawDataEvent*>(ev.get());
      //infoevent->AddBlock(0, s.c_str(), s.length());
      //// infoevent->SetTimeStampToNow();
      //infoevent->SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
      //infoevent->SetTag("BoardID", to_string(m_tlu->GetBoardID()));
      //eudaq::mSleep(500);               // temporarily, to fix startup with EUDRB
      //SendEvent(std::move(ev));
      //eudaq::mSleep(500);
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
   }
   _TLUStarted = true;
   std::cout << "Starting Run DoStartRun finished" << std::endl;

}

void caliceahcalbifProducer::DoStopRun() {
   std::cout << "Stop Run" << std::endl;
   _TLUStarted = false;
   _TLUJustStopped = true;
   while (_TLUJustStopped) {
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
      std::cout << "\tavgTrig/s: " << _stats.triggers / duration_sec << " ";
      std::cout << "\tavgTrig/roc: " << trigs_per_roc << std::endl;
      std::cout << "================================================================" << std::endl;
   }
   if (!_redirectedInputFileName.length()) {
      m_tlu->AllTriggerVeto();
   } else {
      _ifs.close();
   }
   sleep(1);
   if (_writeRaw) _rawFile.close();
   std::cout << "Run stopped" << std::endl;
}

void caliceahcalbifProducer::DoTerminate() {
   std::cout << "Terminate (press enter)" << std::endl;
   done = true;
}

void caliceahcalbifProducer::DoReset() {
}

//void caliceahcalbifProducer::OnStatus() {
//   SetStatusTag("TRIG", to_string(_stats.triggers)); //to_string(_ReadoutCycle));
//   //SetStatusTag("PARTICLES", to_string(_stats.triggers));
//   setStatusTag("EventN",)
//}

//void caliceahcalbifProducer::OnUnrecognised(const std::string & cmd, const std::string & param) {
//   std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
//   if (param.length() > 0) std::cout << " (" << param << ")";
//   std::cout << std::endl;
//   EUDAQ_WARN("Unrecognised command");
//}

bool caliceahcalbifProducer::FetchBifDataWasSuccessfull() {
   if (_redirectedInputFileName.length()) { //reading from file instead of bif
      uint64_t dword;
      bool readResult = false;
      if (_ifs.eof()) return false;
      while (_redirectedInputData.size() < 1024) {
         _ifs.read(reinterpret_cast<char*>(&dword), sizeof(dword)); // >> dword;
         //std::cout << "<"<< _ifs.gcount() << ";" << to_hex(dword, 16) << ">"<< std::flush;
         if (_ifs.good()) {
            readResult = true;
         } else {
            std::cout << "Bad result of read (End of file?):" << _ifs.rdstate() << std::endl;
            break;
         };
         _redirectedInputData.push_back(dword);
      }
      return readResult;
   } else {         //communicate with BIF
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
}

void caliceahcalbifProducer::ProcessQueuedBifData() {
   //std::cout << "DEBUG: processing data" << std::endl;
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

//for (auto it = m_tlu->GetEventData()->begin(); it != m_tlu->GetEventData()->end(); it++) { //iterate the local buffer
   for (auto it = _redirectedInputFileName.length() ? _redirectedInputData.begin() : m_tlu->GetEventData()->begin();
         it != (_redirectedInputFileName.length() ? _redirectedInputData.end() : m_tlu->GetEventData()->end());
         it++) { //iterate the local buffer
      //std::cout<<"^"<<std::flush;//DEBUG
      event_complete = false;
      word1 = *(it);
      uint64_t timestamp = word1 & 0x0000FFFFFFFFFFFF; //take only first 48 bits
      uint16_t inputs = (word1 >> 48) & 0x0fff; //either input mas or shutter cycle counter
      uint32_t evtNumber; //trigger number
      uint8_t fineStamps[4] = { 0, 0, 0, 0 };
      uint32_t cycleLengthBxids;
      switch (word1 >> 60) {
         case 0x00: //internal trigger
//                  std::cout << "i";
            break;
         case 0x01: //external trigger
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
                  buildEudaqEvents(_deqEvent);
               } else if ((difference >= 1) && (difference <= 500)) { //Don't compensate too big event loss
                  buildEudaqEvents(_deqEvent);
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
               sprintf(errorMessage, "BIF Cycle not in sequence in Run %d! expected: %04X arrived: %04X. Possible data loss", m_run, expected_cycle_modulo,
                     arrived_cycle_modulo);
               EUDAQ_WARN(errorMessage);
               uint16_t difference = (arrived_cycle_modulo - expected_cycle_modulo) & 0x0FFF;
               //                                 if (difference==0x0FFF) differnece=-1;//differnece is too big for correction
               while (difference) { //write out anything (or empty events) if the data is coming from wrong readout cycle
                  std::cout << "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run " << m_run << ", ROC " << _ReadoutCycle << std::endl;
                  char errorMessage[200];
                  sprintf(errorMessage, "Writing dummy/incomplete BIF cycles for missing readoutcycle data. Run %d, ROC %d.", m_run, _ReadoutCycle);
                  EUDAQ_WARN(errorMessage);
                  buildEudaqEvents(_deqEvent);
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
            _acq_stop_ts = timestamp;
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
               case 3:
                  std::cout << std::dec << (int) _ReadoutCycle << " " << _acq_start_ts << " " << " 1 " << (timestamp - _acq_start_ts) << " #start" << std::endl;
                  std::cout << std::dec << (int) _ReadoutCycle << " " << timestamp << " " << " 0 " << (timestamp - _acq_start_ts) << " #stop" << std::endl;
                  break;
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
               trigger_push_back(_cycleData, 0x02000000, static_cast<uint32_t>(_ReadoutCycle), timestamp << 5);
               //trigger_push_back(_cycleData, 0x02000000, (uint32_t) _ReadoutCycle, _acq_start_ts << 5);
               //trigger_push_back(_cycleData, 0x02000000, inputs, _acq_start_ts << 5);
               buildEudaqEvents(_deqEvent);
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
      if (_writeRaw && (!_word1WrittenPreviously)) {
         _rawFile.write((char*) &word1, sizeof(word1));
      }
      _word1WrittenPreviously = false;
      //                      printf("raw64: %016lX\n", word1);
      if (event_complete) continue;
      it++; //treat next word as 2nd word of the packet
      if (it == (_redirectedInputFileName.length() ? _redirectedInputData.end() : m_tlu->GetEventData()->end())) break; //incomplete event
      word2 = *(it);
      evtNumber = word2 & 0x00000000FFFFFFFF;
      if (evtNumber != ++_trigger_id) {
         _trigger_id = evtNumber;
         if (_firstTrigger) {
            _firstTrigger = false;
            _firstTriggerNumber = evtNumber;
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
            _triggersInCycle++;

            switch (_dumpTriggerInfoLevel) {
               case 2:
                  std::cout << std::hex << "Internal trigger TS: " << (timestamp << 5) << "\tinputs: " << inputs;
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
            trigger_push_back(_cycleData, 0x01000000, (uint32_t) evtNumber, (uint32_t) ((timestamp << 5) | fineStamps[0]));
            break;
         case 0x01: //external trigger
            if ((timestamp << 5) - _lastTriggerTime > _consecutiveTriggerIgnorePeriod) {
               _triggersInCycle++;
               _lastTriggerTime = (timestamp << 5);
               for (int input = 3; input >= 0; --input) {
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
                        case 3:
                           std::cout << input << std::flush;
                           break;
                        default:
                           break;
                     }
                     uint32_t type = 0x01000000 | (input << 16);
                     uint64_t Timestamp = (timestamp << 5) | fineStamps[input];
                     trigger_push_back(_cycleData, type, evtNumber, Timestamp);
                  }
               }
            } else {
               //std::cout << "Trigger too soon after the previous trigger, eventNr=" << evtNumber << ", Difference" << ((timestamp << 5) - _lastTriggerTime) << std::endl;
            }

            break;
         default:
            break;
      }

      if (_writeRaw) _rawFile.write((char*) &word2, sizeof(word2));
      event_complete = true;
   } //for eventdata
   if (_redirectedInputFileName.length()) {
      _redirectedInputData.clear();
      if (!event_complete) _redirectedInputData.push_back(word1); //data in the fifo ends in the middle of the packet. Let's put back the word1. It will be completed in the next iteration
   } else {
      m_tlu->ClearEventFIFO();
      if (!event_complete) m_tlu->GetEventData()->push_back(word1); //data in the fifo ends in the middle of the packet. Let's put back the word1. It will be completed in the next iteration
   }
   if (!event_complete) {
      _word1WrittenPreviously = true;
   }
}

void caliceahcalbifProducer::trigger_push_back(std::vector<uint32_t> &cycleData, const uint32_t type, const uint32_t evtNumber, const uint64_t Timestamp) {
   //std::cout << "storing " << to_hex(type, 8) << " " << static_cast<uint32_t>(evtNumber - _firstTriggerNumber) << " " << to_hex(Timestamp, 16) << " " << _stats.triggers << std::endl;
   cycleData.push_back(type);
   cycleData.push_back((uint32_t) (evtNumber));
   cycleData.push_back((uint32_t) (Timestamp));
   cycleData.push_back((uint32_t) ((Timestamp >> 32)));
}

void caliceahcalbifProducer::buildEudaqEventsROC(std::deque<eudaq::EventUP>& deqEvent) {
   //complete readout cycle treated as single event
   auto ev = eudaq::Event::MakeUnique("CaliceObject");
   std::string s = "EUDAQDataBIF";
   auto CycleEvent = dynamic_cast<eudaq::RawEvent*>(ev.get());
   CycleEvent->AddBlock(0, s.c_str(), s.length());
   s = "i:Type,i:EventCnt,i:TS_Low,i:TS_High";
   CycleEvent->AddBlock(1, s.c_str(), s.length());
   ev->SetTag("ROCStartTS", _acq_start_ts);
   ev->SetTimestamp(_acq_start_ts << 5, _acq_stop_ts << 5, false);
   ev->SetTriggerN(_ReadoutCycle, false); //RAW ROC is used as a trigger number
   if (_eventNumberingPreference == EventNumbering::TIMESTAMP) ev->SetFlagTimestamp();
   if (_eventNumberingPreference == EventNumbering::TRIGGERNUMBER) ev->SetFlagTrigger();
   ev->SetTag("ROC", _ReadoutCycle + _countRocFrom);
   unsigned int times[2];
   struct timeval tv;
   ::gettimeofday(&tv, NULL);
   times[0] = tv.tv_sec;
   times[1] = tv.tv_usec;
   CycleEvent->AddBlock(2, times, sizeof(times));
   CycleEvent->AddBlock(3, std::vector<uint8_t>());
   CycleEvent->AddBlock(4, std::vector<uint8_t>());
   CycleEvent->AddBlock(5, std::vector<uint8_t>());
   CycleEvent->AddBlock(6, _cycleData);
   deqEvent.push_back(std::move(ev));
//   SendEvent(std::move(ev));
   _cycleData.clear();
}

void caliceahcalbifProducer::buildEudaqEventsTriggers(std::deque<eudaq::EventUP>& deqEvent) {
   //cherrypicking just the triggers + adding copy of the start acq and stop acq
   //std::cout << "<" << std::flush;
   std::vector<uint32_t> start_packet;
   std::vector<uint32_t> stop_packet;
   std::vector<std::vector<uint32_t>> trigger_packets;

   //prepare list of trigger by extracting them from the _cycleData
   //TODO quite inefficient due to memory copy
   for (int i = 0; i < (static_cast<int>(_cycleData.size()) - 3); i += 4) {
      switch (_cycleData[i] & 0xFF000000) {
         case 0x03000000:
            start_packet = std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]);
            break;
         case 0x02000000:
            stop_packet = std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]);
            break;
         case 0x01000000:
            trigger_packets.push_back(std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]));
            break;
         default:
            break;
      }
   }
   //std::cout << "." << std::flush;
   //make new event for every trigger
   for (auto trigger : trigger_packets) {
      auto ev = eudaq::Event::MakeUnique("CaliceObject");
      ev->SetTag("ROCStartTS", _acq_start_ts);
      std::string s = "EUDAQDataBIF";
      //auto CycleEvent = dynamic_cast<eudaq::RawEvent*>(ev.get());
      ev->AddBlock(0, s.c_str(), s.length());
      //CycleEvent->AddBlock(0, s.c_str(), s.length());
      s = "i:Type,i:EventCnt,i:TS_Low,i:TS_High";
      ev->AddBlock(1, s.c_str(), s.length());
      //std::cout << ":" << std::flush;
      uint64_t trig_ts = ((uint64_t) trigger[2] + (((uint64_t) trigger[3]) << 32));
      uint32_t trig_number = trigger[1];
      ev->SetTimestamp(trig_ts, trig_ts + 1, false);
      ev->SetTriggerN(trig_number - _firstTriggerNumber, false);
      if (_eventNumberingPreference == EventNumbering::TIMESTAMP) ev->SetFlagTimestamp();
      if (_eventNumberingPreference == EventNumbering::TRIGGERNUMBER) ev->SetFlagTrigger();
      ev->SetTag("ROC", _ReadoutCycle + _countRocFrom);
      int bxid = (((int64_t) trig_ts - (int64_t) (_acq_start_ts << 5) - (int64_t) _firstBxidOffsetBins) / _bxidLengthBins);
      ev->SetTag("BXID", bxid);
//      std::cout << "DEBUG: trig_ts=" << trig_ts << ", _acq_start_ts=" << _acq_start_ts << ", _firstBxidOffsetBins=" << _firstBxidOffsetBins
//            << ", _bxidLengthBins=" << _bxidLengthBins << std::endl;
      unsigned int times[2];
      struct timeval tv;
      ::gettimeofday(&tv, NULL);
      times[0] = tv.tv_sec;
      times[1] = tv.tv_usec;
      ev->AddBlock(2, times, sizeof(times));
      ev->AddBlock(3, std::vector<uint8_t>());
      ev->AddBlock(4, std::vector<uint8_t>());
      ev->AddBlock(5, std::vector<uint8_t>());
      std::vector<uint32_t> data;
      std::copy(start_packet.begin(), start_packet.end(), std::back_inserter(data));
      std::copy(trigger.begin(), trigger.end(), std::back_inserter(data));
      std::copy(stop_packet.begin(), stop_packet.end(), std::back_inserter(data));

//      std::cout << "Data: ";
//      for (auto value : data) {
//         std::cout << to_hex(value, 8) << " ";
//      }
//      std::cout << std::endl;

      ev->AddBlock(6, data);
      deqEvent.push_back(std::move(ev));
      //   SendEvent(std::move(ev));
   }
   //std::cout << ">" << std::flush;
   _cycleData.clear();
}

void caliceahcalbifProducer::buildEudaqEventsBxid(std::deque<eudaq::EventUP>& deqEvent) {
   //cherrypicking just the triggers + adding copy of the start acq and stop acq
   //std::cout << "<" << std::flush;
   std::vector<uint32_t> start_packet;
   std::vector<uint32_t> stop_packet;
   std::vector<std::vector<uint32_t>> trigger_packets;

   //prepare list of trigger by extracting them from the _cycleData
   //TODO quite inefficient due to memory copy
   for (int i = 0; i < (static_cast<int>(_cycleData.size()) - 3); i += 4) {
      switch (_cycleData[i] & 0xFF000000) {
         case 0x03000000:
            start_packet = std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]);
            break;
         case 0x02000000:
            stop_packet = std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]);
            break;
         case 0x01000000:
            trigger_packets.push_back(std::vector<uint32_t>(&_cycleData[i], &_cycleData[i + 4]));
            break;
         default:
            break;
      }
   }
   int lastBxid = INT_MIN;
   int lastTrigNumber = INT_MIN;
   std::vector<uint32_t> data;   //data field for eudaq packet
   auto ev = eudaq::Event::MakeUnique("CaliceObject");
   if (_eventNumberingPreference == EventNumbering::TIMESTAMP) ev->SetFlagTimestamp();
   if (_eventNumberingPreference == EventNumbering::TRIGGERNUMBER) ev->SetFlagTrigger();
   ev->SetTag("ROCStartTS", _acq_start_ts);
   std::string s = "EUDAQDataBIF";
   ev->AddBlock(0, s.c_str(), s.length());
   s = "i:Type,i:EventCnt,i:TS_Low,i:TS_High";
   ev->AddBlock(1, s.c_str(), s.length());
   unsigned int times[2];
   struct timeval tv;
   ::gettimeofday(&tv, NULL);
   times[0] = tv.tv_sec;
   times[1] = tv.tv_usec;
   ev->AddBlock(2, times, sizeof(times));
   ev->AddBlock(3, std::vector<uint8_t>());
   ev->AddBlock(4, std::vector<uint8_t>());
   ev->AddBlock(5, std::vector<uint8_t>());
   std::copy(start_packet.begin(), start_packet.end(), std::back_inserter(data));
   ev->SetTag("ROC", _ReadoutCycle + _countRocFrom);
//   std::cout << "< ROC=" << _ReadoutCycle + _countRocFrom << ", ";
   for (auto trigger : trigger_packets) {
      uint64_t trig_ts = ((uint64_t) trigger[2] + (((uint64_t) trigger[3]) << 32));
      uint32_t trig_number = trigger[1];
      int bxid = (((int64_t) trig_ts - (int64_t) (_acq_start_ts << 5) - (int64_t) _firstBxidOffsetBins) / _bxidLengthBins);
      if (lastBxid != INT_MIN) {   //if ev already contains a trigger record
         if ((trig_number == lastTrigNumber) || (bxid == lastBxid)) {   //the same bxid
            //std::copy(trigger.begin(), trigger.end(), std::back_inserter(data));
            //we will addpend this event to the same bxid
            std::cout<<"+";
            //std::cout << "#Adding trigger. ROC=" << _ReadoutCycle + _countRocFrom << ", BXID=" << bxid << ", Trig#= " << trig_number  << std::endl;
         } else {            //different bxid
            //previous event must be closed and send out first
            ev->SetTimestamp(trig_ts, trig_ts + 1, false);
            std::copy(stop_packet.begin(), stop_packet.end(), std::back_inserter(data));
            ev->AddBlock(6, data);
            deqEvent.push_back(std::move(ev));
            data.clear();
            lastBxid = INT_MIN;
            //initialize new event:
            ev = eudaq::Event::MakeUnique("CaliceObject");
            if (_eventNumberingPreference == EventNumbering::TIMESTAMP) ev->SetFlagTimestamp();
            if (_eventNumberingPreference == EventNumbering::TRIGGERNUMBER) ev->SetFlagTrigger();
            ev->SetTag("ROCStartTS", _acq_start_ts);
            std::string s = "EUDAQDataBIF";
            ev->AddBlock(0, s.c_str(), s.length());
            s = "i:Type,i:EventCnt,i:TS_Low,i:TS_High";
            ev->AddBlock(1, s.c_str(), s.length());
            unsigned int times[2];
            struct timeval tv;
            ::gettimeofday(&tv, NULL);
            times[0] = tv.tv_sec;
            times[1] = tv.tv_usec;
            ev->AddBlock(2, times, sizeof(times));
            ev->AddBlock(3, std::vector<uint8_t>());
            ev->AddBlock(4, std::vector<uint8_t>());
            ev->AddBlock(5, std::vector<uint8_t>());
            std::copy(start_packet.begin(), start_packet.end(), std::back_inserter(data));
            ev->SetTriggerN(trig_number - _firstTriggerNumber, false);            //save the trigger number only for the first trigger
            ev->SetTag("ROC", _ReadoutCycle + _countRocFrom);
         }
      } else {            //no trigger in the event yet
         ev->SetTriggerN(trig_number - _firstTriggerNumber, false);            //save the trigger number only for the first trigger
         std::copy(trigger.begin(), trigger.end(), std::back_inserter(data));
         ev->SetTriggerN(trig_number - _firstTriggerNumber, false);            //save the trigger number only for the first trigger
         lastTrigNumber = trig_number - _firstTriggerNumber;
      }
      ev->SetTag("BXID", bxid);
      lastBxid = bxid;
      std::copy(trigger.begin(), trigger.end(), std::back_inserter(data));
   }
   if (lastBxid != INT_MIN) {
      std::copy(stop_packet.begin(), stop_packet.end(), std::back_inserter(data));
      ev->AddBlock(6, data);
      deqEvent.push_back(std::move(ev));
   }
   data.clear();
   //std::cout << ">" << std::flush;
   _cycleData.clear();
}

void caliceahcalbifProducer::buildEudaqEvents(std::deque<eudaq::EventUP> & deqEvent) {
   switch (_eventBuildingMode) {
      case EventBuildingMode::ROC:
         // eudaq::RawDataEvent CycleEvent("CaliceObject", m_run, _ReadoutCycle);
         buildEudaqEventsROC(deqEvent);
         break;
      case EventBuildingMode::TRIGGERS:
         buildEudaqEventsTriggers(deqEvent);
         break;
      case EventBuildingMode::BXID:
         buildEudaqEventsBxid(deqEvent);
         break;
      default:
         break;
   }
}

void caliceahcalbifProducer::sendallevents(std::deque<eudaq::EventUP> & deqEvent, int minimumsize) {
   while (deqEvent.size() > minimumsize) {
      //std::this_thread::sleep_for(std::chrono::seconds(1));
      std::lock_guard<std::mutex> lock(_deqEventAccessMutex); //minimal lock for pushing ne
      if (!_BORESent) {
         deqEvent.front()->SetBORE();
         _BORESent = true;
         deqEvent.front()->SetTag("FirstROCStartTS", _stats.runStartTS << 5);
         if (!_redirectedInputFileName.length()) {
            deqEvent.front()->SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
            deqEvent.front()->SetTag("BoardID", to_string(m_tlu->GetBoardID()));
         }
      }
      if ((minimumsize == 0) && (deqEvent.size() == 1)) {
         deqEvent.front()->SetEORE();
         deqEvent.front()->SetTag("RunFirstROC", _firstStutterCycle);
         deqEvent.front()->SetTag("RunFirstTriggerNumber", _firstTriggerNumber);
      }
//         std::cout<<"DEBUG sending evt";
//         deqEvent.front()->Print(std::cout,0);
      SendEvent(std::move(deqEvent.front()));
      deqEvent.pop_front();
   }
}

//int main(int /*argc*/, const char ** argv) {
//   eudaq::OptionParser op("EUDAQ AHCAL BIF Producer", "1.0", "The Producer task for the BIF (based on mini-tlu)");
//   eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
//   eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
//
//   try {
//      op.Parse(argv);
//      EUDAQ_LOG_LEVEL(level.Value());
//      auto app = eudaq::Factory<eudaq::Producer>::MakeShared<const std::string&, const std::string&>
//            (eudaq::cstr2hash("caliceahcalbifProducer"), "caliceahcalbifProducer", rctrl.Value());
//      app->Exec();
//   } catch (...) {
//      return op.HandleMainException();
//   }
//   return 0;
//}
