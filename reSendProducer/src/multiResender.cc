#include "multiResender.h"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Event.hh"
using namespace std;

void eudaq::multiResender::ProcessEvent( const DetectorEvent & devent)
{
	if (devent.IsBORE()) {

		firstEvent =true;
		for (unsigned i=0;i<devent.NumEvents();++i)
		{

			addProducer(devent.GetEventPtr(i));
		}
		m_runNumber=m_producer[0]->getRunNumber();
		return;
	} else if (devent.IsEORE()) {
		return;
	}else{
		while (!getStarted())
		{
			Sleep(20);
		}

	for (unsigned i=0;i<devent.NumEvents();++i)
	{
		
		m_producer[i]->resendEvent(devent.GetEventPtr(i));
		Sleep(10);
	}
	Sleep(20);
	}
}
std::string getProducerName(const std::string& eventName,const std::string& subEventName){

	if (eventName.compare("_TLU")==0)
	{
		return "TLU";
	}

	if (eventName.compare("_RAW")==0)
	{
		if (subEventName.compare("NI")==0)
		{
			return "MimosaNI";
		}
		if (subEventName.compare("SCTupgrade")==0)
		{
			return "SCTProducer";
		}
	}

	return "Error";
}
void eudaq::multiResender::addProducer(std::shared_ptr<Event> ev )
{
	cout << "ev->get_id(): "<<Event::id2str(ev->get_id())<<endl;
	cout<< "ev->GetSubType(): "<<ev->GetSubType()<<endl;
	std::string ProducerName=getProducerName(Event::id2str(ev->get_id()),ev->GetSubType());
	std::cout<<"ProducerName: "<<ProducerName<<std::endl;
	m_producer.push_back(std::make_shared<resender>(ProducerName,m_runcontrol));
	m_producer.back()->setBoreEvent(ev);
	Sleep(1000);
}

bool eudaq::multiResender::getStarted()
{
	for (auto& e:m_producer)
	{
		if (e->getStarted()==false)
		{
			return false;
		}
		
	}
	return true;
}
