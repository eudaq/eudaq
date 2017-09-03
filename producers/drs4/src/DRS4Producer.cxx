// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "DRS4Producer.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>
#include <cmath>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#include <Processthreadsapi.h>
#include <Windows.h>
#else 
#include <sched.h>
#endif

using namespace std;
static const std::string EVENT_TYPE = "DRS4";

DRS4Producer::DRS4Producer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)  
	                       : eudaq::Producer(name, runcontrol),
                             m_run(0),
                             m_ev(0),
                             m_producerName(name),
                             m_tlu_waiting_time(4000),
                             m_event_type(EVENT_TYPE),
                             m_self_triggering(false),
                             m_inputRange(0.),
                             m_running(false), 
                             m_terminated(false),
                             is_initalized(false)
{
	n_channels = 4;
	cout << "Started DRS4Producer with Name: \"" << name << "\"" <<endl;
	m_b = 0;
	m_drs = 0;
	cout << "Event Type: " << m_event_type << endl;
	cout << " Get DRS() ..."<<endl;
	if (m_drs) delete m_drs;
	m_drs = new DRS();
	cout <<" DRS: "<< m_drs << endl;
	int nBoards = m_drs->GetNumberOfBoards();
	cout << " NBoards : " << nBoards << endl;
	for (int ch = 0; ch < 8; ch++) m_chnOn[ch] = false; //set all channels on per default
}

void DRS4Producer::OnStartRun(unsigned runnumber)
{
	m_run = runnumber;
	m_ev = 0;
	try {
		 this->SetTimeStamp();
		 std::cout << "Start Run: " << m_run << " @ "<<m_timestamp<<std::endl;
		 std::cout<<"Create event"<<m_event_type<<" "<<m_run<<std::endl;
		 eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(m_event_type, m_run));
		 std::cout << "drs4 board"<<std::endl;
		 bore.SetTag("DRS4_n_channels", n_channels);
		 bore.SetTag("DRS4_serial_no", std::to_string(m_b->GetBoardSerialNumber()));

		 // Set Firmware name
		 std::cout << "Set tag fw"<<std::endl;
		 bore.SetTag("DRS4_FW", std::to_string(m_b->GetFirmwareVersion()));
		 bore.SetTag("actived_channels",std::to_string(activated_channels));
		 bore.SetTag("device_name",(string)"DRS4");
		 // Set Names for different channels
		 for (int ch = 0; ch < n_channels; ch++)
		     {
		 	  string tag = "CH_"+std::to_string(ch+1);
			  string value;
			  if (!m_chnOn[ch]) value = "OFF";
			  else value = m_config.Get(tag, tag);
			  bore.SetTag(tag, value);
			  cout<<"\tBore tag: "<<tag<<"\t"<<value<<endl;
		     }

		bore.SetTag("DRS4_Board_type",std::to_string(m_b->GetBoardType()));
		bore.SetTag("input_Range",std::to_string(m_inputRange));
		unsigned char *buffer = (unsigned char *)malloc(10);
		unsigned char *p = buffer;
		int block_id = 0;
		int waveDepth = m_b->GetChannelDepth();
		cout<<"WaveformDepth: "<<waveDepth<<endl;
		for (int i=0 ; i<n_channels ; i++) 
		    {
			 if (m_chnOn[i]) 
			    {
				 cout<<"TimeCalibration CH"<<i+1<<":"<<endl;
				 float tcal[2048];
				 m_b->GetTimeCalibration(0, i*2, 0, tcal, 0);
				 if (waveDepth == 2048) for (int j=0 ; j<waveDepth ; j++) {
						// save binary time as 32-bit float value
						tcal[j/2] = (tcal[j]+tcal[j+1])/2;
						j++;
					}
				cout << endl;
                //cout << "Set Buffer" << i+1 << endl;
				char buffer [6];
				sprintf((char *)buffer, "C%03d\n", i+1);
				//				bore.AddBlock(block_id,reinterpret_cast<const char*>(&i),sizeof(i));
				//cout<<"Add block"<<endl;
				bore.AddBlock(block_id++,reinterpret_cast<const char*>(buffer),sizeof(*buffer)*4);
				//cout<<"Add block"<<endl;
				bore.AddBlock(block_id,reinterpret_cast<const char*>(&tcal), sizeof( tcal[0])*1024);
			}
		}
		std::cout << "Send event"<<std::endl;
		// Send the event out:
		SendEvent(bore);

		std::cout << "BORE for DRS4 Board: (event type"<<m_event_type<<")\tself trigger: "<<m_self_triggering<<endl;
		SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
		m_running = true;
		/* Start Readout */
		if (m_b) m_b->StartDomino();
	}
	catch (...){
		EUDAQ_ERROR(string("Unknown exception."));
		SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
	}

};

void DRS4Producer::OnStopRun() 
{
	// Break the readout loop

    // Wait before we stop the DAQ because TLU takes some time to pick up the OnRunStop signal
    // otherwise the last triggers get lost.
    eudaq::mSleep(m_tlu_waiting_time);
	m_running = false;
	std::cout << "Run stopped." << std::endl;
	try {
	    // Stop DRS Board
		if (m_b && m_b->IsBusy()) {
			m_b->SoftTrigger();
			for (int i=0 ; i<10 && m_b->IsBusy() ; i++)
				eudaq::mSleep(10);//todo not mt save
		}

	    // Sending the final end-of-run event:
	    SendEvent(eudaq::RawDataEvent::EORE(m_event_type, m_run, m_ev));
	    std::cout << "Stopped" << std::endl;

	    // Output information for the logbook:
	    std::cout << "RUN " << m_run << " DRS4 " << std::endl
	          << "\t Total triggers:   \t" << m_ev << std::endl;
	    EUDAQ_USER(string("Run " + std::to_string(m_run) + ", ended with " + std::to_string(m_ev)
	              + " Events"));

		SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
	} catch ( ... ){
		printf("While Stopping: Unknown exception\n");
		SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
	}

};

void DRS4Producer::OnTerminate() 
{
	m_terminated = true;

	  // If we already have a pxarCore instance, shut it down cleanly:
	  if (m_drs != NULL) 
	     {
	      delete m_drs;
	      m_drs = NULL;
	     }

	  std::cout << "DRS4Producer " << m_producerName << " terminated." << std::endl;
};

void DRS4Producer::ReadoutLoop() 
{
    std::cout<<"Start ReadoutLoop "<<m_terminated<<std::endl;
	int k = 0;
	while (!m_terminated) {
		// No run is m_running, cycle and wait:
		if(!m_running) {
			// Move this thread to the end of the scheduler queue:
#ifdef _MSC_VER
			SwitchToThread();
#else
			sched_yield();
#endif
			continue;
		}
		else {
			if (!m_b) continue;
			k++;
			//Check if ready for triggers
			if ( m_b->IsBusy()){
				if (m_self_triggering && k%(int)1e4 == 0 ){
					cout<<"Send software trigger"<<endl;
					m_b->SoftTrigger();
					eudaq::mSleep(10); //todo not mt save
				}
				else continue;

			}
			// Trying to get the next event, daqGetRawEvent throws exception if none is available:
			try {
				 SendRawEvent();
                 std::cout << "DRS4 Event number: " << m_ev << std::endl;
				 if(m_ev%1000 == 0) std::cout << "DRS4 Board " << " EVT " << m_ev << std::endl;
			    }
			catch(int e) 
			     {
				  // No event available in derandomize buffers (DTB RAM), return to scheduler:
				  cout << "An exception occurred. Exception Nr. " << e << '\n';
#ifdef _MSC_VER
				  SwitchToThread();
#else
				  sched_yield();
#endif
			     }
		}
	}
    std::cout<<"ReadoutLoop Done. " << std::endl;
};

void DRS4Producer::OnConfigure(const eudaq::Configuration& conf) 
{
	cout << "Configure DRS4 board"<<endl;
	m_config = conf;
	while (!m_drs){
		cout<<"wait for configure"<<endl;
		eudaq::mSleep(10);
	}

	try {
		/* do initial scan */
		m_serialno = m_config.Get("serialno",-1);

		int nBoards = m_drs->GetNumberOfBoards();
		m_self_triggering = m_config.Get("self_triggering",false);
		m_n_self_trigger = m_config.Get("n_self_trigger",1e5);
		cout<<"Config: "<<endl;
		cout<<"Show boards..."<<endl;
		cout<<"There are "<<nBoards<<" DRS4-Evaluation-Board(s) connected:"<<endl;
		/* show any found board(s) */
		int board_no = 0;
		for (int i=0 ; i < m_drs->GetNumberOfBoards() ; i++) {
			DRSBoard* board = m_drs->GetBoard(i);
			printf("    #%2d: serial #%d, firmware revision %d\n",
					(int)i, board->GetBoardSerialNumber(), board->GetFirmwareVersion());
			if (board->GetBoardSerialNumber() == m_serialno)
				board_no = i;
		}

		/* exit if no board found */
		if (!nBoards){
			EUDAQ_ERROR(string("No Board connected exception."));
			return;
		}

		cout <<"Get board no: "<<board_no<<endl;
		/* continue working with first board only */
		if (m_b != m_drs->GetBoard(board_no)){
		    m_b = m_drs->GetBoard(board_no);
		    cout<<"Init"<<endl;
		    /* initialize board */
		    m_b->Init();
		}
		else if (m_b == 0){
		    throw eudaq::Exception("Cannot find board");
		}
		else{
		    cout<<"ReInit"<<endl;
		    m_b->Reinit();
		}

		double sampling_frequency = m_config.Get("sampling_frequency",5);
		bool wait_for_PLL_lock = m_config.Get("wait_for_PLL_lock",true);
		cout<<"Set Freqeuncy: "<<sampling_frequency<<" | Wait for PLL lock" <<wait_for_PLL_lock<<endl;
		/* set sampling frequency */
		m_b->SetFrequency(sampling_frequency, wait_for_PLL_lock);

		cout<<"Set Transparent mode"<<endl;
		/* enable transparent mode needed for analog trigger */
		m_b->SetTranspMode(1);

		/* set input range to -0.5V ... +0.5V */
		m_inputRange =  m_config.Get("input_range_center",0.0f);
		if (std::abs(m_inputRange)>.5){
			EUDAQ_WARN(string("Could not set valid input range,choose input range center to be 0V"));
			m_inputRange = 0;
		}
		EUDAQ_INFO(string("Set Input Range to: "+std::to_string(m_inputRange-0.5) + "V - "+std::to_string(m_inputRange-0.5) + "V"));
		m_b->SetInputRange(m_inputRange);

		/* use following line to turn on the internal 100 MHz clock connected to all channels  */
		if (m_config.Get("enable_calibration_clock",false)){
			cout<<" turn on the internal 100 MHz clock connected to all channels "<<endl;
			m_b->EnableTcal(1);
		}


		/* use following lines to enable hardware trigger on CH1 at 50 mV positive edge */
		if (m_b->GetBoardType() >= 8) {        // Evaluation Board V4&5
			m_b->EnableTrigger(1, 0);           // enable hardware trigger
			//m_b->SetTriggerSource(1<<0);        // set CH1 as source
		} else if (m_b->GetBoardType() == 7) { // Evaluation Board V3
			m_b->EnableTrigger(0, 1);           // lemo off, analog trigger on
			//m_b->SetTriggerSource(0);           // use CH1 as source
		}

		float trigger_level = m_config.Get("trigger_level",-0.05f);
		unsigned int trigger_delay = m_config.Get("trigger_delay",500);
		bool trigger_polarity = m_config.Get("trigger_polarity",false);

		m_b->SetTriggerLevel(trigger_level);            // 0.05 V
		EUDAQ_INFO(string("Set Trigger Level to: "+std::to_string(trigger_level) + " mV"));

		m_b->SetTriggerDelayNs(trigger_delay);
		EUDAQ_INFO(string("Set Trigger Delay to: "+std::to_string(trigger_delay)+" ns"));

		m_b->SetTriggerPolarity(trigger_polarity);        // positive edge
		if (trigger_polarity)
			EUDAQ_INFO(string("Set Trigger Polarity to negative edge"));
		else
			EUDAQ_INFO(string("Set Trigger Polarity to positive edge"));

		int trigger_source = m_config.Get("trigger_source",1);
		std::cout<<string("Set Trigger Source to: "+std::to_string(trigger_source))<<endl;
		m_b->SetTriggerSource(trigger_source);
		std::cout<<string("Read Trigger Source to: "+std::to_string(trigger_source))<<endl;
		EUDAQ_INFO(string("Set Trigger Source to: "+std::to_string(trigger_source)));

		activated_channels = m_config.Get("activated_channels",255);
		for (int i = 0; i< 8; i++){
			m_chnOn[i] = ((activated_channels)&(1<<i))==1<<i;
			cout<<"CH"<<i+1<<": ";
			if (m_chnOn[i])
				cout<<"ON"<<endl;
			else
				cout<<"OFF"<<endl;
		}

		SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" +m_config.Name() + ")");

	}catch ( ... ){
		EUDAQ_ERROR(string("Unknown exception."));
		SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
	}
	cout<<"Configured! Ready to take data"<<endl;
}


int main(int /*argc*/, const char ** argv) {
	// You can use the OptionParser to get command-line arguments
	// then they will automatically be described in the help (-h) option
	eudaq::OptionParser op("DRS4 Producer", "0.0",
			"Run options");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
			"tcp://localhost:44000", "address", "The address of the RunControl.");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
			"The minimum level for displaying log messages locally");
	eudaq::Option<std::string> name (op, "n", "name", "DRS4", "string",
			"The name of this Producer");
	eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
	try {
		// This will look through the command-line arguments and set the options
		op.Parse(argv);
		// Set the Log level for displaying messages based on command-line
		EUDAQ_LOG_LEVEL(level.Value());
		// Create a producer
		DRS4Producer producer(name.Value(), rctrl.Value(), verbosity.Value());
		// And set it running...
		producer.ReadoutLoop();
		// When the readout loop terminates, it is time to go
		std::cout << "Quitting" << std::endl;
	} catch (...) {
		// This does some basic error handling of common exceptions
		return op.HandleMainException();
	}

	return 0;
}


void DRS4Producer::SendRawEvent()
{

	SetTimeStamp(); // Set Timestamp
	/* read all waveforms */
	m_b->TransferWaves(0, 8);//todo: ist das anpassbar?

	for (int ch = 0; ch < n_channels; ch++){
		/* read time (X) array of  channel in ns
						   Note: On the evaluation board input #1 is connected to channel 0 and 1 of
						   the DRS chip, input #2 is connected to channel 2 and 3 and so on. So to
						   get the input #2 we have to read DRS channel #2, not #1. */
		//					m_b->GetTime(0, ch*2, m_b->GetTriggerCell(0), time_array[ch*2]);
		/* decode waveform (Y) array of first channel in mV */
		m_b->GetWave(0, ch*2, wave_array[ch]);
//		m_b->GetRawWave(0,ch*2,raw_wave_array[ch]);

	}
	int trigger_cell = m_b->GetTriggerCell(0);


	/* Restart Readout */
	m_b->StartDomino();
	eudaq::RawDataEvent ev(m_event_type, m_run, m_ev);
	unsigned int block_no = 0;
	ev.AddBlock(block_no, reinterpret_cast<const char*>(&trigger_cell), sizeof( trigger_cell));				//				ev.AddBlock(block_no,"EHDR");
	block_no++;
	ev.AddBlock(block_no, reinterpret_cast<const char*>(&m_timestamp), sizeof( m_timestamp));				//				ev.AddBlock(block_no,"EHDR");
	block_no++;
	for (int ch = 0; ch < n_channels; ch++)
	    {
		 if (!m_chnOn[ch]) continue;
		 char buffer [6];
		 sprintf(buffer, "C%03d\n", ch+1);
		 ev.AddBlock(block_no++, reinterpret_cast<const char*>(buffer),sizeof(buffer));
		 /* Set data block of ch, each channel ist connected to to DRS channels*/
		 int n_samples = 1024;
		 int n_drs_channels = 1;
		 unsigned short raw_wave_array[1024];
		 //Merge both drs channels to the output of the channel
		 for (int drs_ch = 0; drs_ch < n_drs_channels; drs_ch++) {
			 for (int i = 0; i<1024 / n_drs_channels; i++)
				 raw_wave_array[i + 1024 * drs_ch] = (unsigned short)((wave_array[ch + drs_ch][i] / 1000.0 - m_inputRange + 0.5) * 65535);
		 }
		 ev.AddBlock(block_no++, reinterpret_cast<const char*>(&raw_wave_array), sizeof(raw_wave_array[0])* 1024);
	}
    if ( m_ev < 50 || m_ev % 100 == 0) 
	   {
	    cout<< "\rSend Event" << std::setw(7) << m_ev << " " << std::setw(1) <<  m_self_triggering << "Trigger cell: " << std::setw(4) << trigger_cell << ", " << std::flush;
       }
	SendEvent(ev);
	m_ev++;
}

void DRS4Producer::SetTimeStamp() {
	std::chrono::high_resolution_clock::time_point epoch;
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = now - epoch;
	// you need to be explicit about your timestamp type
	m_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()/ 100u;
	//std::cout<<"Set Timestamp: "<<m_timestamp<<endl;
}
