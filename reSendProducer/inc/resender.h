#ifndef resender_h__
#define resender_h__
#include <string>
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"


namespace eudaq{


  class resender : public eudaq::Producer {
  public:

    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    resender(const std::string & name, const std::string & runcontrol);

    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config);

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param);

    // This gets called whenever a run is stopped
    virtual void OnStopRun();

    // This gets called when the Run Control is terminating,
    // we should also exit.
    virtual void OnTerminate();

    // This is just an example, adapt it to your hardware
    void resendEvent(std::shared_ptr<eudaq::Event> ev);


    void setBoreEvent(std::shared_ptr<eudaq::Event> boreEvent);
    bool getStarted(){ return started; }
    unsigned getRunNumber(){ return m_run; }
  private:
    // This is just a dummy class representing the hardware
    // It here basically that the example code will compile
    // but it also generates example raw data to help illustrate the decoder

    unsigned m_run, m_ev, m_exampleparam;
    bool stopping, done, started;
    std::shared_ptr<eudaq::Event> m_boreEvent;
    std::string m_name;
  };
}

#endif // resender_h__
