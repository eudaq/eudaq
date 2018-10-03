#ifndef AHCALPRODUCER_HH
#define AHCALPRODUCER_HH

#include "eudaq/Producer.hh"

#include <vector>
#include <deque>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>

namespace eudaq {

   class AHCALProducer;

   class AHCALReader {
      public:
         virtual void Read(std::deque<char> & buf, std::deque<eudaq::EventUP> & deqEvent) = 0;
         virtual void buildEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
         }
         virtual void OnStart(int runNo) {
         }
         virtual void OnStop(int waitQueueTimeS) {
         }
         virtual void OnConfigLED(std::string _fname) {
         }

         AHCALReader(AHCALProducer *r) :
               _producer(r) {
         }
         virtual ~AHCALReader() {
         }
      public:
         std::mutex _eventBuildingQueueMutex;

      protected:

         AHCALProducer * _producer;
   };

   class AHCALProducer: public eudaq::Producer {
      public:
         enum class EventBuildingMode {
            ROC, TRIGGERID, BUILD_BXID_ALL, BUILD_BXID_VALIDATED
         };
         enum class EventNumbering {
            TRIGGERID, TIMESTAMP
         };

         AHCALProducer(const std::string & name, const std::string & runcontrol);
         void DoConfigure() override final;
         void DoStartRun() override final;
         void DoStopRun() override final;
         void DoTerminate() override final;
         void DoReset() override final {
         }
         void RunLoop() override final;

         void SetReader(AHCALReader *r) {
            _reader = r;
         }
         bool OpenConnection(); //
         void CloseConnection(); //
         bool OpenConnection_unsafe(); //
         void CloseConnection_unsafe(); //
         void SendCommand(const char *command, int size = 0);

         void OpenRawFile(unsigned param, bool _writerawfilename_timestamp);
         void sendallevents(std::deque<eudaq::EventUP> &deqEvent, int minimumsize);

         AHCALProducer::EventBuildingMode getEventMode() const;
         AHCALProducer::EventNumbering getEventNumberingPreference() const;
         int getLdaTrigidOffset() const;
         int getLdaTrigidStartsFrom() const;
         int getAhcalbxid0Offset() const;
         int getAhcalbxidWidth() const;
         int getInsertDummyPackets() const;
         int getDebugKeepBuffered() const;
         int getGenerateTriggerIDFrom() const;
         int getColoredTerminalMessages() const;
         int getIgnoreLdaTimestamps() const;
         int getMaxTrigidSkip() const;
         int getKeepBuffered() const;
         int getMaxRocJump() const;
         int getAppendDifidToChipidBitPosition() const;
         int getChipidAddAfterMasking() const;
         int getChipidAddBeforeMasking() const;
         int getChipidKeepBits() const;
         int getMinimumBxid() const;

         static const uint32_t m_id_factory = eudaq::cstr2hash("AHCALProducer");
         private:
         AHCALProducer::EventBuildingMode _eventBuildingMode;
         AHCALProducer::EventNumbering _eventNumberingPreference;
         int _LdaTrigidOffset; //LdaTrigidOffset to compensate trigger number differences between TLU (or other trigger number source) and LDA. Eudaq Event counting starts from this number and will be always subtracted from the eudaq event triggerid.
         int _LdaTrigidStartsFrom;   // triggerID number of first valid event in case it doesn't start from 0
         int _AHCALBXID0Offset; //offset from start acquisition Timestamp to BXID0 (in 25ns steps). Varies with AHCAL powerpulsing setting and DIF firmware
         int _AHCALBXIDWidth; //length of the bxid in 25 ns steps
         int _InsertDummyPackets; //1=Put dummy packets to maintain an uninterrupted sequence of TriggerIDs. 0=don't inset anything
         int _DebugKeepBuffered; //1=keep events in buffer and don't send them to data collector
         int _KeepBuffered; // how many events to keep in the buffer
         int _maxRocJump;//how many ROC can the data go backward and forward. This is needed for winglda with 2 FPGAs
         int _GenerateTriggerIDFrom; //sets from which triggerID number should be data generated (and filled with dummy triggers if necessary). Only works when insert_dummy_packets is enabled and in selected event building mode
         int _ColoredTerminalMessages; //1 for colored error worning and info messages
         int _IgnoreLdaTimestamps; //ignores the timestamp in the AHCAL LDA data stream
         int _StartWaitSeconds; //wait a fixed amount of seconds, befor the DoStartRun is executed
         int _AppendDifidToChipidBitPosition; //whether to append the DIF-ID as upper N bits of Chip-ID. if -1, nothing is added
         int _ChipidKeepBits;//how many bits to keep from the chipid
         int _ChipidAddBeforeMasking;//a number to be added before the bit masking
         int _ChipidAddAfterMasking;//a number to be added to the chipid after bit masking
         int _minimumBxid; // minimal accepted BXID
         int _runNo;
         int _eventNo; //last sent event - for checking of correct event numbers sequence during sending events
         int _minEventHits;//minimum number of hits in the event
#ifdef _WIN32
         SOCKET _fd;
         std::ifstream _redirectedInputFstream;
#else
         int _fd;
         #endif
         int _maxTrigidSkip;

         std::mutex _mufd;

         std::string _redirectedInputFileName; // if set, this filename will be used as input

         bool _running;
         bool _stopped;
         bool _terminated;
         bool _BORE_sent;         //was first event sent? (id has to be marked with BORE tag)

         // debug output
//         bool _dumpRaw;
         bool _writeRaw;
         std::string _rawFilename;
         bool _writerawfilename_timestamp;
         std::ofstream _rawFile;

         //run type:
         //std::string _runtype;
         std::string _fileLEDsettings;

         //std::string _filename; // input file name at file mode
         int _waitmsFile; // period to wait at each ::read() at file mode
         int _waitsecondsForQueuedEvents; // period to wait after each run to read the queued events
         int _port; // input port at network mode
         std::string _ipAddress; // input address at network mode

         //std::time_t _last_readout_time; //last time when there was any data from AHCAL

         uint32_t m_id_stream;

         AHCALReader * _reader;
   };

}

#endif // AHCALPRODUCER_HH

