#ifndef CALICERECEIVER_HH
#define CALICERECEIVER_HH

#include "eudaq/Producer.hh"

#include <vector>
#include <deque>
#include <string>
#include <pthread.h>
#include <fstream>

#include "eudaq/RawDataEvent.hh"

namespace calice_eudaq {

  class CaliceReceiver;
  
  class CaliceReader {
    public:
      virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent) = 0;
      virtual void OnStart(int runNo){}
      virtual void OnStop(int waitQueueTimeS){}
    protected:
      CaliceReader(CaliceReceiver *r){_receiver = r;}
      ~CaliceReader(){}
      
      CaliceReceiver * _receiver;
  };
  
  class CaliceReceiver : public eudaq::Producer {
    public:
      CaliceReceiver(const std::string & name, const std::string & runcontrol);
      void SetReader(CaliceReader *r){_reader = r;}
      virtual void OnPrepareRun(unsigned param);
      virtual void OnStartRun(unsigned param);
      virtual void OnStopRun();
      virtual void OnConfigure(const eudaq::Configuration & param);
      void MainLoop();
      
      void OpenConnection();
      void CloseConnection();
      void SendCommand(const char *command,int size = 0);
      
    private:
      int _runNo;
      int _eventNo;
      int _fd;
      pthread_mutex_t _mufd;
      
      bool _running;
      bool _configured;

      // debug output
      bool _dumpRaw;
      bool _writeRaw;
      std::string _rawFilename;
      std::ofstream _rawFile;
      
      bool _filemode; // true: input from file: false: input from network
      std::string _filename; // input file name at file mode
      int _waitmsFile; // period to wait at each ::read() at file mode
    int _waitsecondsForQueuedEvents; // period to wait after each run to read the queued events
      int _port; // input port at network mode
      std::string _ipAddress; // input address at network mode

      CaliceReader * _reader;
  };

}

#endif // CALICERECEIVER_HH

