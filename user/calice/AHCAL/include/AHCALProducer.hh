#ifndef AHCALPRODUCER_HH
#define AHCALPRODUCER_HH

#include "eudaq/Producer.hh"

#include <vector>
#include <deque>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>

#include "eudaq/RawDataEvent.hh"

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

         AHCALReader(AHCALProducer *r) {
            _producer = r;
         }
         ~AHCALReader() {
         }
      public:
         std::mutex _eventBuildingQueueMutex;

      protected:

         AHCALProducer * _producer;
   };

   class AHCALProducer: public eudaq::Producer {
      public:

         enum class EventMode {
            ROC, TRIGID, BUILT_BXID_ALL, BUILT_BXID_VALIDATED
         };

         AHCALProducer(const std::string & name, const std::string & runcontrol);
         void DoConfigure() override final;
         void DoStartRun() override final;
         void DoStopRun() override final;
         void DoTerminate() override final;
         void DoReset() override final {
         }
         void Exec() override final;

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

         EventMode getEventMode()
         {
            return _eventMode;
         }
         int getLdaTrigidOffset();
         int getLdaTrigidStartsFrom();

      private:
         AHCALProducer::EventMode _eventMode;
         int _LdaTrigidOffset; //LdaTrigidOffset to compensate differences between TLU (or other trigger number source) and LDA. Eudaq Event counting starts from this number and will be always subtracted from the eudaq event triggerid.
         int _LdaTrigidStartsFrom;   // triggerID of first valid event in case it doesn't start from 0

         int _runNo;
         int _eventNo; //last sent event - for checking of correct event numbers sequence during sending events
         int _fd;
         //airqui
         //    pthread_mutex_t _mufd;
         std::mutex _mufd;

         std::string _redirectedInputFileName;

         bool _running;
         bool _stopped;
         bool _configured;
         bool _terminated;
         bool _BORE_sent;         //was first event sent? (id has to be marked with BORE tag

         // debug output
         bool _dumpRaw;
         bool _writeRaw;
         std::string _rawFilename;
         bool _writerawfilename_timestamp;
         std::ofstream _rawFile;

         //run type:
         std::string _runtype;
         std::string _fileLEDsettings;

         bool _filemode; // true: input from file: false: input from network
         std::string _filename; // input file name at file mode
         int _waitmsFile; // period to wait at each ::read() at file mode
         int _waitsecondsForQueuedEvents; // period to wait after each run to read the queued events
         int _port; // input port at network mode
         std::string _ipAddress; // input address at network mode

         std::time_t _last_readout_time; //last time when there was any data from AHCAL

         //std::deque<eudaq::RawDataEvent *> deqEvent;

         uint32_t m_id_stream;

//         std::unique_ptr<AHCALReader> _reader;
         AHCALReader * _reader;
   };

}

#endif // AHCALPRODUCER_HH

