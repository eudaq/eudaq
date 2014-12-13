#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include "eudaq/Mutex.hh"
#include "DevicePalpidess.hh"

#include <sys/socket.h>

#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>


template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
  unsigned char * src =
    static_cast < unsigned char* >(static_cast < void * >(&data));
  dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (std::vector <unsigned char >& src, int index, T& data) {
  copy (&src[index], &src[index + sizeof (T)], &data);
}

typedef std::pair<std::string,std::string> config;

unsigned long extract_event_id(unsigned char* event) {
  unsigned long id = 0x0;
  for (int i=3; i>=0; --i) {
    id |= event[i] << (3 - i);
  }
  return id;
}

// A name to identify the raw data format of the events generate.

// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "pALPIDEssRaw";

// Declare a new class that inherits from eudaq::Producer
class PalpidessProducer : public eudaq::Producer {
public:

  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  PalpidessProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), running(false), done(false),
      request_sent(false) {
    fPalpidess= new DevicePalpidess();
  }

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & param) {
    std::cout << "Configuring: " << param.Name() << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "Wait");

    printf("\n--------------------- Get Config --------------------\n");
    //read from config
    options[0].first="PalpidessNumber";              //SetOptionTags
    options[1].first="Vrst";
    options[2].first="Vcasn";
    options[3].first="Vcasp";
    options[4].first="Ithr";
    options[5].first="Vlight";
    options[6].first="AcqTime";
    options[7].first="Vbb";
    options[8].first="TrigDelay";

    std::stringstream ss;
    const int PalpidessNum = param.Get(param_str[0], 0);
    ss << PalpidessNum;
    options[0].first  = param_str[0];
    options[0].second = ss.str();
    ss.str("");

    std::stringstream config_cmd_arg;
    for (int i_param = 1; i_param < n_param; ++i_param) {
      options[i_param].first  = param_str[i_param];
      options[i_param].second = param.Get(options[i_param].first, param_default[i_param]);
      ss << options[i_param].first << "_" << options[0].second;
      options[i_param].second = param.Get(ss.str(), options[i_param].second);
      std::cout << options[i_param].first << "=" << options[i_param].second
                << std::endl;
      ss.str("");
      config_cmd_arg << " " << options[i_param].second;
    }

    printf("\n--------------------- Start Servers --------------------\n");
    if (fPalpidess->get_SD() < 0) fPalpidess->create_server_udp();
    else printf("Server 0 for FEC HLVDS has already been up\n");

    printf("\n--------------------- Configure Palpidess --------------------\n");
    fPalpidess->Configure(config_cmd_arg.str());

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;

    printf("\n--------------------- Configure DAQ --------------------\n");
    fPalpidess->ConfigureDAQ();       //calls python script ConfigureDAQ.py

    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));

    //attache tags to bore
    for(int i=0;i<n_param;i++){ // TODO use constant here
      bore.SetTag(options[i].first,options[i].second);
    }

    printf("\n--------------------- Start Run: %d --------------------\n\n",
           m_run);

    // Send the event to the Data Collector
    SendEvent(bore);
    std::cout << "[pALPIDEssProducer] Sent BORE " << std::endl;

    stopping = false;
    request_sent = false;
    running = true;
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Running");
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;

    // Set a flag to signal to the polling loop that the run is over
    stopping = true;
    running = false;
    while (stopping) {
      eudaq::mSleep(100);
    }
    std::cout << "Stopped Run" << std::endl;
  }

///////////////////////////////////////////////////////////////////////////////
// This gets called when the Run Control is terminating and we should also exit
///////////////////////////////////////////////////////////////////////////////
  virtual void OnTerminate() {
    printf("\n---------------------- Terminating... ---------------------\n");

    fPalpidess->close_server_udp(0);              //close data servers
    done = true;

    eudaq::mSleep(1000);
  }

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    int sd; // socket Descriptor

    const unsigned int buffer_size = 9000;
    unsigned char data_packet[buffer_size]; // buffer for the UDP frame
    unsigned int num_byte_rcv = 0;          // number of received bytes
    std::vector<unsigned char> evt_size;    // number of bytes in the event,
                                            // including data, header and
                                            // frame sizes as byte vector
    bool evt_bad = false;                   // flag to indicate something
                                            // is wrong with the data
                                            // => discard event
    unsigned long evt_status = 0x0;         // indicates the frames received
    unsigned long evt_id     = 0x0;
    short max_frame_id       = 0x0;
    bool evt_finished        = false;
    bool request_sent        = false;

    float timeout = 0.1;    // (in s) will be only used if data is expected

    // Loop until Run Control tells us to terminate
    while (!done) {
      if (!running && !stopping) {          //called while run not started yet
        eudaq::mSleep(50);
        continue;
      }
      else if (running) {       //running
        while (!request_sent) {
          request_sent = fPalpidess->GetSingleEvent();
        }
        std::vector<unsigned char> bufferEvent_LVDS;              //Eventbuffer
        std::vector<unsigned char> bufferEvent_ADC;
        bufferEvent_LVDS.reserve(70000); // 65656 is necessary

        sd = fPalpidess->get_SD();     // get socket descriptor

        while(!check_socket(sd, timeout) && !stopping) {
          //std::cout << "Requested event - waiting for its arrival"
          //          << std::endl;
        }

        if (!stopping) {
          // LVDS event
          evt_finished = false;
          evt_bad      = false;
          evt_status   = 0x0;
          max_frame_id = -1;
          bufferEvent_LVDS.reserve(70000); // 65656 is necessary
          while ((!evt_finished || evt_bad) && check_socket(sd, timeout)) {
            // exploting the short-circuiting here!
            num_byte_rcv = recvfrom(sd, data_packet, buffer_size, 0, NULL, NULL);
            if (!evt_bad) {
              // append data to the event buffer
              pack(bufferEvent_LVDS, num_byte_rcv);  // add frame length first
              for( unsigned i = 0 ; i < num_byte_rcv ; ++i ){
                bufferEvent_LVDS.push_back(data_packet[i]);
              }
              // check event id
              if (evt_status == 0 && max_frame_id == -1)
                evt_id = extract_event_id(data_packet);
              else if (evt_id != extract_event_id(data_packet)) {
                evt_bad = true;
                std::cout << "** found wrong event id "
                          << extract_event_id(data_packet)
                          << " instead of " << evt_id
                          << " in the LVDS event " << std::endl;
              }
              // handle the frame id
              if (num_byte_rcv == 9) {
                max_frame_id = data_packet[11];
              }
              else {
                evt_status |= 1 << data_packet[11];
              }
              // check whether we are finished
              if (max_frame_id != -1 &&
                  evt_status == (unsigned long)(1 << (max_frame_id+1))-1)
                evt_finished = true;
            }
          }
          if (evt_finished) {
            // generate byte vector of the event size and prepend it to the data
            evt_size.clear();
            unsigned int s = bufferEvent_LVDS.size();
            pack(evt_size, s);
            //insert infront of the event data
            bufferEvent_LVDS.insert(bufferEvent_LVDS.begin(),
                                    evt_size.begin(), evt_size.end());
            // add event check
            // TODO TODO
          }
          else {
            std::cout << "** LVDS event not finished" << std::endl
                      << "** maximum frame id: " << max_frame_id << std::endl
                      << "** event status:     0x" << std::hex << evt_status
                      << std::dec << std::endl;
          }

          //Print number of Event
          if (m_ev<10 || m_ev%100==0)
            printf("\n----------------------Event # %d------------------\n",
                   m_ev);
          // Create a RawDataEvent to contain the event data to be sent
          eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

          fPalpidess->GetSingleEvent();
          if (!evt_bad) {
            ev.AddBlock(0, bufferEvent_LVDS);
          }
          // Send the event to the Data Collector
          SendEvent(ev);
          // Now increment the event number
          m_ev++;
        }
      }
      if (stopping) {
	//in case the run is stopped: finish event and send EORE
	std::cout << "[PalpidessProducer] Sent EORE " << std::endl;
	while(check_socket(sd, 1.)) { //read LVDS until empty
	  recvfrom(sd, data_packet, buffer_size, 0, NULL, NULL);
	}
	fPalpidess->StopDAQ();
	stopping = false;
	SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
	SetStatus(eudaq::Status::LVL_OK, "Stopped");
	printf("\n-------------------- Run %i Stopped --------------------\n",
	       m_run);
	request_sent = false;
      }
    } //end while
  }//end Readout loop

private:
  unsigned m_run, m_ev;         //run and event number of the instance
  bool stopping, running, done; //run information booleans
  bool request_sent;            // ensure that we don't send a double request to the DUT
  DevicePalpidess* fPalpidess;   //pointer to the device object

  //Config values
  static std::string const param_str[];
  static std::string const param_default[];
  static int const n_param = 9;
  config options[PalpidessProducer::n_param];

  //------------------------------------------------------------------------------------------------
  bool check_socket(int fd, float timeout) // timeout in s
    {
      fd_set fdset;
      FD_ZERO( &fdset );
      FD_SET( fd, &fdset );

      struct timeval tv_timeout = { (int)timeout, (int)(timeout*1000000.)%1000000 };

      int select_retval = select( fd+1, &fdset, NULL, NULL, &tv_timeout );

      if ( select_retval == -1 ) {
        std::cout << "Could not read from the socket!" << std::endl; // ERROR!
      }

      if ( FD_ISSET( fd, &fdset ) )
        return true; // ready to read

      return false; // timeout
    }
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ pALPIDEss Producer", "1.0",
                         "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "", "pALPIDEss", "string",
                                   "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    PalpidessProducer producer(name.Value(), rctrl.Value());
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

std::string const PalpidessProducer::param_str[] = { "PalpidessNumber", "Vrst", "Vcasn", "Vcasp", "Ithr", "Vlight", "AcqTime", "Vbb", "TrigDelay" }; // crosscheck n_param!
std::string const PalpidessProducer::param_default[] = { "0", "1.6", "0.4", "0.6", "2.05", "0.0", "1.54", "0.0", "0.0" }; // crosscheck n_param!
