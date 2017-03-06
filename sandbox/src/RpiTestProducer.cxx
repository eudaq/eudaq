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
      if (!con)
	EUDAQ_WARN("Open conection failed");
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
      
      CloseConnection();
      
      m_running=false;
      // Set a flag to signal to the polling loop that the run is over
      m_stopping = true;

      // wait until all events have been read out from the hardware
      while (m_stopping) {
        eudaq::mSleep(20);
      }

      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

      m_stopped = true;
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
	
	if (m_stopping)
	  m_stopping = false;
	  
	if (!m_running)
	  {
	    eudaq::mSleep(2000);
	    continue;
	  }

	
	// APZ: I'm not sure what those locks are for.
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
	
	struct sockaddr_in cli_addr;

	socklen_t cli_len = sizeof(cli_addr);

	// This call is blocking by default. This would prevent us stopping the run:
	m_cli_sockfd = accept(m_sockfd, (struct sockaddr *) &cli_addr, &cli_len);
	if (m_cli_sockfd < 0)
	  EUDAQ_ERROR("Sockets: ERROR on accept");
	
	std::cout<<" after accept. fd="<<m_cli_sockfd<<std::endl;

	const int bufsize = 4096;
	char buffer[bufsize];
	bzero(buffer, bufsize);


	int n = read(m_cli_sockfd, buffer, bufsize);
	if (n < 0) EUDAQ_ERROR("Sockets: ERROR reading from socket");
	
	std::cout<<" after read. n="<<n<<std::endl;

	// If we get here, there must be data to read out
	// Create a RawDataEvent to contain the event data to be sent
	eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);	  

	printf("In ReadoutLoop.  Here is the message from client: %s\n",buffer);

	m_last_readout_time = std::time(NULL);

	  
	std::cout <<"size = "<<n<< "  m_last_readout_time:"<< m_last_readout_time
		  <<" buf = "<<buffer<<"  client Socket fd="<<m_cli_sockfd<< std::endl;
	
	n = write(m_cli_sockfd,"Server: I got your message",26);

	if (n < 0) EUDAQ_ERROR("Sockets: ERROR writing to socket");
	else EUDAQ_EXTRA("Sent confirmation to client");

	close(m_cli_sockfd);
	EUDAQ_EXTRA("Closed the client's fd");
		
	eudaq::mSleep(1000);

	//if (_writeRaw && _rawFile.is_open()) _rawFile.write(buf, size);
	// C array to vector
	//copy(buf, buf + sk_size, back_inserter(bufRead));
	
	//if (_reader) _reader->Read(bufRead, deqEvent);
	
	// send events : remain the last event
	//deqEvent = sendallevents(deqEvent, 1);
	
	//std::vector<unsigned char> buffer;
	
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
      // We are going to set up this producer as SERVER
      // Using this tutorial as guidence: http://www.linuxhowtos.org/C_C++/socket.htm

      m_sockfd = socket(AF_INET, SOCK_STREAM, 0);

      if (m_sockfd<0) { 
	SetStatus(eudaq::Status::LVL_ERROR, "No Socket.");
	EUDAQ_ERROR("Can't open socket. m_sockfd="+eudaq::to_string(m_sockfd));
	return 0;
      }
      
      struct sockaddr_in serv_addr;
      bzero((char *) &serv_addr, sizeof(serv_addr));
      
      int m_port = 55511;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(m_port);

      if (bind(m_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	EUDAQ_ERROR("Sockets: ERROR on binding");
      
      listen(m_sockfd,5);

      /* -->> The code here is for the case we use TCP client here 
      int ret = connect(m_sockfd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
      if (ret != 0 || m_sockfd<0) { 
	SetStatus(eudaq::Status::LVL_WARN, "No Socket.");
	EUDAG_WARN("Can't open socket. ret="+to_string(ret)+"  dockfd="+m_sockfd);
	return 0;
      }
      <<-- */
      
      std::cout << "  Opened TCP socket: "<<m_sockfd << std::endl;
      return 1;
    }


    void CloseConnection()
    {
      //std::unique_lock<std::mutex> myLock(m_mufd);
      close(m_cli_sockfd);
      close(m_sockfd);
      m_sockfd = 0;

      std::cout << "  Closed TCP socket: "<<m_sockfd << std::endl;

    }


  private:
    unsigned m_run, m_ev, m_exampleparam;
    unsigned m_ski;
    bool m_stopping, m_stopped, m_done, m_started, m_running, m_configured;
    int m_sockfd, m_cli_sockfd; //TCP socket connection file descriptors (fd)
    //std::mutex m_mufd; 

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
  eudaq::Option<std::string> name (op, "n", "name", "RPI_v0", "string",
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
