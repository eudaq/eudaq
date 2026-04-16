#include "eudaq/Producer.hh"
#include "DRS_EUDAQ.h"
#include "CAENDigitizer.h"
#include "WDconfig.h"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
//#include <random>
#ifndef _WIN32
#include <sys/file.h>
#endif






//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class DRSProducer : public eudaq::Producer {
  public:
  DRSProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  //void make_evtCnt_corr(std::map<int, std::deque<CAEN_DGTZ_X742_EVENT_t>>* m_conn_evque);
  CAEN_DGTZ_X742_EVENT_t* deep_copy_event(const CAEN_DGTZ_X742_EVENT_t *src) ;
  void free_deep_copied_event(CAEN_DGTZ_X742_EVENT_t* event);


  static const uint32_t m_id_factory = eudaq::cstr2hash("DRSProducer");
private:
  bool m_flag_ts;
  bool m_flag_tg;
  uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  std::chrono::microseconds m_us_evt_length; // fake event length used in sync
  bool m_exit_of_run;

  int handle =-1;                 // Single Board handle
  int vhandle[16];		  // All Boards handles
  int PID_DRS[16];

  WaveDumpConfig_t   WDcfg;
  int V4718_PID =58232; // yes!, hardcoded ...
  int ret, NBoardsDRS =0 ;
  uint32_t AllocatedSize, BufferSize, NumEvents;
  CAEN_DGTZ_BoardInfo_t   BoardInfo;
  CAEN_DGTZ_EventInfo_t   EventInfo;
  CAEN_DGTZ_X742_EVENT_t  *Event742=NULL;  /* custom event struct with 8 bit data (only for 8 bit digitizers) */
  char *EventPtr = NULL;
  char *buffer = NULL;
  double TTimeTag_calib = 58.59125; // 58.594 MHz from CAEN manual


  // WC DRS config
  int WC_DRS_ID = -1;
  int WC_DRS_FREQ = 0;
  uint32_t WC_DRS_RECORD_LENGTH = 1024;
  int WC_DRS_POST_TRIGGER = 1; 
  int DRS_BASELINE_CORR = 1;

  std::map<int, std::deque<CAEN_DGTZ_X742_EVENT_t*>> m_conn_evque;
  std::map<int, CAEN_DGTZ_X742_EVENT_t> m_conn_ev;


// Add to DRSProducer class declaration
  std::vector<CAEN_DGTZ_X742_EVENT_t*> m_allocated_events;

  struct shmseg *shmp;
  int shmid;

};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<DRSProducer, const std::string&, const std::string&>(DRSProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
DRSProducer::DRSProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_file_lock(0), m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void DRSProducer::DoInitialise(){

  shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644|IPC_CREAT);
  if (shmid == -1) {
           perror("Shared memory");
  }
  EUDAQ_WARN("producer constructor: shmid = "+std::to_string(shmid));

  // Attach to the segment to get a pointer to it.
  shmp = (shmseg*)shmat(shmid, NULL, 0);
  if (shmp == (void *) -1) {
           perror("Shared memory attach");
  }


  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("DRS_DEV_LOCK_PATH", "drslockfile.txt");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if(flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
    EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
  }
#endif

  char *s, *sep, ss[20][16];

  std::string DRS_BASE_ADDRESS = ini->Get("DRS_BASE_ADDRESS", "3210");
  // split DRS_BASE_ADDRESS into strings separated by ':'
  NBoardsDRS = 0;
  s = &DRS_BASE_ADDRESS[0];
  while(s < DRS_BASE_ADDRESS.c_str()+strlen(DRS_BASE_ADDRESS.c_str())) {
          sep = strchr(s, ':');
          if (sep == NULL) break;
          strncpy(ss[NBoardsDRS], s, sep-s);
          ss[NBoardsDRS][sep-s] = 0;
          s += sep-s+1;
          NBoardsDRS++;
          if (NBoardsDRS == 20) break;
        }
        strcpy(ss[NBoardsDRS++], s);
  EUDAQ_INFO("DRS: found "+std::to_string(NBoardsDRS)+ " addresses in fers.ini"
                );

  char BA[100];

  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
          std::sprintf(BA,"%s0000", ss[brd]);
	  ret = CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_USB_V4718, (void *)&V4718_PID, 0 , std::stoi(BA, 0, 16), &handle);
	  if (ret) {
		EUDAQ_THROW("Unable to open DRS at 0x"+std::string(ss[brd])+", ret = " + std::to_string(ret));
	  }
	  vhandle[brd]=handle;
    	  ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    	  if (ret) {
	  	EUDAQ_THROW("DRS: CAEN_DGTZ_GetInfo failed on board"+std::string(ss[brd]));
	  }else{
	  	EUDAQ_INFO("DRS: opened at 0x"+std::string(ss[brd])+ "0000 is model "+BoardInfo.ModelName
		+" SN "+std::to_string(BoardInfo.SerialNumber)
		+" | PCB Revision "+std::to_string(BoardInfo.PCB_Revision)
		+" | ROC FirmwareRel "+std::string(BoardInfo.ROC_FirmwareRel)
		+" | AMX FirmwareRel "+std::string(BoardInfo.AMC_FirmwareRel)
		);
		PID_DRS[brd]=BoardInfo.SerialNumber;
	  }

	  m_conn_evque[brd].clear();
          shmp->nevtDRS[brd]=0;

  }
  //shmp->isEvtCntCorrDRSReady = false;
  shmp->connectedboardsDRS = NBoardsDRS;
  //shmp->DRS_trigC = 0;
  //shmp->DRS_trigT_last = 0;
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void DRSProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("DRS_PLANE_ID", 0);
  m_ms_busy = std::chrono::milliseconds(conf->Get("DRS_DURATION_BUSY_MS", 100));
  m_us_evt_length = std::chrono::microseconds(400); // used in sync.

  m_flag_ts = conf->Get("DRS_ENABLE_TIMESTAMP", 0);
  m_flag_tg = conf->Get("DRS_ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }

  DRS_BASELINE_CORR = conf->Get("DRS_BASELINE_CORR", 1);
  WC_DRS_ID = conf->Get("WC_DRS_ID", -1);
  WC_DRS_FREQ = conf->Get("WC_DRS_FREQ", 0);
  WC_DRS_RECORD_LENGTH = conf->Get("WC_DRS_RECORD_LENGTH", 1024);
  WC_DRS_POST_TRIGGER = conf->Get("WC_DRS_POST_TRIGGER", 1);

  FILE *f_ini;
  std::string drs_conf_filename= conf->Get("DRS_CONF_FILE","NOFILE");
  f_ini = fopen(drs_conf_filename.c_str(), "r");

  ParseConfigFile(f_ini, &WDcfg);
  fclose(f_ini);

  /* *************************************************************************************** */
  /* program the digitizer                                                                   */
  /* *************************************************************************************** */
  EUDAQ_INFO("DRS: # boards in Conf "+std::to_string(NBoardsDRS));
  CAEN_DGTZ_TriggerPolarity_t Polarity;
  for( int brd = 0 ; brd<NBoardsDRS;brd++) {

     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = ProgramDigitizer(vhandle[brd], WDcfg, BoardInfo);
     //EUDAQ_INFO("DRS: # boards in Conf "+std::to_string(*Polarity)+ "  " +std::to_string(WDcfg.PulsePolarity[0]));
     EUDAQ_INFO("DRS: Trigger Polarity in conf. : "+std::to_string(WDcfg.PulsePolarity[0]));

     //if (brd==0)
     //      CAEN_DGTZ_SetDRS4SamplingFrequency(vhandle[brd], (CAEN_DGTZ_DRS4Frequency_t)2);// change to 1GHz

     // Trigger on rising or falling edge
     //ret = CAEN_DGTZ_SetChannelPulsePolarity(handle, channel, CAEN_DGTZ_PulsePolarityPositive);
     for(int i=0; i<4; i++) {
        if (WDcfg.PulsePolarity[i]&&WDcfg.GroupTrgEnableMask) ret =  CAEN_DGTZ_SetTriggerPolarity(vhandle[brd], i , CAEN_DGTZ_TriggerOnFallingEdge);
     }
     ret =  CAEN_DGTZ_GetTriggerPolarity(vhandle[brd], 0 , &Polarity);
     //if (ret != CAEN_DGTZ_Success) {
     //   printf("Error setting pulse polarity\n");
     //}
     

     if (ret) {
    	EUDAQ_THROW("DRS: ProgramDigitizer failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
        switch (Polarity) {
            case CAEN_DGTZ_TriggerOnRisingEdge:
                EUDAQ_INFO("Trigger Polarity status: Rising Edge");
                break;
            case CAEN_DGTZ_TriggerOnFallingEdge:
                EUDAQ_INFO("Trigger Polarity status: Falling Edge ");
                break;
            default:
                EUDAQ_INFO("Trigger Polarity status: Unknown "+std::to_string( Polarity));
        }
     }


  }

  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     uint32_t status = 0;
     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_ReadRegister(vhandle[brd], 0x8104, &status);

     if (ret) {
    	EUDAQ_THROW("DRS: ProgramDigitizer-read failed on board"+std::to_string(BoardInfo.SerialNumber));
     }

  }
  //read twice (first read clears the previous status)
  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     uint32_t status = 0;
     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_ReadRegister(vhandle[brd], 0x8104, &status);

     if (ret) {
    	EUDAQ_THROW("DRS: ProgramDigitizer-read failed on board"+std::to_string(BoardInfo.SerialNumber));
     }


     // Configure the WC DRS
     if ( brd == WC_DRS_ID){
       EUDAQ_INFO("DRS: ID for TDC = "+std::to_string(WC_DRS_ID));

       if ((ret = CAEN_DGTZ_SetDRS4SamplingFrequency(vhandle[brd], (CAEN_DGTZ_DRS4Frequency_t) WC_DRS_FREQ)) != CAEN_DGTZ_Success)
           EUDAQ_INFO("DRS: Cannot CAEN_DGTZ_SetDRS4SamplingFrequency on board "+std::to_string(brd));
       if ((ret = CAEN_DGTZ_SetRecordLength(vhandle[brd], WC_DRS_RECORD_LENGTH)) != CAEN_DGTZ_Success)
           EUDAQ_INFO("DRS: Cannot CAEN_DGTZ_SetRecordLength on board "+std::to_string(brd));
       if ((ret = CAEN_DGTZ_SetPostTriggerSize(vhandle[brd], WC_DRS_POST_TRIGGER)) != CAEN_DGTZ_Success)
           EUDAQ_INFO("DRS: Cannot CAEN_DGTZ_SetDRS4SamplingFrequency on board "+std::to_string(brd));
       if ((ret = CAEN_DGTZ_LoadDRS4CorrectionData(vhandle[brd], (CAEN_DGTZ_DRS4Frequency_t) WC_DRS_FREQ)) != CAEN_DGTZ_Success)
           EUDAQ_INFO("DRS: Cannot LoadDRS4CorrectionData on board "+std::to_string(brd));
       if ((ret = CAEN_DGTZ_EnableDRS4Correction(vhandle[brd])) != CAEN_DGTZ_Success)
          EUDAQ_INFO("DRS: Cannot EnableDRS4Correction on board "+std::to_string(brd));

     }else{
     // Load DRS factory calibration - baseline

       if (DRS_BASELINE_CORR==1) {
          if ((ret = CAEN_DGTZ_LoadDRS4CorrectionData(vhandle[brd], WDcfg.DRS4Frequency)) != CAEN_DGTZ_Success)
              EUDAQ_INFO("DRS: Cannot LoadDRS4CorrectionData on board "+std::to_string(brd));
          if ((ret = CAEN_DGTZ_EnableDRS4Correction(vhandle[brd])) != CAEN_DGTZ_Success)
             EUDAQ_INFO("DRS: Cannot EnableDRS4Correction on board "+std::to_string(brd));
       }
     }

  }
  // Allocate memory for the event data and readout buffer
  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_AllocateEvent(vhandle[brd], (void**)&Event742);
     if (ret) {
    	EUDAQ_THROW("DRS: Allocate memory for the event data failed on board"+std::to_string(BoardInfo.SerialNumber));
     }
     ret = CAEN_DGTZ_MallocReadoutBuffer(vhandle[brd], &buffer,&AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
     if (ret) {
    	EUDAQ_THROW("DRS: Allocate memory for the readout buffer failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
  	EUDAQ_INFO("DRS: ProgramDigitizer successful on board SN "+std::to_string(BoardInfo.SerialNumber));
  	EUDAQ_INFO("DRS: Allocate memory for the readout buffer "+std::to_string(AllocatedSize));
     }
  }


}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void DRSProducer::DoStartRun(){
  m_exit_of_run = false;
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_start_aq = std::chrono::high_resolution_clock::now();
  shmp->DRS_Aqu_start_time_us=tp_start_aq;

  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     m_conn_evque[brd].clear();
     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_SWStartAcquisition(vhandle[brd]);


     if (ret) {
    	EUDAQ_THROW("DRS: StartAcquisition failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
  	EUDAQ_INFO("DRS: StartAcquisition successful on board SN "+std::to_string(BoardInfo.SerialNumber));
     }
  }



}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void DRSProducer::DoStopRun(){
  m_exit_of_run = true;
  // here the hardware is told to stop data acquisition
  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_SWStopAcquisition(vhandle[brd]);
     if (ret) {
    	EUDAQ_THROW("DRS: StopAcquisition failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
  	EUDAQ_INFO("DRS: StopAcquisition successful on board SN "+std::to_string(BoardInfo.SerialNumber));
     }

     m_conn_evque[brd].clear();
  }
  //shmp->DRS_offset_us = 0;

}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void DRSProducer::DoReset(){
  m_exit_of_run = true;
  if(m_file_lock){
#ifndef _WIN32
    flock(fileno(m_file_lock), LOCK_UN);
#endif
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;

  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     //ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_Reset(vhandle[brd]);

     if (ret) {
    	EUDAQ_THROW("DRS: Reset failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
  	EUDAQ_INFO("DRS: Reset on board SN "+std::to_string(BoardInfo.SerialNumber));
     }
  }

}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void DRSProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_file_lock){
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  for( int brd = 0 ; brd<NBoardsDRS;brd++) {
     //ret = CAEN_DGTZ_GetInfo(vhandle[brd], &BoardInfo);
     ret = CAEN_DGTZ_Reset(vhandle[brd]);

     if (ret) {
    	EUDAQ_THROW("DRS: Reset failed on board"+std::to_string(BoardInfo.SerialNumber));
     }else{
  	EUDAQ_INFO("DRS: Reset on board SN "+std::to_string(BoardInfo.SerialNumber));
     }
  }
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void DRSProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;


  while(!m_exit_of_run){
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;


    for( int brd = 0 ; brd<NBoardsDRS;brd++) {

       ret = CAEN_DGTZ_ReadData(vhandle[brd], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);



       NumEvents = -1;
       if (ret) {
	EUDAQ_THROW("DRS: ReadData failed ret ="+ std::to_string(ret));
       }

       ret = CAEN_DGTZ_GetNumEvents(vhandle[brd], buffer, BufferSize, &NumEvents);
       if (ret) {
 	EUDAQ_THROW("DRS: CAEN_DGTZ_GetNumEvents failed");
       }

       shmp->nevtDRS[brd] = (int)NumEvents;


       for(int i = 0; i < (int)NumEvents; i++) {
          ret = CAEN_DGTZ_GetEventInfo(vhandle[brd], buffer, BufferSize, i, &EventInfo, &EventPtr);
          if (ret) {
	        EUDAQ_THROW("DRS: CAEN_DGTZ_GetEventInfo failed");
	  }
          ret = CAEN_DGTZ_DecodeEvent(vhandle[brd], EventPtr, (void**)&Event742);
          if (ret) {
	        EUDAQ_THROW("DRS: CAEN_DGTZ_DecodeEvent failed");
	  }

          Event742->DataGroup[1].TriggerTimeTag = EventInfo.EventCounter; // A workaround ...

	  auto copied_event = DRSProducer::deep_copy_event(Event742); // Fix for shallow copy in CAEN DIgitizer

	  if (copied_event) {
	    m_conn_evque[brd].push_back(copied_event);
	  }


       }

    }




    uint32_t block_id = m_plane_id;

    int Nevt = 128;

    for( int brd = 0 ; brd<NBoardsDRS;brd++) {
	int qsize = m_conn_evque[brd].size();
	if( qsize < Nevt) 
		Nevt = qsize;
    }



    // Ready to transmit the queued events with calculate Evt# offset
    for(int ievt = 0; ievt<Nevt; ievt++) {
    	    auto ev = eudaq::Event::MakeUnique("DRSProducer");
    	    ev->SetTag("Plane ID", std::to_string(m_plane_id));


            trigger_n=-1;
	    for(auto &conn_evque: m_conn_evque){
	        int trigger_n_ev = conn_evque.second.front()->DataGroup[1].TriggerTimeTag ;  // DRS Ev counter is stored instead

        	if(trigger_n_ev< trigger_n||trigger_n<0)
	          trigger_n = trigger_n_ev;
	    }
	    if(m_flag_tg)
      		ev->SetTriggerN(trigger_n);


            m_conn_ev.clear(); // Just in case ...

	    for(auto &conn_evque: m_conn_evque){
	    	CAEN_DGTZ_X742_EVENT_t* ev_front = conn_evque.second.front();
		int ibrd = conn_evque.first;

		if (ev_front->DataGroup[1].TriggerTimeTag > shmp->DRS_last_trigID[ibrd]){
	                shmp->DRS_last_event_time_us=std::chrono::high_resolution_clock::now();
			shmp->DRS_last_trigID[ibrd]= ev_front->DataGroup[1].TriggerTimeTag;
		}

                std::cout<<"---1111---Evt= "<<ievt<<", brd= "<<ibrd
		<<", trig= "<<ev_front->DataGroup[1].TriggerTimeTag <<std::endl;

		//auto nowT = std::chrono::system_clock::now();
		//std::time_t time = std::chrono::system_clock::to_time_t(nowT);
		//auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowT.time_since_epoch()) % 1000;
		//std::tm localTime = *std::localtime(&time);
		//std::cout << std::put_time(&localTime, "%H:%M:%S") << '.' 
              	//	<< std::setfill('0') << std::setw(3) << ms.count() << '\n';


 		if((ev_front->DataGroup[1].TriggerTimeTag) == trigger_n){
      			m_conn_ev[ibrd]= *ev_front;
			m_allocated_events.push_back(ev_front);
			conn_evque.second.pop_front();

	    	}
	    }


	    if(m_conn_ev.size()==NBoardsDRS) {
	    	for( int brd = 0 ; brd<NBoardsDRS;brd++) {

                     if( m_flag_ts && brd==0 ){
                          auto du_ts_beg_us = std::chrono::duration_cast<std::chrono::microseconds>(shmp->DRS_Aqu_start_time_us - get_midnight_today());
			  uint64_t CTriggerTimeTag= static_cast<uint64_t>(m_conn_ev[brd].DataGroup[0].TriggerTimeTag);
                          auto tp_trigger0 = std::chrono::microseconds(static_cast<long int>(CTriggerTimeTag/TTimeTag_calib/2.));
                          du_ts_beg_us += tp_trigger0;
                          std::chrono::microseconds du_ts_end_us(du_ts_beg_us + m_us_evt_length);
                          ev->SetTimestamp(static_cast<uint64_t>(du_ts_beg_us.count()), static_cast<uint64_t>(du_ts_end_us.count()));
                     }

		     std::vector<uint8_t> data;
		     make_header(brd, PID_DRS[brd], &data);

		     DRSpack_event(static_cast<void*>(&m_conn_ev[brd]),&data);

	    	     ev->AddBlock(m_plane_id+brd, data);
		} // loop over boards



	    	SendEvent(std::move(ev));
                m_conn_ev.clear();


		for (auto event_ptr : m_allocated_events) {
		  free_deep_copied_event(event_ptr);
		}
		m_allocated_events.clear();

	    } // if complete event with data from all boards


    }// loop over all collected events in the transfer


    std::this_thread::sleep_until(tp_end_of_busy);

  } // end   while(!m_exit_of_run){

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------


CAEN_DGTZ_X742_EVENT_t* DRSProducer::deep_copy_event(const CAEN_DGTZ_X742_EVENT_t *src) {
    // Allocate memory for the new event
    CAEN_DGTZ_X742_EVENT_t *copy = (CAEN_DGTZ_X742_EVENT_t*)malloc(sizeof(CAEN_DGTZ_X742_EVENT_t));
    if (copy == NULL) {
        return NULL; // Memory allocation failed
    }

    // Initialize all pointers in copy to NULL for safety
    memset(copy, 0, sizeof(CAEN_DGTZ_X742_EVENT_t)); // ** Added for robustness **

    // Copy GrPresent array
    memcpy(copy->GrPresent, src->GrPresent, sizeof(src->GrPresent));

    // Loop through each group and copy the data
    for (int i = 0; i < MAX_X742_GROUP_SIZE; i++) {
        if (src->GrPresent[i]) { // If the group has data
            // Copy ChSize array
            memcpy(copy->DataGroup[i].ChSize, src->DataGroup[i].ChSize, sizeof(src->DataGroup[i].ChSize));

            // Copy TriggerTimeTag and StartIndexCell
            copy->DataGroup[i].TriggerTimeTag = src->DataGroup[i].TriggerTimeTag;
            copy->DataGroup[i].StartIndexCell = src->DataGroup[i].StartIndexCell;

            // Copy DataChannel pointers
            for (int j = 0; j < MAX_X742_CHANNEL_SIZE; j++) {
                if (src->DataGroup[i].ChSize[j] > 0) {
                    // Allocate memory for the channel data
                    copy->DataGroup[i].DataChannel[j] = (float*)malloc(src->DataGroup[i].ChSize[j] * sizeof(float));
                    if (copy->DataGroup[i].DataChannel[j] == NULL) {
                        // If allocation fails, free previously allocated memory
                        for (int k = 0; k < j; k++) { // Free previously allocated channels
                            free(copy->DataGroup[i].DataChannel[k]);
                        }
                        // Free memory from earlier groups
                        for (int g = 0; g < i; g++) { 
                            for (int c = 0; c < MAX_X742_CHANNEL_SIZE; c++) {
                                if (copy->DataGroup[g].DataChannel[c]) {
                                    free(copy->DataGroup[g].DataChannel[c]);
                                }
                            }
                        }
                        free(copy); // Free the main structure
                        return NULL; // Memory allocation failed
                    }
                    // Copy the channel data
                    memcpy(copy->DataGroup[i].DataChannel[j], src->DataGroup[i].DataChannel[j], src->DataGroup[i].ChSize[j] * sizeof(float));
                } else {
                    copy->DataGroup[i].DataChannel[j] = NULL; // Ensure unallocated channels are NULL
                }
            }
        } else {
            // If no data, set all pointers to NULL
            memset(copy->DataGroup[i].DataChannel, 0, sizeof(copy->DataGroup[i].DataChannel));
        }
    }

    return copy;
}


void DRSProducer::free_deep_copied_event(CAEN_DGTZ_X742_EVENT_t* event) {
    if (!event) return;

    for (int i = 0; i < MAX_X742_GROUP_SIZE; ++i) {
        for (int j = 0; j < MAX_X742_CHANNEL_SIZE; ++j) {
            if (event->DataGroup[i].DataChannel[j]) {
                free(event->DataGroup[i].DataChannel[j]);
            }
        }
    }
    free(event);
}



/*

typedef struct
{
    uint32_t             EventSize;
    uint32_t             BoardId;
    uint32_t             Pattern;
    uint32_t             ChannelMask;
    uint32_t             EventCounter;
    uint32_t             TriggerTimeTag;
} CAEN_DGTZ_EventInfo_t;

typedef struct
{
    uint32_t                 ChSize[MAX_X742_CHANNEL_SIZE];           // the number of samples stored in DataChannel array
    float                    *DataChannel[MAX_X742_CHANNEL_SIZE];     // the array of ChSize samples
    uint32_t                 TriggerTimeTag;
    uint16_t                 StartIndexCell;
} CAEN_DGTZ_X742_GROUP_t;

typedef struct
{
    uint8_t                    GrPresent[MAX_X742_GROUP_SIZE]; // If the group has data the value is 1 otherwise is 0
    CAEN_DGTZ_X742_GROUP_t    DataGroup[MAX_X742_GROUP_SIZE]; // the array of ChSize samples
} CAEN_DGTZ_X742_EVENT_t;




*/
