#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "api.h"
#include "ConfigParameters.hh"
#include "PixTestParameters.hh"
#include "constants.h"
#include "dictionaries.h"
#include "log.h"

#include "CMSPixelEvtMonitor.hh"
#include "CMSPixelProducer.hh"

#include <iostream>
#include <ostream>
#include <vector>
using namespace pxar; 
using namespace std; 

/*========================================================================*/
// Define buffer readout scheme via setting flags:
// 1 read out full buffer when full
// 2 read out single events withing eudaq loop
// 3 read out via eudaq data collector
#define BUFFER_RO_SCHEME 3
// Write output to binary 1 or ASCII 2 for R/O schemes 1 or 2
#define OUTPUT_FILE_SCHEME 1
/*========================================================================*/

// event type name, needed for readout with eudaq. Links to /main/lib/src/CMSPixelConverterPlugin.cc:
static const std::string EVENT_TYPE_DUT = "CMSPixelDUT";
static const std::string EVENT_TYPE_REF = "CMSPixelREF";

CMSPixelProducer::CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)
  : eudaq::Producer(name, runcontrol),
    m_run(0),
    m_ev(0),
    m_ev_filled(0),
    stopping(false),
    done(false),
    started(0),
    m_api(NULL),
    m_verbosity(verbosity),
    m_dacsFromConf(false),
    m_trimmingFromConf(false),
    m_pattern_delay(0),
    m_trigger_is_pg(false),
    m_fout(0),
    m_foutName(""),
    m_perFull(0),
    triggering(false),
    m_roctype(""),
    m_usbId(""),
    m_producerName(name),
    m_detector(""),
    m_event_type("")
{
  m_t = new eudaq::Timer;
  if(m_producerName.find("REF") != std::string::npos) {
    m_detector = "REF";
    m_event_type = EVENT_TYPE_REF;
  }
  else {
    m_detector = "DUT";
    m_event_type = EVENT_TYPE_DUT;
  }
}

void CMSPixelProducer::OnConfigure(const eudaq::Configuration & config) {

  std::cout << "Configuring: " << config.Name() << std::endl;
  m_config = config;
  bool confTrimming(false), confDacs(false);
  // declare config vectors
  std::vector<std::pair<std::string,uint8_t> > sig_delays;
  std::vector<std::pair<std::string,double> > power_settings;
  std::vector<std::pair<std::string,uint8_t> > pg_setup; 
  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;
  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
  std::vector<std::vector<pxar::pixelConfig> > rocPixels;

  uint8_t hubid = config.Get("hubid", 31);

  // DTB delays
  sig_delays.push_back(std::make_pair("clk",config.Get("clk",4)));
  sig_delays.push_back(std::make_pair("ctr",config.Get("ctr",4)));
  sig_delays.push_back(std::make_pair("sda",config.Get("sda",19)));
  sig_delays.push_back(std::make_pair("tin",config.Get("tin",9)));
  sig_delays.push_back(std::make_pair("deser160phase",config.Get("deser160phase",4)));
  sig_delays.push_back(std::make_pair("level",config.Get("level",15)));
  sig_delays.push_back(std::make_pair("triggerlatency",config.Get("triggerlatency",120)));
  //sig_delays.push_back(std::make_pair("triggertimeout",config.Get("triggertimeout",65000)));

  //Power settings:
  power_settings.push_back( std::make_pair("va",config.Get("va",1.8)) );
  power_settings.push_back( std::make_pair("vd",config.Get("vd",2.5)) );
  power_settings.push_back( std::make_pair("ia",config.Get("ia",1.10)) );
  power_settings.push_back( std::make_pair("id",config.Get("id",1.10)) );

  // Pattern Generator:
  bool testpulses = config.Get("testpulses", false);
  if(testpulses) {
    pg_setup.push_back(std::make_pair("resetroc",config.Get("resetroc",25)) );
    pg_setup.push_back(std::make_pair("calibrate",config.Get("calibrate",106)) );
    pg_setup.push_back(std::make_pair("trigger",config.Get("trigger", 16)) );
    pg_setup.push_back(std::make_pair("token",config.Get("token", 0)));
    m_pattern_delay = config.Get("patternDelay", 100) * 10;
    EUDAQ_USER("Using testpulses, pattern delay " + eudaq::to_string(m_pattern_delay) + "\n");
  }
  else {
    pg_setup.push_back(std::make_pair("trigger",46));
    pg_setup.push_back(std::make_pair("token",0));
    m_pattern_delay = config.Get("patternDelay", 100);
  }

  // Read DACs and trimming from config
  std::string trimFile;
  if(config.Get("trimFile", "") != "") { trimFile = config.Get("trimFile", ""); }
  else { trimFile = config.Get("trimDir", "") + string("/trimParameters.dat"); }

  rocDACs.push_back(GetConfDACs());
  rocPixels.push_back(GetConfTrimming(trimFile));

  // Set the type of the ROC correctly:
  m_roctype = config.Get("roctype","psi46digv2");  
      
  try {
    // create api
    if(m_api)
      delete m_api;
      
    m_usbId = config.Get("usbId","*");
    EUDAQ_USER("Trying to connect to USB id: " + m_usbId + "\n");

    // Allow overwriting of verbosity level via command line:
    m_verbosity = config.Get("verbosity", m_verbosity);

    m_api = new pxar::pxarCore(m_usbId, m_verbosity);

    if(!m_api->initTestboard(sig_delays, power_settings, pg_setup)) { 
      EUDAQ_ERROR(string("Firmware mismatch."));
      throw pxar::pxarException("Firmware mismatch");
    }
    m_api->initDUT(hubid,"tbm08",tbmDACs,m_roctype,rocDACs,rocPixels);

    // Read current:
    std::cout << "Analog current: " << m_api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << m_api->getTBid()*1000 << "mA" << std::endl;

    if(m_api->getTBid()*1000 < 15) EUDAQ_ERROR(string("Digital current too low: " + std::to_string(1000*m_api->getTBid()) + "mA"));
    else EUDAQ_INFO(string("Digital current: " + std::to_string(1000*m_api->getTBid()) + "mA"));

    if(m_api->getTBia()*1000 < 15) EUDAQ_ERROR(string("Analog current too low: " + std::to_string(1000*m_api->getTBia()) + "mA"));
    else EUDAQ_INFO(string("Analog current: " + std::to_string(1000*m_api->getTBia()) + "mA"));

    // Send a single RESET to the ROC to initialize its status:
    if(!m_api->daqSingleSignal("resetroc")) {
      throw InvalidConfig("Unable to send ROC reset signal!");
    }
    else {
      EUDAQ_INFO(string("ROC Reset signal issued."));
    }

    // Switching to external clock if requested and check if DTB returns TRUE status:
    if(!m_api->setExternalClock(config.Get("external_clock",1) != 0 ? true : false)) {
      throw InvalidConfig("Couldn't switch to " + string(config.Get("external_clock",1) != 0 ? "external" : "internal") + " clock.");
    }
    else {
      EUDAQ_INFO(string("Clock set to " + string(config.Get("external_clock",1) != 0 ? "external" : "internal")));
    }

    // Switching to the selected trigger source and check if DTB returns TRUE:
    std::string triggersrc = config.Get("trigger_source","pg_dir");
    if(!m_api->daqTriggerSource(triggersrc)) {
      throw InvalidConfig("Couldn't select trigger source " + string(triggersrc));
    }
    else {
      if(triggersrc == "pg" || triggersrc == "pg_dir" || triggersrc == "patterngenerator") {
	m_trigger_is_pg = true;
      }
      EUDAQ_INFO(string("Trigger source selected: " + triggersrc));
    }

    // Output the configured signal to the probes:
    m_api->SignalProbe("d1", config.Get("signalprobe_d1","ctr"));
    EUDAQ_USER("Setting scope output D1 to \"" + config.Get("signalprobe_d1","ctr") + "\"\n");
    m_api->SignalProbe("d2", config.Get("signalprobe_d2","tout"));
    EUDAQ_USER("Setting scope output D2 to \"" + config.Get("signalprobe_d2","tout") + "\"\n");

    EUDAQ_USER("API set up succesfully...\n");

    // All on!
    m_api->_dut->maskAllPixels(false);
    m_api->_dut->testAllPixels(false);

    // test pixels
    if(testpulses) {
      std::cout << "Setting up pixels for calibrate pulses..." << std::endl << "col \t row" << std::endl;
      for(int i = 40; i < 45; i++){
	m_api->_dut->testPixel(25,i,true);
      }
    
    }
    // Read DUT info, should print above filled information:
    m_api->_dut->info();

    std::cout << "Current DAC settings:" << std::endl;
    m_api->_dut->printDACs(0);

    /*if(!m_dacsFromConf)
      SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all DAC parameters from config file " + config.Name() + ".");*/
    if(!m_trimmingFromConf)
      SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all trimming parameters from config file " + config.Name() + ".");
    else
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
    std::cout << "=============================\nCONFIGURED\n=============================" << std::endl;

  }

  catch (pxar::InvalidConfig &e){
    EUDAQ_ERROR(string("Invalid configuration settings: " + string(e.what())));
    SetStatus(eudaq::Status::LVL_ERROR, string("Invalid configuration settings: ") + e.what());
    delete m_api;
  }
  catch (pxar::pxarException &e){
    EUDAQ_ERROR(string("pxarCore Error: " + string(e.what())));
    SetStatus(eudaq::Status::LVL_ERROR, string("pxarCore Error: ") + e.what());
    delete m_api;
  }
  catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
    delete m_api;
  }
}

void CMSPixelProducer::OnStartRun(unsigned param) {
  m_run = param;
  m_ev = 0;
  m_ev_filled = 0;

  EUDAQ_INFO("Switching Sensor Bias HV ON.");
  m_api->HVon();

  std::cout << "Start Run: " << m_run << std::endl;

#if BUFFER_RO_SCHEME == 3
  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(m_event_type, m_run));
  // Set the ROC type for decoding:
  bore.SetTag("ROCTYPE", m_roctype);

  // Set the detector for correct plane assignment:
  bore.SetTag("DETECTOR", m_detector);

  // Send the event out:
  SendEvent(bore);

  std::cout << "BORE with detector " << m_detector << " (event type " << m_event_type << ") and ROC type " << m_roctype << std::endl;
#else
  // Get output file
  std::ostringstream filename;
  std::cout << std::endl << m_run << std::endl;
  filename << m_config.Get("outputFilePath", "../data/") << m_producerName.c_str() 
	   << setfill('0') << setw(5) << (int)m_run << ".dat" ;
  m_foutName = filename.str();

  EUDAQ_INFO(string("Writing CMS pixel data into: ") + m_foutName + "\n");
  std::cout << "Writing CMS pixel data into: " << m_foutName << std::endl;
  m_fout.open(m_foutName.c_str(), std::ios::out | std::ios::binary);
#endif
  m_api -> daqStart();

  // If we run on Pattern Generator, activate the PG loop:
  if(m_trigger_is_pg) {
    m_api -> daqTriggerLoop(m_pattern_delay);
    triggering = true;
  }

  SetStatus(eudaq::Status::LVL_OK, "Running");
  started = true;
}

// This gets called whenever a run is stopped
void CMSPixelProducer::OnStopRun() {
  stopping = true;
  std::cout << "Stopping Run" << std::endl;
  try {
    eudaq::mSleep(20);

    // If running with PG, halt the loop:
    if(m_trigger_is_pg) {
      m_api -> daqTriggerLoopHalt();
      triggering = false;
    }

    // assure single threading, wait till data is read out
    while(stopping){
      eudaq::mSleep(20);
    }
    m_api -> daqStop();
    eudaq::mSleep(100);
    
    EUDAQ_INFO("Switching Sensor Bias HV OFF.");
    m_api->HVoff();

#if BUFFER_RO_SCHEME == 3
    // Read the rest of events from DTB buffer:
    std::vector<pxar::rawEvent> daqEvents = m_api->daqGetRawEventBuffer();
    std::cout << "Post run read-out: sending " << daqEvents.size() << " evt." << std::endl;
    for(size_t i = 0; i < daqEvents.size(); i++) {
      eudaq::RawDataEvent ev(m_event_type, m_run, m_ev);
      ev.AddBlock(0, reinterpret_cast<const char*>(&daqEvents.at(i).data[0]), sizeof(daqEvents.at(i).data[0])*daqEvents.at(i).data.size());
      SendEvent(ev);
      m_ev++;
    }
    // Sending the final end-of-run event:
    SendEvent(eudaq::RawDataEvent::EORE(m_event_type, m_run, m_ev));
#else
    m_fout.close();
    std::cout << "Trying to run root application and show run summary." << std::endl;
    try{
#if OUTPUT_FILE_SCHEME == 1       
      CMSPixelEvtMonitor::Instance() -> DrawFromBinaryFile(m_foutName, m_roctype); 
#else       
      CMSPixelEvtMonitor::Instance() -> DrawFromASCIIFile(m_foutName); 
#endif      
      //CMSPixelEvtMonitor::Instance() -> DrawROTiming();
    }
    catch(...){
      std::cout << "Didn't work..." << std::endl;
    }
#endif
    std::cout << "Stopped" << std::endl;

    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  } catch (const std::exception & e) {
    printf("While Stopping: Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("While Stopping: Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }

}

void CMSPixelProducer::OnTerminate() {
  std::cout << "CMSPixelProducer terminating..." << std::endl;

  // Stop the readout loop:
  done = true;

  // If we already have a pxarCore instance, shut it down cleanly:
  if(m_api) {
    delete m_api;
  }

  std::cout << "CMSPixelProducer " << m_producerName << " terminated." << std::endl;
}

void CMSPixelProducer::ReadoutLoop() {
  // Loop until Run Control tells us to terminate
  while (!done) {
    if(!started){
      eudaq::mSleep(50);
      continue;
    } else {
      if(m_t -> Seconds() > 120){
	m_api -> daqStatus(m_perFull);
	m_t -> Restart();
	std::cout << "DAQ buffer is " << int(m_perFull) << "\% full." << std::endl; 
      }

#if BUFFER_RO_SCHEME == 1
#if OUTPUT_FILE_SCHEME == 1
      ReadInFullBufferWriteBinary();
#else
      ReadInFullBufferWriteASCII();
#endif
#elif BUFFER_RO_SCHEME == 2
#if OUTPUT_FILE_SCHEME == 1
      ReadInSingleEventWriteBinary();
#else
      ReadInSingleEventWriteASCII();
#endif
#else 
      // Trying to get the next event, daqGetRawEvent throws exception if none is available:
      try {
	pxar::rawEvent daqEvent = m_api->daqGetRawEvent();

	eudaq::RawDataEvent ev(m_event_type, m_run, m_ev);
	ev.AddBlock(0, reinterpret_cast<const char*>(&daqEvent.data[0]), sizeof(daqEvent.data[0])*daqEvent.data.size());

	SendEvent(ev);
	m_ev++;
	if(daqEvent.data.size() > 1) m_ev_filled++;

	if(m_ev%1000 == 0) {
	  std::cout << "CMSPIX EVT " << m_ev << " " << m_ev_filled << " w/ px" << std::endl;
	  std::cout << " this: " << daqEvent.data.size() << std::endl;
	}
      }
      catch(...) {}

      if(stopping){
	while(triggering)
	  eudaq::mSleep(20);
	stopping = false;
	started = false;
      }
#endif
    } 
  }
}

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("CMS Pixel Producer", "0.0",
			 "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address", "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "CMSPixel", "string",
				   "The name of this Producer");
  eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    CMSPixelProducer producer(name.Value(), rctrl.Value(), verbosity.Value());
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

