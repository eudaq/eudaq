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

#include "CMSPixelEvtMonitor.hh"

#include <iostream>
#include <ostream>
#include <vector>
using namespace pxar; 
using namespace std; 

// define buffer read out scheme
// 1 read out full buffer when full
// 2 read out single events withing eudaq loop
// 3 read out via eudaq data collector
#define BUFFER_RO_SCHEME 3
// Write output to binary 1 or ASCII 2 for R/O scheme 1 or 2
#define OUTPUT_FILE_SCHEME 2

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
 static const std::string EVENT_TYPE = "EXAMPLE";

// Declare a new class that inherits from eudaq::Producer
class CMSPixelProducer : public eudaq::Producer {
  public:
    //-------------------------------------------------
    CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)
      : eudaq::Producer(name, runcontrol),
      m_run(0),
      m_ev(0),
      stopping(false),
      done(false),
      started(0),
      m_api(0),
      m_verbosity(verbosity),
      m_dacsFromConf(false),
      m_trimmingFromConf(false),
      m_pattern_delay(0),
      m_fout(0),
			m_foutName(""),
      m_perFull(0),
			triggering(false)
  {
    m_t = new eudaq::Timer;
    m_api = new pxar::pxarCore("*", m_verbosity);
  }// Constructor
     //-------------------------------------------------
    //
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
        pg_setup.push_back(std::make_pair("calibrate",config.Get("calibrate",106)) ); // PG_CAL
        pg_setup.push_back(std::make_pair("trigger",config.Get("trigger", 16)) );    // PG_TRG
        pg_setup.push_back(std::make_pair("token",config.Get("token", 0)));     // PG_TOK
        m_pattern_delay = config.Get("patternDelay", 100) * 10;
      }
      else{
        pg_setup.push_back(std::make_pair("trigger",46));    // PG_TRG
        pg_setup.push_back(std::make_pair("token",0));     // PG_TOK
        m_pattern_delay = config.Get("patternDelay", 100);
      }
      std::cout << "m_pattern_delay = " << m_pattern_delay << std::endl;

      // read dacs and trimming from config
      std::string trimFile = config.Get("trimDir", "") + string("/trimParameters.dat");
      rocDACs.push_back(GetConfDACs());
      rocPixels.push_back(GetConfTrimming(trimFile));

      // Set the type of the ROC correctly:
      std::string roctype = config.Get("roctype","psi46digv2");  
			
			std::cout << "roctype " << roctype << std::endl;

      try {
        m_api -> initTestboard(sig_delays, power_settings, pg_setup);
        m_api -> initDUT(hubid,"tbm08",tbmDACs,roctype,rocDACs,rocPixels);

        // Read current:
        std::cout << "Analog current: " << m_api->getTBia()*1000 << "mA" << std::endl;
        std::cout << "Digital current: " << m_api->getTBid()*1000 << "mA" << std::endl;

        m_api -> HVon();
        // m_api -> Pon();
        EUDAQ_USER("API set up succesfully...");
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
      m_api->_dut->testAllPixels(true);
      m_api->_dut->maskAllPixels(false);

			// efficiency map
			if(m_config.Get("doEffMap", false)){
				char mapName[256] = "efficiency map";
				m_t -> Restart();
				std::cout << "start time efficiency map: " << m_t -> Seconds() << std::endl; 
				std::vector<pxar::pixel> effMap = m_api->getEfficiencyMap(0, 100);
				std::cout << "stop time efficiency map: " << m_t -> Seconds() << std::endl; 
				CMSPixelEvtMonitor::Instance()->DrawMap(effMap, mapName);
				std::cout << "stop time drawing: " << m_t -> Seconds() << std::endl; 
			}

      m_api->_dut->testAllPixels(false);

      // Set some pixels up for getting calibrate signals:
      if(testpulses) {
        std::cout << "Setting up pixels for calibrate pulses..." << std::endl;
        for(int i = 0; i < 3; i++){ 
//					for(int j = 0; j < 3; j++){
          	m_api->_dut->testPixel(i,1,true);
          	m_api->_dut->testPixel(i,2,true);
          	m_api->_dut->testPixel(i,3,true);
        }
      }

			// Get output file
			std::ostringstream filename;
      filename << m_config.Get("outputFileTrunk", "../data/defaultOutput") << (int)m_run << ".dat" ;
			
			m_foutName = filename.str();
			EUDAQ_INFO(string("Writing CMS pixel data into: ") + m_foutName);
      std::cout << "Writing CMS pixel data into: " << m_foutName << std::endl;
      // Read DUT info, should print above filled information:
      m_api->_dut->info();

			std::cout << "Current DAC settings:" << std::endl;
			m_api->_dut->printDACs(0);

      if(!m_dacsFromConf)
        SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all DAC parameters from config file " + config.Name() + ".\n");
      else if(!m_trimmingFromConf)
        SetStatus(eudaq::Status::LVL_WARN, "Couldn't read all trimming parameters from config file " + config.Name() + ".\n");
      else
        SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

    }//end On Configure
    //-------------------------------------------------
    //
    //-------------------------------------------------
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << m_run << std::endl;

#if BUFFER_RO_SCHEME == 3
			eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      bore.SetTag("CMSPixel", "TAG");
      SendEvent(bore);
#endif

      m_fout.open(m_foutName.c_str(), std::ios::out | std::ios::binary);

      m_api -> daqStart();
      m_api -> daqTriggerLoop(m_pattern_delay);   
			triggering = true;
      started = true;
      SetStatus(eudaq::Status::LVL_OK, "Running");
    }// OnStartRun
    //-------------------------------------------------
    //
    //-------------------------------------------------
    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      stopping = true;
      std::cout << "Stopping Run" << std::endl;
      try {
				eudaq::mSleep(20);
        m_api -> daqTriggerLoopHalt();
				triggering = false;
        while(stopping){
          eudaq::mSleep(20);
				}
				m_api -> daqStop();
				eudaq::mSleep(100);
				m_fout.close();

#if BUFFER_RO_SCHEME == 3
				SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
#endif

#if OUTPUT_FILE_SCHEME == 2
				try{
					std::cout << "Trying to run root application and show run summary." << std::endl;
					CMSPixelEvtMonitor::Instance() -> DrawFromFile(m_foutName); 
				}
				catch(...){
					printf("Exception\n");
				}
#endif

				SetStatus(eudaq::Status::LVL_OK, "Stopped");
			} catch (const std::exception & e) {
				printf("While Stopping: Caught exception: %s\n", e.what());
				SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
			} catch (...) {
				printf("While Stopping: Unknown exception\n");
				SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
			}

		}// OnStopRun
    //-------------------------------------------------
    //
    //-------------------------------------------------
    virtual void OnTerminate() {
      std::cout << "Terminating..." << std::endl;
      m_api -> HVoff();
      // m_api -> Poff();
      done = true;
    }// OnTerminate
    //-------------------------------------------------
    //
    //-------------------------------------------------
   void ReadoutLoop() {
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
					eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
					pxar::rawEvent daqEvent = m_api -> daqGetRawEvent();
					uint16_t rocHeader = 0;
					ev.AddBlock(rocHeader, reinterpret_cast<const char*>(&daqEvent.data[0]), sizeof(daqEvent.data[0])*daqEvent.data.size());
					SendEvent(ev);
					m_ev++;
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
    //-------------------------------------------------
    //
    //-------------------------------------------------
  private:
    void ReadInSingleEventWriteBinary()
    {
      if(int(m_perFull) > 80){
        std::cout << "Warning: Buffer almost full. Please stop the run!" << std::endl;
        SetStatus(eudaq::Status::LVL_WARN, "BUFFER almost full. Stop run!");
        // OnStopRun(); // FIXME this doesn't seem to work (yet)
      }
      pxar::rawEvent daqEvent = m_api -> daqGetRawEvent();
      m_fout.write(reinterpret_cast<const char*>(&daqEvent.data[0]), sizeof(daqEvent.data[0])*daqEvent.data.size());
      if(stopping){
				while(triggering)
					eudaq::mSleep(20);
        std::cout << "Reading remaining data from buffer." << std::endl;
        std::vector<uint16_t> daqdat = m_api->daqGetBuffer();
        m_fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
        eudaq::mSleep(10);         
        stopping = false;
        started = false;
      }
    }
    //-------------------------------------------------
    //
    //-------------------------------------------------
    void ReadInSingleEventWriteASCII(){
      if(int(m_perFull) > 80){
        std::cout << "Warning: Buffer almost full. Please stop the run!" << std::endl;
        SetStatus(eudaq::Status::LVL_WARN, "BUFFER almost full. Stop run!");
        // OnStopRun(); // FIXME this doesn't seem to work (yet)
      }
      pxar::Event daqEvent = m_api -> daqGetEvent();
			std::cout << "Number of decoder errors: "<< m_api -> daqGetNDecoderErrors() << std::endl;
      m_fout << hex << daqEvent.header << "\t";
      int col, row;
      double value;
      for(std::vector<pxar::pixel>::iterator it = daqEvent.pixels.begin(); it != daqEvent.pixels.end(); ++it){
        col = (int) it -> getColumn();
        row = (int) it -> getRow();
        value = it -> getValue();
        m_fout << col << "\t" << row << "\t" << value << "\t";
      } 
      m_fout << "\n";
      // if OnStopRun has been called read out remaining buffer
      if(stopping){
				while(triggering)
					eudaq::mSleep(20);
        std::vector<pxar::Event> daqdat = m_api->daqGetEventBuffer();
        for(std::vector<pxar::Event>::iterator daqEventIt = daqdat.begin(); daqEventIt != daqdat.end(); ++daqEventIt){
          m_fout << hex << daqEventIt -> header << "\t";
          for(std::vector<pxar::pixel>::iterator it = daqEventIt->pixels.begin(); it != daqEventIt->pixels.end(); ++it){
            col = (int) it -> getColumn();
            row = (int) it -> getRow();
            value = it -> getValue();
            m_fout << col << "\t" << row << "\t" << value << "\t";
          } 
          m_fout << std::endl;
        }
        std::cout << "Read " << daqdat.size() << " remaining events." << std::endl;
        stopping = false;
        started = false;
      }
    }
    //-------------------------------------------------
    //
    //-------------------------------------------------
    void ReadInFullBufferWriteBinary()
		{
			if(int(m_perFull) > 80 || stopping){
				if(stopping)
					while(triggering)
						eudaq::mSleep(20);
				std::cout << "DAQ buffer is " << int(m_perFull) << "\% full." << std::endl;     
				std::cout << "Start reading data from DTB RAM." << std::endl;
				std::cout << "Halting trigger loop." << std::endl;
				m_api -> daqTriggerLoopHalt();
				std::vector<uint16_t> daqdat = m_api->daqGetBuffer();
				std::cout << "Read " << daqdat.size() << " words of data: ";
				if(daqdat.size() > 550000) std::cout << (daqdat.size()/524288) << "MB." << std::endl;
				else std::cout << (daqdat.size()/512) << "kB." << std::endl;
				m_fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
				if(stopping){
					stopping = false;
					started = false;
        }
        else{
          std::cout << "Continuing trigger loop.\n";
          m_api -> daqTriggerLoop(m_pattern_delay);
        }
        m_api -> daqStatus(m_perFull);    
      }
      eudaq::mSleep(1000);
    }
    //-------------------------------------------------
		//
		//-------------------------------------------------
		void ReadInFullBufferWriteASCII()
		{
			if(int(m_perFull) > 10 || stopping){
				if(stopping)
					while(triggering)
						eudaq::mSleep(20);
        std::cout << "DAQ buffer is " << int(m_perFull) << "\% full." << std::endl;     
        std::cout << "Start reading data from DTB RAM." << std::endl;
        std::cout << "Halting trigger loop." << std::endl;
        m_api -> daqTriggerLoopHalt();
        std::vector<pxar::Event> daqdat = m_api->daqGetEventBuffer();
        std::cout << "Read " << daqdat.size() << " events." << std::endl;
        for(std::vector<pxar::Event>::iterator daqEventIt = daqdat.begin(); daqEventIt != daqdat.end(); ++daqEventIt){
          m_fout << hex << daqEventIt -> header << "\t";
          int col, row;
          double value;
          for(std::vector<pxar::pixel>::iterator it = daqEventIt->pixels.begin(); it != daqEventIt->pixels.end(); ++it){
            col = (int) it -> getColumn();
            row = (int) it -> getRow();
            value = it -> getValue();
            m_fout << col << "\t" << row << "\t" << value << "\t";
          } 
          m_fout << "\n";
        }
        if(stopping){
          stopping = false;
          started = false;
        }
        else{
          std::cout << "Continuing trigger loop.\n";
          m_api -> daqTriggerLoop(m_pattern_delay);
        }
        m_api -> daqStatus(m_perFull);    
      }
      eudaq::mSleep(1000);
    }   //-------------------------------------------------
    //
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
      if(m_dacsFromConf)
        //        EUDAQ_USER(string("All DACs successfully read from ") + m_config.Name());
        return dacs;
    } // GetConfDACs
    //-------------------------------------------------
    //
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
        m_trimmingFromConf = true;
      }
      else{
        std::cout << "Couldn't read trim parameters from " << string(filename) << ". Setting all to 15." << std::endl;
        EUDAQ_WARN(string("Couldn't read trim parameters from ") + string(filename) + (". Setting all to 15."));
        for(int col = 0; col < 52; col++) {
          for(int row = 0; row < 80; row++) {
            pixels.push_back(pxar::pixelConfig(col,row,15));
          }
        }
        m_trimmingFromConf = false;
      }
      if(m_trimmingFromConf)
        EUDAQ_USER(string("Trimming successfully read from ") + m_config.Name() + string(": ") + string(filename));
      return pixels;
    } // GetConfTrimming
    //-------------------------------------------------
    //
    //-------------------------------------------------
    unsigned m_run, m_ev;
		std::string m_verbosity, m_foutName;
    bool stopping, done, started, triggering;
    bool m_dacsFromConf, m_trimmingFromConf;
    eudaq::Configuration m_config;
    pxar::pxarCore *m_api;
    int m_pattern_delay;
    uint8_t m_perFull;
    std::ofstream m_fout;
    eudaq::Timer* m_t;
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

