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
    Producer(name, runcontrol), _runNo(0), _eventNo(0), _fd(0), _running(false),
    _stopped(true), _configured(false){
    m_id_stream = eudaq::cstr2hash(name.c_str());
  }

  void AHCALProducer::DoConfigure(){
    const eudaq::Configuration &param = *GetConfiguration();
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

  void AHCALProducer::DoStartRun() {
    _runNo = GetRunNumber();
    _eventNo = -1;
    // raw file open
    if(_writeRaw) OpenRawFile(_runNo, _writerawfilename_timestamp);
    std::cout << "AHCALProducer::OnStartRun _reader->OnStart(param);" << std::endl; //DEBUG
    _reader->OnStart(_runNo);
    std::cout << "AHCALProducer::OnStartRun _SendEvent(RawDataEvent::BORE(CaliceObject, _runNo));"
	      << std::endl; //DEBUG
    auto ev = eudaq::RawDataEvent::MakeUnique("CaliceObject");
    ev->SetBORE();
    SendEvent(std::move(ev));
    std::cout << "Start Run: " << _runNo << std::endl;
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

  
  void AHCALProducer::DoStopRun() {
          std::cout << "AHCALProducer::OnStopRun:  Stop run" << std::endl;
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

      std::cout << "AHCALProducer::OnStopRun sending EORE event with _eventNo"
		<< _eventNo << std::endl;
      //SendEvent(RawDataEvent::EORE("CaliceObject", _runNo, _eventNo));
      auto ev = eudaq::RawDataEvent::MakeUnique("CaliceObject"); 
      ev->SetEORE();
      SendEvent(std::move(ev));
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
#ifdef _WIN32
    closesocket(_fd);
#else
    close(_fd);
#endif
    _fd = 0;
  }

   
  // send a string without any handshaking
  void AHCALProducer::SendCommand(const char *command, int size){

    if(size == 0)size = strlen(command);

    if(_fd <= 0)
      cout << "AHCALProducer::SendCommand(): cannot send command because connection is not open." << endl;
    else {
	    size_t bytesWritten = write(_fd, command, size);
	    if ( bytesWritten  < 0 ) {
		    cout << "There was an error writing to the TCP socket" << endl;
	    } else {
		    cout << bytesWritten  << " out of " << size << " bytes is  written to the TCP socket" << endl;
	    }
    }

  }

  deque<eudaq::RawDataEvent *> AHCALProducer::sendallevents(deque<eudaq::RawDataEvent *> deqEvent, int minimumsize) {
    while (deqEvent.size() > minimumsize) {
      RawDataEvent *ev = deqEvent.front();
      if (from_string(ev->GetTag("TriggerValidated"), -1) == 1) {
	if (ev->GetEventNumber() != (_eventNo + 1)) {
	  EUDAQ_WARN("Run "+to_string(_runNo)+" Event " + to_string(ev->GetEventNumber()) + " not in sequence. Expected " + to_string(_eventNo + 1));

	  //fix for a problem of 2 triggers, that came in the same ROC
	  if (ev->GetEventNumber() == (_eventNo + 2)) {
	    EUDAQ_WARN(" Sending event " + to_string(ev->GetEventNumber()) + " twice");
	    // SendEvent(*(deqEvent.front()));
	    SendEvent(deqEvent.front()->Clone());
	  } else {
	    int jump = (ev->GetEventNumber() - (_eventNo + 1));
	    EUDAQ_ERROR("Run "+to_string(_runNo)+" Event "+ to_string(ev->GetEventNumber()) + " cannot be fixed by sending the packet twice. Problem more complex, which is not possible to fix a jump by " + to_string(jump) );
	  }
	}

	    
	_eventNo = ev->GetEventNumber(); //save the last event number
	// SendEvent(*(deqEvent.front()));
	SendEvent(deqEvent.front()->Clone());
	if (from_string(ev->GetTag("TriggerInvalid"), -1) == 1) {
	  cout << "Send DUMMY eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
	} else {
	  cout << "Send eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
	}
      } else
	cout << "Discard eventN=" << ev->GetEventNumber() << " with " << ev->NumBlocks() << " Blocks, and TriggerTag=" << from_string(ev->GetTag("TriggerValidated"), -1) << endl;
      delete deqEvent.front();
      deqEvent.pop_front();
    }
    return deqEvent;
  }


  void AHCALProducer::Exec(){    
    std::cout << " Main loop " << std::endl;
    StartCommandReceiver();
    deque<char> bufRead;
    // deque for events: add one event when new acqId is arrived: to be determined in reader
    deque<eudaq::RawDataEvent *> deqEvent;

    while (true) {
      // wait until configured and connected
      std::unique_lock<std::mutex> myLock(_mufd);

      const int bufsize = 4096;
      // copy to C array, then to vector
      char buf[bufsize];
      int size = 0;

      if (_fd <= 0 || _stopped) {
	myLock.unlock();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	continue;
      }
      size = ::read(_fd, buf, bufsize);
      if (size) {
	_last_readout_time = std::time(NULL);
	if (_writeRaw && _rawFile.is_open())
	  _rawFile.write(buf, size);
	// C array to vector
	copy(buf, buf + size, back_inserter(bufRead));
	if (_reader)
	  _reader->Read(bufRead, deqEvent);
	// send events : remain the last event
	deqEvent = sendallevents(deqEvent, 1);
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
	_stopped = 1;
	deqEvent = sendallevents(deqEvent, 0);
	bufRead.clear();
	deqEvent.clear();
      }
      if (!_running) {
      }
    }
  }

}//end namespace eudaq

