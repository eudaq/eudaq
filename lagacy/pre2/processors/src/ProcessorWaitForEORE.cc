#include "ProcessorBase.hh"
#include "Processors.hh"
#include <atomic>


#define  waiting 0
#define runnning 1
#ifndef MAXINT32
#define MAXINT32 2147483647
#endif

namespace eudaq {
  class addEORE {
  public:
    addEORE(bool isEore, std::atomic<int>* eore_counter) :m_isEore(isEore), m_EORE_counter(eore_counter) {
    }
    ~addEORE() {
      if (m_isEore)
      {
        ++(*m_EORE_counter);
      }
    }

    bool m_isEore;
    std::atomic<int>* m_EORE_counter;
  };

  class Processor_WaitForEORE :public ProcessorBase {

  public:
    Processor_WaitForEORE(int timeWaiting);
    virtual void init() override;
    virtual void end() override;
    virtual ReturnParam ProcessEvent(event_sp, ConnectionName_ref con) override;
    virtual void wait() override;
  private:
    std::atomic<int> m_numOfBoreEvents, m_numOfEoreEvents;
    int m_timeWaiting;
    std::atomic<int> m_lastEvent;
    std::atomic<int> m_status;
  };

  Processor_WaitForEORE::Processor_WaitForEORE(int timeWaiting) : m_timeWaiting(timeWaiting), m_numOfBoreEvents(0), m_numOfEoreEvents(0), m_status(waiting) {
    m_lastEvent = MAXINT32;
  }

  void Processor_WaitForEORE::init() {
    m_numOfBoreEvents = 0;
    m_numOfEoreEvents = 0;
    m_lastEvent = MAXINT32;
    m_status = runnning;

  }

  void Processor_WaitForEORE::end() {
    m_status = waiting;
  }

  ProcessorBase::ReturnParam Processor_WaitForEORE::ProcessEvent(event_sp ev, ConnectionName_ref con) {
    m_lastEvent = static_cast<int>(std::clock());
    if (ev->IsBORE()) {
      ++m_numOfBoreEvents;
    }

    addEORE eore_counter(ev->IsEORE(), &m_numOfEoreEvents);
    return  processNext(std::move(ev), con);
  }

  void Processor_WaitForEORE::wait() {

    while (true) {
      eudaq::mSleep(100);
      if (m_status != runnning) {
        break;
      }
      if (m_numOfBoreEvents == 0) {
        continue;
      }
      if (m_numOfBoreEvents == m_numOfEoreEvents) {
        break;
      }

      if ((static_cast<int>(std::clock()) - m_lastEvent) > m_timeWaiting) {
        std::cout << "timeout" << std::endl;
        break;
      }

    }
  }
  Processors::processor_up Processors::waitForEORE(int timeIn_ms/*=200*/) {
    return make_Processor_up<Processor_WaitForEORE>(timeIn_ms);
  }
}
