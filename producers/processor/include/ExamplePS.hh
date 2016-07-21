#ifndef EXAMPLEPS_HH_
#define EXAMPLEPS_HH_


#include<string>

#include"Platform.hh"
#include"Processor.hh"

namespace eudaq{

  class ExamplePS:public Processor{
    // class DLLEXPORT ExamplePS{
  public:
    ExamplePS(uint32_t psid)
      :Processor("ExamplePS", psid){};
    ExamplePS(std::string cmd)
      :Processor("ExamplePS", cmd){};

    
    virtual ~ExamplePS() {};
    
    virtual void ProcessUserEvent(EVUP ev);
    virtual void ProcessCmdEvent(EVUP ev);
    virtual void ProduceEvent();
    
    static Processor* Create(uint32_t psid);
  };
}

#endif
