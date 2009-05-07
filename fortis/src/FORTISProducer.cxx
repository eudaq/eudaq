#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <cctype>
#include <vector>
#include <cassert>

// Declare their use so we don't have to type namespace each time ....
using eudaq::RawDataEvent;
using eudaq::to_string;
using std::vector;

// put this next definition somewhere more sensible....
#define FORTIS_DATATYPE_NAME "FORTIS"

class FORTISProducer : public eudaq::Producer {

public:
  
  FORTISProducer(const std::string & name,   // Hard code to FORTIS (used to select which portion of cfg file to use)
		 const std::string & runcontrol // Points to run control process
		 )
    : eudaq::Producer(name, runcontrol),
      done(false),
      started(false),
      juststopped(false),
      m_rawData()
  {} // empty constructor


  // Poll for new data
  // N.B. This will have to be changed when we don't veto triggers during FORTIS readout and we end up with several
  // telescope events

  void Process() {

    if (!started) { // If the run isn't started just sleep for a while and return
      eudaq::mSleep(1);
      return;
    }

    if (juststopped) started = false; // this risks loosing the last event or two in the buffers. cf. EUDRBProducer for ideas how to fix this.

    // wait for data here....

    // OK, we have a trigger
    //FORTISPackData ( m_rawData , number_of_triggers_this_frame , ..... ); // this is pseudo code....


    // loop round sending empty events to data collector for each telecope trigger that we haven't read out...

    // end of loop

    RawDataEvent ev(FORTIS_DATATYPE_NAME, m_run, m_ev); // create an instance of the RawDataEvent with FORTIS-ID

    ev.AddBlock(m_rawData); // add the raw data block to the event

    SendEvent( ev ); // send the data to the data-collector
    

    if ( m_ev < 10 ) { // print out a debug message for each of first ten events, then every 10th 
      std::cout << "Sent event " << m_ev << std::endl;
    } else if ( m_ev % 10 == 0 ) {
      std::cout << "Sent event " << m_ev << std::endl;
    }

    ++m_ev; // increment the internal event counter.
  }



  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    
    m_param = param;


    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;

      // put our configuration stuff in here...
      m_rawData.resize(1000); // set the size of our event. We should be able to calculate this in advance.

      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }


  virtual void OnStartRun(unsigned param) {


    try {
      m_run = param;
      m_ev = 0;
      
      std::cout << "Start Run: " << param << std::endl;
      
      RawDataEvent ev( RawDataEvent::BORE( m_run ) );
      ev.SetTag("InitialRow", to_string( m_param.Get("InitialRow", 0x0) )) ; // put run parameters into BORE
  
      SendEvent( ev );
      eudaq::mSleep(100);
      started=true;
      SetStatus(eudaq::Status::LVL_OK, "Started");

    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
  }
  

  virtual void OnStopRun() {
    try {
      std::cout << "Stopping Run" << std::endl;
      juststopped = true;
      while (started) {
        eudaq::mSleep(100);
      }
      juststopped = false;
      SendEvent(RawDataEvent::EORE(m_run, ++m_ev));
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }


  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
  
    done = true;
  }


      // Declare members of class FORTISProducer.
  unsigned m_run, m_ev;
  bool done, started, juststopped;
  eudaq::Configuration  m_param;

  std::vector<unsigned short> m_rawData; // buffer for raw data frames. 

};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ FORTISProducer", "1.0", "The Producer task for reading out FORTIS chip via OptoDAQ");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "FORTIS", "string",
                                   "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    FORTISProducer producer(name.Value(), rctrl.Value());

    do {
      producer.Process();
      //      usleep(100);
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
