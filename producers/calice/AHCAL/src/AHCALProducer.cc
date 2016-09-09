// CaliceReceiver.cc

#include "AHCALProducer.hh"

#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"
#include "ScReader.hh"
#include "stdlib.h"
#include <unistd.h>
#include <iomanip>

#include <thread>
#include <mutex>

// socket headers
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace eudaq;
using namespace std;

namespace eudaq {

  AHCALProducer::AHCALProducer(const std::string & name, const std::string & runcontrol) :
    Producer(name, runcontrol), _runNo(0), _eventNo(0), _fd(0), _running(false), _stopped(true), _configured(false)
  {

  }

   // This gets called whenever the DAQ is initialised
    void AHCALProducer::OnInitialise(const eudaq::Configuration & init) {

      try {
	
	std::cout << "Reading: " << init.Name() << std::endl;
      
      // Do any initialisation of the hardware here 
      // "start-up configuration", which is usally done only once in the beginning
      // Configuration file values are accessible as config.Get(name, default)
      //  m_exampleInitParam = init.Get("InitParameter", 0);
      
      // send information
      // Message as cout in the terminal of your producer
	//      std::cout << "Initialise with parameter = " << m_exampleInitParam << std::endl;
      // or to the LogCollector, depending which log level you want. These are the possibilities just as an example here:
      //  EUDAQ_INFO("Initialise with parameter = " + m_exampleInitParam);
      
      // send it to your hardware
      // hardware.Setup(m_exampleInitParam);
      
      // At the end, set the ConnectionState that will be displayed in the Run Control.
      // and set the state of the machine.
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
    } 
      catch (...) {
        // Message as cout in the terminal of your producer
        std::cout << "Unknown exception" << std::endl;
        // Message to the LogCollector
        EUDAQ_ERROR("Error Message to the LogCollector from ExampleProducer");
        // Otherwise, the State is set to ERROR
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
      }
    }



  void AHCALProducer::OnConfigure(const eudaq::Configuration & param)
  {
      try {

    std::cout<< " START AHCAL CONFIGURATION "<< std::endl;
    // run rype: LED run or normal run ""
    _fileLEDsettings = param.Get("FileLEDsettings", "");

    // file name
    _filename = param.Get("FileName", "");
    _waitmsFile = param.Get("WaitMillisecForFile", 100);
    _waitsecondsForQueuedEvents = param.Get("waitsecondsForQueuedEvents", 2);

    // raw output
    _writeRaw = param.Get("WriteRawOutput", 0);
    _rawFilename = param.Get("RawFileName", "run%d.raw");
    _writerawfilename_timestamp = param.Get("WriteRawFileNameTimestamp",0);

    // port
    _port = param.Get("Port", 9011);
    _ipAddress = param.Get("IPAddress", "127.0.0.1");
    
    string reader = param.Get("Reader", "");
    SetReader(new ScReader(this)); // in sc dif ID is not specified
    
    _reader->OnConfigLED(_fileLEDsettings); //newLED

    _configured = true;
   
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");

    std::cout<< " END AHCAL congfiguration "<< std::endl;
      }
    catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
      }

  }

  void AHCALProducer::OnStartRun(unsigned param) {
    try{
    _runNo = param;
    _eventNo = 0;
    // raw file open
    if(_writeRaw) OpenRawFile(param, _writerawfilename_timestamp);

    _reader->OnStart(param);

    SendEvent(RawDataEvent::BORE("CaliceObject", _runNo));
    std::cout << "Start Run: " << param << std::endl;
    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");

    _running = true;
    _stopped = false;

    }  catch (...) {
      // Otherwise, the State is set to ERROR
      printf("Unknown exception\n");
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Starting Error");
    }

  }

  void AHCALProducer::OpenRawFile(unsigned param, bool _writerawfilename_timestamp) {

    //	read the local time and save into the string myString
    time_t  ltime;
    struct tm *Tm;
    ltime=time(NULL);
    Tm=localtime(&ltime);
    char file_timestamp[25];
    std::string myString;
    if(_writerawfilename_timestamp==1) {
      sprintf(file_timestamp,"__%02dp%02dp%02d__%02dp%02dp%02d.raw",
	      Tm->tm_mday,
	      Tm->tm_mon+1,
	      Tm->tm_year+1900,
	      Tm->tm_hour,
	      Tm->tm_min,
	      Tm->tm_sec);
      myString.assign(file_timestamp, 26);
    } else myString=".raw";
	
    std::string _rawFilenameTimeStamp;
    //if chosen like this, add the local time to the filename
    _rawFilenameTimeStamp=_rawFilename+myString;
    char _rawFilename[256];
    sprintf(_rawFilename, _rawFilenameTimeStamp.c_str(), (int)param);
	
    _rawFile.open(_rawFilename);
  }


  void AHCALProducer::OnPrepareRun(unsigned param) {
    cout << "OnPrepareRun: runID " << param << " set." << endl;
  }
  
  void AHCALProducer::OnStopRun() {
      try {

	_reader->OnStop(_waitsecondsForQueuedEvents);
	_running = false;
	while (_stopped == false) {
	  eudaq::mSleep(100);
	}

	if(_writeRaw)
	  _rawFile.close();

	SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");

      }
      catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stopping Error");
      }

  }

  bool AHCALProducer::OpenConnection()
  {
    struct sockaddr_in dstAddr;
    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_port = htons(_port);
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_addr.s_addr = inet_addr(_ipAddress.c_str());

    std::unique_lock<std::mutex> myLock(_mufd);
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = connect(_fd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
    if(ret != 0)  return 0;
    return 1;
  }

 
  void AHCALProducer::CloseConnection()
  {
    std::unique_lock<std::mutex> myLock(_mufd);
    close(_fd);
    _fd = 0;
  }

   
  // send a string without any handshaking
  void AHCALProducer::SendCommand(const char *command, int size){

    if(size == 0)size = strlen(command);

    if(_fd <= 0)cout << "AHCALProducer::SendCommand(): cannot send command because connection is not open." << endl;
    else {
	    size_t bytesWritten = write(_fd, command, size);
	    if ( bytesWritten  < 0 ) {
		    cout << "There was an error writing to the TCP socket" << endl;
	    } else {
		    cout << bytesWritten  << " out of " << size << " bytes is  written to the TCP socket" << endl;
	    }
    }

  }

  deque<eudaq::RawDataEvent *>  AHCALProducer::sendallevents(deque<eudaq::RawDataEvent *> deqEvent, int minimumsize) { 
    while(deqEvent.size() > minimumsize){
      SendEvent(*(deqEvent.front()));
      delete deqEvent.front();
      deqEvent.pop_front();
    }
    return deqEvent;
  }

  void AHCALProducer::MainLoop()
  {

    deque<char> bufRead;
    // deque for events: add one event when new acqId is arrived: to be determined in reader
    deque<eudaq::RawDataEvent *> deqEvent;

    while( true ){
      // wait until configured and connected
      std::unique_lock<std::mutex> myLock(_mufd);
      //      if file is not ready  just wait
      if(_fd <= 0 ){
	myLock.unlock();
	::usleep(1000);
	continue;
      }

      const int bufsize = 4096;
      // copy to C array, then to vector
      char buf[bufsize];
      int size = 0;        
     
      // system call: from socket to C array
      size = ::read(_fd, buf, bufsize);
      if(size == -1 || size == 0){
        if(size == -1)
          std::cout << "Error on read: " << errno << " Disconnect and going to the waiting mode." << endl;
        else
          {
	    std::cout << "Socket disconnected. going to the waiting mode." << endl;         
	    if(!_running && _stopped==false) {
	      _stopped=true;
	      deqEvent=sendallevents(deqEvent,0);
	      
	      SendEvent(RawDataEvent::EORE("CaliceObject", _runNo, _eventNo));
	      bufRead.clear();
	      deqEvent.clear();
	    }
	  }
        close(_fd);
        _fd = -1;
	continue;
      } 
    
      if(_writeRaw && _rawFile.is_open()) _rawFile.write(buf, size);

      // C array to vector
      copy(buf, buf + size, back_inserter(bufRead));

      if(_reader){
	_reader->Read(bufRead, deqEvent);
      }
       
      deqEvent=sendallevents(deqEvent,1);
 
    }
  }

}//end namespace eudaq

int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Calice Producer Producer", "1.0",
                         "hogehoge");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "", "Calice1", "string",
                                   "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    cout << name.Value() << " " << rctrl.Value() << endl;
    eudaq::AHCALProducer producer(name.Value(), rctrl.Value());
    //producer.SetReader(new eudaq::SiReader(0));
    // And set it running...
    producer.MainLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
