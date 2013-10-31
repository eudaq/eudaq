#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include "eudaq/Mutex.hh"
#include "eudaq/DeviceMimosa32.hh"

#include <iostream>
#include <ostream>
#include <vector>

template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
  unsigned char * src = static_cast < unsigned char* >(static_cast < void * >(&data));
  dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (std::vector <unsigned char >& src, int index, T& data) {
  copy (&src[index], &src[index + sizeof (T)], &data);
}

// A name to identify the raw data format of the events generate.

// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "MIMOSA32Raw";

// Declare a new class that inherits from eudaq::Producer
class Mimosa32Producer : public eudaq::Producer {
  public:

    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    Mimosa32Producer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), done(false) {
        fMimosa32= new DeviceMimosa32();

	running = false;
        }
    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config) {
      std::cout << "Configuring: " << config.Name() << std::endl;
      port=5000;
      sprintf(ip,"192.168.2.103");
      num_chip=1;
      fSD=fMimosa32->create_client_tcp(port,ip);
      fMimosa32->connect_client_tcp(fSD);
      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Configured");
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      last_event=0;
      if (num_chip==1) num_word=652;
      //sprintf(fFileData,"../Mimosa32Producer/data/Mimosa32_Run_%d",m_run);
      //fFP = fopen (fFileData, "w");
      running = true;
      fMimosa32->cmdStartRun();    
      std::cout << "Start Run: " << m_run << std::endl;
      //    }
      // It must send a BORE to the Data Collector
      eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      // You can set tags on the BORE that will be saved in the data file
      // and can be used later to help decoding
      //bore.SetTag("DET", eudaq::to_string(m_exa));
      // Send the event to the Data Collector
      SendEvent(bore);

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Running");
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      std::cout << "Stopping Run" << std::endl;
      int control;
      control=fMimosa32->cmdStopRun();
      if(control>0)last_event=1;
      running = false;
      num_word=13;
      // Set a flag to signal to the polling loop that the run is over
      stopping = true;   
      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
      std::cout << "[Mimosa32Producer] Sent EORE " << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    }

    // This gets called when the Run Control is terminating
    // we should also exit.
    virtual void OnTerminate() {
      std::cout << "Terminating..." << std::endl;
      fMimosa32->close_client_tcp(fSD);
      done = true;
      eudaq::mSleep(1000);
    }

    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
       int i;
      // Loop until Run Control tells us to terminate
      while (!done) {

	if (!running){
	    eudaq::mSleep(50);
	    continue;
	    }
        if (running){
	    printf("ReadoutLoop Running\n");
	    fMimosa32->readout_event(num_word,fFP,fCdh,fDh,fData);
	    std::vector<unsigned char> bufferCdh;
	    std::vector<unsigned char> bufferDh;
	    std::vector<unsigned char> bufferData;
	    // printf("CDH\n");
            for(i=0;i<32;i++){
	        pack(bufferCdh,fCdh[i]);
                if(i%4 == 0) printf("\n");
		// printf("%02X",(unsigned char)fCdh[i]);
	        }
	    // printf("\nDH\n");
	    for(i=0;i<16;i++){
	        pack(bufferDh,fDh[i]);
                 if(i%4 == 0) printf("\n");
		 // printf("%02X",(unsigned char)fDh[i]);
	            }
	    // printf("\nData\n");
            for(i=0;i<2560;i++){
	        pack(bufferData,fData[i]);
		// if(i%4 == 0) printf("\n");
		// printf("%02X",(unsigned char)fData[i]);
	        } 
           
         if(m_ev%100==0 || m_ev<100) std::cout << "event #" << m_ev << std::endl;           
        // If we get here, there must be data to read out
        // Create a RawDataEvent to contain the event data to be sent
            eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
            ev.AddBlock(0, bufferCdh);
 	    ev.AddBlock(1, bufferDh);
	    ev.AddBlock(2, bufferData);
        // Send the event to the Data Collector      
            SendEvent(ev);
        // Now increment the event number
            m_ev++;
	    }//end running
        if (last_event==1){
            printf("ReadoutLoop Last Event \n");
	    fMimosa32->readout_event(num_word,fFP,fCdh,fDh,fData);
            }             
        } //end while
    }//end Readout loop

  private:
  // This is just a dummy class representing the hardware
  // It here basically that the example code will compile
  // but it also generates example raw data to help illustrate the decoder
  eudaq::ExampleHardware hardware;
  unsigned m_run, m_ev;
  char ip[14];
  int port;
  int num_chip;
  int num_word;
  int last_event;
  bool stopping, running, done,configure;
  char fFileData[100]; //nome del file dove vengono salvati i dati.
  DeviceMimosa32* fMimosa32;
  int fSD; //Socket Descriptor
  FILE *fFP;
  char fCdh[32];
  char fDh[16];
  char fData[2560];


};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Mimosa32 Producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Mimosa32", "string",
      "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    Mimosa32Producer producer(name.Value(), rctrl.Value());
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
