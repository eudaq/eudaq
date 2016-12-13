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
    Producer(name, runcontrol), _runNo(0), _eventNo(0), _fd(0), _running(false),
    _stopped(true), _configured(false){
    m_id_stream = eudaq::cstr2hash(name.c_str());
  }

  void AHCALProducer::OnConfigure(const eudaq::Configuration & param)
  {

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
   
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    std::cout<< " END AHCAL congfiguration "<< std::endl;

  }

  void AHCALProducer::OnStartRun(unsigned param) {
    _runNo = param;
    _eventNo = 0;
    // raw file open
    if(_writeRaw) OpenRawFile(param, _writerawfilename_timestamp);

    _reader->OnStart(param);

    eudaq::RawDataEvent ev("CaliceObject", m_id_stream, _runNo, 0);
    ev.SetBORE();
    SendEvent(ev);
    std::cout << "Start Run: " << param << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "");
    _running = true;
    _stopped = false;

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

  
  void AHCALProducer::OnStopRun() {

    _reader->OnStop(_waitsecondsForQueuedEvents);
    _running = false;
    while (_stopped == false) {
      eudaq::mSleep(100);
    }

    if(_writeRaw)
      _rawFile.close();

    SetStatus(eudaq::Status::LVL_OK, "");
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
      
      RawDataEvent *ev = deqEvent.front();
      ev->SetStreamN(m_id_stream);
      if( from_string(ev->GetTag("TriggerValidated"),-1) == 1 )    {
	SendEvent(*(deqEvent.front()));
	cout<< "Send eventN="<<ev->GetEventNumber() << " with "<< ev->NumBlocks() <<" Blocks, and TriggerTag=" <<from_string(ev->GetTag("TriggerValidated"),-1)<< endl;
      } else cout<< "Discard eventN="<<ev->GetEventNumber() << " with "<< ev->NumBlocks() <<" Blocks, and TriggerTag=" <<from_string(ev->GetTag("TriggerValidated"),-1)<< endl;

      delete deqEvent.front();
      deqEvent.pop_front();
    }
    return deqEvent;
  }

  void AHCALProducer::Exec()
  {

    std::cout<<" Main loop " <<std::endl;
    
    _last_readout_time = std::time(NULL);
	 
    deque<char> bufRead;
    // deque for events: add one event when new acqId is arrived: to be determined in reader
    deque<eudaq::RawDataEvent *> deqEvent;

    while(true){
      // wait until configured and connected
      std::unique_lock<std::mutex> myLock(_mufd);

      const int bufsize = 4096;
      // copy to C array, then to vector
      char buf[bufsize];
      int size = 0;        

      // if(!_running && deqEvent.size()) deqEvent=sendallevents(deqEvent,0);

      //      if file is not ready  just wait
      if(_fd <= 0 || !_running ){
	myLock.unlock();
	::usleep(1000);
	continue;
      }
       
      // system call: from socket to C array
      size = ::read(_fd, buf, bufsize);
      if(size == -1 || size == 0){
        if(size == -1)
          std::cout << "Error on read: " << errno << " Disconnect and going to the waiting mode." << endl;
        else {
          std::cout << "Socket disconnected. going to the waiting mode." << endl;         	    
	  if(!_running && _stopped==false) {
	    _stopped=true;
	    deqEvent=sendallevents(deqEvent,0);
	    
	    eudaq::RawDataEvent ev("CaliceObject", m_id_stream, _runNo, _eventNo);
	    ev.SetEORE();
	    SendEvent(ev);
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

      if(_reader)_reader->Read(bufRead, deqEvent);
        
      // send events : remain the last event
      deqEvent=sendallevents(deqEvent,1);

     if(!_running){

       if (std::difftime(std::time(NULL), _last_readout_time) <  _waitsecondsForQueuedEvents) continue;
	bufRead.clear();

	eudaq::RawDataEvent ev("CaliceObject", m_id_stream, _runNo, _eventNo);
	ev.SetEORE();
	SendEvent(ev);
	SetStatus(eudaq::Status::LVL_OK, "");
	continue;
     }
    }
  }

}//end namespace eudaq

