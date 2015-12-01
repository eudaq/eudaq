
// CaliceReceiver.cc

#include "CaliceReceiver.hh"

#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"
#include "SiReader.hh"
#include "ScReader.hh"
#include "stdlib.h"
#include <unistd.h>
#include <iomanip>


// socket headers
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace eudaq;
using namespace std;

namespace calice_eudaq {

  CaliceReceiver::CaliceReceiver(const std::string & name, const std::string & runcontrol) :
    Producer(name, runcontrol), _runNo(0), _eventNo(0), _fd(0), _running(false), _configured(false)
  {
    pthread_mutex_init(&_mufd,NULL);
  }

  void CaliceReceiver::OnConfigure(const eudaq::Configuration & param)
  {
    // file mode
    _filemode = param.Get("FileMode", false);
    // file name
    _filename = param.Get("FileName", "");
    _waitmsFile = param.Get("WaitMillisecForFile", 100);
    _waitsecondsForQueuedEvents = param.Get("waitsecondsForQueuedEvents", 5);

    // raw output
    _dumpRaw = param.Get("DumpRawOutput", 0);
    _writeRaw = param.Get("WriteRawOutput", 0);
    _rawFilename = param.Get("RawFileName", "run%d.raw");

    // port
    _port = param.Get("Port", 9011);
    _ipAddress = param.Get("IPAddress", "127.0.0.1");
    
    string reader = param.Get("Reader", "Silicon");

    int difId = param.Get("DIFID", 0);


    if(reader == "Silicon"){
      SetReader(new SiReader(this,difId));
      OpenConnection(); // for silicon we open at configuration
    }
    else {
      SetReader(new ScReader(this)); // in sc dif ID is not specified
    } // for scintillator we open at start
    
    _configured = true;

    cout << "Calice Receiver configured: FileMode = " << _filemode << ", file = " << _filename;
    cout << ", port = " << _port << ", ipAddr = " << _ipAddress << ", reader = " << _reader << endl;
    
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");

  }

  void CaliceReceiver::OnStartRun(unsigned param) {
      _runNo = param;
      _eventNo = 0;

      // raw file open
      if(_writeRaw){

	//	read the local time and save into the string myString
	time_t  ltime;
	struct tm *Tm;
	ltime=time(NULL);
	Tm=localtime(&ltime);
	char file_timestamp[25];
	sprintf(file_timestamp,"__%02dp%02dp%02d__%02dp%02dp%02d.raw",
		Tm->tm_mday,
		Tm->tm_mon+1,
		Tm->tm_year+1900,
		Tm->tm_hour,
		Tm->tm_min,
		Tm->tm_sec);
	std::string myString;
	myString.assign(file_timestamp, 26);
	
	std::string _rawFilenameTimeStamp;
	//add the local time to the filename
	_rawFilenameTimeStamp=_rawFilename+myString;
	
	char rawFilename[256];
	sprintf(rawFilename, _rawFilenameTimeStamp.c_str(), (int)param);
	_rawFile.open(rawFilename);
      }

      _reader->OnStart(param);

      SendEvent(RawDataEvent::BORE("Test", _runNo));
      std::cout << "Start Run: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "");
      _running = true;

  }

  void CaliceReceiver::OnPrepareRun(unsigned param) {
    cout << "OnPrepareRun: runID " << param << " set." << endl;
  }
  
  void CaliceReceiver::OnStopRun() {
    _reader->OnStop(_waitsecondsForQueuedEvents);
    _running = false;
    sleep(1); 
    // Fixed bug.
    // Error:  uncaught exception: Deserialize asked for X only have Y
    // sometimes appears when stopping the run and events are in the queue are being read
    // this crashes the Labview
    // seems to be a race condition, 
    // for the moment fixed with this extra time after (1s) of sleep after the stop.
    // following https://github.com/eudaq/eudaq/issues/29

    if(_writeRaw)
      _rawFile.close();
    
    SendEvent(RawDataEvent::EORE("Test", _runNo, _eventNo));
    
    SetStatus(eudaq::Status::LVL_OK, "");
  }

  void CaliceReceiver::OpenConnection()
  {
    if(_filemode){

      pthread_mutex_lock(&_mufd);
      _fd = open(_filename.c_str(), O_RDONLY);
      pthread_mutex_unlock(&_mufd);
      if(_fd <= 0){
        return;
      }
    }else{

      // open socket
      struct sockaddr_in dstAddr;
      memset(&dstAddr, 0, sizeof(dstAddr));
      dstAddr.sin_port = htons(_port);
      dstAddr.sin_family = AF_INET;
      dstAddr.sin_addr.s_addr = inet_addr(_ipAddress.c_str());
      _fd = socket(AF_INET, SOCK_STREAM, 0);

      pthread_mutex_lock(&_mufd);
      int ret = connect(_fd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
      pthread_mutex_unlock(&_mufd);

      if(ret != 0){
        return;
      }
    }
  }
  
  void CaliceReceiver::CloseConnection()
  {

      pthread_mutex_lock(&_mufd);
     close(_fd);
     _fd = 0;
     pthread_mutex_unlock(&_mufd);
     
    
  }
  
  // send a string without any handshaking
  void CaliceReceiver::SendCommand(const char *command, int size){

    if(size == 0)size = strlen(command);

    // maybe mutex lock is not needed because we're not conflict with
    // open/close of socket because it's considered to be called from 
    // event thread, which is same as thread of open/close the socket

    //pthread_mutex_lock(&_mufd);
    if(_fd <= 0)cout << "CaliceReceiver::SendCommand(): cannot send command because connection is not open." << endl;
    else write(_fd, command, size);
    //pthread_mutex_unlock(&_mufd);
  }
  
  void CaliceReceiver::MainLoop()
  {

    deque<char> bufRead;
    // deque for events: add one event when new acqId is arrived: to be determined in reader
    deque<eudaq::RawDataEvent *> deqEvent;

    while(true){

      // wait until configured and connected
      pthread_mutex_lock(&_mufd);

      const int bufsize = 4096;

      // copy to C array, then to vector
      char buf[bufsize];
      int size = 0;
      if(!_running && deqEvent.size()){

	// send all events
	while(deqEvent.size() > 0){
	  SendEvent(*(deqEvent.front()));
	  delete deqEvent.front();
	  deqEvent.pop_front();
	}
      }

      // file is not ready or not running in file mode: just wait
      if(_fd <= 0 || (!_running && _filemode)){
	pthread_mutex_unlock(&_mufd);
	::usleep(1000);
	continue;
      }

      // in file mode we need some wait
      if(_filemode) ::usleep(1000 * _waitmsFile);   // put some wait on filemode
        
      // system call: from socket to C array
      size = ::read(_fd, buf, bufsize);
      if(size == -1 || size == 0){
        if(size == -1)
          std::cout << "Error on read: " << errno << " Disconnect and going to the waiting mode." << endl;
        else
          std::cout << "Socket disconnected. going to the waiting mode." << endl;
          
        close(_fd);
        _fd = -1;
        pthread_mutex_unlock(&_mufd);

        if(_filemode)
          break;
        else
          continue;
      }
        
      if(!_running){
	bufRead.clear();
	pthread_mutex_unlock(&_mufd);
	continue;
      }

      if(_writeRaw && _rawFile.is_open()){
	_rawFile.write(buf, size);
      }
        
      // C array to vector
      copy(buf, buf + size, back_inserter(bufRead));
      //bufRead.append(buf, size);


      if(_reader){
	_reader->Read(bufRead, deqEvent);
      }
        
      // send events : remain the last event
      while(deqEvent.size() > 1){
	SendEvent(*(deqEvent.front()));
	delete deqEvent.front();
	deqEvent.pop_front();
      }


      
      pthread_mutex_unlock(&_mufd);

    }
  }

}

int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Calice Receiver Producer", "1.0",
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
    calice_eudaq::CaliceReceiver producer(name.Value(), rctrl.Value());
    //producer.SetReader(new calice_eudaq::SiReader(0));
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
