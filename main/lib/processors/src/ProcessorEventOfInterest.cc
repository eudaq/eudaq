#include "Processor_inspector.hh"
#include "eudaq/OptionParser.hh"
#include <algorithm>



namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;

  class Process_eventOfInterest :public Processor_Inspector{
  public:
    

    virtual ReturnParam inspecktEvent(const Event&);

  



    
    Process_eventOfInterest(const std::vector <unsigned>& );
    
  private:
    std::vector <unsigned> m_events;
    bool CheckIfCurrentEventIsBeyondLastElementOfIntrest(unsigned eventNR);
    bool CheckIfElementIsElementOfIntrest(unsigned eventNR);
  };






  Process_eventOfInterest::Process_eventOfInterest( const std::vector <unsigned>& events) : m_events(events)
  {

  }

  ReturnParam Process_eventOfInterest::inspecktEvent(const Event& ev)
  {
    if (ev.IsBORE()||ev.IsEORE())
    {
      return sucess;
    }
    if (m_events.empty())
    {
      return sucess;
    }

    if (CheckIfCurrentEventIsBeyondLastElementOfIntrest(ev.GetEventNumber()))
    {
      return stop;
    }
    if (!CheckIfElementIsElementOfIntrest(ev.GetEventNumber()))
    {
      return skip_event;
    }
    return sucess;
  }


  bool Process_eventOfInterest::CheckIfCurrentEventIsBeyondLastElementOfIntrest(unsigned eventNR){
    if (eventNR> m_events.back())
    {
      return true;
    }
    return false;
  }
  bool Process_eventOfInterest::CheckIfElementIsElementOfIntrest(unsigned eventNR){

    if (std::find(m_events.begin(), m_events.end(),eventNR) != m_events.end())
    {
      return true;
    }
    return false;
  }
}

