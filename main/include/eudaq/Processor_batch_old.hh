#ifndef Processor_h__
#define Processor_h__

#include "eudaq/Platform.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/factory.hh"
#include <memory>

#define RegisterProcessor(derivedClass,ID) registerClass(eudaq::Processor,derivedClass,ID)



namespace eudaq{
  class Processor;
 
  using Processor_sp = std::shared_ptr < Processor > ;
  class DLLEXPORT Processor{
  public:
    using MainType = std::string;
    using Parameter_t = eudaq::Configuration;
    using Parameter_ref = const Parameter_t&;


    Processor(Parameter_ref conf);
    virtual void init() =0;
    virtual void ProcessorEvent(event_sp ev) =0;
    virtual void end() =0;


    
    void setNextProducer(Processor_sp next);
  private:
    void Send2NextProcessor(event_sp ev);
    Processor_sp m_next;

  };

  
}
#endif // Processor_h__
