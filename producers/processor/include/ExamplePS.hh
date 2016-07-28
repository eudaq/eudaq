#ifndef EXAMPLEPS_HH_
#define EXAMPLEPS_HH_


#include<string>

#include"Platform.hh"
#include"Processor.hh"

namespace eudaq{

  class ExamplePS:public Processor{
  public:
    ExamplePS(std::string cmd);
    virtual ~ExamplePS() {};
    
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProcessCmdEvent(EVUP ev);
    virtual void ProduceEvent();
  };
}

#endif
