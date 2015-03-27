#ifndef readAndProcessDataTemplate_h__
#define readAndProcessDataTemplate_h__
#include "eudaq/MultiFileReader.hh"
#include "eudaq/OptionParser.hh"


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

  ReadAndProcess() {

  }
  //   void addFileReader(const std::string & filename, const std::string & filepattern){
  //     m_reader->addFileReader(filename, filepattern);
  //   }
  //   void addFileReader(const std::string & filename){
  //     m_reader->addFileReader(filename);
  //   }
  void addFileReader(std::unique_ptr<eudaq::baseFileReader> fileReader){

    m_reader = std::move(fileReader);
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

  void setWriter(std::unique_ptr<processClass> newPrecessor){
    if (m_processor)
    {
      EUDAQ_THROW("processor alredy defined");
    }
    m_processor = std::move(newPrecessor);

  }

  template <typename T>
  inline void SetParameter(const std::string& TagName, T TagValue){
    helper_setParameter(*m_processor, TagName, TagValue);
  }
  void StartRun(unsigned runnumber) {
    helper_StartRun(*m_processor, runnumber);
  }
  void StartRun(){
    helper_StartRun(*m_processor, m_reader->RunNumber());

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
      helper_ProcessEvent(*m_processor, m_reader->GetEvent());
    }
    return m_reader->NextEvent();
  }

private:
  bool CheckIfCurrentEventIsBeyondLastElementOfIntrest(){
    if (m_eventsOfInterest.empty())
    {
      return false;
    }
    if (m_reader->GetEvent().GetEventNumber() > m_eventsOfInterest.back())
    {
      return true;
    }
    return false;
  }
  bool CheckIfElementIsElementOfIntrest(){

    if (m_reader->GetEvent().IsBORE()
        || m_reader->GetEvent().IsEORE()
        || m_eventsOfInterest.empty()
        || std::find(m_eventsOfInterest.begin(), m_eventsOfInterest.end(),
        m_reader->GetEvent().GetEventNumber()) != m_eventsOfInterest.end()
        )
    {
      return true;
    }
    return false;
  }
  std::unique_ptr<eudaq::baseFileReader> m_reader;
  std::unique_ptr<processClass> m_processor;
  std::vector<unsigned> m_eventsOfInterest;
public:
  static std::unique_ptr<eudaq::Option<std::string>>  add_Command_line_option_EventsOfInterest(eudaq::OptionParser & op)
  {

    return std::unique_ptr<eudaq::Option<std::string>>(new eudaq::Option<std::string>(op, "e", "events", "", "numbers", "Event numbers to process (eg. '1-10,99' default is all)"));


  }
};

#endif // readAndProcessDataTemplate_h__
