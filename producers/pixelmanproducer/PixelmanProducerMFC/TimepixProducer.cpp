#include "stdafx.h"
#include "eudaq/Producer.hh"
#include "TimepixProducer.h"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cctype> 

#ifndef WINVER
#define WINVER = 0x0501
#endif



TimepixProducer::TimepixProducer(const std::string & name,
					   const std::string & runcontrol, CPixelmanProducerMFCDlg* pixelmanCtrl)
    : eudaq::Producer(name, runcontrol), m_done(false), m_run(0) , m_ev(0)
{
    // First initialise the mutex attributes
    pthread_mutexattr_init(&m_mutexattr);

    // Inititalise the mutexes
    pthread_mutex_init( &m_done_mutex, 0 );
    pthread_mutex_init( &m_run_mutex, 0 );
    pthread_mutex_init( &m_ev_mutex, 0 );
	pthread_mutex_init( &m_status_mutex, 0);
	pthread_mutex_init( &m_commandQueue_mutex, 0);

	this->pixelmanCtrl = pixelmanCtrl;
}

TimepixProducer::~TimepixProducer()
{
	//PushCommand(TERMINATE);
    pthread_mutex_destroy( &m_done_mutex );
    pthread_mutex_destroy( &m_run_mutex );
    pthread_mutex_destroy( &m_ev_mutex );
	pthread_mutex_destroy( &m_status_mutex );
	pthread_mutex_destroy( &m_commandQueue_mutex );
}

bool TimepixProducer::GetDone()
{
    bool retval;
    pthread_mutex_lock( &m_done_mutex );
      retval = m_done;
    pthread_mutex_unlock( &m_done_mutex );    
    return retval;
}


unsigned int TimepixProducer::GetRunNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_run_mutex );
      retval = m_run;
    pthread_mutex_unlock( &m_run_mutex );    
    return retval;
}

unsigned int TimepixProducer::GetEventNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_ev_mutex );
      retval = m_ev;
    pthread_mutex_unlock( &m_ev_mutex );    
    return retval;
}

unsigned int TimepixProducer::GetIncreaseEventNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_ev_mutex );
      retval = m_ev++;
    pthread_mutex_unlock( &m_ev_mutex );
    return retval;
}

void TimepixProducer::SetDone(bool done)
{
    pthread_mutex_lock( &m_done_mutex );
       m_done = done;
    pthread_mutex_unlock( &m_done_mutex );    
}



void TimepixProducer::SetRunStatusFlag(timepix_producer_status_t status)
{
	pthread_mutex_lock( &m_status_mutex );
       m_status = status;
    pthread_mutex_unlock( &m_status_mutex );   

}

TimepixProducer::timepix_producer_status_t TimepixProducer::GetRunStatusFlag()
{
    timepix_producer_status_t retval;
    pthread_mutex_lock( &m_status_mutex );
      retval = m_status;
    pthread_mutex_unlock( &m_status_mutex );
    return retval;
}

TimepixProducer::timepix_producer_command_t TimepixProducer::PopCommand()
{
	timepix_producer_command_t retval;
	pthread_mutex_lock( &m_commandQueue_mutex );
		if (m_commandQueue.empty())
		{
			retval=NONE;
		}
		else
		{
			retval = m_commandQueue.front();
			m_commandQueue.pop();
		}
    pthread_mutex_unlock( &m_commandQueue_mutex );
	return retval;
}

void TimepixProducer::PushCommand(timepix_producer_command_t command)
{
	pthread_mutex_lock( &m_commandQueue_mutex );
		m_commandQueue.push(command);
    pthread_mutex_unlock( &m_commandQueue_mutex );
}

void TimepixProducer::SetEventNumber(unsigned int eventnumber)
{
    pthread_mutex_lock( &m_ev_mutex );
       m_ev = eventnumber;
    pthread_mutex_unlock( &m_ev_mutex );    
}

void TimepixProducer::SetRunNumber(unsigned int runnumber)
{
    pthread_mutex_lock( &m_run_mutex );
       m_run = runnumber;
    pthread_mutex_unlock( &m_run_mutex );    
}

void TimepixProducer::Event(i16 *timepixdata, u32 size)
{
    //static unsigned char *serialdatablock;

	eudaq::RawDataEvent ev("Timepix",GetRunNumber(), GetIncreaseEventNumber() );
	
    unsigned char *serialdatablock = new unsigned char[2*size];
        
    for (unsigned i=0; i < size ; i ++)
    {
		serialdatablock[2*i] = (timepixdata[i] & 0xFF00) >> 8 ;
		serialdatablock[2*i + 1] = timepixdata[i] & 0xFF ;
	}

    ev.AddBlock(  pixelmanCtrl->m_ModuleID.getInt(), serialdatablock , 2*size);

//	bool acqact = pixelmanCtrl->getAcquisitionActive();
//	Sleep(100);
//	bool acqact2 = pixelmanCtrl->getAcquisitionActive();

	for (int i=0; i < 3; i++)
	{
		try
		{
			SendEvent(ev);
			break;
		}
		catch(...)
		{
			if (i < 2)
			{
				EUDAQ_WARN("Device "+eudaq::to_string(pixelmanCtrl->m_ModuleID.getInt()) + ": " +
					eudaq::to_string(i) + ". Could not send event " + eudaq::to_string( ev.GetEventNumber() )
					+ ". Retrying...");
			}
				// Sleep to wait for possible network problems to resolve
			Sleep(500);
		}
		if (i==2)
		{
			EUDAQ_ERROR("Device "+eudaq::to_string(pixelmanCtrl->m_ModuleID.getInt()) + ": "
					     "Finally failed to  send event " + eudaq::to_string( ev.GetEventNumber() ));
			EUDAQ_WARN("Device "+eudaq::to_string(pixelmanCtrl->m_ModuleID.getInt()) + ": "
					     "Sending empty error event " + eudaq::to_string( ev.GetEventNumber() ));
			eudaq::RawDataEvent emptyEv("Timepix",GetRunNumber(),ev.GetEventNumber() );
			try
			{
				SendEvent(ev);
				break;
			}	
			catch(...)
			{
				EUDAQ_ERROR("Device "+eudaq::to_string(pixelmanCtrl->m_ModuleID.getInt()) + ": " +
					"Even sending the empty event failed, sorry");
			}	
		}
	}

	delete[] serialdatablock;
}

void TimepixProducer::SimpleEvent()
{
    // a 64 kWord data block in the computer whatever endian format
    unsigned short rawdatablock[65536];

    for (unsigned i=0; i < 65536 ; i ++)
    {
	rawdatablock[i] = i;
    }

    //ev.SetTag("Debug", "foo");
  //  Event(rawdatablock);
}

void TimepixProducer::BlobEvent()
{
//    TestEvent ev(m_run, ++m_ev, str);
//    //ev.SetTag("Debug", "foo");
//    SendEvent(ev);
}


void TimepixProducer::OnConfigure(const eudaq::Configuration & param) 
{
	if ( GetRunStatusFlag() == TimepixProducer::RUN_ACTIVE )
	{
		// give a warning to eudaq and do nothing
	    pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Cannot configure, run is active"));
		EUDAQ_WARN("Configure requested when run already active");
		SetStatus(eudaq::Status::LVL_WARN, "Configure requested when run already active");
		return;
	}
	else
	{ 
		DEVID devId = pixelmanCtrl->mpxDevId[pixelmanCtrl->mpxCurrSel].deviceId;
		pixelmanCtrl->mpxCtrlInitMpxDevice(devId);
		pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Configuring"));
		EUDAQ_INFO("Configured (" + param.Name() + ")");
		
		SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
	}
}

void TimepixProducer::OnStartRun(unsigned param) 
{	
	pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Start Of Run."));

	if ( GetRunStatusFlag() == TimepixProducer::RUN_ACTIVE )
	{
		// give a warning to eudaq and do nothing
	    EUDAQ_WARN("StartRun requested when run already active");
		SetStatus(eudaq::Status::LVL_WARN, "StartRun requested when run already active");
		return;
	}
	else
	{ 
		SetRunNumber( param );
		SetEventNumber( 1 ); // has to be 1 because BORE is event 0 :-(
		SendEvent(eudaq::RawDataEvent::BORE(_T("Timepix"), param )); // send param instead of GetRunNumber
		//std::cout << "Start Run: " << param << std::endl;
		//MessageBox(NULL, "Start of Run", "EudaqMessage", NULL);

		// send START_RUN command to the daq thread
		PushCommand( START_RUN );

		//SetStatus(eudaq::Status::LVL_OK, "Run Started1");
		// wait for the daq thread to raise the run active flag
		while ( GetRunStatusFlag() != RUN_ACTIVE )
		{
			Sleep(1);
		}

		EUDAQ_INFO("Run Started");
		SetStatus(eudaq::Status::LVL_OK, "Run Started");
			//Sleep(1);

	}
}

void TimepixProducer::OnStopRun()
{
	pixelmanCtrl->m_commHistRunCtrl.AddString(_T("End Of Run."));

	if ( GetRunStatusFlag() == TimepixProducer::RUN_STOPPED )
	{
		// give a warning to eudaq and do nothing
	    EUDAQ_WARN("StopRun requested when run not active");
		SetStatus(eudaq::Status::LVL_WARN, "StopRun requested when run not active");
	}
	else
	{
		// send STOP_RUN command to the daq thread
		PushCommand( STOP_RUN );

		// wait for the daq thread to lower the run active flag
		while ( GetRunStatusFlag() == RUN_ACTIVE )
		{
			Sleep(1);
		}
		
		//EUDAQ_DEBUG("Sending EORE");
		Sleep(1000);
		SendEvent(eudaq::RawDataEvent::EORE(_T("Timepix"),GetRunNumber(), GetEventNumber()));
		//std::cout << "Stop Run" << std::endl;
		//MessageBox(NULL, "End Of Run", "Message from Runcontrol",NULL);
		EUDAQ_INFO("Run Stopped");
		SetStatus(eudaq::Status::LVL_OK, "Run Stopped");

	}
}
 
void TimepixProducer::OnTerminate()
{	
	DEVID devId = pixelmanCtrl->mpxDevId[pixelmanCtrl->mpxCurrSel].deviceId;
	pixelmanCtrl->mpxCtrlAbortOperation(devId);
	pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Terminated (I'll be back)"));
	//std::cout << "Terminate (press enter)" << std::endl;
    SetDone( true );
	PushCommand( TERMINATE );
	
}
 
void TimepixProducer::OnReset()
{
	pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Reset."));
	PushCommand( RESET );
	
	//std::cout << "Reset" << std::endl;
    //SetStatus(eudaq::Status::LVL_OK);
}

void TimepixProducer::OnStatus()
{
	//pixelmanCtrl->m_commHistRunCtrl.AddString(_T("OnStatus Not Implemented."));
	//std::cout << "Status - " << m_status << std::endl;
    //SetStatus(eudaq::Status::LVL_OK, "Only joking");
}

void TimepixProducer::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
	pixelmanCtrl->m_commHistRunCtrl.AddString(_T("Unrecognised Command."));
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Just testing");
}