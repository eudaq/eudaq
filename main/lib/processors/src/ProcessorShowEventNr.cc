#include "Processor_inspector.hh"
#include <tuple>
#include "Processor_showEventNr.hh"
#include "Processors.hh"


using namespace std;

namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;

  ShowEventNR::ShowEventNR(size_t repetition) :m_rep(repetition) {

  }





  void ShowEventNR::init() {
    m_timming.reset();
  }




  void ShowEventNR::show(const Event& ev)
  {
    m_timming.processEvent(ev);
    std::cout << "Event nr: " << ev.GetEventNumber() << " current rate = " << (int)m_timming.freq_current << " [events/s].  overall rate  = " << (int)m_timming.freq_all_time<< " [events/s]." <<std::endl;
  }

  ReturnParam ShowEventNR::inspectEvent(const Event& ev, ConnectionName_ref con)
  {

    if (ev.GetEventNumber() <10)
    {
      show(ev);
    }
    else if (ev.GetEventNumber() % m_rep == 0)
    {
      show(ev);
    }
   
    return sucess;
  }
  void ShowEventNR::timeing::processEvent(const Event& ev) {
    auto newClock = std::clock();

    freq_all_time = (ev.GetEventNumber()) / ((double)(newClock - m_start_time) / CLOCKS_PER_SEC);

    freq_current = (ev.GetEventNumber() - m_last_event) / ((double)(newClock - m_last_time) / CLOCKS_PER_SEC);
    m_last_event = ev.GetEventNumber();
    m_last_time = std::clock();
  }
  void ShowEventNR::timeing::reset() {
    freq_all_time = 0;
    freq_current = 0;
    m_last_event = 0;
    m_last_time = std::clock();
    m_start_time = std::clock();
  }

  Processors::processor_i_up Processors::ShowEventNR(size_t repetition) {
    return __make_unique<eudaq::ShowEventNR>(repetition);
  }





}

