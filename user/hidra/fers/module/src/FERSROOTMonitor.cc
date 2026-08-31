#include "eudaq/ROOTMonitor.hh"
#include "eudaq/RawEvent.hh"

#include "FERSlib.h"
#undef max

#include "FERS_EUDAQ.h"
#include "DRS_EUDAQ.h"
#include "QTP_EUDAQ.h"

#include "TH1.h"
#include "TH2.h"
#include "TGraph2D.h"
#include "TProfile.h"

// let there be a user-defined Ex0EventDataFormat, containing e.g. three
// double-precision attributes get'ters:
//   double GetQuantityX(), double GetQuantityY(), and double GetQuantityZ()
//struct Ex0EventDataFormat {
//  Ex0EventDataFormat(const eudaq::Event&) {}
//  double GetQuantityX() const { return (double)rand()/RAND_MAX; }
//  double GetQuantityY() const { return (double)rand()/RAND_MAX; }
//  double GetQuantityZ() const { return (double)rand()/RAND_MAX; }
//};

extern struct shmseg *shmp;
extern int shmid;


class FERSROOTMonitor : public eudaq::ROOTMonitor {
public:
  FERSROOTMonitor(const std::string& name, const std::string& runcontrol):
    eudaq::ROOTMonitor(name, "FERS-DRS ROOT monitor", runcontrol){}

  void AtConfiguration() override;
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("FERSROOTMonitor");

private:
  TH1D* m_FERS_LG_Ch_ADC[20][FERSLIB_MAX_NCH_5202];
  TH1D* m_FERS_HG_Ch_ADC[20][FERSLIB_MAX_NCH_5202];

  TProfile* m_DRS_Pulse_Ch[20][MAX_X742_CHANNEL_SIZE*MAX_X742_GROUP_SIZE];  // 4gr x 9ch/gr

  TH1D* m_QDC_ADC[8][32]; // 8 possible boards with 32 channels


  TH1D* m_FERS_tempFPGA;
  TH1D* m_FERS_tempDetector;
  TH1D* m_FERS_hv_Vmon;
  TH1D* m_FERS_hv_Imon;
//  TH1I* m_FERS_hv_status_on;
  TH1I* m_DRS_nEvt;


  TH2F* m_FERS_SUMadc;

  //TH1D* m_trgTime_diff;
  //TH1D* m_my_hist;
  //TGraph2D* m_my_graph;

  //TH2D* m_DRS_FERS;


  int brd;
  int PID; 

  uint64_t trigTime[2];
  int energyLG[FERSLIB_MAX_NCH_5202];
  int energyHG[FERSLIB_MAX_NCH_5202];
  int tstamp[FERSLIB_MAX_NCH_5202];
  int ToT[FERSLIB_MAX_NCH_5202];

};

namespace{
  auto mon_rootmon = eudaq::Factory<eudaq::Monitor>::
    Register<FERSROOTMonitor, const std::string&, const std::string&>(FERSROOTMonitor::m_id_factory);
}

void FERSROOTMonitor::AtConfiguration(){

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


  m_FERS_tempFPGA =  m_monitor->Book<TH1D>("Monitor1/FERS_tempFPGA","FERS_tempFPGA","h_FERS_tempFPGA","FERS_tempFPGA;Board;tempFPGA [C]",16,-0.5,15.5);
  m_FERS_tempDetector =  m_monitor->Book<TH1D>("Monitor1/FERS_tempDetector","FERS_tempDetector","h_FERS_tempDetector","FERS_tempDetector;Board;tempDetector [C]",16,-0.5,15.5);
  m_FERS_hv_Vmon =  m_monitor->Book<TH1D>("Monitor1/FERS_hv_Vmon","FERS_hv_Vmon","h_FERS_hv_Vmon","FERS_hv_Vmon;Board;hv_Vmon [V]",16,-0.5,15.5);
  m_FERS_hv_Imon =  m_monitor->Book<TH1D>("Monitor1/FERS_hv_Imon","FERS_hv_Imon","h_FERS_hv_Imon","FERS_hv_Imon;Board;hv_Imon [mA]",16,-0.5,15.5);
  //m_FERS_hv_status_on =  m_monitor->Book<TH1I>("Monitor/FERS_hv_status_on","FERS_hv_status_on","h_FERS_hv_status_on","FERS_hv_status_on;Board;hv_status_on",16,-0.5,15.5);
  m_DRS_nEvt =  m_monitor->Book<TH1I>("Monitor1/DRS_nEvt","DRS_nEvt","h_DRS_nEvt","DRS Nevt Mem;Number of events in buffer;Entries",128,-0.5,128.5);

  int nbins = 0;
  for (int i = 0; i < MAX_NGR; ++i)    nbins += shmp->connectedboards[i];
  m_FERS_SUMadc  =m_monitor->Book<TH2F>("Monitor2/FERS_SUMadc", "FERS_SUMadc" ,"h_FERS_SUMadc","FERS: (Sum ADC)/evt;FERS channel;FERS brd x2", 64,0,64,nbins*2,0.,nbins*2);
  //m_trgTime_diff = m_monitor->Book<TH1D>("Monitor/trigTime_diff_FD","trigTime_diff_FD","h_trigTime_diff_FD","time difference between FERS and DRS in ms;;time(FERS-DRS) [ms]",3,0.,1.);

/*
  for(int ich=0;ich<16;ich++) {
		char hname[256];
		char tname[256];
		char sname[256];
		sprintf (hname,"FERS/Board_%d_HG_SiPM/ADC_Ch%02d",1,brd1[ich]);
		sprintf (tname,"Board %d HG_ADC_Ch%02d",1,brd1[ich]);
		sprintf (sname,"h_Board%d_HG_ADC_Ch%02d",1,brd1[ich]);
		m_FERS_HG_Ch_ADC1[ich] =  m_monitor->Book<TH1D>(hname,tname , sname,
			"HG ADC;ADC;# of evt", 4096, 0., 8192.);
  }
*/
  for(int k=0;k<MAX_NGR;k++) {
    for(int ii=0;ii<shmp->connectedboards[k];ii++) {
	int i = shmp->flat_idx[k][ii];
	for(int ich=0;ich<FERSLIB_MAX_NCH_5202;ich++) {
		char hname[256];
		char tname[256];
		char sname[256];
		sprintf (hname,"FERS/Board_%02d_LG/ADC_Ch%02d",i,ich);
		sprintf (tname,"Board %02d LG_ADC_Ch%02d",i,ich);
		sprintf (sname,"h_Board%02d_LG_ADC_Ch%02d",i,ich);
		m_FERS_LG_Ch_ADC[i][ich] =  m_monitor->Book<TH1D>(hname,tname , sname,
			"LG ADC;ADC;# of evt", 4096, 0., 8192.);

		sprintf (hname,"FERS/Board_%02d_HG/ADC_Ch%02d",i,ich);
		sprintf (tname,"Board %02d HG_ADC_Ch%02d",i,ich);
		sprintf (sname,"h_Board%02d_HG_ADC_Ch%02d",i,ich);
		m_FERS_HG_Ch_ADC[i][ich] =  m_monitor->Book<TH1D>(hname,tname , sname,
			"HG ADC;ADC;# evt", 4096, 0., 8192.);
	}
    }
  }
  for(int i=0;i<shmp->connectedboardsQTP;i++) {
  //for(int i=0;i<1;i++) {
	for(int ich=0;ich<32;ich++) {
		char hname[256];
		char tname[256];
		char sname[256];
		sprintf (hname,"QDC/Board_%02d/ADC_Ch%02d",i,ich);
		sprintf (tname,"Board %02d ADC_Ch%02d",i,ich);
		sprintf (sname,"h_Board%02d_ADC_Ch%02d",i,ich);
		m_QDC_ADC[i][ich] =  m_monitor->Book<TH1D>(hname,tname , sname,
			"ADC;ADC;# of evt", 2048, 0., 65536.);

	}
  }

  for(int i=0;i<shmp->connectedboardsDRS;i++) {
        char hname[256];
	char tname[256];
	char sname[256];
	//sprintf (hname,"DRS/Board_%d/Ch0_TS0_ADC",i);
        //m_DRS_Ch_TS0_ADC[i] =  m_monitor->Book<TH1D>(hname,"Ch0_TS0_ADC" ,
        //        "h_Ch_TS0_ADC", "ADC in TS0;ADC;# of evt", 4096, 0., 4096.);
	for( int ich =0; ich < 36; ich++) {
	sprintf (hname,"DRS/Board_%d/Pulse_Ch%02d",i, ich);
	sprintf (tname,"Board %d Pulse Ch%02d",i,ich);
	sprintf (sname,"h_Board%d_Pulse_Ch%02d",i,ich);
  	m_DRS_Pulse_Ch[i][ich] = m_monitor->Book<TProfile>(hname, tname , sname, 
    		"Pulse ;TS;ADC", 1024, 0., 1024.);
	}
  }
  //m_my_graph = m_monitor->Book<TGraph2D>("Channel 0/my_graph", "Example graph");
  //m_my_graph->SetTitle("A graph;x-axis title;y-axis title;z-axis title");
  //m_monitor->SetDrawOptions(m_my_graph, "colz");
  //m_DRS_FERS =  m_monitor->Book<TH2D>("Test/FERS_DRS_2D", "FERS_DRS_2D" ,"h_FERS_DRS_2D","Aplitude FERS vs DRS int;DRS intADC;FERS ADC", 200,0.,1000.,200,0.,8000.);

}

void FERSROOTMonitor::AtEventReception(eudaq::EventSP ev){
	//if(m_en_print)
        //         ev->Print(std::cout);

	//double sigFERS=0;
	//double sigDRS=0;
	//double pedDRS=0;
	int trigN = 0;
	int fers_group=0;
	uint32_t nsub_ev = ev->GetNumSubEvent();
	float hv_Vmon,hv_Imon,tempDet,tempFPGA;
	for(int iev=0; iev<nsub_ev;iev++) {
		auto ev_sub = ev->GetSubEvent(iev);
		trigN = ev->GetTriggerN();
		if(ev_sub->GetDescription()=="FERSProducer") { // Decode FERS
			trigTime[0]=ev_sub->GetTimestampBegin()/1000;
			auto block_n_list = ev_sub->GetBlockNumList();
			std::ofstream outFile("Monitor_out.txt", std::ios::out);
			outFile<<"Events: "<<trigN<<"\n";
			shmp->SUMevt++;
			int ybin=0;
			for(auto &block_n: block_n_list){
		                std::vector<uint8_t> block = ev_sub->GetBlock(block_n);
				int index = read_headerFERS(&block, &brd, &PID, &hv_Vmon, &hv_Imon, &tempDet, &tempFPGA); // read shared mem instead ...
				std::vector<uint8_t> data(block.begin()+index, block.end());

				//std::cout<<"---7777--- block.size() = "<<block.size()<<std::endl;
				//std::cout<<"---7777--- data.size() = "<<data.size()<<std::endl;
				//std::cout<<"---7777--- brd = "<<brd<<std::endl;
				//std::cout<<"---7777--- PID = "<<PID<<std::endl;

  				SpectEvent_t EventSpect = FERSunpack_spectevent(&data);
  				//SpectEvent_t EventSpect = FERSunpack_tspectevent(&data);

				m_FERS_hv_Vmon->SetBinContent(brd+1,hv_Vmon);
			        m_FERS_hv_Imon->SetBinContent(brd+1,hv_Imon);
			        m_FERS_tempFPGA->SetBinContent(brd+1,tempFPGA);
			        m_FERS_tempDetector->SetBinContent(brd+1,tempDet);
			        //m_FERS_hv_status_on->SetBinContent(brd+1,shmp->hv_status_on[brd]);
				outFile << brd <<", "<<tempFPGA<<", "<<hv_Vmon<<", "<<hv_Imon<<"\n";
                                for (int i=0; i<FERSLIB_MAX_NCH_5202; i++)
                                {
                                        energyHG[i] = EventSpect.energyHG[i];
                                        energyLG[i] = EventSpect.energyLG[i];
					shmp->SUMenergyHG[brd][i]+=energyHG[i];
					shmp->SUMenergyLG[brd][i]+=energyLG[i];

					ybin=brd*2;
					//m_FERS_SUMadc->Fill(i+1,ybin+1,shmp->SUMenergyLG[brd][i]/shmp->SUMevt);
					//m_FERS_SUMadc->Fill(i+1,ybin+2,shmp->SUMenergyHG[brd][i]/shmp->SUMevt);
					m_FERS_SUMadc->Fill(i,ybin,(double)(shmp->SUMenergyLG[brd][i]) / (double)(shmp->SUMevt));
					m_FERS_SUMadc->Fill(i,ybin+1,(double)(shmp->SUMenergyHG[brd][i]) / (double)(shmp->SUMevt));

                                        //tstamp[i] = EventSpect.tstamp[i];
                                        //ToT[i] = EventSpect.ToT[i];
					m_FERS_LG_Ch_ADC[brd][i]->Fill(energyLG[i]);
					m_FERS_HG_Ch_ADC[brd][i]->Fill(energyHG[i]);
					//m_FERS_ToA_Ch_ADC[brd][i]->Fill(tstamp[i]);
					//m_FERS_ToT_Ch_ADC[brd][i]->Fill(ToT[i]);
					//std::cout<<"---7777--- ToA "<<ToT[i]<<"["<<i<<"]"<<EventSpect.ToT[i]<<std::endl;
					//if(brd==2 && i==3) sigFERS=energyLG[i];
					//if(brd==1) {
					//	for(int ik=0;ik<16;ik++){
					//		if(i==brd1[ik]) m_FERS_HG_Ch_ADC1[ik]->Fill(energyHG[i]);
					//	}
					//}
                                }




			}

			outFile.close();
		}else if(ev_sub->GetDescription()=="DRSProducer"){  //Decode DRS

			trigTime[1]=ev_sub->GetTimestampBegin()/1000;
			auto block_n_list = ev_sub->GetBlockNumList();
			for(auto &block_n: block_n_list){
		                std::vector<uint8_t> block = ev_sub->GetBlock(block_n);
				int index = read_header(&block, &brd, &PID);
				std::vector<uint8_t> data(block.begin()+index, block.end());

				//std::cout<<"---7777--- block.size() = "<<block.size()<<std::endl;
				//std::cout<<"---7777--- data.size() = "<<data.size()<<std::endl;
				//std::cout<<"---7777--- brd = "<<brd<<std::endl;
				//std::cout<<"---7777--- PID = "<<PID<<std::endl;

				m_DRS_nEvt->AddBinContent(shmp->nevtDRS[brd],1);
				if(data.size()>0) {
  					//CAEN_DGTZ_X742_EVENT_t* Event = DRSunpack_event (&data);
  					auto Event = DRSunpack_event (&data);

					int hch = 0;
					for (int igr=0; igr<4;igr++){
						if (Event->GrPresent[igr]) {
							for (int ich=0; ich<MAX_X742_CHANNEL_SIZE;ich++){
							hch = ich + igr*MAX_X742_CHANNEL_SIZE;
								m_DRS_Pulse_Ch[brd][hch]->Reset();
								for(int its=0;its<Event->DataGroup[igr].ChSize[ich];its++){
									m_DRS_Pulse_Ch[brd][hch]->Fill(Float_t(its+0.5),Event->DataGroup[igr].DataChannel[ich][its]);
								}
							}
						}
					}
					FreeDRSEvent(Event);

				}
			}

		}else if(ev_sub->GetDescription()=="QTPDProducer"){  //Decode QTP
			trigTime[0]=ev_sub->GetTimestampBegin()/1000;
			auto block_n_list = ev_sub->GetBlockNumList();
			uint16_t ADCdata[32];
			for(auto &block_n: block_n_list){
		                std::vector<uint8_t> block = ev_sub->GetBlock(block_n);
				int index = read_header(&block, &brd, &PID);
				std::vector<uint8_t> data(block.begin()+index, block.end());

				//std::cout<<"---7777--- block.size() = "<<block.size()<<std::endl;
				//std::cout<<"---7777--- data.size() = "<<data.size()<<std::endl;
				//std::cout<<"---7777--- brd = "<<brd<<std::endl;
				//std::cout<<"---7777--- PID = "<<PID<<std::endl;

				 QTPunpack_event(&data, ADCdata);


                                for (int i=0; i<32; i++)
                                {
					m_QDC_ADC[brd][i]->Fill(ADCdata[i]);
                                }




			}

		}else { // Decode Beam elements (to be included later)
		}
	}
		//m_trgTime_diff->SetBinContent(2,static_cast<double>(trigTime[0])-static_cast<double>(trigTime[1]));
		//m_DRS_FERS->Fill((sigDRS-pedDRS+93000)/10.,sigFERS);
		//std::cout<<"---9999---"<<sigFERS<<" , "<<sigDRS-pedDRS<<" , "<<pedDRS<<" , "<<sigDRS<<std::endl;


  //auto event = std::make_shared<Ex0EventDataFormat>(*ev);
  //m_my_hist->Fill(event->GetQuantityX());
  //m_my_graph->SetPoint(m_my_graph->GetN(),
  //  event->GetQuantityX(), event->GetQuantityY(), event->GetQuantityZ());
  //m_my_prof->Fill(event->GetQuantityX(), event->GetQuantityY());


}

