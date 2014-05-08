#ifdef __CINT__

#include "C:\Atlas_upgrade_workspace\sctdaq\stdll\TST.h"
#include "RQ_OBJECT.h"
#include "RTypes.h"
#include "SCTProducer.h"
#include <vector>


class  sct_root_interface{
	
RQ_OBJECT("sct_root_interface")
public:
	
	sct_root_interface(TST* eIn,const char* nameIn,const char* IP_AdresseIn){
		m_tst_instance=eIn;
		std::string name(nameIn),ip(IP_AdresseIn);

		m_sct=new ROOTProducer(name,ip);
		m_sct->Connect("send2TST_start_run()","sct_root_interface",this,"start_run()");

	}
	~sct_root_interface(){
		delete m_sct;
	}

	//slots 
	void setEventNo(int evNo){
		m_tst_instance->eventNumber=evNo;
	}
	void setTelStatus(int telStat){
		m_tst_instance->tel_status=telStat;
	}
	void SetEvent_Pending_status(){
		
		m_sct->recieve_eventPendingStatus(m_tst_instance->EventsPending());
	}
	void fillEventVector(){
		m_tst_instance->get_event(&m_sct->outputVector);

	}
	void start_run(){

		setEventNo(0);
		setTelStatus(1);
		// Set lemo to output the IDCs (128) for debugging
		m_tst_instance->ConfigureVariable(10021, 0x0080);


		// Save settings before the scan
		m_tst_instance->SaveState();

		if(m_tst_instance->abort==0){
			m_tst_instance->do_fits=0;           // integrated fitting off?
			m_tst_instance->burst_trtype=0;      // Single L1 only! see defintions in st_single function in stlib.c
			//e->burst_ntrigs=10;

			m_tst_instance->do_cal_loop=0;        // cal loop ON 
			m_tst_instance->do_fill_evtree=1;

			//readScanConfig("D:\\sctvar\\config\\Test_beam.dat"); 

			// External Trigger 
			short burstType = 21; //$$change
			st_scan(burstType,-1);//$$change


		}

	
		void ReadoutLoop(){
			std::vector<unsigned char> inVector;
			while (!m_sct->getDone()) {
				if (!m_tst_instance->EventsPending()) {
					// No events are pending, so check if the run is stopping
					if (m_sct->getstopping()) {
						// if so, signal that there are no events left
						m_tst_instance->tel_status=2;
						m_sct->setStopped(false);
						
					}
					// Now sleep for a bit, to prevent chewing up all the CPU
					eudaq::mSleep(1);
					// Then restart the loop
					continue;
				}
				if (!m_sct->getStarted())
				{
					eudaq::mSleep(20);
					// Then restart the loop
					continue;
				}

				// If we get here, there must be data to read out
				// Create a RawDataEvent to contain the event data to be sent
				
				m_sct->createNewEvent();

				//eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

				//ev.SetTag("blabla",1);


// 				getEvent();
// 
// 				long long t1=(long long)outputVector.at(10);
// 				long long t2=(long long)outputVector.at(11)<<8;
// 				long long t3=(long long)outputVector.at(12)<<16;
// 				long long t4=(long long)outputVector.at(13)<<24;
// 				long long t5=t1+t2+t3+t4;

				unsigned long long deltaTime;//=t5-startTime_;
				//ev.setTimeStamp(static_cast<unsigned long long>(deltaTime));$$ needs to be implemented
				
				m_sct->setTimeStamp(deltaTime);
				m_sct->AddPlane2Event(0,inVector);
// 				unsigned plane =0;
// 				ev.AddBlock(plane, outputVector);


				m_sct->sendEvent();

		}
	}

 ROOTProducer* m_sct;
TST* m_tst_instance;
};



#endif