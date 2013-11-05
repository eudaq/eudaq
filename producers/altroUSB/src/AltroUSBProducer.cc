#include "AltroUSBProducer.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cctype>

#define REAL_DAQ

#ifdef REAL_DAQ

#include "ilcdaq.h"
#include "ilcproto.h"
#include <u2f/u2f.h>
#include <fec/fec.h>

#endif


AltroUSBProducer::AltroUSBProducer(const std::string & name,
					   const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_runactive(false),  m_configured(false), m_run(0) , m_ev(0), 
      m_daq_config(0), m_block_size(0), m_data_block(0)
{
    // Inititalise the mutexes
    pthread_mutex_init( &m_ilcdaq_mutex, 0 );
    pthread_mutex_init( &m_commandqueue_mutex, 0 );
    pthread_mutex_init( &m_runactive_mutex, 0 );
    pthread_mutex_init( &m_run_mutex, 0 );
    pthread_mutex_init( &m_ev_mutex, 0 );
//    pthread_mutex_init( &m_data_mutex, 0 );

    std::cout << "end of constructor" << std::endl;
}

AltroUSBProducer::~AltroUSBProducer()
{

#ifdef REAL_DAQ
    pthread_mutex_lock( &m_ilcdaq_mutex );

    if (DAQ_Close() != ILCDAQ_SUCCESS )
    {
	EUDAQ_INFO("U2F DAQ closed.");
	SetStatus(eudaq::Status::LVL_OK, "U2F DAQ closed.");
    }
    else
    {
	EUDAQ_INFO("Error closing DAQ");
	SetStatus(eudaq::Status::LVL_ERROR, "Could not close DAQ correctly");
    }

    // the ending sequence of  ilcsa
    DaqWriteLastConf();

    delete[] m_data_block;

    pthread_mutex_unlock( &m_ilcdaq_mutex );
#endif

    // Destroy all mutexes
    pthread_mutex_destroy( &m_ilcdaq_mutex );
    pthread_mutex_destroy( &m_commandqueue_mutex );
    pthread_mutex_destroy( &m_runactive_mutex );
    pthread_mutex_destroy( &m_run_mutex );
    pthread_mutex_destroy( &m_ev_mutex );
}

unsigned int AltroUSBProducer::GetRunNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_run_mutex );
      retval = m_run;
    pthread_mutex_unlock( &m_run_mutex );    
    return retval;
}

unsigned int AltroUSBProducer::GetEventNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_ev_mutex );
      retval = m_ev;
    pthread_mutex_unlock( &m_ev_mutex );    
    return retval;
}

bool AltroUSBProducer::GetRunActive()
{
    bool retval;
    pthread_mutex_lock( &m_runactive_mutex );
      retval = m_runactive;
    pthread_mutex_unlock( &m_runactive_mutex );    
    return retval;
}

unsigned int AltroUSBProducer::GetIncreaseEventNumber()
{
    unsigned int retval;
    pthread_mutex_lock( &m_ev_mutex );
      retval = m_ev++;
    pthread_mutex_unlock( &m_ev_mutex );
    return retval;
}

void AltroUSBProducer::SetEventNumber(unsigned int eventnumber)
{
    pthread_mutex_lock( &m_ev_mutex );
       m_ev = eventnumber;
    pthread_mutex_unlock( &m_ev_mutex );    
}

void AltroUSBProducer::SetRunNumber(unsigned int runnumber)
{
    pthread_mutex_lock( &m_run_mutex );
       m_run = runnumber;
    pthread_mutex_unlock( &m_run_mutex );    
}

void AltroUSBProducer::SetRunActive(bool activestatus)
{
    pthread_mutex_lock( &m_runactive_mutex );
       m_runactive = activestatus;
    pthread_mutex_unlock( &m_runactive_mutex );    
}

//void AltroUSBProducer::Event(volatile unsigned long *altrodata, int length)
//{
//    eudaq::RawDataEvent ev("AltroUSB",GetRunNumber(), GetIncreaseEventNumber() );
//
//    // a data block of unsigned char, in this the data is stored in little endian
//    unsigned char *serialdatablock =  new unsigned char[length*sizeof(unsigned long)];
//    
//    for (int i=0; i < length ; i ++)
//    {
//	for (unsigned int j = 0; j < sizeof(unsigned long); j++)
//	{
//	    // send little endian, i. e. the most significant first
//	    serialdatablock[sizeof(unsigned long)*i+j] 
//		= (altrodata[i] & (0xFF << 8*j)) >> (sizeof(unsigned long)-j-1)*8 ;
//	}
//    }
//
//    ev.AddBlock(serialdatablock , length*sizeof(unsigned long));
//
//    SendEvent(ev);
//    delete[] serialdatablock;
//}

void AltroUSBProducer::OnConfigure(const eudaq::Configuration & param) 
{
//    std::cout << "Configuring." << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "Wait (" + param.Name() + ")");

    // Test if the run is active, return a warning that we cannot reconfigure
    if ( GetRunActive() )
    {
	EUDAQ_WARN("Ignoring request to reconfigure because run is still active.");
	SetStatus(eudaq::Status::LVL_WARN, "Cannot reconfigure, run is active.");
	return;
    }

    SetStatus(eudaq::Status::LVL_WARN, "Wait while configuring ...");    

    // get the file names from the config
    std::string configdaqfilename   =  param.Get("ConfigDaq",   CONFIG_DAQ);
    std::string configaltrofilename =  param.Get("ConfigAltro", CONFIG_ALTRO);
    
    std::cout << "configuring" << std::flush;

#ifdef REAL_DAQ
    std::cout << " with real hardware" << std::flush;
    
    // lock the mutex to protext the C library
    pthread_mutex_lock( &m_ilcdaq_mutex );    

    // the initialisation part "copied" from ilcsa
    if ( DaqConfigRead( configdaqfilename.c_str() ) )
    {
	EUDAQ_ERROR("Error reading daq config file " + configdaqfilename
		    + " (" + param.Name() + ")");
	SetStatus(eudaq::Status::LVL_THROW, "Config Error (" + param.Name() + ")");
	std::cout << "throw an exception?" << std::endl;
	return;
    }

    if ( AltroConfigRead( configaltrofilename.c_str() ) )
    {
	EUDAQ_ERROR("Error reading altro config file " + configaltrofilename
		    + " (" + param.Name() + ")");
	SetStatus(eudaq::Status::LVL_THROW, "Config Error (" + param.Name() + ")");
	std::cout << "throw an exception?" << std::endl;
	return;
    }

    DaqReadLastConf();

    m_daq_config =  GetDaqConfig();  


    // now comes the testing part (??? what does tis comment mean, it's from the testdaq.cxx
    if (DAQ_Open() != ILCDAQ_SUCCESS )
    {
	EUDAQ_ERROR("Failed to open daq (" + param.Name() + ")");
	SetStatus(eudaq::Status::LVL_ERROR, "Could not open DAQ (" + param.Name() + ")");	
    }
    else
    {
	EUDAQ_INFO("DAQ opened (" + param.Name() + ")");
	SetStatus(eudaq::Status::LVL_OK, "DAQ opened (" + param.Name() + ")");
    }
#endif

    //allocate memory depending on config?
    // no max 1024 10bitWords per channel (2 bytes each) = 2048 bytes / channel
    // + 8 bytes altro trailer word per channel
    // max 16 FECs with 128 channels each
    // + U2F trailer (a few (32bit?) words)
    // (2048+8)*16*128 = 4210688 bytes
    // allocate  4300800 = 4200 * 1024 bytes , that should always be enough for one u2f
    if (m_data_block == 0)
    {
	m_block_size = 4300800;
	m_data_block = new unsigned char[m_block_size];
    }
    

    pthread_mutex_unlock( &m_ilcdaq_mutex );

    std::cout << " ... done " << std::endl;

    m_configured = true;
    EUDAQ_INFO("Configured (" + param.Name() + ")");
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");

}

void AltroUSBProducer::OnStartRun(unsigned param) 
{
    // check that no run is active
    if (GetRunActive())
    {
	EUDAQ_WARN("Ignoring request to start run, run is already active.");
	SetStatus(eudaq::Status::LVL_WARN, "run is active, cannot start run");	
	return;
    }

    // check that configuration has been run. This ensures that the hardware is set up correctly
    // and the memory block is allocated.
    if (!m_configured)
    {
	EUDAQ_WARN("Cannot start run, hardware is nor confiugred. Run configure first!");
	SetStatus(eudaq::Status::LVL_WARN, "Not configured, cannot start run.");	
	return;
    }
    
    SetStatus(eudaq::Status::LVL_WARN, "Wait until run has started...");    

    SetRunNumber( param );
    SetEventNumber( 0 ); // has to be 1 because BORE is event 0 :-(
    SendEvent(eudaq::RawDataEvent::BORE( "AltroUSB", param )); // send param instead of GetRunNumber
//    std::cout << "Start Run: " << param << std::endl;

    // Tell the main loop to stop the run
    CommandPush( START_RUN );

    // This is important:
    // Wait for DAQ to turn on the runactive flag
    // The communication thread must not continue until the run active flag is on
    while (!GetRunActive())
	eudaq::mSleep(1);
    

    EUDAQ_INFO("U2F is ready to accept triggers");
    SetStatus(eudaq::Status::LVL_OK, "U2F is ready to accept triggers");
}

void AltroUSBProducer::OnStopRun()
{
    if (!GetRunActive())
    {
	EUDAQ_WARN("Ignoring request to stop run, no run active.");
	SetStatus(eudaq::Status::LVL_WARN, "Cannot stop run, no run active");	
	return;
    }

    SetStatus(eudaq::Status::LVL_WARN, "Wait until run has stopped...");    


    // Tell the main loop to stop the run
    CommandPush( STOP_RUN );

    // Wait for DAQ to turn off the runactive flag
    while (GetRunActive())
	eudaq::mSleep(1);

    // now we know the run has stopped, send eore
    SendEvent(eudaq::RawDataEvent::EORE("AltroUSB", GetRunNumber(), GetEventNumber()));
    
    EUDAQ_INFO("U2F has finished the run. DAQ is off");
    SetStatus(eudaq::Status::LVL_OK, "run finished.");

}
 
void AltroUSBProducer::OnTerminate()
{
    EUDAQ_DEBUG("Sending terminate command to readout thread.");
    // Tell the main loop to terminate
    CommandPush( TERMINATE );
}
 
void AltroUSBProducer::OnReset()
{
    std::cout << "Reset" << std::endl;
    // Tell the main loop to terminate
    CommandPush( RESET );
//    SetStatus(eudaq::Status::LVL_OK);
}

void AltroUSBProducer::OnStatus()
{
    // Don't send status to the main loop. There are so many status requests that 
    // they will obscure all other commands. Handle this in the communication thread if necessary, 
    // i. e. in this function.
    // For the time being: do nothing

}

void AltroUSBProducer::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Received unkown command " + cmd);
}

AltroUSBProducer::Commands AltroUSBProducer::CommandPop()
{
    Commands retval;

    pthread_mutex_lock( &m_commandqueue_mutex );
       if (m_commandQueue.empty())
       {
	   retval = NONE;
       }
       else
       {
	   retval = m_commandQueue.front();
	   m_commandQueue.pop();
       }
    pthread_mutex_unlock( &m_commandqueue_mutex );

    return retval;
}

void  AltroUSBProducer::CommandPush(Commands c)
{
    pthread_mutex_lock( &m_commandqueue_mutex );
       m_commandQueue.push(c);
    pthread_mutex_unlock( &m_commandqueue_mutex );    
}

void  AltroUSBProducer::Exec()
{
    std::cout << "starting exec" << std::endl;
    int counter =0;

    // flag whether to terminate the producer
    bool terminate=false;
    // flag set if the run is to be finished, i.e. trigger is switched off, all remaining events are read out
    bool finish_run = false;

    // mode flag for the acquisiton, can contain M_FIRST, M_TRIGGER and M_LAST
    unsigned int acquisition_mode = 0;


    while(!terminate)
    {
	if ( (counter++%1000 == 0) || GetRunActive() )
	    std::cout << "blip" <<counter <<std::endl;

	// look if there are any commands in the buffer
	Commands command = CommandPop();
	if (command && (command!=STATUS)) 
	    std::cout << "command is " << command << std::endl;
	
	switch( command )
	{
	    case NONE: break; // no command, nothing to do
	    case START_RUN:    
		
#ifdef REAL_DAQ
		pthread_mutex_lock( &m_ilcdaq_mutex );
		
		  // start the daq
		  DAQ_Start();

		  // enalble the trigger
		  // bit 0: u2f will push data to usb
		  // bit 1: enable hardware trigger
		  U2F_Reg_Write(m_daq_config->devices[0].handle, O_TRCFG2, 3);
		  acquisition_mode = M_FIRST;

		pthread_mutex_unlock( &m_ilcdaq_mutex );
#endif
		  
		// set the run active flag
		SetRunActive(true);
		finish_run = false;
		break; 

	    case TERMINATE:
		// turn off an ongoing run
		EUDAQ_DEBUG("Received terminate command in readout thread");
		terminate=true;
		// no break here to execute the stop run functionality

	    case STOP_RUN:
#ifdef REAL_DAQ
		pthread_mutex_lock( &m_ilcdaq_mutex );

		// turn off the trigger on the hardware
		U2F_Reg_Write(m_daq_config->devices[0].handle, O_TRCFG2, 0);

		pthread_mutex_unlock( &m_ilcdaq_mutex );
#endif

		// set the finish run flag which reads out all events that have arrived 
		finish_run = true;
//		// stop the daq
//		DAQ_Stop();
//		
//		// set run active flag to false
//		SetRunActive(false);
		break; 	       
	    case STATUS:
		// send the status to eudaq
		break;
	    case RESET:
		// don't know how to reset
		EUDAQ_WARN("Reset requested, don't know how to perform that.");
		SetStatus(eudaq::Status::LVL_WARN, "Reset is not implemented");
		break;
	    default:
		EUDAQ_THROW("Unknown command in AltroUSBProducer::Exec()");
	}

	if (!GetRunActive())
	{
	    // if the run is not active
	    // wait 1 ms and return to the start of the loop
	    eudaq::mSleep(1);
	    continue;
	}
	else // run is active, read out next event
	{
#ifdef REAL_DAQ
	    pthread_mutex_lock( &m_ilcdaq_mutex );
	    // perform the readout
	    if (finish_run)
	    {
		// set the last acquisiton flag
		acquisition_mode |= M_LAST;
	    }

	    // read the altro in block of 1024 bytes. This is the minimal size, and the size of a USB burst
	    // Like this it is ensured that the data is shipped once the event is finished, and is read
	    // as soon as it is available
	    unsigned int nbytesread = 0;
	    for (unsigned int block_index = 0 ;   block_index < m_block_size/1024 ; block_index++)
	    {

		std::cout << "bing "<< std::endl;

		unsigned int retval =0;
		unsigned int osize = 0;
		
		std::cout <<  "Readings in mode "<< acquisition_mode <<" to address " <<
		    std::hex << reinterpret_cast<void *>(m_data_block +( block_index * 1024)) << std::dec << std::endl;

		// read the next (up to) 1024 bytes. Always write to the next block
		retval = U2F_ReadOut(m_daq_config->devices[0].handle, 1024, &osize, 
				     m_data_block +( block_index * 1024) , acquisition_mode);

		// delete the first acquisition flag
		acquisition_mode &= (~M_FIRST);

		std::cout <<  "Return value of  U2F_ReadOut is "<< retval 
			  << " , osize is " << osize<< std::endl;
		
		nbytesread += osize;

		if (osize >= 8)
		{
		    std::cout << std:: hex << "0x "  << std::setfill('0')
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-8])
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-7])
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-6]) 
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-5])
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-4]) 
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-3]) 
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-2]) 
			      << std::setw(2) << static_cast<unsigned int>(m_data_block[nbytesread-1]) 
			      << std::dec << std::endl;
			}

		// check if event is finished, the last 8 bytes have to be 0xff
		if( osize ) // only do this check if the outsize is not 0
		{
		    if ( ( m_data_block[nbytesread-1] == 0xff ) && 
			 ( m_data_block[nbytesread-2] == 0xff ) && 
			 ( m_data_block[nbytesread-3] == 0xff ) && 
			 ( m_data_block[nbytesread-4] == 0xff ) && 
			 ( m_data_block[nbytesread-5] == 0xff ) && 
			 ( m_data_block[nbytesread-6] == 0xff ) && 
			 ( m_data_block[nbytesread-7] == 0xff ) && 
			 ( m_data_block[nbytesread-8] == 0xff ) )
		    { // end of event signature found
			std::cout << "End of event signature found" << std::endl;
			break;
		    }
		    else // number of bytes has to be 1024, otherwise there is something wrong
		    {
			if (osize != 1024)
			{
			    EUDAQ_THROW("Error reading U2F, data block is < 1024 bytes");
			}
		    }
		}
		else // check for new commands 
		{
		    break;
		}//if osize
		 
		std::cout << "bong "<< std::endl;
	    } // for blockindex
	    
            // dump 32 bit words instead of sending an eudaq event for the time being
	    
	    // if the input buffer is full, but there is no end of event signature:
	    // throw an exception, the event is incomplete. How could this happen?
	    if (nbytesread == m_block_size)
	    {
		EUDAQ_THROW("Error: Input buffer in memory is full!");
	    }

	    // create a RawDataEvent with the data that has been read
	    if (nbytesread)
	    {
		eudaq::RawDataEvent event("AltroUSB",GetRunNumber(),GetIncreaseEventNumber());
		// the last 12 bytes are the u2f trailer, they are not altro data
		event.AddBlock(0, m_data_block, nbytesread - 12);
	    
		// Send the event to the data collector
		SendEvent (event);
	    }	    

//	    for (unsigned int i = 0; i < nbytesread ; i++)
//	    {
//		if (i%4==0)
//		    std::cout << std::dec<<  std::endl << i/4  <<"\t 0x " << std::hex << std::setfill('0');
//		
//		std::cout << std::setw(2) << static_cast<unsigned short>(m_data_block[i]) <<" ";
//	    }
//	    std::cout << std::dec << std::endl;
//	    


	    pthread_mutex_unlock( &m_ilcdaq_mutex );
#endif
	    
	    // if the run is to be finished turn off the daq and the run active flagg
	    if (finish_run)
	    {
#ifdef REAL_DAQ
		pthread_mutex_lock( &m_ilcdaq_mutex );
		  DAQ_Stop();
		pthread_mutex_unlock( &m_ilcdaq_mutex );
#endif

		SetRunActive(false);
	    }

	    // if the run is not active
	    // wait 1 ms and return to the start of the loop
//	    eudaq::mSleep(100);
	    std::cout << "processing event" << GetEventNumber() << std::endl;;	    
	}//  if (getRunActive)

    }// while !terminate
}
