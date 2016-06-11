#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_

#include"Processor.hh"
#include<string>

namespce eudaq{
 class ExamplePS: public Processor{
 public:
   ExamplePS(uint32_t psid, uint32_t psid_mother)
     :Processor("ExamplePS", psid, psid_mother){};
   virtual ~ExamplePS() {}; //? should it call the base destroyer?

   virtual void ProcessUserEvent(EVUP ev);
   virtual void ProcessCmdEvent(EVUP ev);
   
   static Processor* Create(uint32_t psid, uint32_t psid_mother);
 };
}

#endif
