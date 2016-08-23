#ifndef PROCESSORMANAGER_HH
#define PROCESSORMANAGER_HH

#include<utility>
#include<queue>
#include<map>
#include<mutex>

#include "Platform.hh"
#include "Processor.hh"

namespace eudaq{

  class Processor;
  class ProcessorManager;
  
  class DLLEXPORT ProcessorManager{
  public:
    static ProcessorManager* GetInstance();
  public:
    PSSP operator>>(PSSP psr);
    PSSP operator>>(std::string psr_str);
   
  private:
    std::vector<PSSP> m_pslist_root;
  };
}

#endif
