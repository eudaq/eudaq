/////////////////////////////////////////////////////////////////////
//                         2023 May 08                             //
//                   authors: R. Persiani & F. Tortorici           //
//                email: rinopersiani@gmail.com                    //
//                email: francesco.tortorici@enea.it               //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////


#include "eudaq/Producer.hh"
#include "FERS_Registers_520X.h"
#include "FERS_Registers_5215.h"
#include "FERS_paramparser.h"
#include "FERS_config.h"
#include "FERSlib.h"
//#include "FERSutils.h"

#undef max
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
//#include <random>
#include "stdlib.h"
#ifndef _WIN32
#include <sys/file.h>
#endif
#include <set>
#include "FERS_EUDAQ.h"
#include <sys/time.h>

//#include "configure.h"
//#include "JanusC.h"
//#include "paramparser.h"



//RunVars_t RunVars;
//int SockConsole;	// 0: use stdio console, 1: use socket console
//char ErrorMsg[250];
//int NumBrd=2; // number of boards


//Janus_Config_t J_cfg;                                   // struct with all parameters

Config_t WDcfg;

struct shmseg *shmp;
int shmid;





//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class FERSProducer : public eudaq::Producer {
	public:
		FERSProducer(const std::string & name, const std::string & runcontrol);
		void DoInitialise() override;
		void DoConfigure() override;
		void DoStartRun() override;
		void DoStopRun() override;
		void DoTerminate() override;
		void DoReset() override;
		void RunLoop() override;
		void checkEntries(const std::map<int, std::deque<SpectEvent_t>>& m_conn_evque);
		size_t splitStringToIntArray(const std::string& input, char delimiter, int* result, size_t maxSize);
		int read_pedestal(const char *filename, int pid, uint16_t lgped[64], uint16_t hgped[64]);
		int check_TRIG_alignment();
		static const uint32_t m_id_factory = eudaq::cstr2hash("FERSProducer");

	private:
		bool m_flag_ts;
		bool m_flag_tg;
		uint32_t m_plane_id;
		FILE* m_file_lock;
		std::chrono::milliseconds m_ms_busy;
		std::chrono::microseconds m_us_evt_length; // fake event length used in sync
		bool m_exit_of_run;
		int no_trigg = -1;
		int sw_trigger = 0;
		int spill_detect = 0;
		int read_boards = 0;
		int disable_ped = 0;

		std::string c_ip;
	        int cnc=0;
        	int cnc_handle[FERSLIB_MAX_NCNC];               // Concentrator handles

		std::string fers_ip_address;  // TDLink IP address of the board
		std::string fers_eth_address;  // Eth IP address of the board
		std::string fers_id;
		int fers_group = 0;
		int handle =-1;		 	// Board handle
		//float fers_hv_vbias;
		//float fers_hv_imax;
		int fers_acq_mode;
                uint32_t StatusReg[FERSLIB_MAX_NBRD];   // Acquisition Status Register
		//int vhandle[FERSLIB_MAX_NBRD];
        	//int retriesTDL = 0;

		// staircase params
		//uint8_t stair_do;
		//uint16_t stair_start, stair_stop, stair_step, stair_shapingt;
		//uint32_t stair_dwell_time;


  		std::map<int, std::deque<SpectEvent_t>> m_conn_evque;
  		std::map<int, SpectEvent_t> m_conn_ev;

		//struct shmseg *shmp;
		//int shmid;
		int brd; // current board

};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
namespace{
	auto dummy0 = eudaq::Factory<eudaq::Producer>::
		Register<FERSProducer, const std::string&, const std::string&>(FERSProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------

FERSProducer::FERSProducer(const std::string & name, const std::string & runcontrol)
	:eudaq::Producer(name, runcontrol), m_file_lock(0), m_exit_of_run(false)
{  
}

//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void FERSProducer::DoInitialise(){
	// see https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm
	shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644|IPC_CREAT);
	//shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0);
	if (shmid == -1) {
		perror("Shared memory");
	}
	EUDAQ_WARN("producer constructor: shmid = "+std::to_string(shmid));

	// Attach to the segment to get a pointer to it.
	shmp = (shmseg*)shmat(shmid, NULL, 0);
	if (shmp == (void *) -1) {
		perror("Shared memory attach");
	}

	initshm( shmid );


	auto ini = GetInitConfiguration();
	std::string fers_prodid = ini->Get("FERS_PRODID","my_fers0");
	std::string number_str;
	for (char c : fers_prodid) {
        	if (std::isdigit(c)) {
            	number_str += c;
        	}
    	}
	fers_group = number_str.empty() ? 0 : std::stoi(number_str);
	shmp->connectedboards[fers_group]=0;

        EUDAQ_WARN("FERS "+fers_prodid+", GROUP = "+std::to_string(fers_group));
	std::string lock_path = ini->Get("FERS_DEV_LOCK_PATH", "ferslockfile.txt");
	m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
	if(flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
		EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
	}
#endif

	fers_ip_address = ini->Get("FERS_IP_ADDRESS", "");
	fers_eth_address = ini->Get("FERS_ETH_ADDRESS", "");
	fers_id = ini->Get("FERS_ID","");
	//memset(&handle, -1, sizeof(handle));
	//for (int i=0; i<FERSLIB_MAX_NBRD; i++)
	//	vhandle[i] = -1;


        if (!fers_eth_address.empty()) {
	   char eth_address[30];
           int eth_ip[16]={0};
           size_t count = splitStringToIntArray(fers_eth_address, ',', eth_ip, 16);
           int ROmode = ini->Get("FERS_RO_MODE",0);
           //std::string fers_prodid = ini->Get("FERS_PRODID","no prod ID");
           int allocsize;
           char tmp_path[100];
	   for (size_t iIP = 0; iIP < count; ++iIP) {
		if (eth_ip[iIP] < 1) continue;
                              //std::sprintf(tmp_path,"%s%d", connection_path,brd);
                              std::sprintf(tmp_path,"eth:192.168.50.%d",eth_ip[iIP]);// Eth IP
                              int ret = FERS_OpenDevice(tmp_path, &handle); // open conection to a board
                              if(ret==0){
                                    EUDAQ_INFO("Bords at "+std::string(tmp_path)
                                       +" connected to handle "+std::to_string(handle)
                                       );
                                    FERS_InitReadout(handle,ROmode,&allocsize);

                                    // fill shared struct
                                    //vhandle[shmp->connectedboards[fers_group]]=handle;
                                    shmp->handle[fers_group][shmp->connectedboards[fers_group]] = handle;
				    shmp->FERS_TDLink[fers_group][shmp->connectedboards[fers_group]]=0;
 				    shmp->FERS_LastSrvEvent_us[fers_group][shmp->connectedboards[fers_group]]=std::chrono::high_resolution_clock::now();


                                    EUDAQ_INFO("check shared on board "+std::to_string(shmp->connectedboards[fers_group])+": "
                                               +std::string(tmp_path)
                                               +"*"+std::to_string(shmp->handle[fers_group][shmp->connectedboards[fers_group]])
                                                );
                                                m_conn_evque[shmp->connectedboards[fers_group]].clear();
                                                shmp->connectedboards[fers_group]++;
                              }else{
                                    EUDAQ_THROW("Bords at "+std::string(tmp_path)
                                                +" error "+std::to_string(ret)
                                                );
                              }

           }
        }



        if (fers_ip_address.find("tdl") != std::string::npos) {
	  char ip_address[30];
	  char connection_path[50];

	  strcpy(ip_address, fers_ip_address.c_str());
	  sprintf(connection_path,"eth:%s",ip_address);

          char cpath[100];
	  FERS_Get_CncPath(connection_path, cpath);
	  std::string str_cpath(cpath);
          c_ip = str_cpath.substr(0,str_cpath.length() - 4); // removing ":cnc"
	  EUDAQ_INFO("cpath "+std::string(cpath) + " c_ip "+c_ip );

	  int ret ;

	  if (!FERS_IsOpen(cpath)) {
                FERS_CncInfo_t CncInfo;
                ret = FERS_OpenDevice(cpath, &cnc_handle[cnc]);  // open connection to the concetrator
                ret |= FERS_ReadConcentratorInfo(cnc_handle[cnc], &CncInfo);
                for (int i = 0; i < 8; i++) { // Loop over all the TDlink chains
                	if (CncInfo.ChainInfo[i].BoardCount > 0) {
   				EUDAQ_INFO("TDlink "+std::to_string(i)
					+" Connected Boards Count "+std::to_string(CncInfo.ChainInfo[i].BoardCount)
		 		);
				//connection_path[strlen(connection_path) - 1] = '\0';
				int ROmode = ini->Get("FERS_RO_MODE",0);
				int allocsize;
				char tmp_path[100];
                		for (brd = 0; brd < CncInfo.ChainInfo[i].BoardCount; brd++) { // Loop over all the boards
					//std::sprintf(tmp_path,"%s%d", connection_path,brd);
					std::sprintf(tmp_path,"%s:tdl:%d:%d", c_ip.c_str(),i,brd);// CNC IP : chain#: Brd#
				        ret = FERS_OpenDevice(tmp_path, &handle); // open conection to a board
					if(ret==0){
		   				EUDAQ_INFO("Bords at "+std::string(tmp_path)
						+" connected to handle "+std::to_string(handle)
				 		);
						FERS_InitReadout(handle,ROmode,&allocsize);

						// fill shared struct
						//vhandle[shmp->connectedboards[fers_group]]=handle;
						shmp->handle[fers_group][shmp->connectedboards[fers_group]] = handle;
						shmp->FERS_TDLink[fers_group][shmp->connectedboards[fers_group]]=1;
						shmp->FERS_LastSrvEvent_us[fers_group][shmp->connectedboards[fers_group]]=std::chrono::high_resolution_clock::now();

						EUDAQ_INFO("check shared on board "+std::to_string(shmp->connectedboards[fers_group])+": "
							+std::string(tmp_path)
						);
						m_conn_evque[shmp->connectedboards[fers_group]].clear();
						shmp->connectedboards[fers_group]++;
					}else{
		   				EUDAQ_THROW("Bords at "+std::string(tmp_path)
						+" error "+std::to_string(ret)
				 		);
					}

				}
			}
		}
	  }
	}


	EUDAQ_WARN("FERS: # connected boards is "+std::to_string(shmp->connectedboards[fers_group]));

}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void FERSProducer::DoConfigure(){
	int bcnt=0;
	if (fers_group==0) {  // create flat index
		for (int igr=0;igr<MAX_NGR;igr++){
			for(int ibrd=0;ibrd<shmp->connectedboards[igr];ibrd++){
				shmp->flat_idx[igr][ibrd]=bcnt;
				bcnt++;
			}
		}
	}
	auto conf = GetConfiguration();
	conf->Print(std::cout);
	read_boards = conf->Get("FERS_DIRECT_READ", 0);
	no_trigg = conf->Get("FERS_NO_TRIGG", 1);
	spill_detect = conf->Get("FERS_SPILL_DETECT", 0);
	sw_trigger = conf->Get("FERS_SW_TRIGGER", 0);
	m_plane_id = conf->Get("EX0_PLANE_ID", 100);
	m_ms_busy = std::chrono::milliseconds(conf->Get("EX0_DURATION_BUSY_MS", 1000));
	m_us_evt_length = std::chrono::microseconds(60); // used in readou.

	m_flag_ts = conf->Get("EX0_ENABLE_TIMESTAMP", 0);
	m_flag_tg = conf->Get("EX0_ENABLE_TRIGERNUMBER", 0);
	if(!m_flag_ts && !m_flag_tg){
		EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
		m_flag_ts = false;
		m_flag_tg = true;
	}
	std::string fers_conf_filename= conf->Get("FERS_CONF_FILE","NOFILE");
	std::string fers_ped_filename= conf->Get("FERS_PED_FILE","");
	disable_ped = conf->Get("FERS_DisablePedestalCalibration", 0);

	fers_acq_mode = conf->Get("FERS_ACQ_MODE",0);

	int ret = -1; // to store return code from calls to fers
	ret = FERS_LoadConfigFile(const_cast<char*>(fers_conf_filename.c_str()));

        if (ret != 0)
           EUDAQ_THROW("Cannot load FERS configuration from file " + fers_conf_filename);


	//fers_hv_vbias = conf->Get("FERS_HV_Vbias", 28.);
	//fers_hv_imax = conf->Get("FERS_HV_IMax", 1.);
	//std:: cout<< " FERS_HV_Vbias " << fers_hv_vbias <<std::endl;
	float fers_dummyvar = 0;
	int ret_dummy = 0;

        //FERS_SetEnergyBitsRange(0);

        for(int kbrd = 0; kbrd < shmp->connectedboards[fers_group]; kbrd++) { // loop over boards

                //std::cout << "FERS_configure vhandle = "<<shmp->handle[fers_group][kbrd]<<std::endl;
		ret |= FERS_SendCommand(shmp->handle[fers_group][kbrd], CMD_RESET);  // Reset
		//std::this_thread::sleep_for(std::chrono::milliseconds(15));


		if (!fers_ped_filename.empty() && !disable_ped) {
			uint16_t LGped[64] = {0};
			uint16_t HGped[64] = {0};

	                char date[20];
	                uint16_t DCoffset[4];
			uint16_t LGped_check[64] = {0};
			uint16_t HGped_check[64] = {0};
			ret = read_pedestal(fers_ped_filename.c_str(), FERS_pid(shmp->handle[fers_group][kbrd]), LGped, HGped);

			FERS_WritePedestals(shmp->handle[fers_group][kbrd], LGped, HGped, NULL);
			//FERS_EnablePedestalCalibration(shmp->handle[fers_group][brd], 1);


                        FERS_ReadPedestalsFromFlash(shmp->handle[fers_group][kbrd], date, LGped_check, HGped_check, DCoffset);
	                std::cout <<"Ped FERS PID = "<<FERS_pid(shmp->handle[fers_group][kbrd])<<std::endl;
			for (int kk = 0;kk<64;kk++)
				std::cout <<kk<< " LG = "<<LGped[kk]<<", HG = "<<HGped[kk]
				<< " LGread = "<<LGped_check[kk]<<", HGread = "<<HGped_check[kk]
				<<std::endl;
		}


	        ret |= FERS_configure(shmp->handle[fers_group][kbrd], CFG_HARD);
		//std::cout<<"3333 - 3333 FERScfg[6]->HV_IndivAdj[52] = "<<FERScfg[6]->HV_IndivAdj[52]<<std::endl;

		if (ret == 0) {
			EUDAQ_INFO("FERS_configure done");
		} else {
			EUDAQ_THROW("FERS_configure failed !!!");
		}


		if(disable_ped)
			FERS_EnablePedestalCalibration(shmp->handle[fers_group][kbrd], 0);
/*
		// Pedestal calibration: check the factory ped data stored in flash

                char date[20];
                uint16_t pedLG[FERSLIB_MAX_NCH], pedHG[FERSLIB_MAX_NCH];
                uint16_t DCoffset[4];
                FERS_ReadPedestalsFromFlash(shmp->handle[fers_group][brd], date, pedLG, pedHG, DCoffset);
                std::cout <<"FERS PID = "<<FERS_pid(shmp->handle[fers_group][brd])<<std::endl;
                std::cout <<"CH   PedLG   PedHG        CH   PedLG   PedHG"<<std::endl;
                for(int ch=0; ch<32; ch++) {
                     std::cout <<ch<<", "<<  pedLG[ch]<<", "<<  pedHG[ch]<<", "<<  32+ch<<", "<<  pedLG[32+ch]<<", "<<  pedHG[32+ch]<<std::endl;
                }
                std::cout <<"DCoffset: "<<DCoffset[0]<<", "<< DCoffset[1]<<", "<<DCoffset[2]<<", "<<DCoffset[3]<<std::endl;
*/


		ret_dummy = FERS_HV_Get_Imax( shmp->handle[fers_group][kbrd], &fers_dummyvar); // read back from fers
		if (ret == 0) {
			EUDAQ_INFO("I max set!");
			std::cout << "**** readback Imax value: "<< fers_dummyvar << std::endl;
		} else {
			EUDAQ_THROW("I max NOT set");
		}
		ret_dummy = FERS_HV_Get_Vbias( shmp->handle[fers_group][kbrd], &fers_dummyvar); // read back from fers
		if (ret == 0) {
			EUDAQ_INFO("HV bias set!");
			shmp->HVbias[fers_group][kbrd] = fers_dummyvar;
			std::string temp=conf->Get("EUDAQ_DC","no data collector");
			EUDAQ_WARN("check shared in board "+std::to_string(kbrd)
			+": HVbias = "+std::to_string(shmp->HVbias[fers_group][kbrd])
			+" acqmode="+std::to_string(WDcfg.AcquisitionMode));
			//std::this_thread::sleep_for(std::chrono::seconds(1));
			sleep(1);
			FERS_HV_Set_OnOff(shmp->handle[fers_group][kbrd], 1); // set HV on
		} else {
			EUDAQ_THROW("HV bias NOT set");
		}
	} // end loop over boards
	//stair_do = (bool)(conf->Get("stair_do",0));
	//stair_shapingt = (uint16_t)(conf->Get("stair_shapingt",0));
	///stair_start = (uint16_t)(conf->Get("stair_start",0));
	//stair_stop  = (uint16_t)(conf->Get("stair_stop",0));
	//stair_step  = (uint16_t)(conf->Get("stair_step",0));
	//stair_dwell_time  = (uint32_t)(conf->Get("stair_dwell_time",0));






}

//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void FERSProducer::DoStartRun(){
	m_exit_of_run = false;

  std::chrono::time_point<std::chrono::high_resolution_clock> tp_start_aq = std::chrono::high_resolution_clock::now();
  shmp->FERS_Aqu_start_time_us=tp_start_aq;


	// here the hardware is told to startup
        for(brd =0; brd < shmp->connectedboards[fers_group]; brd++) { // loop over boards
		FERS_SendCommand( shmp->handle[fers_group][brd], CMD_ACQ_START );
		m_conn_evque[brd].clear();
     		//auto return_sq = std::chrono::steady_clock::now();
     		//auto time_difference = std::chrono::duration_cast<std::chrono::microseconds>(return_sq - tp_start_aq);

	}
	EUDAQ_INFO("FERS_ReadoutStatus (0=idle, 1=running) = "+std::to_string(FERS_ReadoutStatus));
	//std::this_thread::sleep_for(std::chrono::seconds(3));
	sleep(3);

	if (!sw_trigger) {
		for( int brd = 0 ; brd<shmp->connectedboards[fers_group];brd++) {
			FERS_WriteRegister(shmp->handle[fers_group][brd], a_t1_out_mask, 8);
			//FERS_WriteRegister(shmp->handle[fers_group][brd], a_t0_out_mask, 32); // Sends BUSY status
		}
		EUDAQ_INFO("FERS: Trigger Veto is off");
		shmp->FERS_last_event_time_us=std::chrono::high_resolution_clock::now();
		shmp->DRS_last_event_time_us=std::chrono::high_resolution_clock::now();
	}

/*
                        if (streq(value, "T1-IN"))                      FERScfg[brd]->T1_outMask = 0x001;
                        else if (streq(value, "BUNCHTRG"))              FERScfg[brd]->T1_outMask = 0x002;
                        else if (streq(value, "Q-OR"))                  FERScfg[brd]->T1_outMask = 0x004;
                        else if (streq(value, "RUN"))                   FERScfg[brd]->T1_outMask = 0x008;
                        else if (streq(value, "PTRG"))                  FERScfg[brd]->T1_outMask = 0x010;
                        else if (streq(value, "BUSY"))                  FERScfg[brd]->T1_outMask = 0x020;
                        else if (streq(value, "DPROBE"))                FERScfg[brd]->T1_outMask = 0x040;
                        else if (streq(value, "TLOGIC"))                FERScfg[brd]->T1_outMask = 0x080;
                        else if (streq(value, "SQ_WAVE"))               FERScfg[brd]->T1_outMask = 0x100;
                        else if (streq(value, "TDL_SYNC"))              FERScfg[brd]->T1_outMask = 0x200;
                        else if (streq(value, "RUN_SYNC"))              FERScfg[brd]->T1_outMask = 0x400;
                        else if (streq(value, "ZERO"))                  FERScfg[brd]->T1_outMask = 0x000;
*/
}

//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void FERSProducer::DoStopRun(){
	m_exit_of_run = true;
	for( int brd = 0 ; brd<shmp->connectedboards[fers_group];brd++) {
		FERS_WriteRegister(shmp->handle[fers_group][brd], a_t1_out_mask, 0);
	}
	EUDAQ_INFO("FERS: Trigger Veto is activated");
	std::this_thread::sleep_for(std::chrono::seconds(1));

        for(brd =0; brd < shmp->connectedboards[fers_group]; brd++) { // loop over boards
		FERS_SendCommand( shmp->handle[fers_group][brd], CMD_ACQ_STOP );
		m_conn_evque[brd].clear();
	}
	//std::this_thread::sleep_for(std::chrono::seconds(1));
}

//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void FERSProducer::DoReset(){
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
        for(brd =0; brd < shmp->connectedboards[fers_group]; brd++) { // loop over boards
		FERS_HV_Set_OnOff( shmp->handle[fers_group][brd], 0); // set HV off
	}
}

//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void FERSProducer::DoTerminate(){
	m_exit_of_run = true;
	if(m_file_lock){
		fclose(m_file_lock);
		m_file_lock = 0;
	}

	if ( shmp != NULL )
        for(brd =0; brd < shmp->connectedboards[fers_group]; brd++) { // loop over boards
		FERS_CloseReadout(shmp->handle[fers_group][brd]);
		FERS_HV_Set_OnOff( shmp->handle[fers_group][brd], 0); // set HV off
		FERS_CloseDevice(shmp->handle[fers_group][brd]);
	}

	// free shared memory
	if (shmdt(shmp) == -1) {
		perror("shmdt");
	}
}


//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void FERSProducer::RunLoop(){
	auto tp_start_run = std::chrono::steady_clock::now();


	//std::chrono::time_point<std::chrono::high_resolution_clock> runloop_time = std::chrono::high_resolution_clock::now();
	//auto run_time =  std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() - runloop_time);

	uint32_t trigger_n = 0;

        std::set<int> expected_boards; 
        for (int i = 0; i < shmp->connectedboards[fers_group]; ++i) {
            expected_boards.insert(i);
        }

	int newData =0; 
	auto time_diff = std::chrono::system_clock::now() - shmp->FERS_LastSrvEvent_us[fers_group][0];
	auto time_diff_sec = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
	if (time_diff_sec.count()>2){
		for (int ibrd = 0; ibrd < shmp->connectedboards[fers_group]; ibrd++) {
			int ret;
			float tempFPGA,tempDetector,hv_Vmon,hv_Imon;
                	ret |= FERS_HV_Get_Vmon(shmp->handle[fers_group][ibrd], &hv_Vmon);
	                ret |= FERS_HV_Get_Imon(shmp->handle[fers_group][ibrd], &hv_Imon);
   			ret |= FERS_HV_Get_DetectorTemp(shmp->handle[fers_group][ibrd], &tempDetector);
			ret |= FERS_Get_FPGA_Temp(shmp->handle[fers_group][ibrd], &tempFPGA);
			shmp->tempFPGA[fers_group][ibrd]=tempFPGA;
			shmp->tempDet[fers_group][ibrd]=tempDetector;
			shmp->hv_Vmon[fers_group][ibrd]=hv_Vmon;
			shmp->hv_Imon[fers_group][ibrd]=hv_Imon;

			shmp->FERS_LastSrvEvent_us[fers_group][shmp->connectedboards[fers_group]]=std::chrono::high_resolution_clock::now();
		}
	}

	while(!m_exit_of_run){


			auto tp_trigger = std::chrono::steady_clock::now();
			auto tp_end_of_busy = tp_trigger + m_ms_busy;

			int nchan = 64;
			int DataQualifier = -1;
			void *Event;

			double tstamp_us = -1;
			int nb = -1;
			int bindex = -1;
			int status = -1;

			int r_status=0;

			auto now = std::chrono::high_resolution_clock::now();
			auto elapsedFERS = std::chrono::duration_cast<std::chrono::milliseconds>( now - shmp->FERS_last_event_time_us);
			auto elapsedDRS = std::chrono::duration_cast<std::chrono::milliseconds>( now - shmp->DRS_last_event_time_us);


			if (spill_detect && elapsedFERS.count() > 2500 && elapsedFERS.count() < 3000 && elapsedDRS.count() > 2000) {
				//std::cout<<"---3333---  End od spill" <<std::endl;

				int ret = check_TRIG_alignment(); // All trigger IDs should match here.
				if( ret <0 ){
				        for(int cbrd =0; cbrd < shmp->connectedboards[fers_group]; cbrd++) { // loop over boards
				                FERS_SendCommand( shmp->handle[fers_group][cbrd], CMD_ACQ_STOP );
			        	}

					EUDAQ_THROW("FERS and DRS Trigger IDs are not aligned between spills.");
				}
				auto time_diff = std::chrono::system_clock::now() - shmp->FERS_LastSrvEvent_us[fers_group][0];
				auto time_diff_sec = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
				if (time_diff_sec.count()>2){
					for (int ibrd = 0; ibrd < shmp->connectedboards[fers_group]; ibrd++) {
						int ret;
						float tempFPGA,tempDetector,hv_Vmon,hv_Imon;
			                	ret |= FERS_HV_Get_Vmon(shmp->handle[fers_group][ibrd], &hv_Vmon);
				                ret |= FERS_HV_Get_Imon(shmp->handle[fers_group][ibrd], &hv_Imon);
        				        ret |= FERS_HV_Get_DetectorTemp(shmp->handle[fers_group][ibrd], &tempDetector);
			        	        ret |= FERS_Get_FPGA_Temp(shmp->handle[fers_group][ibrd], &tempFPGA);
						shmp->tempFPGA[fers_group][ibrd]=tempFPGA;
						shmp->tempDet[fers_group][ibrd]=tempDetector;
						shmp->hv_Vmon[fers_group][ibrd]=hv_Vmon;
						shmp->hv_Imon[fers_group][ibrd]=hv_Imon;

						shmp->FERS_LastSrvEvent_us[fers_group][shmp->connectedboards[fers_group]]=std::chrono::high_resolution_clock::now();
					}
				}
			}


			if (read_boards) {
			   for (int ibrd = 0; ibrd < shmp->connectedboards[fers_group]; ibrd++) {
				//run_time =  std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() - runloop_time);
				//std::cout<<"---3333--- time in ms before FERS_GetEvent " <<run_time.count()/1000.<<std::endl;

				int iibrd=ibrd;
				if (sw_trigger) {
					FERS_SendCommand(shmp->handle[fers_group][ibrd], CMD_TRG);   // SW trg
				}

				if (shmp->FERS_TDLink[fers_group][ibrd]) {
					status = FERS_GetEvent(shmp->handle[fers_group], &iibrd, &DataQualifier, &tstamp_us, &Event, &nb);
				}else{
					status = FERS_GetEventFromBoard(shmp->handle[fers_group][ibrd], &DataQualifier, &tstamp_us, &Event, &nb);
				}


				if(status<0){
			            EUDAQ_THROW("FERS: Readout failure,  ret = " + std::to_string(status)+" board = "+std::to_string(iibrd));
				}



				if(nb>0&&DataQualifier==17) { // Data event in Spec 
					newData++; // data - events*boards
					SpectEvent_t* EventSpect = (SpectEvent_t*)Event;
					m_conn_evque[iibrd].push_back(*EventSpect);
					if(EventSpect->trigger_id > shmp->FERS_last_trigID[fers_group][iibrd]){
	   		  			shmp->FERS_last_event_time_us=std::chrono::high_resolution_clock::now();
						shmp->FERS_last_trigID[fers_group][iibrd] = EventSpect->trigger_id;
					}
					std::cout<<"---3333--- board=" << iibrd
					//	<<" DataQualifier= "<<DataQualifier
						<<" trigger_id = "<<EventSpect->trigger_id
						<<" tstamp_us = "<< std::fixed << std::setprecision(3)<<EventSpect->tstamp_us
					//	<<" nb= "<<nb
						<<std::endl;


				}

				//run_time =  std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() - runloop_time);
				//std::cout<<"---3333--- time in ms after FERS_GetEvent " <<run_time.count()/1000.<<std::endl;

			   }
			}else{
 			   DataQualifier=1000;
		           while(newData < shmp->connectedboards[fers_group]&&DataQualifier>0) { // read all data from the boards
				status = FERS_GetEvent(shmp->handle[fers_group], &bindex, &DataQualifier, &tstamp_us, &Event, &nb);
				if(status<0){
			            EUDAQ_THROW("FERS: Readout failure,  ret = " + std::to_string(status));
				}

				if(nb>0&&DataQualifier==17) { // Data event in Spec 
					newData++; // data - events*boards
					SpectEvent_t* EventSpect = (SpectEvent_t*)Event;
					m_conn_evque[bindex].push_back(*EventSpect);

					std::cout<<"---3333---  status="<<status<<" board=" << bindex
						<<" DataQualifier= "<<DataQualifier
						<<" trigger_id = "<<EventSpect->trigger_id
					//	<<" energyLG[1] = "<<EventSpect->energyLG[1]
					//	<<" newData = "<<newData
						<<" nb= "<<nb<<std::endl;
				}
			   } // end of - read all data from the boards
			} // choose read method
		if(no_trigg>0) checkEntries(m_conn_evque); // detect no data sent by FERS board(s)


		if( newData >= shmp->connectedboards[fers_group]) {  // if evt*boards >= boards, i.e. at least one complete event candidate

			newData=0;
    			int Nevt = 1000;

			// check if there is enough FERS data to asamble an event
			// Find the min number of events that could be assambled - Nevt
    			for( int brd = 0 ; brd<shmp->connectedboards[fers_group];brd++) {  
                		int qsize = m_conn_evque[brd].size();
               			if( qsize < Nevt) Nevt = qsize;
    			}


			// Ready to transmit the queued events with calculate Evt# offset
			for(int ievt = 0; ievt<Nevt; ievt++) {

				auto ev = eudaq::Event::MakeUnique("FERSProducer");
				ev->SetTag("Plane ID", std::to_string(m_plane_id));

	            		trigger_n=-1;
        	    		for(auto &conn_evque: m_conn_evque){ // find the min trigger cnt in the queue
                			int trigger_n_ev = conn_evque.second.front().trigger_id ;  // Ev counter
       	        			if(trigger_n_ev< trigger_n||trigger_n<0)
               	  				trigger_n = trigger_n_ev;
				}

				if(m_flag_tg)
					ev->SetTriggerN(trigger_n);



				m_conn_ev.clear(); // Just in case ...

				int bCntr = 0;
				std::set<int> processed_boards;

				// assemble one event with same trig count for all boards
		            	for(auto &conn_evque: m_conn_evque){
        	        		auto &ev_front = conn_evque.second.front();
                			int ibrd = conn_evque.first;

                			if(ev_front.trigger_id == trigger_n){
                        			m_conn_ev[ibrd]=ev_front;
                        			conn_evque.second.pop_front();
						bCntr++;
						processed_boards.insert(ibrd);
	                		}
        	    		}

	            		if(bCntr!=shmp->connectedboards[fers_group]) {  // check for missing data from some of the FERS boards
					std::ostringstream missing_boards_stream;

					for (int expected : expected_boards) {
					    if (processed_boards.find(expected) == processed_boards.end()) {
					        if (!missing_boards_stream.str().empty()) {
					            missing_boards_stream << ", ";
					        }
					        missing_boards_stream << expected;
					    }
					}

					std::string missing_boards = missing_boards_stream.str();

        	        		//EUDAQ_WARN("FERS: Event alignment failed with "+std::to_string(m_conn_ev.size())
                	        	//	+" board's records instead of "+std::to_string(shmp->connectedboards) );

					EUDAQ_WARN("FERS: Event alignment failed with " + std::to_string(m_conn_ev.size())
					        + " board's records instead of " + std::to_string(shmp->connectedboards[fers_group]) +
					        ". Missing boards: " + missing_boards);

					ev->Print(std::cout);
					//SendEvent(std::move(ev));

					m_conn_ev.clear();
            			}else{
                			for( int brd = 0 ; brd<shmp->connectedboards[fers_group];brd++) {
						if( m_flag_ts && brd==0 ){
							auto du_ts_beg_us = std::chrono::duration_cast<std::chrono::microseconds>(shmp->FERS_Aqu_start_time_us - get_midnight_today());
							auto tp_trigger0 = std::chrono::microseconds(static_cast<long int>(m_conn_ev[brd].tstamp_us));
							du_ts_beg_us += tp_trigger0;
							std::chrono::microseconds du_ts_end_us(du_ts_beg_us + m_us_evt_length);
							ev->SetTimestamp(static_cast<uint64_t>(du_ts_beg_us.count()), static_cast<uint64_t>(du_ts_end_us.count()));
						}

                        			std::vector<uint8_t> data;
						make_headerFERS(shmp->flat_idx[fers_group][brd], FERS_pid(shmp->handle[fers_group][brd]),
							shmp->hv_Vmon[fers_group][brd],shmp->hv_Imon[fers_group][brd], shmp->tempDet[fers_group][brd],shmp->tempFPGA[fers_group][brd], &data);
        	                		// Add data here
						FERSpackevent( static_cast<void*>(&m_conn_ev[brd]), 1, &data);
						uint32_t block_id = m_plane_id + shmp->flat_idx[fers_group][brd];

						int n_blocks = ev->AddBlock(block_id, data);

	                		}

					//ev->Print(std::cout);
					SendEvent(std::move(ev));

					m_conn_ev.clear(); // clear single-event buffer

        	    		}//m_conn_ev.size check
			} // Nevt

		} // End NewData


		std::this_thread::sleep_until(tp_end_of_busy);

	}// while !m_exit_of_run 
}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
void FERSProducer::checkEntries(const std::map<int, std::deque<SpectEvent_t>>& m_conn_evque) {
    size_t max_size = 0;

    // Find the maximum size among all deques
    for (const auto& pair : m_conn_evque) {
        max_size = std::max(max_size, pair.second.size());
    }

    // Check for deques with sizes 30 less than max_size
    for (const auto& pair : m_conn_evque) {
        size_t size_diff = max_size - pair.second.size();

        if (size_diff >= no_trigg+500) {
            EUDAQ_THROW("FERS Board missed 500 events !");
        }
    }
}


size_t FERSProducer::splitStringToIntArray(const std::string& input, char delimiter, int* result, size_t maxSize) {
    std::stringstream ss(input);
    std::string item;
    size_t index = 0;
    size_t index1 = 0;

    while (std::getline(ss, item, delimiter)) {
        if (index < maxSize) {
            try {
                result[index++] = std::stoi(item); // Convert to int and store in the array
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid input: " << item << " is not an integer." << std::endl;
            }
        } else {
            std::cerr << "Warning: Too many elements, ignoring excess." << std::endl;
            break;
        }
    }

    // Fill remaining array slots with zeros
    index1 = index;
    while (index1 < maxSize) {
        result[index1++] = 0;
    }

    return index; // Return the count of successfully populated entries
}

int FERSProducer::read_pedestal(const char *filename, int pid, uint16_t lgped[64], uint16_t hgped[64]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    char line[4096]; // Large enough buffer for one line

    while (fgets(line, sizeof(line), file)) {
        // Parse PID
        char *pid_str = strtok(line, ":");
        if (!pid_str) continue;

        int current_pid = atoi(pid_str);
        if (current_pid != pid) continue;

        // Parse LGped and HGped parts
        char *lgped_str = strtok(NULL, ";");
        char *hgped_str = strtok(NULL, "\n");
        if (!lgped_str || !hgped_str) {
            fclose(file);
            fprintf(stderr, "Malformed line for PID %d\n", pid);
            return -1;
        }

        // Parse LGped values
        int count = 0;
        char *token = strtok(lgped_str, "|");
        while (token && count < 64) {
            lgped[count++] = (uint32_t)atoi(token);
            token = strtok(NULL, "|");
        }
        if (count != 64) {
            fclose(file);
            fprintf(stderr, "LGped must have exactly %d values, got %d\n", 64, count);
            return -1;
        }

        // Parse HGped values
        count = 0;
        token = strtok(hgped_str, "|");
        while (token && count < 64) {
            hgped[count++] = (uint32_t)atoi(token);
            token = strtok(NULL, "|");
        }
        if (count != 64) {
            fclose(file);
            fprintf(stderr, "HGped must have exactly %d values, got %d\n", 64, count);
            return -1;
        }

        fclose(file);
        return 0; // Success
    }

    fclose(file);
    fprintf(stderr, "PID %d not found in file\n", pid);
    return -1; // PID not found
}

int FERSProducer::check_TRIG_alignment() {
    int max_fers_val = 0;
    int max_drs_val = 0;

    // Find the maximum FERS trigger ID
    for (int i = 0; i < MAX_NGR; i++) {
        for (int j = 0; j < shmp->connectedboards[i]; j++) {
            int fers_val = static_cast<int>(shmp->FERS_last_trigID[i][j]);
            if (fers_val > max_fers_val) {
                max_fers_val = fers_val;
            }
        }
    }

    // Find the maximum DRS trigger ID
    for (int index = 0; index < shmp->connectedboardsDRS; index++) {
        int drs_val = static_cast<int>(shmp->DRS_last_trigID[index]);
        if (drs_val > max_drs_val) {
            max_drs_val = drs_val;
        }
    }

    // Compare the maximum trigger IDs
    if (max_fers_val > 0 && max_drs_val > 0) { // Ensure non-zero values
        if (max_fers_val != max_drs_val) {
            std::cerr << "Trigger ID mismatch"
                      << ": Max DRS=" << max_drs_val
                      << ", Max FERS=" << max_fers_val << "\n";
            auto nowT = std::chrono::system_clock::now();
            std::time_t time = std::chrono::system_clock::to_time_t(nowT);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowT.time_since_epoch()) % 1000;
            std::tm localTime = *std::localtime(&time);
            std::cout << std::put_time(&localTime, "%H:%M:%S") << '.'
                << std::setfill('0') << std::setw(3) << ms.count() << '\n';



            return -1;
        }
    } 

    return 0; // All good
}
