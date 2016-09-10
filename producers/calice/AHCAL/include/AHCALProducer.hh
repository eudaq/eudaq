#ifndef AHCALPRODUCER_HH
#define AHCALPRODUCER_HH

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

  class AHCALProducer;
  
  class AHCALReader {
  public:
    virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent) = 0;
    virtual void OnStart(int runNo){}
    virtual void OnStop(int waitQueueTimeS){}
    virtual void OnConfigLED(std::string _fname){}
  protected:
    AHCALReader(AHCALProducer *r){_producer = r;}
    ~AHCALReader(){}
      
    AHCALProducer * _producer;
  };
  
  class AHCALProducer : public eudaq::Producer {
  public:
    AHCALProducer(const std::string & name, const std::string & runcontrol);
    void SetReader(AHCALReader *r){_reader = r;}
    virtual void OnPrepareRun(unsigned param);
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnConfigure(const eudaq::Configuration & param);
    virtual void OnInitialise(const eudaq::Configuration & init);

    void MainLoop();//  
    bool OpenConnection();//
    void CloseConnection();//
    bool OpenConnection_unsafe();//
    void CloseConnection_unsafe();//
    void SendCommand(const char *command,int size = 0);

    virtual void OpenRawFile(unsigned param, bool _writerawfilename_timestamp);
    virtual std::deque<eudaq::RawDataEvent *> sendallevents(std::deque<eudaq::RawDataEvent *> deqEvent, int minimumsize=0);
      
  private:
    int _runNo;
    int _eventNo;
    int _fd;
    //airqui 
    //    pthread_mutex_t _mufd;
    std::mutex _mufd;
      
    bool _running;
    bool _stopped;
    bool _configured;

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

    std::deque<eudaq::RawDataEvent *> deqEvent;

    AHCALReader * _reader;
  };

}

#endif // AHCALPRODUCER_HH

