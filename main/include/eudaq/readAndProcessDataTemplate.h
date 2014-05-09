#ifndef readAndProcessDataTemplate_h__
#define readAndProcessDataTemplate_h__
#include "MultiFileReader.hh"



template <typename processClass>
class ReadAndProcess{
public:
	void addFileReader(const std::string & filename, const std::string & filepattern){
		m_reader.addFileReader(filename,filepattern);
	}
	void addFileReader(const std::string & filename){
		m_reader.addFileReader(filename);
	}
	void setEventsOfInterest(std::vector<unsigned> EventsOfInterest){

		std::sort(EventsOfInterest.begin(),EventsOfInterest.end());
		m_eventsOfInterest=EventsOfInterest;
	}
	void setWriter(processClass* newPrecessor){
		if (m_processor)
		{
			EUDAQ_THROW("processor alredy defined");
		}
		m_processor=std::shared_ptr<processClass>(newPrecessor);

	}
	

	template <typename T>
	inline void SetParameter(const std::string& TagName,T TagValue){
		helper_setParameter(*m_processor,TagName,TagValue);
	}
	void StartRun(unsigned runnumber) {
		helper_StartRun(*m_processor,runnumber);
	}
	void StartRun(){
		helper_StartRun(*m_processor,m_reader.RunNumber());
		
	}
	void EndRun(){
		helper_EndRun(*m_processor);
		
	}
	void process(){
		int event_nr=0;
		do {
		
					++event_nr;
					if (event_nr%1000==0)
					{
						std::cout<<"Processing event "<< event_nr<<std::endl;
					}
	
		} while (processNextEvent());
	}
	bool processNextEvent(){
		if (CheckIfCurrentEventIsBeyondLastElementOfIntrest())
		{
			return false;
		}else if (CheckIfElementIsElementOfIntrest()) {
			helper_ProcessEvent(*m_processor,m_reader.GetDetectorEvent());
		}
		return m_reader.NextEvent();
	}

private:
	bool CheckIfCurrentEventIsBeyondLastElementOfIntrest(){
		if (m_eventsOfInterest.empty())
		{
			return false;
		}
		if (m_reader.GetDetectorEvent().GetEventNumber()>m_eventsOfInterest.back())
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
std::vector<unsigned> m_eventsOfInterest ;
};

#endif // readAndProcessDataTemplate_h__
