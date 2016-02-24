#ifndef CALICEPRODUCER_HH
#define CALICEPRODUCER_HH

#include "eudaq/Producer.hh"

#include <vector>
#include <deque>
#include <string>
//#include <pthread.h>
#include <fstream>
#include <thread>
#include <mutex>


#include "eudaq/RawDataEvent.hh"

namespace eudaq {

  class CaliceProducer;
  
  class CaliceReader {
  public:
    virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent) = 0;
    virtual void OnStart(int runNo){}
    virtual void OnStop(int waitQueueTimeS){}
  protected:
    CaliceReader(CaliceProducer *r){_producer = r;}
    ~CaliceReader(){}
      
    CaliceProducer * _producer;
  };
  
  class CaliceProducer : public eudaq::Producer {
  public:
    CaliceProducer(const std::string & name, const std::string & runcontrol);
    void SetReader(CaliceReader *r){_reader = r;}
    virtual void OnPrepareRun(unsigned param);
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnConfigure(const eudaq::Configuration & param);
    
    void MainLoop();//  
    void OpenConnection();//
    void CloseConnection();//
    void SendCommand(const char *command,int size = 0);

    virtual void OpenRawFile(unsigned param, bool _writerawfilename_timestamp);
    virtual std::deque<eudaq::RawDataEvent *> sendallevents(std::deque<eudaq::RawDataEvent *> deqEvent, int minimumsize=0);
      
  private:
    int _runNo;
    int _eventNo;
    int _fd;
    //airqui 
    //pthread_mutex_t _mufd;
    std::mutex _mufd;
      
    bool _running;
    bool _configured;

    // debug output
    bool _dumpRaw;
    bool _writeRaw;
    std::string _rawFilename;
    bool _writerawfilename_timestamp;
    std::ofstream _rawFile;
      
    bool _filemode; // true: input from file: false: input from network
    std::string _filename; // input file name at file mode
    int _waitmsFile; // period to wait at each ::read() at file mode
    int _waitsecondsForQueuedEvents; // period to wait after each run to read the queued events
    int _port; // input port at network mode
    std::string _ipAddress; // input address at network mode

     std::deque<eudaq::RawDataEvent *> deqEvent;



    CaliceReader * _reader;
  };

}

#endif // CALICEPRODUCER_HH

