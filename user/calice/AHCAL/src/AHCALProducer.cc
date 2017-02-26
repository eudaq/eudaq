// CaliceReceiver.cc

#include "AHCALProducer.hh"

#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "ScReader.hh"
#include "stdlib.h"

#include <iomanip>
#include <iterator>
#include <thread>
#include <mutex>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace eudaq;
using namespace std;

namespace eudaq {

   AHCALProducer::AHCALProducer(const std::string & name, const std::string & runcontrol) :
         Producer(name, runcontrol),
               _runNo(0),
               _eventNo(0),
               _fd(0),
               _running(false),
               _stopped(true),
               _terminated(false),
               _BORE_sent(false),
               _reader(NULL),
               _ColoredTerminalMessages(1),
               _IgnoreLdaTimestamps(0),
               _eventBuildingMode(EventBuildingMode::ROC),
               _eventNumberingPreference(EventNumbering::TRIGGERID),
               _AHCALBXID0Offset(0),
               _AHCALBXIDWidth(0),
               _DebugKeepBuffered(0),
               _GenerateTriggerIDFrom(0),
               _InsertDummyPackets(0),
               _LdaTrigidOffset(0),
               _LdaTrigidStartsFrom(0),
               _port(5622),
               _waitmsFile(0),
               _waitsecondsForQueuedEvents(2),
               _writerawfilename_timestamp(true),
               _writeRaw(true),
               _StartWaitSeconds(0)
   {
      m_id_stream = eudaq::cstr2hash(name.c_str());
   }

   void AHCALProducer::DoTerminate() {
      _terminated = true;
   }
   void AHCALProducer::DoConfigure() {
      const eudaq::Configuration &param = *GetConfiguration();
      std::cout << " START AHCAL CONFIGURATION " << std::endl;
      // run rype: LED run or normal run ""
      _fileLEDsettings = param.Get("FileLEDsettings", "");

      // file name
      //_filename = param.Get("FileName", "");
      _waitmsFile = param.Get("WaitMillisecForFile", 100);
      _waitsecondsForQueuedEvents = param.Get("waitsecondsForQueuedEvents", 2);

      // raw output
      _writeRaw = param.Get("WriteRawOutput", 0);
      _rawFilename = param.Get("RawFileName", "run%d.raw");
      _writerawfilename_timestamp = param.Get("WriteRawFileNameTimestamp", 0);

      // port
      _port = param.Get("Port", 9011);
      _ipAddress = param.Get("IPAddress", "127.0.0.1");

      string reader = param.Get("Reader", "");
      if (!_reader) {
         SetReader(new ScReader(this));
      }
      //      if (_reader != nullptr)
      //         SetReader(std::unique_ptr<ScReader>(new ScReader(this))); // in sc dif ID is not specified

      _reader->OnConfigLED(_fileLEDsettings); //newLED

      _redirectedInputFileName = param.Get("RedirectInputFromFile", "");
      _ColoredTerminalMessages = param.Get("ColoredTerminalMessages", 1);
      _LdaTrigidOffset = param.Get("LdaTrigidOffset", 0);
      _LdaTrigidStartsFrom = param.Get("LdaTrigidStartsFrom", 0);
      _AHCALBXID0Offset = param.Get("AHCALBXID0Offset", 2123); //default for mini-LDA, new DIF and no powerpulsing
      _AHCALBXIDWidth = param.Get("AHCALBXIDWidth", 160); //4us Testbeam mode is default
      _GenerateTriggerIDFrom = param.Get("GenerateTriggerIDFrom", 0);
      _IgnoreLdaTimestamps = param.Get("IgnoreLdaTimestamps", 0);
      _StartWaitSeconds = param.Get("StartWaitSeconds", 0);

      _InsertDummyPackets = param.Get("InsertDummyPackets", 0);
      _DebugKeepBuffered = param.Get("DebugKeepBuffered", 0);

      string eventBuildingMode = param.Get("EventBuildingMode", "ROC");
      if (!eventBuildingMode.compare("ROC")) _eventBuildingMode = AHCALProducer::EventBuildingMode::ROC;
      if (!eventBuildingMode.compare("TRIGGERID")) _eventBuildingMode = AHCALProducer::EventBuildingMode::TRIGGERID;
      if (!eventBuildingMode.compare("BUILD_BXID_ALL")) _eventBuildingMode = AHCALProducer::EventBuildingMode::BUILD_BXID_ALL;
      if (!eventBuildingMode.compare("BUILD_BXID_VALIDATED")) _eventBuildingMode = AHCALProducer::EventBuildingMode::BUILD_BXID_VALIDATED;
      std::cout << "Creating events in \"" << eventBuildingMode << "\" mode" << std::endl;

      string eventNumberingMode = param.Get("EventNumberingPreference", "TRIGGERID");
      if (!eventNumberingMode.compare("TRIGGERID")) _eventNumberingPreference = AHCALProducer::EventNumbering::TRIGGERID;
      if (!eventNumberingMode.compare("TIMESTAMP")) _eventNumberingPreference = AHCALProducer::EventNumbering::TIMESTAMP;
      std::cout << "Preferring event numbering type: \"" << eventNumberingMode << "\"" << std::endl;

      //_configured = true;

      std::cout << " END AHCAL congfiguration " << std::endl;

   }

   void AHCALProducer::DoStartRun() {
      _runNo = GetRunNumber();
      _eventNo = -1;
      _BORE_sent = false;
      // raw file open
      if (_writeRaw) OpenRawFile(_runNo, _writerawfilename_timestamp);
      if (_StartWaitSeconds) {
         std::cout << "Delayed start by " << _StartWaitSeconds << " seconds. Waiting";
         for (int i = 0; i < _StartWaitSeconds; i++) {
            std::cout << "."<<std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
         std::cout << std::endl;
      }
      std::cout << "AHCALProducer::OnStartRun _reader->OnStart(param);" << std::endl; //DEBUG
      _reader->OnStart(_runNo);
//      std::cout << "AHCALProducer::OnStartRun _SendEvent(RawDataEvent::BORE(CaliceObject, _runNo));" << std::endl; //DEBUG
//      auto ev = eudaq::RawDataEvent::MakeUnique("CaliceObject");
//      ev->SetBORE();
//      SendEvent(std::move(ev));
      std::cout << "Start Run: " << _runNo << std::endl;
      _running = true;
      _stopped = false;

   }

   void AHCALProducer::OpenRawFile(unsigned param, bool _writerawfilename_timestamp) {

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

   void AHCALProducer::DoStopRun() {
      std::cout << "AHCALProducer::DoStopRu:  Stop run" << std::endl;
      _reader->OnStop(_waitsecondsForQueuedEvents);
      _running = false;
      std::this_thread::sleep_for(std::chrono::seconds(1));

      std::cout << "AHCALProducer::OnStopRun waiting for _stopped" << std::endl;
      while (!_stopped) {
         std::cout << "!" << std::flush;
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      if (_writeRaw)
         _rawFile.close();
      std::cout << "AHCALProducer::DoStopRun() finished" << std::endl;
      //std::cout << "AHCALProducer::OnStopRun sending EORE event with _eventNo" << _eventNo << std::endl;
      //SendEvent(RawDataEvent::EORE("CaliceObject", _runNo, _eventNo));
      //auto ev = eudaq::RawDataEvent::MakeUnique("CaliceObject");
      //ev->SetEORE();
      //SendEvent(std::move(ev));
   }

   bool AHCALProducer::OpenConnection()
   {
      if (_redirectedInputFileName.empty()) {
         struct sockaddr_in dstAddr;
         memset(&dstAddr, 0, sizeof(dstAddr));
         dstAddr.sin_port = htons(_port);
         dstAddr.sin_family = AF_INET;
         dstAddr.sin_addr.s_addr = inet_addr(_ipAddress.c_str());

         std::unique_lock<std::mutex> myLock(_mufd);
         _fd = socket(AF_INET, SOCK_STREAM, 0);
         int ret = connect(_fd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
         if (ret != 0) return 0;
         return 1;
      } else {
         std::cout << "Redirecting intput from file: " << _redirectedInputFileName << std::endl;
         _fd = open(_redirectedInputFileName.c_str(), O_RDONLY);
         if (_fd < 0) {
            cout << "open redirected file failed from this path:" << _redirectedInputFileName << endl;
            return false;
         }
         return true;
      }
   }

   void AHCALProducer::CloseConnection() {
      std::unique_lock<std::mutex> myLock(_mufd);
#ifdef _WIN32
      closesocket(_fd);
#else
      close(_fd);
#endif
      _fd = 0;
   }

// send a string without any handshaking
   void AHCALProducer::SendCommand(const char *command, int size) {
      cout << "DEBUG: in AHCALProducer::SendCommand(const char *command, int size)" << endl;
      if (size == 0) size = strlen(command);

      if (_fd <= 0)
         cout << "AHCALProducer::SendCommand(): cannot send command because connection is not open." << endl;
      else {
         if (_redirectedInputFileName.empty()) {
            cout << "DEBUG: sending command over TCP" << endl;
            size_t bytesWritten = write(_fd, command, size);
            if (bytesWritten < 0) {
               cout << "There was an error writing to the TCP socket" << endl;
            } else {
               cout << bytesWritten << " out of " << size << " bytes is  written to the TCP socket" << endl;
            }
         } else {
            std::cout << "input overriden from file. No command is send." << std::endl;
            std::cout << "sending " << size << " bytes:";
            for (int i = 0; i < size; ++i) {
               std::cout << " " << to_hex(command[i], 2);
            }
            std::cout << std::endl;

         }
      }

   }

//   int AHCALProducer::SendBuiltEvents(eudaq::RawDataEvent * ReadoutCycleEvent) {
//      u_int64_t
//      start_timestamp = 0;
//      return 0;
//   }

   void AHCALProducer::sendallevents(std::deque<eudaq::EventUP> & deqEvent, int minimumsize) {
      while (deqEvent.size() > minimumsize) {

         //std::this_thread::sleep_for(std::chrono::seconds(1));

         std::lock_guard<std::mutex> lock(_reader->_eventBuildingQueueMutex);
         if (deqEvent.front()) {
            if (!_BORE_sent) {
               _BORE_sent = true;
               deqEvent.front()->SetBORE();
               deqEvent.front()->SetTag("FirstROCStartTS", dynamic_cast<ScReader*>(_reader)->getRunTimesStatistics().first_TS);
            }
            if ((minimumsize == 0) && (deqEvent.size() == 1))
               deqEvent.front()->SetEORE();

//            if (deqEvent.front()->GetEventN() != (_eventNo + 1)) {
//               std::cout << "SENDEVENTS: Run " + to_string(_runNo) + " Event " + to_string(deqEvent.front()->GetEventN()) + " not in sequence. Expected " + to_string(_eventNo + 1) << std::endl;
//               EUDAQ_WARN("Run " + to_string(_runNo) + " Event " + to_string(deqEvent.front()->GetEventN()) + " not in sequence. Expected " + to_string(_eventNo + 1));
//            }
            _eventNo = deqEvent.front()->GetEventN();
            SendEvent(std::move(deqEvent.front()));
         }

         deqEvent.pop_front();
      }

//      eudaq::EventUP ev = deqEvent.front();
//      if (from_string(ev->GetTag("TriggerValidated"), -1) == 1) {
//         if (ev->GetEventNumber() != (_eventNo + 1)) {
//            EUDAQ_WARN("Run " + to_string(_runNo) + " Event " + to_string(ev->GetEventNumber()) +
//                  " not in sequence. Expected " + to_string(_eventNo + 1), std::cout, NULL)
//            ;
//
//            //fix for a problem of 2 triggers, that came in the same ROC
//            if (ev->GetEventNumber() == (_eventNo + 2)) {
//               EUDAQ_WARN(" Sending event " + to_string(ev->GetEventNumber()) + " twice");
//               // SendEvent(*(deqEvent.front()));
//               SendEvent(deqEvent.front()->Clone());
//            } else {
//               int jump = (ev->GetEventNumber() - (_eventNo + 1));
//               EUDAQ_ERROR(
//                     "Run " + to_string(_runNo) + " Event " + to_string(ev->GetEventNumber())
//                           + " cannot be fixed by sending the packet twice. Problem more complex, which is not possible to fix a jump by " + to_string(jump));
//            }
//         }
//
//         _eventNo = ev->GetEventNumber(); //save the last event number
//         // SendEvent(*(deqEvent.front()));
//         SendEvent(deqEvent.front()->Clone());
//         if (from_string(ev->GetTag("TriggerInvalid"), -1) == 1) {
//            cout << "Send DUMMY eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
//         } else {
//            cout << "Send eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
//         }
//      } else
//         cout << "Discard eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
//      delete deqEvent.front();
//      deqEvent.pop_front();
////      }
//      return deqEvent;
   }

   void AHCALProducer::Exec() {
      std::cout << " Main loop " << std::endl;
      StartCommandReceiver();
      deque<char> bufRead;
      // deque for events: add one event when new acqId is arrived: to be determined in reader
//      deque<eudaq::RawDataEvent *> deqEvent2;
      std::deque<eudaq::EventUP> deqEvent;

      const int bufsize = 4 * 1024;
      // copy to C array, then to vector
      char buf[bufsize]; //buffer to read from TCP socket

      while (!_terminated) {
         // wait until configured and connected
         std::unique_lock<std::mutex> myLock(_mufd);

         int size = 0;

         if (_fd <= 0 || _stopped) {
            myLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
         }
         size = ::read(_fd, buf, bufsize);
         //std::cout << "DEBUG: producer Exec(): read" << size << " bytes" << std::endl;
         if (size > 0) {
            //_last_readout_time = std::time(NULL);
            if (_writeRaw && _rawFile.is_open())
               _rawFile.write(buf, size);
            // C array to vector
            copy(buf, buf + size, back_inserter(bufRead));
            //bufRead.insert(bufRead.end(), buf, buf + size);
            if (_reader)
               _reader->Read(bufRead, deqEvent);
            // send events : remain the last event
            sendallevents(deqEvent, 1);
            continue;
         }

         if (size == -1 || size == 0) {
            if (size == -1)
               std::cout << "Error on read: " << errno
                     << " Disconnect and going to the waiting mode." << endl;
            else {
               std::cout << "Socket disconnected. going to the waiting mode." << endl;
            }
#ifdef _WIN32
            closesocket(_fd);
#else
            close(_fd);
#endif
            _fd = -1;
            std::cout << "sending the rest of the event" << std::endl;
            _reader->buildEvents(deqEvent, true);
            _stopped = 1;
            sendallevents(deqEvent, 0);
            bufRead.clear();
            deqEvent.clear();
         }
      }
   }

   AHCALProducer::EventBuildingMode AHCALProducer::getEventMode() const {
      return _eventBuildingMode;
   }

   int AHCALProducer::getLdaTrigidOffset() const {
      return _LdaTrigidOffset;
   }
   int AHCALProducer::getLdaTrigidStartsFrom() const {
      return _LdaTrigidStartsFrom;
   }

   int AHCALProducer::getAhcalbxid0Offset() const {
      return _AHCALBXID0Offset;
   }

   int AHCALProducer::getAhcalbxidWidth() const {
      return _AHCALBXIDWidth;
   }

   int AHCALProducer::getInsertDummyPackets() const
   {
      return _InsertDummyPackets;
   }

   int AHCALProducer::getDebugKeepBuffered() const
   {
      return _DebugKeepBuffered;
   }

   int AHCALProducer::getGenerateTriggerIDFrom() const
   {
      return _GenerateTriggerIDFrom;
   }

   AHCALProducer::EventNumbering AHCALProducer::getEventNumberingPreference() const
   {
      return _eventNumberingPreference;
   }

   int AHCALProducer::getColoredTerminalMessages() const
   {
      return _ColoredTerminalMessages;
   }

   int AHCALProducer::getIgnoreLdaTimestamps() const
   {
      return _IgnoreLdaTimestamps;
   }

}	//end namespace eudaq

