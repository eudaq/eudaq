/*
 * TimepixProducer.cxx
 *
 *
 *      Author: Mathieu Benoit
 *      		Szymon Kulis
 *      		PH-LCD , CERN
 */

//#define USE_KEITHLEY
#define KEITHLEYDEBUG

#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include "eudaq/Mutex.hh"
#include "mpxmanagerapi.h"
#include "mpxhw.h"
#include <iostream>
#include <ostream>
#include <vector>
#include <signal.h>
#include <pthread.h>    /* POSIX Threads */
#include "MIMTLU.h"
#include "TimepixDevice.h"
#include <time.h>

#ifdef USE_KEITHLEY
#include "Keithley2000.h"
#include "Keithley2410.h"
#endif

using namespace std;

//#define DEBUGFITPIX


float GetPt100Temperature(float r)
{
      float const Pt100[] = {80.31, 82.29, 84.27, 86.25, 88.22, 90.19, 92.16, 94.12, 96.09, 98.04,
                            100.0, 101.95, 103.9, 105.85, 107.79, 109.73, 111.67, 113.61, 115.54, 117.47,
                            119.4, 121.32, 123.24, 125.16, 127.07, 128.98, 130.89, 132.8, 134.7, 136.6,
                            138.5, 140.39, 142.29, 157.31, 175.84, 195.84};

      int t = -50, i = 0, dt = 0;

      if (r > Pt100[0])
         while (250 > t)
         {
               dt = (t < 110) ? 5 : (t > 110) ? 50 : 40;

               if (r < Pt100 [++i])
                  return t + ( r - Pt100[i-1])*dt/(Pt100[i]-Pt100[i-1]);
                  t += dt;
         };

      return t;

}

struct timeval t0;

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "TimepixRaw";

int status;

struct fitpixstate_t
{
  bool AcqPreStarted;
  bool FrameReady;
  bool AcquisitionStarted;
  bool AcquisitionFinished;
  fitpixstate_t()
  {
    reset();
  }
  void reset(void)
  {
    AcqPreStarted=0;
    FrameReady=0;
    AcquisitionStarted=0;
    AcquisitionFinished=0;
  }};

fitpixstate_t fitpixstate;
MIMTLU *aMIMTLU;


// TOT . start on trigger stop on Timer
//char configName[256] = "config_I10-W0015_TOT_4-06-13" ;
//char AsciiconfigName[256] = "config_I10-W0015_TOT_4-06-13_ascii" ;

//TOT Start and stop on trigger
//char configName[256] = "config_I10-W0015_TOT_6-06-13_start_stop_on_trigger" ;
//char AsciiconfigName[256] = "config_I10-W0015_TOT_6-06-13_start_stop_on_trigger_ascii" ;



void my_handler(int s){
           printf("Caught signal %d\n",s);
           char endmsg[1000];
           sprintf(endmsg,"date >> %s/logs/producer_log.txt",getenv("TPPROD"));
           system(endmsg);
           sprintf(endmsg,"echo \"Producer Interupted\" >> %s/logs/producer_log.txt",getenv("TPPROD"));
           system(endmsg);
           sprintf(endmsg,"echo \"###############################################\" >> %s/logs/producer_log.txt",getenv("TPPROD"));
           system(endmsg);
           exit(1);
}

void * fitpix_acq ( void *ptr );

char timebuf[100];
char * get_time(void)
{
    struct timeval now;
    timebuf[0]=0;
    int rc;
    rc=gettimeofday(&now, NULL);
    if(rc==0) {
        sprintf(timebuf,"%lu.%06lu", now.tv_sec, now.tv_usec);
    }
    return timebuf;
}
void AcquisitionPreStarted(CBPARAM /*par*/,INTPTR /*aptr*/)
{
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] AcquisitionPreStarted" << endl;
#endif
  fitpixstate.AcqPreStarted=1;
}


void AcquisitionStarted(CBPARAM /*par*/,INTPTR /*aptr*/)
{
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] AcquisitionStarted" << endl;
#endif
  fitpixstate.AcquisitionStarted=1;
}

void AcquisitionFinished(int /*stuff*/,int /*stuff2*/){
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] AcquisitionFinished" << endl;
#endif
  fitpixstate.AcquisitionFinished=1;
}

void FrameIsReady(int /*stuff*/,int /*stuff2*/)
{
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] FrameReady" << endl;
#endif
  fitpixstate.FrameReady=1;
}


template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
    unsigned char * src = static_cast < unsigned char* >(static_cast < void * >(&data));
    dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (vector <unsigned char >& src, int index, T& data) {
    copy (&src[index], &src[index + sizeof (T)], &data);
}


// Declare a new class that inherits from eudaq::Producer
class TimepixProducer : public eudaq::Producer {
public:
  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  TimepixProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), done(false) 
  {
    aTimepix = new TimepixDevice();

    buffer   = new char[4*MATRIX_SIZE];

    output = new char[1000];

    sprintf(output,"%s/ramdisk/Run%d_%d",getenv("TPPROD"),m_run,m_ev);
    //output = "/home/mbenoit/workspace/eudaq/data/Run";
    running =false;
    
    aMIMTLU= new MIMTLU();
    int mimtlu_status = aMIMTLU->Connect(const_cast<char *>("192.168.222.200"),const_cast<char *>("23"));
    if(mimtlu_status!=1)     SetStatus(eudaq::Status::LVL_ERROR, "MIMTLU Not Running !!");
    gettimeofday(&t0, NULL);


#ifdef USE_KEITHLEY
	k2410 = new Keithley2410(12);
	k2000 = new Keithley2000(16);
//Setting up Keithley
	sleep(1);
	k2000->SetMeasureResistance4W();
	sleep(1);
	k2410->OutputOn();
	sleep(1);
	k2410->SetMeasureCurrent();
	sleep(1);
	k2410->SetSourceVoltage4W();
#endif
	cout << "[TimepixProducer] Done" << endl;

  }

  void LogMessage(char* msg){

	  char date_cmd[200];
	  char logmsg[1000];
	  sprintf(date_cmd,"date >> %s/logs/producer_log.txt",getenv("TPPROD"));
	  sprintf(logmsg,"echo %s >> %s/logs/producer_log.txt",msg,getenv("TPPROD"));
	  system(date_cmd);
	  system(logmsg);
  }

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & config) 
  {
    std::cout << "Configuring: " << config.Name() << std::endl;
    char logmsg[1000];
    sprintf(logmsg,"Configuring with %s",config.Name().c_str());
    LogMessage(logmsg);

    // Do any configuration of the hardware here
    // Configuration file values are accessible as config.Get(name, default)
//    THL = config.Get("THL", 0);
//    aTimepix->SetTHL(THL);


// std::string Get(const std::string & key, const std::string & def) const;
//      double Get(const std::string & key, double def) const;
//      long long Get(const std::string & key, long long def) const;
//      int Get(const std::string & key, int def) const;
      
    acqTime = config.Get("AcquisitionTime_us", 0);
    aTimepix->SetAcqTime(acqTime*1.0e-6);

    unsigned int ntrig = config.Get("MiMTLU_NumberOfTriggers", 1);
    unsigned int plen = config	.Get("MiMTLU_PulseLength", 15);
    unsigned int slen = config.Get("MiMTLU_ShutterLength", 10000);
    unsigned int smode = config.Get("MiMTLU_ShutterMode", 3);

    bpc_config = config.Get("Binary_Config", "/home/lcd/CLIC_Testbeam_August2013/TimepixAssemblies_Data/C04-W0110/Configs/BPC_C04-W0110_15V_IKrum1_96MHz_08-08-13");
    ascii_config = config.Get("Ascii_Config","/home/lcd/CLIC_Testbeam_August2013/eudaq/TimepixProducer/Pixelman_SCL_2011_12_07/Default_Ascii_Config");
    mode = config.Get("TPMode","TOT");
    bias_voltage=config.Get("Bias",15.0);
    VMax=config.Get("Maximum_Bias",40);
    N_Bias_Step=config.Get("N_Bias_Step",6);
    doRamp=config.Get("doRamp",0);
    return_bias=config.Get("return_bias",25.0);

    init_bias=bias_voltage;
    bias_step = (VMax-bias_voltage)/N_Bias_Step;
    stepcnt=0;


    //THL Scan parameters
    THL_init=config.Get("THL_init",379);
    THL_nstep=config.Get("THL_nstep",1);
    THL_step=config.Get("THL_step",2);
    doTHLRamp=config.Get("doTHLRamp",0);
    THLreturn=config.Get("THLreturn",379);

    //Logging of the Configuration
    sprintf(logmsg,"Binary Config is %s",bpc_config.c_str());
    LogMessage(logmsg);
    sprintf(logmsg,"Ascii Config is %s",ascii_config.c_str());
    LogMessage(logmsg);
    sprintf(logmsg,"Mode is %s",mode.c_str());
    LogMessage(logmsg);
    sprintf(logmsg,"Bias is %f",bias_voltage);
    LogMessage(logmsg);


    //Timepix Configuration
    aTimepix->Configure(bpc_config,ascii_config);

    //MIMTLU Configuration
    aMIMTLU->SetNumberOfTriggers(ntrig);
    aMIMTLU->SetPulseLength(plen);
    aMIMTLU->SetShutterLength(slen);
    aMIMTLU->SetShutterMode(smode);

#ifdef USE_KEITHLEY
	k2410->SetOutputVoltage(bias_voltage);
#endif


	if(doTHLRamp){
		aTimepix->SetTHL(THL_init);
		THLcnt = 0;
		THLactual=THL_init;
	}

	cout << "[TimepixProducer] Bias Voltage is  : " << bias_voltage  << endl;
    cout << "[TimepixProducer] Setting Acquisition time to : " << acqTime*1.e-6 << "s" << endl;
    cout << "[TimepixProducer] Setting MiMTLU Trigger per Shutter to : " << ntrig  << endl;
    cout << "[TimepixProducer] Setting MiMTLU Pulse Length to : " << plen*1.e-8 << "s" << endl;
    cout << "[TimepixProducer] Setting MiMTLU Shutter Length to : " << slen*1.e-8 << "s" << endl;
    cout << "[TimepixProducer] Setting MiMTLU Shutter Mode to : " << smode  << endl;
    cout << "[TimepixProducer] Timepix Mode is  : " << mode  << endl;

    cout << "[TimepixProducer]  Ramp Parameters : "  << endl ;
    cout << "[TimepixProducer]  Initial Voltage : " << init_bias << endl ;
    cout << "[TimepixProducer]  Final Voltage : " << VMax << endl ;
    cout << "[TimepixProducer]  Number of Steps : " << N_Bias_Step << endl ;
    cout << "[TimepixProducer]  Return Voltage : " << return_bias << endl ;
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
	cout << "[TimepixProducer] Done" << endl;

  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 1;
    m_shutter=0;
    std::cout << "Start Run: " << m_run << std::endl;

#ifdef USE_KEITHLEY

    if(fabs(bias_voltage)<=fabs(VMax) && (doRamp==1 && stepcnt<=N_Bias_Step)){

    	k2410->SetOutputVoltage(bias_voltage);
    	stepcnt++;
    }

    else if((doRamp==1 && stepcnt>N_Bias_Step)){
    	k2410->SetOutputVoltage(return_bias);
    	bias_voltage=return_bias;
    }

    else{
    	k2410->SetOutputVoltage(init_bias);
    	bias_voltage=init_bias;
    }

#endif
    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    // You can set tags on the BORE that will be saved in the data file
    // and can be used later to help decoding


    if(doTHLRamp==1 && THLcnt<=THL_nstep){


    	aTimepix->SetTHL(THLactual);
    	THLcnt++;
    	bore.SetTag("THL",THLactual);
    	THLactual=THLactual+THL_step;

    }

    else if (doTHLRamp==1){
    	THLactual=THLreturn;
    	aTimepix->SetTHL(THLactual);
    	bore.SetTag("THL",THLactual);
    }

    else {
    	THLactual = aTimepix->ReadDACs();
    	bore.SetTag("THL",THLactual);

    };


    char logmsg[1000];
    sprintf(logmsg,"Starting Run %i",m_run);
    LogMessage(logmsg);


    
    bore.SetTag("ChipID", eudaq::to_string(aTimepix->GetChipID()));
    bore.SetTag("BPC",bpc_config);
    bore.SetTag("Ascii_Config",ascii_config);
    bore.SetTag("Bias",bias_voltage);
    bore.SetTag("Mode",mode);


    // Send the event to the Data Collector
    SendEvent(bore);

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Running");
    //eudaq::mSleep(5000);

    char folder[200];
    sprintf(folder,"%s/ramdisk/Run%i_*",getenv("TPPROD"),m_run);
    cout << "[TimepixProducer] output Folder" <<  folder << endl;

    running = true;
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;
    char logmsg[1000];
    sprintf(logmsg,"Stopping Run %i",m_run);
    LogMessage(logmsg);

    running = false;
   // aTimepix->Abort();

    SetStatus(eudaq::Status::LVL_OK, "Stopped");

    // Set a flag to signal to the polling loop that the run is over
//    stopping = true;
//
//    // wait until all events have been read out from the hardware
//    while (stopping) {
//      eudaq::mSleep(20);
//    }

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

    cout << "[TimepixProducer] Sent EORE " << endl;
 
    //
    //int status = system("cp $TPPROD/ramdisk/* $TPPROD/data");

    char command[1000];
    sprintf(command,"rm -fr %s/ramdisk/*",getenv("TPPROD"));
    status = system(command);
    //do something with status to waive compiler warning 
    if(status==0){
    status = 1;
    }

    if(doRamp==1 && stepcnt<=N_Bias_Step)bias_voltage+=bias_step;
  }  

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    char logmsg[1000];
    sprintf(logmsg,"Producer Terminated %i",m_run);
    LogMessage(logmsg);
    char endmsg[1000];
    sprintf(endmsg,"echo \"###############################################\" >> %s/logs/producer_log.txt",getenv("TPPROD"));
    system(endmsg);
    eudaq::mSleep(1000);
    running = false;
    done = true;
    exit(1);
  }

void ReadoutLoop() {
  // Loop until Run Control tells us to terminate
  while (!done) 
  {
    if(running)
    {
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] Loop begin" << endl;
#endif
      //sprintf(output,"../data/Run%d_%d",m_run,m_ev);
      sprintf(output,"%s/ramdisk/Run%d_%d",getenv("TPPROD"),m_run,m_ev);

      //pthread_mutex_lock(&m_producer_mutex);
      fitpixstate.reset();
      
       //Multithreading 
      pthread_t thread1;  /* thread variables */
      pthread_create (&thread1, NULL,  fitpix_acq, (void *) this);
      
      //cout << "starting new frame" << endl;


      while(!fitpixstate.AcqPreStarted)
        eudaq::mSleep(0.01);

    //  aMIMTLU->Arm();

#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] after Acq Pre-Started" << endl;
#endif
      
      std::vector<mimtlu_event> events;
      try
      {
        events=aMIMTLU->GetEvents();
      }
      catch(mimtlu_exception e)
      {
        std::cout <<e.what()<<endl;
      }
#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] after TLU Get Events" << endl;
#endif

int timeout=0;
bool badFrame=false;
while(!fitpixstate.FrameReady){
        eudaq::mSleep(0.01);
		timeout++;
		if(timeout>10000){
#ifdef DEBUGFITPIX
			cout << get_time()<<" [FITPIX] Corrupted Frame" << endl;
#endif
			aTimepix->Abort();
			char warn_msg[250];
			sprintf(warn_msg,"Corrupted Frame at event %i, shutter %i",m_ev,m_shutter);
			LogMessage(warn_msg);
			SetStatus(eudaq::Status::LVL_WARN, warn_msg);
			badFrame=true;
			break;
		}
	}

#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] after readout" << endl;
#endif
      pthread_join(thread1, NULL);  

#ifdef DEBUGFITPIX
  cout << get_time()<<" [FITPIX] thread join" << endl;
#endif

  	  int pos =0;
  	  std::vector<unsigned char> bufferOut;
      m_shutter++;

      if(badFrame==false){
    	  control=aTimepix->GetFrameData2(output,buffer);



  

      unsigned int Data[MATRIX_SIZE];
      for(int i =0;i<MATRIX_SIZE;i++)
      {
        memcpy(&Data[i],buffer+i*4,4);
      }

      
      for(unsigned int i=0;i<256;i++)
      {
        for(unsigned int j=0;j<256;j++)
        {
           if(Data[pos]!=0 && Data[pos]!=11810)
           {
             //cout << "[Data] Evt : " << m_ev << " " << i << " " << j << " " << Data[pos] << endl;
             pack(bufferOut,i);
             pack(bufferOut,j);
             pack(bufferOut,Data[pos]);

           }
           //pack(bufferOut,i);
           // pack(bufferOut,j);
           //pack(bufferOut,m_ev);
           pos++;
        }
      }
      }

  if((m_ev%100==0) | (m_ev<100)) cout << "event #" << m_ev << endl;
  if(m_shutter%1000==0 ) {
      
      //system("cp /home/lcd/eudaq/timepixproducer_mb/ramdisk/* ../data");
	    char command[1000];
	    sprintf(command,"rm -fr  %s/ramdisk/*",getenv("TPPROD"));
	    cout << "[TimepixProducer] Clearing cache : " << command << endl;
	    status = system(command);
          if(status==0) { 
            status=1;
        }

#ifdef USE_KEITHLEY
  		double current=k2410->ReadValue();
		double R=k2000->ReadValue();
		char kmsg[1024];
		sprintf(kmsg,"Temperature is : %f C, Bias Voltage is %f V, Current is %f nA",GetPt100Temperature(R),bias_voltage,current*1e9);
		LogMessage(kmsg);
		SetStatus(eudaq::Status::LVL_INFO, kmsg);

#endif

      }


      for(auto& it : events)
      {
     //   std::cout<< "[event] "<<it->to_char();
        eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
        std::vector<unsigned char> buffer;
        for(unsigned int i=0;i<16;i++)
          buffer.push_back(it.to_char()[i]);
        ev.AddBlock(0, bufferOut);
        ev.AddBlock(1, buffer);

        SendEvent(ev);
	
	m_ev++;
      }



    }
  }
}

void runfitpix()
{
#ifdef DEBUGFITPIX  
  cout << get_time() << " [FITPIX] before run"<<endl;
#endif
  control=aTimepix->PerformAcquisition(output);
#ifdef DEBUGFITPIX  
  cout << get_time() << " [FITPIX] after run"<<endl;
#endif
}

private:

  unsigned m_run, m_ev, m_shutter;
  int THL;
  int IKrum;
  bool stopping, done;
  TimepixDevice *aTimepix;
  char *buffer;
  char* output;
  bool running;
  double acqTime;
  pthread_mutex_t m_producer_mutex;
  int control;
  string bpc_config,ascii_config;
  string  mode;
  double bias_voltage, VMax;
  int N_Bias_Step;
  Keithley2410 *k2410;
  Keithley2000 *k2000;
  int doRamp,stepcnt;
  double bias_step,init_bias,return_bias;
  int THL_init;
  int THL_nstep;
  int THL_step;
  int doTHLRamp;
  int THLcnt;
  int THLactual;
  int THLreturn;



};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Timepix Producer", "1.0",
                         "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "TimepixProducer", "string",
                                   "The name of this Producer");

  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = my_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);


  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    TimepixProducer producer(name.Value(), rctrl.Value());
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



void * fitpix_acq ( void *ptr )
{
    TimepixProducer *tp=(TimepixProducer *) ptr;            
    tp->runfitpix();  
    pthread_exit(0); /* exit */
} /* print_message_function ( void *ptr ) */
