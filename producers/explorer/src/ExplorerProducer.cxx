#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/ExampleHardware.hh"
#include "eudaq/Mutex.hh"
#include "DeviceExplorer.hh"

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

uint32_t extract_event_id(unsigned char* event) {
  uint32_t id = 0x0;
  for (int i=3; i>=0; --i) {
    id |= event[i] << (3 - i);
  }
  return id;
}

// A name to identify the raw data format of the events generate.

// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "EXPLORER1Raw";

// Declare a new class that inherits from eudaq::Producer
class ExplorerProducer : public eudaq::Producer {
public:

  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  ExplorerProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), running(false), done(false),
      request_sent(false) {
    fExplorer1= new DeviceExplorer();
  }

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & param) {
    std::cout << "Configuring: " << param.Name() << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "Wait");

    printf("\n--------------------- Get Config --------------------\n");
    //read from config
    options[0].first="ExplorerNumber";              //SetOptionTags
    options[1].first="PedestalMeasurement";
    options[2].first="UsePedestal";
    options[3].first="PedestalFileToUse";
    options[4].first="UseThreshold";
    options[5].first="Threshold";

    //GetTags
    std::stringstream ss;
    ExplorerNum       = param.Get("ExplorerNumber", 0);
    ss << ExplorerNum;
    options[0].second = ss.str();
    ss.str("");

    PedestalMeas      = param.Get("PedestalMeasurement", false);
    ss << PedestalMeas;
    options[1].second = ss.str();
    ss.str("");

    UsePedestal       = param.Get("UsePedestal", false);
    ss << UsePedestal;
    options[2].second = ss.str();
    ss.str("");

    PedestalFile      = param.Get("PedestalFileToUse", "pedestal0.ped");
    PedestalFile      = "../data/"+PedestalFile;
    //saved @end of if/else further down

    UseThreshold      = param.Get("UseThreshold", true);
    ss << UseThreshold;
    options[4].second = ss.str();
    ss.str("");

    Threshold         = param.Get("Threshold", 10);
    ss << Threshold;
    options[5].second = ss.str();
    ss.str("");

    //output for the xterm
    std::cout << std::endl
              << "Parameters:" << std::endl
              << "Explorer Chip:\t\t\t" << ExplorerNum << std::endl
              << "Pedestal Measurement:\t\t"
              << (PedestalMeas == 1 ? "Yes" : "No" ) << std::endl
              << "Use Pedestal?\t\t\t" << (UsePedestal == 1 ? "Yes" : "No ")
              << std::endl
              << "Pedestal Filename(.ped):\t" << PedestalFile << std::endl
              << "Use Threshold?\t\t\t" << (UseThreshold == 1 ? "Yes" : "No" )
              << std::endl
              << "Threshold (ADC-Counts):\t\t" << Threshold << std::endl
              << std::endl;
    //Checks if everything existing and Making sense
    int nfile=-1;
    std::string filename;
    bool good=true;

    //Check logic of input -->Re-Set Tags
    if(PedestalMeas){
      //check file existence
      do{
        nfile++;
        std::stringstream ss2;
        ss2 << nfile;
        std::string str = ss2.str();
        filename= "../data/pedestal"+str+".ped";
        std::ifstream in(filename.c_str());
        good=in.good();
      } while(good);
      //create new filename
      PedestalFile=filename;
      std::cout << std::endl << "New PedestalFile: " << std::endl
                << PedestalFile << std::endl;
      //error catch and re-set
      if(UsePedestal){
        std::cout << "Use of Pedestal not possible!" << std::endl
                  << "This is a Pedestal Measurement!" << std::endl
                  << "Setting UsePedestal to false!" << std::endl
                  << std::endl;
        UsePedestal=false;
      }

      if(UseThreshold){
        std::cout <<"Use of Threshold not possible!" << std::endl
                  <<"This is a Pedestal Measurement!" << std::endl
                  <<"Setting UseThreshold to false!" << std::endl
                  << std::endl;
        UseThreshold=false;
      }
    }

    else{
      //check for file existence
      if(UsePedestal){
        std::ifstream in(PedestalFile.c_str());
        if(!in.good()){
          std::cout << "No Pedestal File Found!" << std::endl
                    << "Switching to Threshold" << std::endl
                    << std::endl;
          UsePedestal=false;
          UseThreshold=true;
        }
      }
      else{
        //threshold case
        if(!UseThreshold){
          std::cout << "No measure method choosen!" << std::endl
                    << "Switching to Threshold" << std::endl
                    << std::endl;
          UseThreshold=true;
          Threshold=0;
        }
      }
    }
    options[3].second       =   PedestalFile;       //Save new name

    printf("\n--------------------- Start Servers --------------------\n");
    if (fExplorer1->get_SD(0) < 0) {
      fExplorer1->create_server_udp(0);              //Fec0(HLVDS)
    }
    else printf("Server 0 for FEC HLVDS has already been up\n");
    if (fExplorer1->get_SD(1) < 0) {
      fExplorer1->create_server_udp(1);              //Fec1(ADC)
    }
    else printf("Server 1 for FEC ADC has already been up\n");

    printf("\n--------------------- Configure Explorer --------------------\n");
    fExplorer1->Configure();              //calls python script ConfigureExplorer.py

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;

    printf("\n--------------------- Configure DAQ --------------------\n");
    fExplorer1->ConfigureDAQ();       //calls python script ConfigureDAQ.py

    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));

    //attache tags to bore
    for(int i=0;i<6;i++){
      bore.SetTag(options[i].first,options[i].second);
    }

    printf("\n--------------------- Start Run: %d --------------------\n\n",
           m_run);

    // Send the event to the Data Collector
    SendEvent(bore);
    std::cout << "[ExplorerProducer] Sent BORE " << std::endl;

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

    fExplorer1->close_server_udp(0);              //close data servers
    fExplorer1->close_server_udp(1);
    done = true;

    eudaq::mSleep(1000);
  }

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    int sd0, sd1; // socket Descriptor

    const unsigned int buffer_size = 9000;
    unsigned char data_packet[buffer_size]; // buffer for the UDP frame
    unsigned int num_byte_rcv = 0;          // number of received bytes
    std::vector<unsigned char> evt_size;    // number of bytes in the event,
                                            // including data, header and
                                            // frame sizes as byte vector
    bool evt_bad = false;                   // flag to indicate something
                                            // is wrong with the data
                                            // => discard event
    uint32_t evt_status = 0x0;         // indicates the frames received
    uint32_t evt_id     = 0x0;
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
          request_sent = fExplorer1->GetSingleEvent();
        }
        std::vector<unsigned char> bufferEvent_LVDS;              //Eventbuffer
        std::vector<unsigned char> bufferEvent_ADC;
        bufferEvent_LVDS.reserve(100); // 57 is necessary
        bufferEvent_ADC.reserve(140800); // upt o 140703 is necessary

        sd0 = fExplorer1->get_SD(0);                     //socketdescriptors
        sd1 = fExplorer1->get_SD(1);

        while(!check_socket(sd0, timeout) && !stopping) {
          //std::cout << "Requested event - waiting for its arrival"
          //          << std::endl;
        }

        if (!stopping) {
          // LVDS event
          evt_finished = false;
          evt_bad      = false;
          evt_status   = 0x0;
          max_frame_id = -1;
          bufferEvent_LVDS.reserve(100); // 57 is necessary
          while ((!evt_finished || evt_bad) && check_socket(sd0, timeout)) {
            // exploting the short-circuiting here!
            num_byte_rcv = recvfrom(sd0, data_packet, buffer_size, 0, NULL, NULL);
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
                  evt_status == (uint32_t)(1 << (max_frame_id+1))-1)
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
            if (57 != bufferEvent_LVDS.size()) { // 36+4 + 9+4 + 4
              std::cout << "** LVDS event size " << bufferEvent_LVDS.size()
                        << " instead of 57" << std::endl;
              evt_bad = true;
            }
          }
          else {
            std::cout << "** LVDS event not finished" << std::endl
                      << "** maximum frame id: " << max_frame_id << std::endl
                      << "** event status:     0x" << std::hex << evt_status
                      << std::dec << std::endl;
          }
          // ADC event
          evt_finished = false;
          evt_status   = 0x0;
          max_frame_id = -1;
          bufferEvent_ADC.reserve(140800); // upt o 140703 is necessary
          while ((!evt_finished || evt_bad) && check_socket(sd1, timeout)) {
            num_byte_rcv = recvfrom(sd1, data_packet, buffer_size, 0, NULL, NULL);
            //very explicit cuts !!!!Data Format!!!!!
            if (num_byte_rcv!=9 && num_byte_rcv!=8844 && num_byte_rcv!=7962 &&
                num_byte_rcv!=7956) // 7962 old firmware, 7956 for the new
              evt_bad = true;
            if (!evt_bad) {
              //pack data to buffer
              pack(bufferEvent_ADC, num_byte_rcv); // add frame length first
              for (unsigned int i = 0 ; i < num_byte_rcv ; ++i){
                bufferEvent_ADC.push_back(data_packet[i]);
              }
              // check event id (based on the one from the LVDS event)
              if (evt_id != extract_event_id(data_packet)) {
                evt_bad = true;
                std::cout << "** found wrong event id "
                          << extract_event_id(data_packet)
                          << " instead of " << evt_id
                          << " in the ADC event " << std::endl;
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
                  evt_status == (uint32_t)(1 <<(max_frame_id+1))-1)
                evt_finished = true;
            }
          }
          if (evt_finished) {
            // generate byte vector of the event size and prepend it to the data
            evt_size.clear();
            unsigned int s = (unsigned int)bufferEvent_ADC.size();
            pack(evt_size, s);
            //insert infront of the event data
            bufferEvent_ADC.insert(bufferEvent_ADC.begin(),
                                   evt_size.begin(), evt_size.end());

            if (140703!=bufferEvent_ADC.size() &&
                140697!=bufferEvent_ADC.size()) {
              // 15*8844+15*4 + 7962+4 + 9+4 + 4 // old
              // 15*8844+15*4 + 7956+4 + 9+4 + 4 // new
              std::cout << "** ADC event size " << bufferEvent_ADC.size()
                        << " instead of 140697 (140703)" << std::endl;
              evt_bad = true;
            }
          }
          else {
            std::cout << "** ADC event not finished" << std::endl
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

          fExplorer1->GetSingleEvent();
          if (!evt_bad) {
            ev.AddBlock(0, bufferEvent_LVDS);
            ev.AddBlock(1, bufferEvent_ADC);
          }
          // Send the event to the Data Collector
          SendEvent(ev);
          // Now increment the event number
          m_ev++;
        }
      }
      if (stopping) {
	//in case the run is stopped: finish event and send EORE
	std::cout << "[ExplorerProducer] Sent EORE " << std::endl;
	while(check_socket(sd0, 1.)) { //read LVDS until empty
	  recvfrom(sd0, data_packet, buffer_size, 0, NULL, NULL);
	}
	while(check_socket(sd1, 1.)) { //read ADC until empty
	  recvfrom(sd1, data_packet, buffer_size, 0, NULL, NULL);
	}
	fExplorer1->StopDAQ();
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

  unsigned m_run, m_ev;         //run and event number of the instace
  char ip[14];                  //ip string
  bool stopping, running, done; //run information booleans
  bool request_sent;            // ensure that we don't send a double request to the DUT
  DeviceExplorer* fExplorer1;   //pointer to the device object

  //Config values
  unsigned int ExplorerNum;
  bool PedestalMeas;
  std::string PedestalFile;
  bool UsePedestal;
  unsigned int Threshold;
  bool UseThreshold;
  config options[6];


  bool check_socket(int fd, float timeout) // timeout in s
    {
      fd_set fdset;
      FD_ZERO( &fdset );
      FD_SET( fd, &fdset );

      struct timeval tv_timeout = { (int)timeout, static_cast<suseconds_t>(timeout*1000000) };

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
  eudaq::OptionParser op("EUDAQ Explorer Producer", "1.0",
                         "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "", "Explorer1", "string",
                                   "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    ExplorerProducer producer(name.Value(), rctrl.Value());
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
