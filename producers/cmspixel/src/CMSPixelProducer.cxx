#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include "api.h"
#include "ConfigParameters.hh"
#include "PixTestParameters.hh"
#include "constants.h"
#include "dictionaries.h"

#include <iostream>
#include <ostream>
#include <vector>
using namespace pxar; 
using namespace std; 

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "CMSPixelEvent";

// Declare a new class that inherits from eudaq::Producer
class CMSPixelProducer : public eudaq::Producer {
  public:
//-------------------------------------------------
    CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)
      : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), done(false),started(0), m_api(0), m_verbosity(verbosity), m_dacsFromConf(false), m_pixelsFromConf(false), m_pattern_delay(0)
    {
      m_api = new pxar::pxarCore("*", m_verbosity);
    }
//-------------------------------------------------
    std::vector<std::pair<std::string,uint8_t> > GetConfDACs()
    {
      std::vector<std::pair<std::string,uint8_t> >  dacs; 

      RegisterDictionary * dict = RegisterDictionary::getInstance();
      std::vector<std::string> DACnames =  dict -> getAllROCNames();
      std::cout << " looping over DAC names \n ";
      m_dacsFromConf = true;
      for (std::vector<std::string>::iterator idac = DACnames.begin(); idac != DACnames.end(); ++idac) {
        if(m_config.Get(*idac, -1) == -1){
          EUDAQ_WARN(string("Roc DAC ") + *idac + string(" not defined in config file ") + m_config.Name() +
              string(". Using default values.\n"));
          std::cout << "WARNING: Roc DAC " << *idac << " not defined in config file " << m_config.Name() 
            << ". Using default values." << std::endl;
            m_dacsFromConf = false;
        }
        else{
          uint8_t dacVal = m_config.Get(*idac, 0);
          std::cout << *idac << " " << (int)dacVal << std::endl;
          dacs.push_back(make_pair(*idac, dacVal));
        }
      }
      return dacs;
    } // GetConfDACs
//-------------------------------------------------
    std::vector<pxar::pixelConfig> GetConfTrimming(std::string filename)
    {
      std::vector<pxar::pixelConfig> pixels;

      std::ifstream file(filename);
      if(!file.fail()){
        std::string line;
        while(std::getline(file, line))
        {
          std::stringstream   linestream(line);
          std::string         dummy;
          int                 trim, col, row;
          linestream >> trim >> dummy >> col >> row;
          pixels.push_back(pxar::pixelConfig(col,row,trim));
        }
        m_pixelsFromConf = true;
      }
      else{
        std::cout << "file not found" << endl;
        EUDAQ_WARN(string("Couldn't read trim parameters from ") + string(filename) + (". Setting all to 15."));
        for(int col = 0; col < 52; col++) {
          for(int row = 0; row < 80; row++) {
            pixels.push_back(pxar::pixelConfig(col,row,15));
          }
        }
        m_pixelsFromConf = false;
      }
      return pixels;
    }
    //-------------------------------------------------
    virtual void OnConfigure(const eudaq::Configuration & config)
    {
      std::cout << "Configuring: " << config.Name() << std::endl;
      m_config = config;
      bool confTrimming(false), confDacs(false);
      //config.Get(name, defaultVal);
      // declare config vectors
      std::vector<std::pair<std::string,uint8_t> > sig_delays;
      std::vector<std::pair<std::string,double> > power_settings;
      std::vector<std::pair<std::string,uint8_t> > pg_setup; 
      std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;
      std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
      std::vector<std::vector<pxar::pixelConfig> > rocPixels;

      uint8_t hubid = config.Get("hubid", 31); 


      // DTB delays
      sig_delays.push_back( std::make_pair("clk",config.Get("clk",4)) );
      sig_delays.push_back( std::make_pair("ctr",config.Get("ctr",4)) );
      sig_delays.push_back( std::make_pair("sda",config.Get("sda",19)) );
      sig_delays.push_back( std::make_pair("tin",config.Get("tin",9)) );
      sig_delays.push_back( std::make_pair("deser160phase",4) );

      //Power settings:
      power_settings.push_back( std::make_pair("va",config.Get("va",1.8)) );
      power_settings.push_back( std::make_pair("vd",config.Get("vd",2.5)) );
      power_settings.push_back( std::make_pair("ia",config.Get("ia",1.190)) );
      power_settings.push_back( std::make_pair("id",config.Get("id",1.10)) );

      // Pattern Generator:
      bool testpulses = config.Get("testpulses", false);
      if(testpulses) {
        pg_setup.push_back(std::make_pair("resetroc",config.Get("resetroc",25)) );    // PG_RESR
        pg_setup.push_back(std::make_pair("calibrate",config.Get("calibrate",105)) ); // PG_CAL
        pg_setup.push_back(std::make_pair("trigger",config.Get("trigger", 16)) );    // PG_TRG
        pg_setup.push_back(std::make_pair("token",config.Get("token", 0)));     // PG_TOK
        m_pattern_delay = 1000;
      }
      else{
        pg_setup.push_back(std::make_pair("trigger",46));    // PG_TRG
        pg_setup.push_back(std::make_pair("token",0));     // PG_TOK
        m_pattern_delay = 100;
      }

      // read dacs and trimming from config
      std::string trimFile = config.Get("trimFile", "");
      rocDACs.push_back(GetConfDACs());
      rocPixels.push_back(GetConfTrimming(trimFile));

      // Set the type of the ROC correctly:
      std::string roctype = config.Get("roctype","psi46digv2");  

      try {
        m_api -> initTestboard(sig_delays, power_settings, pg_setup);
        //FIXME tbmtype tbm08???
        m_api->initDUT(hubid,"tbm08",tbmDACs,roctype,rocDACs,rocPixels);

          // Read current:
        std::cout << "Analog current: " << m_api->getTBia()*1000 << "mA" << std::endl;
        std::cout << "Digital current: " << m_api->getTBid()*1000 << "mA" << std::endl;

        m_api -> HVon();
        // m_api -> Pon();

      }
      catch (pxar::InvalidConfig &e){
        SetStatus(eudaq::Status::LVL_ERROR, string("Configure Error: pxar caught an exception due to invalid configuration settings: ") + e.what());
        delete m_api;
        return;
      }
      catch (pxar::pxarException &e){
        SetStatus(eudaq::Status::LVL_ERROR, string("Configure Error: pxar caught an internal exception: ") + e.what());
        delete m_api;
        return;
      }
      catch (...) {
        SetStatus(eudaq::Status::LVL_ERROR, "Configure Error: pxar caught an unknown exception.");
        delete m_api;
        return;
      }

      // All on!
      m_api->_dut->testAllPixels(false);
      m_api->_dut->maskAllPixels(false);

      // Set some pixels up for getting calibrate signals:
      if(testpulses) {
        std::cout << "Setting up pixels for calibrate pulses..." << std::endl;
        for(int i = 0; i < 3; i++) {
          m_api->_dut->testPixel(i,5,true);
          m_api->_dut->testPixel(i,6,true);
        }
      }

      // Read DUT info, should print above filled information:
      m_api->_dut->info();

      if(!m_dacsFromConf)
        SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all DAC parameters from config file " + config.Name() + ".\n");
      else if(!m_pixelsFromConf)
        SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all trim parameters from config file " + config.Name() + ".\n");
      else
        SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
    }//end On Configure
//-------------------------------------------------
    virtual void OnStartRun(unsigned param) {
      started = true; 
      m_run = param;
      m_ev = 0;

      std::cout << "Start Run: " << m_run << std::endl;
			eudaq::RawDataEvent ev(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      
//      ev.SetTag("key", "val");

			SendEvent(ev);
			eudaq::mSleep(500);

			m_api -> daqStart();
      m_api->daqTriggerLoop(m_pattern_delay);

      SetStatus(eudaq::Status::LVL_OK, "Started");
    }// OnStartRun
    //-------------------------------------------------
    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      started = false;
      std::cout << "Stopping Run" << std::endl;
      try {
        std::cout << "Stop Run" << std::endl;

        m_api->daqStop();
        m_api->daqTriggerLoopHalt();

        eudaq::mSleep(5000);
        eudaq::mSleep(100);
        // Send an EORE after all the real events have been sent
        // You can also set tags on it (as with the BORE) if necessary
        SetStatus(eudaq::Status::LVL_OK, "Stopped");
        SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, m_ev));

      } catch (const std::exception & e) {
        printf("Caught exception: %s\n", e.what());
        SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
      } catch (...) {
        printf("Unknown exception\n");
        SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
      }

      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
			SetStatus(eudaq::Status::LVL_OK, "Stopped");
    }// OnStopRun
//-------------------------------------------------
// This gets called when the Run Control is terminating,
// we should also exit. //FIXME
    virtual void OnTerminate() {
      std::cout << "Terminating..." << std::endl;
      m_api -> HVoff();
     // m_api -> Poff();
      done = true;
    }// OnTerminate
//-------------------------------------------------
    void ReadoutLoop() {
      // Loop until Run Control tells us to terminate
      while (!done) {
           
        if(!started){
        	eudaq::mSleep(50);
				  continue;
        } else{
          std::vector<uint16_t> daqdat = m_api->daqGetBuffer();
          std::cout << "Buffer \n";
          std::cout << std::hex << daqdat[0] << std::endl;
          
          eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
          pxar::rawEvent rawPxarEvent = m_api -> daqGetRawEvent();
          ev.AddBlock(0, VecUint6_tToChar(rawPxarEvent.data)); //needs vector<char>
				  SendEvent(ev);
          std::cout << "events...events everywhere \n";
        }
      }
    }

  private:
    // This is just a dummy class representing the hardware
    // It here basically that the example code will compile
    // but it also generates example raw data to help illustrate the decoder
    unsigned m_run, m_ev, m_exampleparam;
    string m_configDir, m_verbosity;
    bool stopping, done,started;
    bool m_dacsFromConf, m_pixelsFromConf;
    eudaq::Configuration m_config;
    pxar::pxarCore *m_api;
   int m_pattern_delay;



   std::vector<char> VecUint6_tToChar(std::vector<uint16_t> u6vector){
     std::vector<char> charvector;
      for (std::vector<uint16_t>::iterator it = u6vector.begin(); it != u6vector.end(); ++it) {
        char lo = *it & 0xFF;
        char hi = *it >> 8;
        charvector.push_back(hi);
        charvector.push_back(lo);
      }
      return charvector;
   }

    

};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Example Producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
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

