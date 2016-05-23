#ifndef Processor_bussy_test_h__
#define Processor_bussy_test_h__
#include "Processor_inspector.hh"

namespace eudaq{
  class Processor_busy_test :public Processor_Inspector{
  public:
    virtual ReturnParam inspecktEvent(const Event&);
    Processor_busy_test();
  private:
    size_t m_counter = 0;
  };

}

#endif // Processor_bussy_test_h__
