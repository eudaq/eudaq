#include "eudaq/Producer.hh"
#include "QTP_EUDAQ.h"
#include "CAENDigitizer.h"
#include <CAENVMElib.h>
#include <CAENVMEtypes.h>
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


#define DATATYPE_FILLER         0x06000000
#define MAX_BLT_SIZE            (256*1024)
#define DATATYPE_MASK           0x06000000
#define DATATYPE_HEADER         0x02000000
#define DATATYPE_CHDATA         0x00000000
#define DATATYPE_EOB            0x04000000

#define LSB2PHY                 100   // LSB (= ADC count) to Physical Quantity (time in ps, charge in fC, amplitude in mV)

#define LOGMEAS_NPTS            1000

#define ENABLE_LOG                      0




//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class QTPDProducer : public eudaq::Producer {
  public:
  QTPDProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  //void make_evtCnt_corr(std::map<int, std::deque<CAEN_DGTZ_X742_EVENT_t>>* m_conn_evque);
  //CAEN_DGTZ_X742_EVENT_t* deep_copy_event(const CAEN_DGTZ_X742_EVENT_t *src) ;
  void write_reg(uint16_t reg_addr, uint16_t data);
  uint16_t read_reg(uint16_t reg_addr);
  static void findModelVersion(uint16_t model, uint16_t vers, char *modelVersion, int *ch) ;

  int  ConfigureDiscr(uint16_t OutputWidth, uint16_t Threshold[16], uint16_t EnableMask);

  static const uint32_t m_id_factory = eudaq::cstr2hash("QTPDProducer");
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
  int PID_QDC[16];

  WaveDumpConfig_t   WDcfg;
  int V4718_PID =58232;
  int ret, NBoardsQTP =0 ;
  uint32_t AllocatedSize, BufferSize, NumEvents;
  CAEN_DGTZ_BoardInfo_t   BoardInfo;
  CAEN_DGTZ_EventInfo_t   EventInfo;
  CAEN_DGTZ_X742_EVENT_t  *Event742=NULL;  /* custom event struct with 8 bit data (only for 8 bit digitizers) */
  char *EventPtr = NULL;
  char *buffer = NULL;
  double TTimeTag_calib = 58.59125; // 58.594 MHz from CAEN manual

  std::map<int, std::deque<CAEN_DGTZ_X742_EVENT_t>> m_conn_evque;
  std::map<int, CAEN_DGTZ_X742_EVENT_t> m_conn_ev;

  struct shmseg *shmp;
  int shmid;

  CVBoardTypes ctype = cvUSB_V4718;
  int link=0, bdnum=0;

// Base Addresses
  uint32_t BaseAddress;
  uint32_t QTPBaseAddrS[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t QTPBaseAddr = 0;
  uint32_t DiscrBaseAddr = 0;

// handle for the V1718/V2718
  //int32_t handle = -1;

  int VMEerror = 0;
  char ErrorString[100];
  FILE *logfile;

  uint16_t Iped = 255;                    // pedestal of the QDC (or resolution of the TDC)
  uint16_t DiscrChMask = 0;               // Channel enable mask of the discriminator
  uint16_t DiscrOutputWidth = 10; // Output wodth of the discriminator
  uint16_t DiscrThreshold[16] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5}; // Thresholds of the discriminator
  uint16_t QTP_LLD[32] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int EnableSuppression = 1;              // Enable Zero and Overflow suppression if QTP boards
  uint16_t fwrev, vers, model;
  uint32_t sernum;
  char modelVersion[3];
  int brd_nch[8] = {32, 32, 32, 32, 32, 32, 32, 32};
  int pnt, wcnt, bcnt, totnb=0, DataError=0, nch, chindex, nev=0, j, ns[32];
  uint32_t QTPbuffer[MAX_BLT_SIZE/4];// readout buffer (raw data from the board)
  int DataType = DATATYPE_HEADER;
  uint16_t ADCdata[32];
  
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<QTPDProducer, const std::string&, const std::string&>(QTPDProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
QTPDProducer::QTPDProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_file_lock(0), m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void QTPDProducer::DoInitialise(){

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
  std::string lock_path = ini->Get("QTP_DEV_LOCK_PATH", "qtplockfile.txt");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if(flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
    EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
  }
#endif

  char *s, *sep, ss[6][16];

  std::string QTP_BASE_ADDRESS = ini->Get("QTP_BASE_ADDRESS", "05000000");
  // split DRS_BASE_ADDRESS into strings separated by ':'
  std::cout<<"---7777---  QTP_BASE_ADDRESS = " << QTP_BASE_ADDRESS<<std::endl;
  NBoardsQTP = 0;
  s = &QTP_BASE_ADDRESS[0];
  while(s < QTP_BASE_ADDRESS.c_str()+strlen(QTP_BASE_ADDRESS.c_str())) {
          sep = strchr(s, ':');
          if (sep == NULL) break;
          strncpy(ss[NBoardsQTP], s, sep-s);
          ss[NBoardsQTP][sep-s] = 0;
          s += sep-s+1;
          NBoardsQTP++;
          if (NBoardsQTP == 16) break;
        }
        //strcpy(ss[NBoardsQTP++], s);
	sscanf(s, "%x", &QTPBaseAddrS[NBoardsQTP++]);
  EUDAQ_INFO("QTP: found "+std::to_string(NBoardsQTP)+ " addresses in fers.ini"
                );

  char BA[100];

  for( int brd = 0 ; brd<NBoardsQTP;brd++) {
          std::sprintf(BA,"0x%x", QTPBaseAddrS[brd]);
          if (CAENVME_Init2(ctype, &V4718_PID, bdnum, &handle) != cvSuccess) {
                EUDAQ_THROW("Unable to open QTP at "+std::string(BA));
          }else{
                EUDAQ_INFO("Opened a QTP connection at "+std::string(BA));
	  }

	  vhandle[brd]=handle;


	  m_conn_evque[brd].clear();
  }
  shmp->connectedboardsQTP = NBoardsQTP;
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void QTPDProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("QTP_PLANE_ID", 40);
  m_ms_busy = std::chrono::milliseconds(conf->Get("QTP_DURATION_BUSY_MS", 30));
  m_us_evt_length = std::chrono::microseconds(400); // used in sync.

  m_flag_ts = conf->Get("QTP_ENABLE_TIMESTAMP", 0);
  m_flag_tg = conf->Get("QTP_ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }

  FILE *f_ini;
  std::string qtp_conf_filename= conf->Get("QTP_CONF_FILE","NOFILE");
  f_ini = fopen(qtp_conf_filename.c_str(), "r");

  //Read QTP configuration file
       while(!feof(f_ini)) {
                char str[500];
                int data;

                str[0] = '#';
                fscanf(f_ini, "%s", str);
                if (str[0] == '#')
                        fgets(str, 1000, f_ini);
                else {


                        // Base Addresses
                        if (strstr(str, "QTP_BASE_ADDRESS")!=NULL)
                                fscanf(f_ini, "%x", &QTPBaseAddr);
                        if (strstr(str, "DISCR_BASE_ADDRESS")!=NULL)
                                fscanf(f_ini, "%x", &DiscrBaseAddr);

                        // I-pedestal
                        if (strstr(str, "IPED")!=NULL) {
                                fscanf(f_ini, "%d", &data);
                                Iped = (uint16_t)data;
                        }

                        // Discr_ChannelMask
                        if (strstr(str, "DISCR_CHANNEL_MASK")!=NULL) {
                                fscanf(f_ini, "%x", &data);
                                DiscrChMask = (uint16_t)data;
                        }

                        // Discr_OutputWidth
                        if (strstr(str, "DISCR_OUTPUT_WIDTH")!=NULL) {
                                fscanf(f_ini, "%d", &data);
                                DiscrOutputWidth = (uint16_t)data;
                        }

                        // Discr_Threshold
                        if (strstr(str, "DISCR_THRESHOLD")!=NULL) {
                                int ch, thr;
                                fscanf(f_ini, "%d", &ch);
                                fscanf(f_ini, "%d", &thr);
                                if (ch < 0) {
                                        for(int i=0; i<16; i++)
                                                DiscrThreshold[i] = thr;
                                } else if (ch < 16) {
                                        DiscrThreshold[ch] = thr;
                                }
                        }

                        // Discr_Threshold
                        if (strstr(str, "DISCR_THRESHOLD")!=NULL) {
                                int ch, thr;
                                fscanf(f_ini, "%d", &ch);
                                fscanf(f_ini, "%d", &thr);
                                if (ch < 0) {
                                        for(int i=0; i<16; i++)
                                                DiscrThreshold[i] = thr;
                                } else if (ch < 16) {
                                        DiscrThreshold[ch] = thr;
                                }
                        }

                        // LLD for the QTP
                        if (strstr(str, "QTP_LLD")!=NULL) {
                                int ch, lld;
                                fscanf(f_ini, "%d", &ch);
                                fscanf(f_ini, "%d", &lld);
                                if (ch < 0) {
                                        for(int i=0; i<32; i++)
                                                QTP_LLD[i] = lld;
                                } else if (ch < 32) {
                                        QTP_LLD[ch] = lld;
                                }
                        }


                        // I-pedestal
                        if (strstr(str, "ENABLE_SUPPRESSION")!=NULL) {
                                fscanf(f_ini, "%d", &EnableSuppression);
                        }


                }
        }


  fclose(f_ini);

  std::cout<<"---7777---  QTPD config red: DiscrBaseAddr " << DiscrBaseAddr<<std::endl;

  /* *************************************************************************************** */
  /* program the QTP                                                                         */
  /* *************************************************************************************** */
  EUDAQ_INFO("QTP: # boards in Conf "+std::to_string(NBoardsQTP));
  for( int brd = 0 ; brd<NBoardsQTP;brd++) {

        std::cout<<"---7777---  Programming QTP, board  " << brd<<std::endl;
        // Program the discriminator (if the base address is set in the config file)
        if (DiscrBaseAddr > 0) {
                int ret;
                printf("Discr Base Address = 0x%08X\n", DiscrBaseAddr);
                ret = ConfigureDiscr(DiscrOutputWidth, DiscrThreshold, DiscrChMask);
                if (ret < 0) {
                        EUDAQ_THROW("Can't access to the discriminator at Base Address 0x"+std::to_string(DiscrBaseAddr));
                }
        }

        // ************************************************************************
        // QTP settings
        // ************************************************************************
        // Reset QTP board

	handle=vhandle[brd];
        //BaseAddress = QTPBaseAddr;
	BaseAddress = QTPBaseAddrS[brd];
        std::cout<<"---7777---  Reset QTP, board  " << brd<<std::endl;

        write_reg(0x1016, 0);
        std::cout<<"---7777---  Reset QTP done, board  " << brd<<std::endl;
        if (VMEerror) {
                EUDAQ_THROW("Error during QTP programming: "+std::string(ErrorString));
        }

        // Read FW revision
        fwrev = read_reg(0x1000);
        if (VMEerror) {
                EUDAQ_THROW("Error during Read FW revision: "+std::string(ErrorString));
        }

        model = (read_reg(0x803E) & 0xFF) + ((read_reg(0x803A) & 0xFF) << 8);
        // read version (> 0xE0 = 16 channels)
        vers = read_reg(0x8032) & 0xFF;

	EUDAQ_INFO("QTP model, version:  "+std::to_string(model)+", "+std::to_string(vers));
	EUDAQ_INFO("QTP FW revision:  "+std::to_string((fwrev >> 8) & 0xFF)+"."+std::to_string(fwrev & 0xFF));

	findModelVersion(model, vers, modelVersion, &brd_nch[brd]);


        // Read serial number
        sernum = (read_reg(0x8F06) & 0xFF) + ((read_reg(0x8F02) & 0xFF) << 8);
        if (sernum == UINT32_C(0xFFFF)) {
                sernum = (read_reg(0x8EF6) & 0xFF) + ((read_reg(0x8EF4) & 0xFF) << 8) + ((read_reg(0x8EF2) & 0xFF) << 16);+ ((read_reg(0x8EF0) & 0xFF) << 24);
        }
	PID_QDC[0]=sernum; // fix me ...

	EUDAQ_INFO("QTP Serial Number:  "+std::to_string(sernum));
	EUDAQ_INFO("QTP # channels:  "+std::to_string(brd_nch[brd]));

        write_reg(0x1060, Iped);  // Set pedestal
	EUDAQ_INFO("QTP configured pedestal :" +std::to_string(Iped));
        write_reg(0x1010, 0x60);  // enable BERR to close BLT at and of block
	EUDAQ_INFO("QTP enableed BERR to close BLT at end of block  ");

        // Set LLD (low level threshold for ADC data)
        write_reg(0x1034, 0x100);  // set threshold step = 16
        for(int i=0; i<brd_nch[brd]; i++) {
                if (brd_nch[brd] == 16)      write_reg(0x1080 + i*4, QTP_LLD[i]/16);
                else                            write_reg(0x1080 + i*2, QTP_LLD[i]/16);
        }
	EUDAQ_INFO("QTP configured low level threshold for ADC data. ");

        if (!EnableSuppression) {
                write_reg(0x1032, 0x0010);  // disable zero suppression
                write_reg(0x1032, 0x0008);  // disable overrange suppression
                write_reg(0x1032, 0x1000);  // enable empty events
	}

	//uint16_t data = 0x20;  // Clear Bit 5 to enable Multi-Event Mode
	//write_reg(0x1034,0x20); 
	//CAENVME_WriteCycle(handle, 0x1034, &data, cvA32_U_DATA, cvD16);

	uint16_t control;
	control=read_reg(0x1000);
	//CAENVME_ReadCycle(handle, 0x1010, &control, cvA32_U_DATA, cvD16);
	if (control & 0x20) {
	    EUDAQ_WARN("QDC Single-Event Mode Enabled\n");
	} else {
	    EUDAQ_WARN("QDC Multi-Event Mode Enabled\n");
	}



  }

        //for(int i=0; i<32; i++) {
        //        ns[i]=0;
        //}


} // doConfig

//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void QTPDProducer::DoStartRun(){
  m_exit_of_run = false;

  std::chrono::time_point<std::chrono::high_resolution_clock> tp_start_aq = std::chrono::high_resolution_clock::now();
  //shmp->QTP_Aqu_start_time_us=tp_start_aq;


  std::cout<<"---7777---  DoStartRun" <<std::endl;

  pnt = 0;  // word pointer
  wcnt = 0; // num of lword read in the MBLT cycle
  QTPbuffer[0] = DATATYPE_FILLER;

  std::cout<<"---7777---  DoStartRun 2"  <<std::endl;
  for( int brd = 0 ; brd<NBoardsQTP;brd++) {
        std::cout<<"---7777---  DoStartRun, board "<<brd <<std::endl;
        handle=vhandle[brd];
        //BaseAddress = QTPBaseAddr;
        BaseAddress = QTPBaseAddrS[brd];

        // clear Event Counter
        write_reg(0x1040, 0x0);
        // clear QTP
        write_reg(0x1032, 0x4);
        write_reg(0x1034, 0x4);


        m_conn_evque[brd].clear();
  }

        for(int i=0; i<32; i++) {
                ns[i]=0;
        }

  std::cout<<"---7777---  DoStartRun done"  <<std::endl;
  //Sleep(5);
  //ret = CAEN_DGTZ_SetExtTriggerInputMode(vhandle[0], CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);




}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void QTPDProducer::DoStopRun(){
  m_exit_of_run = true;
  // here the hardware is told to stop data acquisition
  for( int brd = 0 ; brd<NBoardsQTP;brd++) {

     m_conn_evque[brd].clear();
  }

}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void QTPDProducer::DoReset(){
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

  for( int brd = 0 ; brd<NBoardsQTP;brd++) {
  }

}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void QTPDProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_file_lock){
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  for( int brd = 0 ; brd<NBoardsQTP;brd++) {
  }
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void QTPDProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  //std::random_device rd;
  //std::mt19937 gen(rd());
  //std::uniform_int_distribution<uint32_t> position(0, x_pixel*y_pixel-1);
  //std::uniform_int_distribution<uint32_t> signal(0, 255);

  while(!m_exit_of_run){
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    //std::cout<<"---7777---  Run loop" <<std::endl;
    
    auto ev = eudaq::Event::MakeUnique("QTPDProducer");
    ev->SetTag("Plane ID", std::to_string(m_plane_id));
    for( int brd = 0 ; brd<NBoardsQTP;brd++) {
        handle=vhandle[brd];
        //BaseAddress = QTPBaseAddr;
        BaseAddress = QTPBaseAddrS[brd];

        //std::cout<<"---7777---  Start reading data from QTP board "<< brd <<std::endl;

	/*
	CAENVME_FIFOMBLTReadCycle(handle, BaseAddress, (char *)QTPbuffer, MAX_BLT_SIZE, cvA32_U_MBLT, &bcnt);
        std::cout<<"---7777---  Read Data Block: size =  "<< bcnt <<" bytes" <<std::endl;
	if( bcnt >0 ){
		for(int b=0; b<(bcnt/4); b++)
                                        std::cout<<"---7777---   "<<b<<": "<< QTPbuffer[b]<<std::endl;

        }
        */
	int quit = 0;
        while (!quit) {
   		//std::cout<<"---7777---   pnt"<<pnt<<", j "<<j<<"  wcnt = "<<wcnt<<std::endl;
		uint16_t status;

		/*
		status = read_reg(0x100E);  // Read the register at offset 0x100E
		if (status & 0x1) {  // Check Bit 0
		    std::cout<<"---7777---   Data is ready to be red"<<std::endl;
    		    std::cout<<"---7777---   pnt"<<pnt<<", j "<<j<<"  wcnt = "<<wcnt<<std::endl;
		} else {
		    //std::cout<<"---7777---   Data is NOT ready to be red"<<std::endl;
    		    std::cout<<"---7777---   NOT pnt"<<pnt<<", j "<<j<<"  wcnt = "<<wcnt<<std::endl;
		}
		*/

                if ((pnt == wcnt) || ((QTPbuffer[pnt] & DATATYPE_MASK) == DATATYPE_FILLER)) {
                        CAENVME_FIFOMBLTReadCycle(handle, BaseAddress, (char *)QTPbuffer, MAX_BLT_SIZE, cvA32_U_MBLT, &bcnt);

                        wcnt = bcnt/4;
                        totnb += bcnt;
                        pnt = 0;
		}
                if (wcnt == 0)  // no data available
                        continue;

                switch (DataType) {
                case DATATYPE_HEADER :
                        if((QTPbuffer[pnt] & DATATYPE_MASK) != DATATYPE_HEADER) {
                                //printf("Header not found: %08X (pnt=%d)\n", buffer[pnt], pnt);
                                DataError = 1;
                        } else {
                                nch = (QTPbuffer[pnt] >> 8) & 0x3F;
                                chindex = 0;
                                nev++;
                                memset(ADCdata, 0xFFFF, 32*sizeof(uint16_t));
                                if (nch>0)
                                        DataType = DATATYPE_CHDATA;
                                else
                                        DataType = DATATYPE_EOB;
                        }
                        break;
                case DATATYPE_CHDATA :
                        if((QTPbuffer[pnt] & DATATYPE_MASK) != DATATYPE_CHDATA) {
                                //printf("Wrong Channel Data: %08X (pnt=%d)\n", buffer[pnt], pnt);
                                DataError = 1;
                        } else {
                                if (brd_nch[brd] == 32)
                                        j = (int)((QTPbuffer[pnt] >> 16) & 0x3F);  // for V792 (32 channels)
                                else
                                        j = (int)((QTPbuffer[pnt] >> 17) & 0x3F);  // for V792N (16 channels)
                                //histo[j][buffer[pnt] & 0xFFF]++;
                                ADCdata[j] = QTPbuffer[pnt] & 0xFFF;
				//std::cout<<"---7777---   ADCdata["<<j<<"] = "<< ADCdata[j]<<std::endl;
                                ns[j]++;
                                if (chindex == (nch-1))
                                        DataType = DATATYPE_EOB;
                                chindex++;
                        }
                        break;
                case DATATYPE_EOB :
                        if((QTPbuffer[pnt] & DATATYPE_MASK) != DATATYPE_EOB) {
                                DataError = 1;
                        } else {
                                DataType = DATATYPE_HEADER;
				ev->SetTriggerN(QTPbuffer[pnt] & 0xFFFFFF);
 				std::cout<<"---7777--- QDC  SetTriggerN "<< (QTPbuffer[pnt] & 0xFFFFFF) <<std::endl;
                        }
			quit=1;

			status = read_reg(0x1022); // Read Status Register 1
			if (status & 0x2) { // Check Bit 1
			    std::cout<<"---7777---   QDC: Buffer is empty."<<std::endl;
			} else if (status & 0x4) { // Check Bit 2
			    std::cout<<"---7777---   QDC: Buffer is full."<<std::endl;
			    EUDAQ_THROW("QDC: Buffer is full.");
			} else {
			    std::cout<<"---7777---   QDC: Buffer is partially filled."<<std::endl;
			}

                        break;
                }
                pnt++;
    	}// while - all records

	//for (int ich=0;ich<32;ich++)
	//	std::cout<<"---7777---   ADCdata["<<ich<<"] = "<< ADCdata[ich]<<std::endl;



        std::vector<uint8_t> data;
        //ev->SetTriggerN(trigger_n);
	make_header(brd, PID_QDC[brd], &data);
	// Add data here
        QTPpack_event(ADCdata,&data);

	ev->AddBlock(m_plane_id+brd, data);


    }// boards
    SendEvent(std::move(ev));
    m_conn_ev.clear();

    std::this_thread::sleep_until(tp_end_of_busy);

  } // while loop


}



/*******************************************************************************/
/*                                WRITE_REG                                    */
/*******************************************************************************/
void QTPDProducer::write_reg(uint16_t reg_addr, uint16_t data)
{
        CVErrorCodes ret;
        //std::cout<<"---7777---  WRITE_REG:  " << handle<<" , "<<BaseAddress<<" , "<<reg_addr<<" , "<<data<<" , "<<cvA32_U_DATA<<" , "<<cvD16<<std::endl;
        ret = CAENVME_WriteCycle(handle, BaseAddress + reg_addr, &data, cvA32_U_DATA, cvD16);
        if(ret != cvSuccess) {
                sprintf(ErrorString, "Cannot write at address %08X\n", (uint32_t)(BaseAddress + reg_addr));
                VMEerror = 1;
        }
        //fprintf(logfile, " Writing register at address %08X; data=%04X; ret=%d\n", (uint32_t)(BaseAddress + reg_addr), data, (int)ret);
}

/*******************************************************************************/
/*                               READ_REG                                      */
/*******************************************************************************/
uint16_t QTPDProducer::read_reg(uint16_t reg_addr)
{
        uint16_t data=0;
        CVErrorCodes ret;
        ret = CAENVME_ReadCycle(handle, BaseAddress + reg_addr, &data, cvA32_U_DATA, cvD16);
        if(ret != cvSuccess) {
                sprintf(ErrorString, "Cannot read at address %08X\n", (uint32_t)(BaseAddress + reg_addr));
                VMEerror = 1;
        }
        //fprintf(logfile, " Reading register at address %08X; data=%04X; ret=%d\n", (uint32_t)(BaseAddress + reg_addr), data, (int)ret);
        return(data);
}


// ************************************************************************
// Discriminitor settings
// ************************************************************************
int QTPDProducer::ConfigureDiscr(uint16_t OutputWidth, uint16_t Threshold[16], uint16_t EnableMask)
{
        int i;

        BaseAddress = DiscrBaseAddr;
        // Set channel mask
        write_reg(0x004A, EnableMask);

        // set output width (same for all channels)
        write_reg(0x0040, OutputWidth);
        write_reg(0x0042, OutputWidth);

        // set CFD threshold
        for(i=0; i<16; i++)
                write_reg(i*2, Threshold[i]);

        if (VMEerror) {
                printf("Error during CFD programming: ");
                printf(ErrorString);
        } else {
                printf("Discriminator programmed successfully\n");
                return 0;
        }
        BaseAddress = QTPBaseAddr;
}

void QTPDProducer::findModelVersion(uint16_t model, uint16_t vers, char *modelVersion, int *ch) {
        switch (model) {
        case 792:
                switch (vers) {
                case 0x11:
                        strcpy(modelVersion, "AA");
                        *ch = 32;
                        return;
                case 0x13:
                        strcpy(modelVersion, "AC");
                        *ch = 32;
                        return;
                case 0xE1:
                        strcpy(modelVersion, "NA");
                        *ch = 16;
                        return;
                case 0xE3:
                        strcpy(modelVersion, "NC");
                        *ch = 16;
                        return;
                default:
                        strcpy(modelVersion, "-");
                        *ch = 32;
                        return;
                }
                break;
        case 965:
                switch (vers) {
                case 0x1E:
                        strcpy(modelVersion, "A");
                        *ch = 16;
                        return;
                case 0xE3:
                case 0xE1:
                        strcpy(modelVersion, " ");
                        *ch = 32;
                        return;
                default:
                        strcpy(modelVersion, "-");
                        *ch = 32;
                        return;
                }
                break;
        case 775:
                switch (vers) {
                case 0x11:
                        strcpy(modelVersion, "AA");
                        *ch = 32;
                        return;
                case 0x13:
                        strcpy(modelVersion, "AC");
                        *ch = 32;
                        return;
                case 0xE1:
                        strcpy(modelVersion, "NA");
                        *ch = 16;
                        return;
                case 0xE3:
                        strcpy(modelVersion, "NC");
                        *ch = 16;
                        return;
                default:
                        strcpy(modelVersion, "-");
                        *ch = 32;
                        return;
                }
                break;
        case 785:
                switch (vers) {
                case 0x11:
                        strcpy(modelVersion, "AA");
                        *ch = 32;
                        return;
                case 0x12:
                        strcpy(modelVersion, "Ab");
                        *ch = 32;
                        return;
                case 0x13:
                        strcpy(modelVersion, "AC");
                        *ch = 32;
                        return;
                case 0x14:
                        strcpy(modelVersion, "AD");
                        *ch = 32;
                        return;
                case 0x15:
                        strcpy(modelVersion, "AE");
                        *ch = 32;
                        return;
                case 0x16:
                        strcpy(modelVersion, "AF");
                        *ch = 32;
                        return;
                case 0x17:
                        strcpy(modelVersion, "AG");
                        *ch = 32;
                        return;
                case 0x18:
                        strcpy(modelVersion, "AH");
                        *ch = 32;
                        return;
                case 0x1B:
                        strcpy(modelVersion, "AK");
                        *ch = 32;
                        return;
                case 0xE1:
                        strcpy(modelVersion, "NA");
                        *ch = 16;
                        return;
                case 0xE2:
                        strcpy(modelVersion, "NB");
                        *ch = 16;
                        return;
                case 0xE3:
                        strcpy(modelVersion, "NC");
                        *ch = 16;
                        return;
                case 0xE4:
                        strcpy(modelVersion, "ND");
                        *ch = 16;
                        return;
                default:
                        strcpy(modelVersion, "-");
                        *ch = 32;
                        return;
                }
                break;
        case 862:
                switch (vers) {
                case 0x11:
                        strcpy(modelVersion, "AA");
                        *ch = 32;
                        return;
                case 0x13:
                        strcpy(modelVersion, "AC");
                        *ch = 32;
                        return;
                default:
                        strcpy(modelVersion, "-");
                        *ch = 32;
                        return;
                }
                break;
        }
}

