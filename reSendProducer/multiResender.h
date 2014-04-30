#ifndef multiResender_h__
#define multiResender_h__
#include <string>
#include <vector>
#include "resender.h"
#include <memory>
#define TAGNAME_RUNCONTROL "runControl"
#include "eudaq/DetectorEvent.hh"

namespace eudaq{

	class Event;
	class multiResender{
	public:
		multiResender():m_runNumber(0),firstEvent(0){}
		void StartRun(unsigned runnumber){
			
		m_runNumber=runnumber;
		
		}
		void EndRun() {};
		void ProcessEvent(const DetectorEvent &);
		void addProducer(std::shared_ptr<Event> borEvent );
		void SetRunControl(const std::string & p) {m_runcontrol=p; }
		bool getStarted();
	private:
		unsigned m_runNumber;
		std::vector<std::shared_ptr<resender>> m_producer;
		bool firstEvent;
		std::string m_runcontrol;
	};
	
	inline void helper_setParameter(multiResender& writer,const std::string& tagName,const std::string& val){
		std::cout<<tagName<<" "<< val<<std::endl;
		if (tagName.compare(TAGNAME_RUNCONTROL)==0)
		{
			//std::cout<<"set output pattern to: "<<tagValue<<std::endl;
			writer.SetRunControl(val);
			
		}
	}
	inline void helper_ProcessEvent(multiResender& writer,const DetectorEvent &ev){writer.ProcessEvent(ev);}
	inline void helper_StartRun(multiResender& writer,unsigned runnumber){writer.StartRun(runnumber);}
	inline void helper_EndRun(multiResender& writer){};

}
#endif // multiResender_h__
