#ifndef readAndProcessDataTemplate_h__
#define readAndProcessDataTemplate_h__
#include "MultiFileReader.hh"


/** Template class to read in multiple data files (raw files).
 *  example:
 *
 *  ReadAndProcess<myNewProcessorClass> readProcess;
 *  std::vector<unsigned> elementsOfInterest={0,1,3,4,76,666,999};
 *  readProcess.setEventsOfInterest(elementsOfInterest);   // adds the elemts of interest
 *  readProcess.addFileReader("myfilename01.raw");         //  adds files to read in
 *  readProcess.addFileReader("myfilename02.raw");         //  you can open multiple files at once
 *
 *  readProcess.setWriter(new myNewProcessorClass());      // add a processor class. only one precessor is allowed.
 *	                                                       // in order to work the processor class needs to have the following non member functions:
 *                                                         //  void helper_setParameter( myNewProcessorClass& writer,const std::string& tagName,const std::string& val)
 *                                                         //
 *                                                         //  void helper_StartRun(multiResender& writer,unsigned runnumber);
 *                                                         //  void helper_ProcessEvent(multiResender& writer,const DetectorEvent &ev);
 *                                                         //  void helper_EndRun(multiResender& writer);
 *
 *  readProcess.SetParameter("myTagName","myTagValue");    //  gives you the possibility to give parameters to the processor class
 *  readProcess.StartRun();                                //  begin of run
 *  readProcess.processNextEvent()                         //  processes only the next event
 *  readProcess.process();                                 //  Processes all events
 *  readProcess.EndRun();                                  //  indecates the end of run
 *
 */
template <typename processClass>
class ReadAndProcess{
public:
  
  ReadAndProcess(bool SyncEvents) :m_reader(SyncEvents){
  
  }
  void addFileReader(const std::string & filename, const std::string & filepattern){
    m_reader.addFileReader(filename, filepattern);
  }
  void addFileReader(const std::string & filename){
    m_reader.addFileReader(filename);
  }
  void setEventsOfInterest(std::vector<unsigned> EventsOfInterest){
    std::sort(EventsOfInterest.begin(), EventsOfInterest.end());
    m_eventsOfInterest = std::move(EventsOfInterest);
  }
  void setWriter(processClass* newPrecessor){
    if (m_processor)
    {
      EUDAQ_THROW("processor alredy defined");
    }
    m_processor = std::shared_ptr<processClass>(newPrecessor);

  }


  template <typename T>
  inline void SetParameter(const std::string& TagName, T TagValue){
    helper_setParameter(*m_processor, TagName, TagValue);
  }
  void StartRun(unsigned runnumber) {
    helper_StartRun(*m_processor, runnumber);
  }
  void StartRun(){
    helper_StartRun(*m_processor, m_reader.RunNumber());

  }
  void EndRun(){
    helper_EndRun(*m_processor);

  }
  void process(){
    int event_nr = 0;
    do {
      ++event_nr;
      if (event_nr % 1000 == 0)
      {
        std::cout << "Processing event " << event_nr << std::endl;
      }

    } while (processNextEvent());
  }
  bool processNextEvent(){
    if (CheckIfCurrentEventIsBeyondLastElementOfIntrest())
    {
      return false;
    }
    else if (CheckIfElementIsElementOfIntrest()) {
      helper_ProcessEvent(*m_processor, m_reader.GetDetectorEvent());
    }
    return m_reader.NextEvent();
  }

private:
  bool CheckIfCurrentEventIsBeyondLastElementOfIntrest(){
    if (m_eventsOfInterest.empty())
    {
      return false;
    }
    if (m_reader.GetDetectorEvent().GetEventNumber() > m_eventsOfInterest.back())
    {
      return true;
    }
    return false;
  }
  bool CheckIfElementIsElementOfIntrest(){

    if (m_reader.GetDetectorEvent().IsBORE()
      || m_reader.GetDetectorEvent().IsEORE()
      || m_eventsOfInterest.empty()
      || std::find(m_eventsOfInterest.begin(), m_eventsOfInterest.end(),
      m_reader.GetDetectorEvent().GetEventNumber()) != m_eventsOfInterest.end()
      )
    {
      return true;
    }
    return false;
  }
  std::shared_ptr<processClass> m_processor;
  eudaq::multiFileReader m_reader;
  std::vector<unsigned> m_eventsOfInterest;
};

#endif // readAndProcessDataTemplate_h__
