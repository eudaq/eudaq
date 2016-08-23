#ifndef EXAMPLEPS_HH_
#define EXAMPLEPS_HH

#define EUDAQ_MODULE

#include<string>

#include"Platform.hh"
#include"Processor.hh"

namespace eudaq{

  class ExamplePS:public Processor{
  public:
    ExamplePS(std::string cmd);
    virtual ~ExamplePS() {};
    
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProduceEvent();
  };
}

#endif
