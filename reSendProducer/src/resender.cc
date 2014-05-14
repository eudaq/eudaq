#include "resender.h"





eudaq::resender::resender( const std::string & name, const std::string & runcontrol ) : eudaq::Producer(name, runcontrol),
	m_run(0), m_ev(0), stopping(false), done(false),started(0),m_name(name)
{

}

void eudaq::resender::OnConfigure( const eudaq::Configuration & config )
{
	std::cout << "Configuring: " << config.Name() << std::endl;


	// At the end, set the status that will be displayed in the Run Control.
	SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
}

void eudaq::resender::OnStartRun( unsigned param )
{
	m_run = param;
	m_ev = 0;

	std::cout << "Start Run: " << m_run << std::endl;

	if (!m_boreEvent)
	{
		EUDAQ_THROW("Bore event not set for Producer "+ m_name);
	}
	m_boreEvent->setRunNumber(m_run);
	std::cout<<" send bore Event. Producer: "<<m_name<<std::endl;
	SendEvent(*m_boreEvent);

	// At the end, set the status that will be displayed in the Run Control.
	SetStatus(eudaq::Status::LVL_OK, "Running");
	started=true;
}

void eudaq::resender::OnStopRun()
{
	std::cout << "Stopping Run" << std::endl;
	started=false;
	// Set a flag to signal to the polling loop that the run is over
	stopping = true;

	// wait until all events have been read out from the hardware
	while (stopping) {
		eudaq::mSleep(20);
	}

	// Send an EORE after all the real events have been sent
	// You can also set tags on it (as with the BORE) if necessary
	SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
}

void eudaq::resender::OnTerminate()
{
	std::cout << "Terminating..." << std::endl;
	done = true;
}

void eudaq::resender::resendEvent( std::shared_ptr<eudaq::Event> ev )
{
	// Loop until Run Control tells us to terminate

		while(!started)
		{
			// Now sleep for a bit, to prevent chewing up all the CPU
			eudaq::mSleep(20);
			// Then restart the loop
			
		}
		// If we get here, there must be data to read out
		// Create a RawDataEvent to contain the event data to be sent


	//	std::cout<<"resend producer: "<<m_name<<" event nr: " <<ev->GetEventNumber()<<std::endl;
		ev->setRunNumber(m_run);

		SendEvent(*ev);
		// Now increment the event number
		m_ev++;

		if (stopping) {
			// if so, signal that there are no events left
			stopping = false;
		}
	
}

void eudaq::resender::setBoreEvent( std::shared_ptr<eudaq::Event> boreEvent )
{
	m_boreEvent=boreEvent;
}
