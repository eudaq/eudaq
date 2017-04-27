#include "TimepixDevice.h"

// TOT . start on trigger stop on Timer
//char configName[256] = "config_I10-W0015_TOT_4-06-13" ;
//char AsciiconfigName[256] = "config_I10-W0015_TOT_4-06-13_ascii" ;


//char configName[256] = "/home/lcd/CLIC_Testbeam_August2013/TimepixAssemblies_Data/C04-W0110/Configs/BPC_C04-W0110_15V_IKrum1_96MHz_08-08-13" ;
//char AsciiconfigName[256] = "/home/lcd/CLIC_Testbeam_August2013/eudaq/TimepixProducer/Pixelman_SCL_2011_12_07/Default_Ascii_Config" ;

FRAMEID id;


TimepixDevice::TimepixDevice(){


		// Init manager
		control = -1;
		u32 flags = MGRINIT_NOEXHANDLING;
		control = mgrInitManager(flags, '\0');
		if(mgrIsRunning()){
			cout << "[TimepixDevice] Manager running " << endl;
		}

		mgrRegisterCallback("mpxctrl",MPXCTRL_CB_ACQCOMPL,&AcquisitionFinished,0);
		mgrRegisterCallback("mpxmgr",MPXMGR_CB_FRAME_NEW,&FrameIsReady,0);
		mgrRegisterCallback("mpxctrl",MPXCTRL_CB_ACQPRESTART,&AcquisitionPreStarted,0);
		mgrRegisterCallback("mpxctrl",MPXCTRL_CB_ACQSTART,&AcquisitionStarted,0);


		// Find device
		count = 0;
		
		
		control = mpxCtrlGetFirstMpx(&devId, &count);
		cout << "[TimepixDevice] found : " << count << " | devId : " << devId << endl;
		numberOfChips=0;
		numberOfRows=0;


		//Get Info on device
		int info = mpxCtrlGetMedipixInfo(devId,&numberOfChips,&numberOfRows,chipBoardID,ifaceName);
		if(info==0) cout << "[TimepixDevice] Number of chips : " << numberOfChips << " Number of rows : " <<  numberOfRows << " chipBoard ID : " <<  chipBoardID << " Interface Name : " << ifaceName <<  endl;
                
		else cout << "[TimepixDevice] board not found !! " << endl;
}

void TimepixDevice::Configure(const std::string & binary_config, const std::string & ascii_config){

			control = mpxCtrlLoadPixelsCfg(devId, binary_config.c_str() , true);
			cout << "[TimepixDevice] Load binary pixels config : " << binary_config << endl;

			mpxCtrlLoadMpxCfg(devId,ascii_config.c_str());
			cout << "[TimepixDevice] Load Ascii pixels config : " << ascii_config << endl;

			mgrSetFrameType(0,TYPE_U32);
}


int  TimepixDevice::ReadFrame(char * Filename, char* buffer){
	ifstream in(Filename,ios::binary|ios::ate);
	long size = in.tellg();
	in.seekg(0,ios::beg);
	in.read(buffer,size);
	in.close();
	return 0;
}


//
//int TimepixDevice::SetTHL(int THL){
//
//		// Get and Set Daqs
//		DACTYPE dacVals[14];
//		int chipnumber = 0;
//
//		dacVals[TPX_THLFINE] = THL; // 1000e- above thl adjustment mean
//		control = mpxCtrlSetDACs(devId, dacVals, 14, chipnumber);
//		cout << "[TimepixProducer] Setting THL Fine to " << THL << endl;
//
//		DACTYPE dacVals_new[14];
//		control = mpxCtrlGetDACs(devId, dacVals_new, 14, chipnumber);
//		cout << "[TimepixProducer] THL Fine Readout = " << dacVals_new[TPX_THLFINE] << endl;
//
//		control = mpxCtrlRefreshDACs(devId);
//		return control;
//
//	}

int TimepixDevice::SetIkrum(int IKrum){

		// Get and Set Daqs
		DACTYPE dacVals[14];
		int chipnumber = 0;

		dacVals[TPX_IKRUM] = IKrum; // 1000e- above thl adjustment mean
		control = mpxCtrlSetDACs(devId, dacVals, 14, chipnumber);
		cout << "[TimepixProducer] Setting Ikrum to " << IKrum << endl;

		DACTYPE dacVals_new[14];
		control = mpxCtrlGetDACs(devId, dacVals_new, 14, chipnumber);
		cout << "[TimepixProducer] IKrum Readout = " << dacVals_new[TPX_IKRUM] << endl;

		control = mpxCtrlRefreshDACs(devId);
		return control;

	}

int TimepixDevice::SetTHL(int THL){

		// Get and Set Daqs
		DACTYPE dacVals[14];
		DACTYPE dacVals_new[14];
		int chipnumber = 0;
		control = mpxCtrlGetDACs(devId, dacVals, 14, chipnumber);

		dacVals[TPX_THLFINE] = THL; // 1000e- above thl adjustment mean
		control = mpxCtrlSetDACs(devId, dacVals, 14, chipnumber);
		cout << "[TimepixProducer] Setting THL to " << THL << endl;
		control = mpxCtrlRefreshDACs(devId);


		control = mpxCtrlGetDACs(devId, dacVals_new, 14, chipnumber);
		cout << "[TimepixProducer] Reading back THL Value , THL = " << dacVals_new[TPX_THLFINE] << endl ;

		cout << "[TimepixProducer] All the DACS values " << endl;
		ReadDACs();

		return control;

	}



int TimepixDevice::ReadDACs(){


		DACTYPE dacVals_readout[14];
		int chipnumber = 0;

		control = mpxCtrlGetDACs(devId, dacVals_readout, 14, chipnumber);
		cout << "[TimepixProducer] IKrum Readout = " << dacVals_readout[TPX_IKRUM] << endl;
		cout << "[TimepixProducer] DISC Readout = " << dacVals_readout[TPX_DISC] << endl;
		cout << "[TimepixProducer] PREAMP Readout = " << dacVals_readout[TPX_PREAMP] << endl;
		cout << "[TimepixProducer] BUFFA Readout = " << dacVals_readout[TPX_BUFFA] << endl;
		cout << "[TimepixProducer] BUFFB Readout = " << dacVals_readout[TPX_BUFFB] << endl;
		cout << "[TimepixProducer] HIST Readout = " << dacVals_readout[TPX_HIST] << endl;
		cout << "[TimepixProducer] THLFINE Readout = " << dacVals_readout[TPX_THLFINE] << endl;
		cout << "[TimepixProducer] THLCOARSE Readout = " << dacVals_readout[TPX_THLCOARSE] << endl;
		cout << "[TimepixProducer] VCAS Readout = " << dacVals_readout[TPX_VCAS] << endl;
		cout << "[TimepixProducer] FBK Readout = " << dacVals_readout[TPX_FBK] << endl;
		cout << "[TimepixProducer] GND Readout = " << dacVals_readout[TPX_GND] << endl;
		cout << "[TimepixProducer] THS Readout = " << dacVals_readout[TPX_THS] << endl;
		cout << "[TimepixProducer] BIASLVDS Readout = " << dacVals_readout[TPX_BIASLVDS] << endl;
		cout << "[TimepixProducer] REFLVDS Readout = " << dacVals_readout[TPX_REFLVDS] << endl;

		return dacVals_readout[TPX_THLFINE];
	}


int TimepixDevice::PerformAcquisition(char *output){

		//cout << "[timepixProducer] Starting acquisition to file " << output << endl;
		control = mpxCtrlPerformFrameAcq(devId, 1,acqTime, FSAVE_BINARY |FSAVE_U32 ,output);

		//mpxCtrlGetFrame32(devId,buffer, MATRIX_SIZE, u32 frameNumber);
		return control;

	}



int TimepixDevice::GetFrameData(byte *buffer){

		control=mpxCtrlGetFrameID(devId,0,&FrameID);
		control=mgrGetFrameData(id,buffer, &size , TYPE_DOUBLE);
		return control;
	}
	
	
int TimepixDevice::GetFrameData2(char * Filename, char* buffer){



			ifstream in(Filename,ios::binary|ios::ate);
			long size = in.tellg();
			in.seekg(0,ios::beg);

			in.read(buffer,size);
			in.close();



			return 0;

	}

int  TimepixDevice::GetFrameDatadAscii(char * Filename, u32* buffer){



			ifstream in(Filename);

			u32 tot;
			int i=0;
			while(!in.eof()){
				in >> tot;
				buffer[i]=tot;
				cout << tot << endl;

				i++;
			}

			in.close();



			return 0;

	}

int TimepixDevice::Abort (){
		control = mpxCtrlAbortOperation(devId);
		return control;
	}

int TimepixDevice::SetAcqTime(double time){
	   acqTime=time;
   	   return 1;
   }
