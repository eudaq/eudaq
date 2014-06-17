#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <iomanip>

#include "SpidrController.h" 
#include "SpidrDaq.h"
#include "tpx3defs.h"

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

using namespace std;

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "Example";

// Declare a new class that inherits from eudaq::Producer
class Timepix3Producer : public eudaq::Producer {
  public:
  
  //////////////////////////////////////////////////////////////////////////////////
  // Timepix3Producer
  //////////////////////////////////////////////////////////////////////////////////

  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  Timepix3Producer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), done(false),started(0) {}
    
  //////////////////////////////////////////////////////////////////////////////////
  // OnConfigure
  //////////////////////////////////////////////////////////////////////////////////

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;
    
    // Do any configuration of the hardware here
    // Configuration file values are accessible as config.Get(name, default)
    m_exampleparam = config.Get("Parameter", 0);
    std::cout << "Example Parameter = " << m_exampleparam << std::endl;
    hardware.Setup(m_exampleparam);
    
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }

  //////////////////////////////////////////////////////////////////////////////////
  // OnStartRun
  //////////////////////////////////////////////////////////////////////////////////
  
  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;
    
    std::cout << "Start Run: " << m_run << std::endl;
    
    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    // You can set tags on the BORE that will be saved in the data file
    // and can be used later to help decoding
    bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleparam));
    // Send the event to the Data Collector
    SendEvent(bore);
    
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Running");
    started=true;
  }

  //////////////////////////////////////////////////////////////////////////////////
  // OnStopRun
  //////////////////////////////////////////////////////////////////////////////////
 
  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;
    started=false;
    // Set a flag to signal to the polling loop that the run is over
    stopping = true;
    
    // wait until all events have been read out from the hardware
    while (stopping) {
      eudaq::mSleep(20);
    }
    
    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
  }

  //////////////////////////////////////////////////////////////////////////////////
  // OnTerminate
  //////////////////////////////////////////////////////////////////////////////////
  
  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }
  
  //////////////////////////////////////////////////////////////////////////////////
  // ReadoutLoop
  //////////////////////////////////////////////////////////////////////////////////

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    // Loop until Run Control tells us to terminate
    while (!done) {
      if (!hardware.EventsPending()) {
	// No events are pending, so check if the run is stopping
	if (stopping) {
	  // if so, signal that there are no events left
	  stopping = false;
	}
	// Now sleep for a bit, to prevent chewing up all the CPU
	eudaq::mSleep(20);
	// Then restart the loop
	continue;
      }
      if (!started)
	{
	  // Now sleep for a bit, to prevent chewing up all the CPU
	  eudaq::mSleep(20);
	  // Then restart the loop
	  continue;
	}
      // If we get here, there must be data to read out
      // Create a RawDataEvent to contain the event data to be sent
      eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
      
      for (unsigned plane = 0; plane < hardware.NumSensors(); ++plane) {
	// Read out a block of raw data from the hardware
	std::vector<unsigned char> buffer = hardware.ReadSensor(plane);
	// Each data block has an ID that is used for ordering the planes later
	// If there are multiple sensors, they should be numbered incrementally
	
	// Add the block of raw data to the event
	ev.AddBlock(plane, buffer);
      }
      hardware.CompletedEvent();
      // Send the event to the Data Collector      
      SendEvent(ev);
      // Now increment the event number
      m_ev++;
    }
  }
  
private:
  // This is just a dummy class representing the hardware
  // It here basically that the example code will compile
  // but it also generates example raw data to help illustrate the decoder
  eudaq::ExampleHardware hardware;
  unsigned m_run, m_ev, m_exampleparam;
  bool stopping, done,started;
};

//////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Timepix3 Producer", "1.0",
			 "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address",
				   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Timepix3", "string",
				   "The name of this Producer");

  /*
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    Timepix3Producer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  */

  std::cout << "==============> Timepix3Producer, main()..." << std::endl;

  // Open a control connection to SPIDR-TPX3 module
  // with address 192.168.100.10, default port number 50000
  SpidrController spidrctrl( 192, 168, 100, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) 
    {
      std::cout << spidrctrl.ipAddressString() << ": " << spidrctrl.connectionStateString() << ", " << spidrctrl.connectionErrString() << std::endl;
      return 1;
    } 
  else 
    {
      std::cout << "\n------------------------------" << std::endl;
      std::cout << "SpidrController is connected!" << std::endl;
      std::cout << "Class version: " << spidrctrl.versionToString( spidrctrl.classVersion() ) << std::endl;
      int firmwVersion, softwVersion = 0;
      if( spidrctrl.getFirmwVersion( &firmwVersion ) ) std::cout << "Firmware version: " << spidrctrl.versionToString( firmwVersion ) << std::endl;
      if( spidrctrl.getSoftwVersion( &softwVersion ) ) std::cout << "Software version: " << spidrctrl.versionToString( softwVersion ) << std::endl;
      std::cout << "------------------------------\n" << std::endl;
    }

  // Interface to Timepix3 pixel data acquisition
  SpidrDaq spidrdaq( &spidrctrl );
  string errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "###SpidrDaq: " << errstr << endl;
  
  int device_nr = 0;
  
  // ----------------------------------------------------------
  // DACs configuration
  if( !spidrctrl.setDacsDflt( device_nr ) )
    {
      error_out( "###setDacsDflt" );
    }
  
  // ----------------------------------------------------------
  // Enable decoder
  if( !spidrctrl.setDecodersEna( 1 ) )
    {
      error_out( "###setDecodersEna" );
    }

  // ----------------------------------------------------------
  // Pixel configuration
  if( !spidrctrl.resetPixels( device_nr ) )
    error_out( "###resetPixels" );

  // Enable test-bit in all pixels
  spidrctrl.resetPixelConfig();
  spidrctrl.setPixelTestEna( ALL_PIXELS, ALL_PIXELS );
  if( !spidrctrl.setPixelConfig( device_nr ) ) 
    {
      error_out( "###setPixelConfig" );
    }

  // ----------------------------------------------------------
  // Test pulse and CTPR configuration

  // Timepix3 test pulse configuration
  if( !spidrctrl.setTpPeriodPhase( device_nr, 100, 0 ) ) 
    {
      error_out( "###setTpPeriodPhase" );
    }
  if( !spidrctrl.setTpNumber( device_nr, 1 ) ) 
    {
      error_out( "###setTpNumber" );
    }

  // Enable test-pulses for some columns
  int col;
  for( col=0; col<256; ++col ) 
    {
      if( col >= 10 && col < 12 ) 
	{
	  spidrctrl.setCtprBit( col );
	}
    }
  
  if( !spidrctrl.setCtpr( device_nr ) ) 
    {
      error_out( "###setCtpr" );
    }

  // ----------------------------------------------------------
  // SPIDR-TPX3 and Timepix3 timers
  if( !spidrctrl.restartTimers() ) 
    {
      error_out( "###restartTimers" );
    }

  // Set Timepix3 acquisition mode
  if( !spidrctrl.setGenConfig( device_nr,
			       TPX3_POLARITY_HPLUS |
			       TPX3_ACQMODE_TOA_TOT |
			       TPX3_GRAYCOUNT_ENA |
			       TPX3_TESTPULSE_ENA |
			       TPX3_FASTLO_ENA |
			       TPX3_SELECTTP_DIGITAL ) )
    {
      error_out( "###setGenConfig" );
    }
  
  // Set Timepix3 into acquisition mode
  if( !spidrctrl.datadrivenReadout() ) 
    {
      error_out( "###datadrivenReadout" );
    }
  
  // ----------------------------------------------------------
  // Configure the shutter trigger
  int trigger_mode = 4;           // SPIDR_TRIG_AUTO;
  int trigger_length_us = 100000; // 100 ms
  int trigger_freq_hz = 2;        // 2 Hz
  int trigger_count = 10;         // 10 triggers
  if( !spidrctrl.setTriggerConfig( trigger_mode, trigger_length_us, trigger_freq_hz, trigger_count ) ) 
    {
      error_out( "###setTriggerConfig" );
    }
  
  // Sample pixel data while writing the data to file (run number 123)
  spidrdaq.setSampling( true );
  spidrdaq.startRecording( "/data/test/test.dat", 123, "This is test data." );

  // ----------------------------------------------------------
  // Get data samples and display some pixel data details
  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    {
      error_out( "###startAutoTrigger" );
    }
  int cnt = 0, size, x, y, pixdata, timestamp;
  bool next_sample = true;
  while( next_sample )
    {
      // Get a sample of (at most) 1000 pixel data packets, waiting up to 3 s for it
      next_sample = spidrdaq.getSample( 10*8, 3000 );
      if( next_sample )
	{
	  ++cnt;
	  size = spidrdaq.sampleSize();
	  cout << "Sample " << cnt << " size=" << size << endl;
	  while( spidrdaq.nextPixel( &x, &y, &pixdata, &timestamp ) ) 
	    {
	      cout << x << "," << y << ": " << dec << pixdata << endl;
	    }
	}
      else
	{
	  cout << "### Timeout --> finish" << endl;
	}
    }
  
  return 0;
}
