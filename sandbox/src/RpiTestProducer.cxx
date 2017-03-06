#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <vector>

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>


// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "RPI";

// Declare a new class that inherits from eudaq::Producer
class RpiTestProducer : public eudaq::Producer {
  public:

    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    RpiTestProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol),
	m_run(0), m_ev(0), m_stopping(false), m_done(false),
	m_sockfd(0), m_running(false), m_configured(false){}

    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config) {
      std::cout << "Configuring: " << config.Name() << std::endl;

      // Do any configuration of the hardware here
      // Configuration file values are accessible as config.Get(name, default)
      m_exampleparam = config.Get("Parameter", 0);
      m_ski = config.Get("Ski", 0);

      m_port = config.Get("port", 55511);
      m_rpi_1_ip = config.Get("RPI_1_IP", "127.0.0.1");

      std::cout << "Example Parameter = " << m_exampleparam << std::endl;
      std::cout << "Example SKI Parameter = " << m_ski << std::endl;

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

      m_configured=true;
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;

      std::cout << "Start Run: " << m_run << std::endl;

      // openinig socket:

      bool con = OpenConnection();
      if (!con) {
	EUDAQ_WARN("Socket conection failed: no server. Can't start the run.");
	SetStatus(eudaq::Status::LVL_ERROR, "No Socket.");
	return;
      }

      SendCommand("START_RUN");

      char answer[20];
      bzero(answer, 20);
      int n = recv(m_sockfd, answer, 20, 0);
      if (n <= 0) {
	std::cout<<"Something is wrong, we can't start the run..."<<std::endl;
	SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	return;
      }
      
      else {
	std::cout<<"Answer to START_RUN: "<<answer<<std::endl;
	if (answer!="GOOD_START"){
	  std::cout<<"Something is wrong, we can't start the run..."<<std::endl;
	  SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	  return;
	}
      }
      // If we're here, then the Run was started on the Hardware side (TCP server will send data)
      
      // It must send a BORE to the Data Collector
      eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      // You can set tags on the BORE that will be saved in the data file
      // and can be used later to help decoding
      bore.SetTag("MyTag", eudaq::to_string(m_exampleparam));
      // Send the event to the Data Collector
      SendEvent(bore);

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Running");
      m_running=true;
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      std::cout << "Stopping Run" << std::endl;

      SendCommand("STOP_RUN");

      if (!m_running){
	// If we're not running, them we need to catch the confirmation here
	// Otherwise it will be caught in the ReadOut loop (hopefully...)

	while (!m_stopped){
	  char answer[20];
	  bzero(answer, 20);
	  int n = recv(m_sockfd, answer, 20, 0);
	  if (n <= 0) {
	    SetStatus(eudaq::Status::LVL_ERROR, "Can't Stop Run on Hardware side.");
	    return;
	  }
	  else {
	    std::cout<<"Answer to STOP_RUN: "<<answer<<std::endl;
	    if (answer!="STOPPED_RUN"){
	      std::cout<<"Something is wrong, we can't stop the run..."<<std::endl;
	      SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	      return;
	    }
	    m_stopped;
	  }
	}
      }
      m_running=false;
      // Set a flag to signal to the polling loop that the run is over
      m_stopping = true;

      // wait until all events have been read out from the hardware
      while (m_stopping) {
	SetStatus(eudaq::Status::LVL_OK, "Stopping...");
        eudaq::mSleep(20);
      }

      m_stopped = true;
      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

      SetStatus(eudaq::Status::LVL_OK, "Stopped.");
      std::cout << "Stopped it!" << std::endl;

    }

    // This gets called when the Run Control is terminating,
    // we should also exit.
    virtual void OnTerminate() {
      SetStatus(eudaq::Status::LVL_OK, "Terminating...");
      std::cout << "Terminating..." << std::endl;
      m_done = true;
    }

    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
      // Loop until Run Control tells us to terminate
      while (!m_done) {

	if (!m_running)
	  {
	    eudaq::mSleep(2000);
	    EUDAQ_EXTRA("Not Running; but sleeping");
	    continue;
	  }


	// APZ: I'm not sure what those mutex locks are for.
	// Tell me if you understand this:
	//std::unique_lock<std::mutex> myLock(m_mufd);

        if (m_sockfd <= 0 || m_stopping) {
	  //myLock.unlock();
	  eudaq::mSleep(200);
	  continue;
	}

	// **********
	// If we are below this point, we listen for data
	// ***********


	// This call is blocking by default. This would prevent us stopping the run:
	//m_cli_sockfd = accept(m_sockfd, (struct sockaddr *) &cli_addr, &cli_len);
	//if (m_cli_sockfd < 0)
	//EUDAQ_ERROR("Sockets: ERROR on accept");
	//std::cout<<" after accept. fd="<<m_cli_sockfd<<std::endl;

	const int bufsize = 4096;
	char buffer[bufsize];
	bzero(buffer, bufsize);

	int n = recv(m_sockfd, buffer, bufsize, 0);
	if (n <= 0) {
	  if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
	    std::cout<<"Sockets timeout?  errno="<<errno<<std::endl;
	    // No need to stop the run, just wait
	    continue;
	  }
	  else {
	    std::cout<<"Sockets timeout?  n="<<n<<" errno="<<errno<<std::endl;
	    SetStatus(eudaq::Status::LVL_WARN, "Nothing to read from socket...");
	    EUDAQ_WARN("Sockets: ERROR reading from socket (it's probably disconnected)");


	    // Let's now close connection to the socket and stop the readout loop
	    //CloseConnection();
	    // m_stopped = 1;
	    //m_running = 0;
	    continue;
	  }
	}

	// We are here if there is data and it's size is > 0

	std::cout<<" After recv. n="<<n<<std::endl;
	std::cout<<"In ReadoutLoop.  Here is the message from Server: %s\n"<<buffer<<std::endl;

	if (m_stopping){
	  // We have sent STOP_RUN command, let's see if we receive a confirmation:
	  if ("STOPPED_OK" == buffer){
	    
	    CloseConnection();
	    m_stopping = false;
	    
	    eudaq::mSleep(100);
	    continue;
	  }
	  // If not, then it's probably the data continuing to come, let's save it.
	}


	// If we get here, there must be data to read out
	m_last_readout_time = std::time(NULL);
	std::cout <<"size = "<<n<< "  m_last_readout_time:"<< m_last_readout_time
		  <<" buf = "<<buffer<<std::endl;

	// Create a RawDataEvent to contain the event data to be sent
	eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

	//n = write(m_sockfd,"Test",4);
	//if (n < 0) EUDAQ_ERROR("Sockets: ERROR writing to socket");
	//else EUDAQ_EXTRA("Sent confirmation to client");

	eudaq::mSleep(1000);

	//if (_writeRaw && _rawFile.is_open()) _rawFile.write(buf, size);
	// C array to vector
	//copy(buf, buf + sk_size, back_inserter(bufRead));

	//string s = "HGCDAQv1";
	//ev->AddBlock(0,s.c_str(), s.length());
	//ev->AddBlock(1, vector<int>()); // dummy block

	SendEvent(ev);
	m_ev++;

	continue;


	/*
		if (sk_size == -1)
		std::cout << "Error on read: " << errno << " Disconnect and going to the waiting mode." << std::endl;
		else
		std::cout << "Socket disconnected. going to the waiting mode." << std::endl;
		close(m_sockfd);
		m_sockfd = -1;
		m_stopping = 1;
	*/

      }// end of while(done) loop

    } // end of ReadLoop

    bool OpenConnection()
    {
      // This opens a TCP socket connection (available for both in-data and out-data)
      // We are going to set up this producer as CLIENT
      // Using this tutorial as guidence: http://www.linuxhowtos.org/C_C++/socket.htm
      // and code from:
      // https://github.com/EUDAQforLC/eudaq/blob/ahcal_telescope_december2016_v1.6/producers/calice/AHCAL/src/AHCALProducer.cc

      m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (m_sockfd<0) {
	EUDAQ_ERROR("Can't open socket: m_sockfd="+eudaq::to_string(m_sockfd));
	return 0;
      }

      struct sockaddr_in dst_addr;
      bzero((char *) &dst_addr, sizeof(dst_addr));
      dst_addr.sin_family = AF_INET;
      dst_addr.sin_addr.s_addr = inet_addr(m_rpi_1_ip.c_str());
      dst_addr.sin_port = htons(m_port);

      int ret = connect(m_sockfd, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
      if (ret != 0) {
	SetStatus(eudaq::Status::LVL_WARN, "No Socket.");
	EUDAQ_WARN("Can't connect() to socket: ret="+eudaq::to_string(ret)+"  sockfd="+eudaq::to_string(m_sockfd));
	return 0;
      }

      // ***********************
      // This makes the recv() command non-blocking.
      // After the timeout, it will get an error which we can catch and continue the loops
      struct timeval timeout;
      timeout.tv_sec = 20;
      timeout.tv_usec = 0;
      setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      //****************

      std::cout << "  Opened TCP socket: "<<m_sockfd << std::endl;
      return 1;
    }


    void CloseConnection()
    {
      //std::unique_lock<std::mutex> myLock(m_mufd);
      close(m_sockfd);
      std::cout << "  Closed TCP socket: "<<m_sockfd << std::endl;
      m_sockfd = -1;

    }

    void SendCommand(const char *command, int size=0) {
      
      if (size == 0) size = strlen(command);
      
      if (m_sockfd <= 0)
	std::cout << "SendCommand(): cannot send command because connection is not open." << std::endl;
      else {
	size_t bytesWritten = write(m_sockfd, command, size);
	if (bytesWritten < 0)
	  std::cout << "There was an error writing to the TCP socket" << std::endl;
	else
	  std::cout << bytesWritten << " out of " << size << " bytes is  written to the TCP socket" << std::endl;
      }
    }
      
  private:
    unsigned m_run, m_ev, m_exampleparam;
    unsigned m_ski;
    bool m_stopping, m_stopped, m_done, m_started, m_running, m_configured;
    int m_sockfd, m_cli_sockfd; //TCP socket connection file descriptors (fd)
    //std::mutex m_mufd;

    std::string m_rpi_1_ip;
    int m_port;

    std::time_t m_last_readout_time;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("RPI Test producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "RPI", "string",
      "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    RpiTestProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
