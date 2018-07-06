/*
 * 2017 Nov 23
 * Author: Mengqing Wu <mengqing.wu@desy.de>
 * notes:
 * 1) data rx is a quick algo to not delay any kpix data streaming
 * 2) data tx can be slower, further dev can be: unfolding the kpix cycle data sent as each buffer
 */
#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
/* linux func libs:*/
#include <unistd.h>
//--> start of kpix libs:
#include "KpixControl.h"
#include "UdpLink.h"
#include "System.h"
#include "ControlServer.h"
#include "Data.h"
//#include "myserver_udp.h"
//--> end of kpix libs:
#include <sys/wait.h>
#include <deque>
#include <mutex>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
//class System;

class kpixProducer : public eudaq::Producer {
  public:
  kpixProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void OnStatus() override;
  
  void RunKpix();
  void KpixDataReceiver();
  void SendKpixEvent();
  
  virtual Data* pollKpixData(int &evt_counter){}; // to be deleted Nov 23 
  void Mainloop();
 
  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixProducer");
  bool stop; //kpixdev

protected:
  std::deque<Data*> _kpixQueue;
  std::mutex _kpixQueue_guard;

private:
  
  uint32_t m_noeudaqbin;
  uint32_t m_nokpixbin;
  std::string m_binaries_database;

  UdpLink udpLink;
  std::string m_defFile;
  std::string m_debug;
  std::string m_kpixRunState;
  //KpixControl kpix(&udpLink, m_defFile, 32);
  KpixControl *m_kpix;

  std::string m_runrate, m_dataOverEvt;

  bool m_exit_of_run;
  std::thread m_thd_run;
  std::thread m_thd_datarx;
  std::thread m_thd_datatx;
  uint32_t m_runcount;

  int m_nEvt;
  
  std::atomic<bool> _enableDataThread;
   
  /*outpuf file in kpix data format trial@Sep 20 */
  int32_t dataFileFd_;
  
};
//----------DOC-MARK-----END*DEC-----DOC-MARK---------

//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<kpixProducer, const std::string&, const std::string&>(kpixProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------

//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
kpixProducer::kpixProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol),
   m_debug("False"), m_kpixRunState("Running"),
   m_runrate("1Hz"), m_dataOverEvt("0 - 0 Hz"),
   dataFileFd_(-1), m_nEvt(0), _enableDataThread(false),
   m_noeudaqbin(0),m_nokpixbin(0){}

//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void kpixProducer::DoInitialise(){
  m_exit_of_run=false;
  // Create and setup PGP link
  udpLink.setMaxRxTx(500000);
  udpLink.open(8192,1,"192.168.1.16"); // to have multiple udplink connections
  // --> start: wp 2018 Feb 8
  m_thd_datarx = std::thread(&kpixProducer::KpixDataReceiver, this);
  // --> edn: wp 2018 Feb 8

  //udpLink.openDataNet("127.0.0.1",8099);
  udpLink.enableEudaq();
  auto ini = GetInitConfiguration();
  // --- read the ini file:
  m_defFile = ini->Get("KPIX_CONFIG_FILE", "defaults.xml"); //kpixdev
  m_debug = ini->Get("KPIX_DEBUG", "False");
  //  m_noDevice = true; // if no device connected, to test sturcture
  
  try{
    if (m_debug!="True" && m_debug!="False")
      throw std::string("      KPIX_DEBUG value error! (must be 'True' or 'False')\n");
    else std::cout<<"[Init:info] kpix system debug == "<< m_debug <<std::endl;
    bool b_m_debug = (m_debug=="True")? true : false;
  
    m_kpix =new KpixControl(&udpLink, m_defFile, 32);
    m_kpix->setDebug(b_m_debug);

    udpLink.setDebug(b_m_debug);
    udpLink.enableSharedMemory("kpix",1);

    std::string xmlString="<system><command><ReadStatus/>\n</command></system>";

    m_kpix->poll(NULL);
    m_kpix->parseXmlString(xmlString);
    
  }catch(std::string error) {
    std::cout <<"Caught error: "<<std::endl;
    std::cout << error << std::endl;
  }

  
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void kpixProducer::DoConfigure(){
  auto conf = GetConfiguration();
  if (!conf) return;
  
  conf->Print(std::cout);

  m_noeudaqbin = conf->Get("DISABLE_EUDAQ_BIN",0);
  m_nokpixbin = conf->Get("DISABLE_KPIX_BIN",0);
  m_binaries_database = conf->Get("EUDAQ_DataBase","./");
  
  std::string conf_defFile = conf->Get("KPIX_CONFIG_FILE","");
  if (conf_defFile!="") m_defFile=conf_defFile; 
  m_kpix->parseXmlFile(m_defFile); // work as set defaults

  //--> read eudaq config to rewrite the kpix config:
  //--> Kpix Config override:
  std::string database = conf->Get("KPIX_DataBase","");
  std::string datafile = conf->Get("KPIX_DataFile","");
  std::string dataauto = conf->Get("KPIX_DataAuto","");
  if (database!="") m_kpix->getVariable("DataBase")->set(database); 
  if (datafile!="") m_kpix->getVariable("DataFile")->set(datafile);
  if (dataauto!="") m_kpix->getVariable("DataAuto")->set(dataauto);
  
  //--> Kpix Run Control
  m_kpixRunState = conf->Get("KPIX_RunState","Running");
  m_runcount = conf->Get("KPIX_RunCount",0);
  if (m_runcount!=0) m_kpix->getVariable("RunCount")->setInt(m_runcount);

  //std::cout<<"[producer:dev] m_runcount = "<< m_runcount <<std::endl;  
  std::cout<<"[producer:dev] run count = "<< m_kpix->getVariable("RunCount")->getInt() <<std::endl;
  //m_runcount = m_kpix->getVariable("RunCount")->getInt();
  //std::cout<<"[producer:dev] run progress = "<<m_kpix->getVariable("RunProgress")->getInt() <<std::endl;
  
  m_runrate = m_kpix->getVariable("RunRate")->get();
  std::cout<<"[producer:dev] run_rate = "<< m_runrate <<std::endl;
  
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void kpixProducer::DoStartRun(){
  //m_kpix->command("OpenDataFile","");

  /* linux open a file for kpix output*/
  std::string run_num = std::to_string(GetRunNumber());
  auto len_run_n = run_num.length();
  std::string myname(6-len_run_n, '0');
  myname+=run_num;
    
  std::string kpixfile(m_binaries_database+"/kpix_output_run"+myname+".bin");
  std::cout<< kpixfile <<std::endl;

  if (!m_noeudaqbin)
    dataFileFd_ = ::open(kpixfile.c_str(),O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  
  _enableDataThread=true;
  //if (_kpixQueue.size()!=0) _kpixQueue.clear(); // some temp fix to run Ext Acq Trig from Nov/Dec 2017
  //m_thd_datarx = std::thread(&kpixProducer::KpixDataReceiver, this);
  m_thd_datatx = std::thread(&kpixProducer::SendKpixEvent, this);
  /* Core start to run*/
  m_thd_run = std::thread(&kpixProducer::RunKpix, this);
  
}

//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void kpixProducer::DoStopRun(){
 
  /* Func1: stop the Mainloop*/
  m_exit_of_run = true; //--> this shall 
  if(m_thd_run.joinable())  m_thd_run.join();
  if (m_thd_datarx.joinable()){
    _enableDataThread=false;
    m_thd_datarx.join();
  }
  if (m_thd_datatx.joinable())  m_thd_datatx.join();
  
  /* Close data file to write*/
  if (dataFileFd_>=0) {
    ::close(dataFileFd_);
    dataFileFd_ = -1;
  }
  
  /* Func2: stop the Kpix*/
  m_kpix->command("CloseDataFile",""); //--> kpix data thread killed @Mengqing
  if (m_kpix->getVariable("RunState")->get() != "Stopped")
    m_kpix->command("SetRunState","Stopped");
    
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void kpixProducer::DoReset(){
  m_kpix->parseXmlString("<system><command><HardReset/></command></system>");

  /* Step1: stop Mainloop (kpix+data collecting)*/
  m_exit_of_run = true;
  if (m_thd_run.joinable())    m_thd_run.join();
  if (m_thd_datarx.joinable()){
    _enableDataThread=false;
    m_thd_datarx.join();
  }
  if (m_thd_datatx.joinable()) m_thd_datatx.join();

  if (m_kpix->getVariable("RunState")->get() != "Stopped")
    m_kpix->command("SetRunState","Stopped");
  /* Close data file to write*/
  if (dataFileFd_>=0) {
    ::close(dataFileFd_);
    dataFileFd_ = -1;
  }
  
  /* Step2: set the thread free and turned off the exit_of_run sign*/
  m_thd_run = std::thread();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void kpixProducer::DoTerminate(){
  //-->you can call join to the std::thread in order to set it non-joinable to be safely destroyed
  /* Func1: stop Mainloop (kpix+data collecting)
   * add kpix stopped command to ensure kpix closed properly.
   */
  m_exit_of_run = true;
  if (m_thd_run.joinable())    m_thd_run.join();
  if (m_thd_datarx.joinable()){
    _enableDataThread=false;
    m_thd_datarx.join();
  }
  if (m_thd_datatx.joinable()) m_thd_datatx.join();

  m_kpix->command("CloseDataFile",""); // no problem though file closed, as protected in kpix/generic/System.cpp
  if (m_kpix->getVariable("RunState")->get() != "Stopped")
    m_kpix->command("SetRunState","Stopped");
  if (dataFileFd_>=0){
    ::close(dataFileFd_);
    dataFileFd_ = -1;
  }

  /* stop listen to the hardware */
  udpLink.close();
  delete m_kpix;

}

//----------DOC-MARK-----BEG*STATUS-----DOC-MARK----------
void kpixProducer::OnStatus(){
  /* Func:
   * SetStatusTag from CommandReceiver;
   * Tags collecting and transferred to euRun GUI as the value of tcp key of map_conn_status.
  */
  SetStatusTag("EventN", std::to_string(m_nEvt));
  SetStatusTag("Status", "test");
  SetStatusTag("Data/Event", m_dataOverEvt);
  SetStatusTag("Run Rate", m_runrate);
  SetStatusTag("Configuration Tab","conf. values computed from .config" );
  SetStatusTag("KpixStopped", m_exit_of_run ? "true" : "false");
}

//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------

/* start wmq-dev: polling data from kpix to eudaq*/

void kpixProducer::KpixDataReceiver(){
  /*
   * This is the Eudaq DataRun, needs to start thread with Eudaq_rxRun, 
   * which locates at kpix_DataRun enabled w/ udpLink.open()
   */
  _enableDataThread=true;

  Data* datKpix;
  while( _enableDataThread ){
    if ( ( datKpix = (Data*)udpLink.Eudaq_DataRun()) != NULL ) {
      std::unique_lock<std::mutex> rxlock(_kpixQueue_guard);
      _kpixQueue.push_front(datKpix);
      rxlock.unlock();
      m_nEvt++;
      std::cout<< " @_@ COUNTER! Event #"<< std::dec << m_nEvt <<std::endl;
    }
    delete datKpix;
  }
  std::cout<<"[] End of data thread"<<std::endl;
  
}/* end wmq-dev: polling data from kpix to eudaq*/

//----------DOC-MARK-----START:DataTX-----DOC-MARK---------

void kpixProducer::SendKpixEvent(){
  int counter_tx=0;

  if (!_enableDataThread) return;// if no rx data thread, then exit
    
  while(1){ 
    if ( _kpixQueue.size()!=0 ){
      
      std::unique_lock<std::mutex> txlock(_kpixQueue_guard);
      auto size1=_kpixQueue.size();
      auto txdata = (Data*)_kpixQueue.back();
      _kpixQueue.pop_back();
      auto size2=_kpixQueue.size();
      txlock.unlock();
      
      cout << "[tx] kpix queue size, before = "<< size1 <<"; after = "<< size2<<endl;
      
      auto eudaqEv = eudaq::Event::MakeUnique("KpixRawEvt");
    
      auto buff = txdata->data();
      auto size = txdata->size();
      eudaqEv->AddBlock(0, buff, size*4);
	
      if (dataFileFd_>0){
	auto wra = write(dataFileFd_, &size, 4);
	auto wrb = write(dataFileFd_, buff, size*4);
	//std::cout << "[dev] wra = " << wra <<"; " << " wrb = " << wrb << std::endl;
      }
      
      SendEvent(std::move(eudaqEv));
      counter_tx++;
    }

    if (!_enableDataThread) break;
  }

  std::cout<< "[tx] DataThread CLOSED ==> \n\tEUDAQ saw kpix data cycles in total = "<<counter_tx<<std::endl;
  
  //auto tp_trigger = std::chrono::steady_clock::now();
  //std::chrono::nanoseconds this_ts_ns(tp_trigger - _tp_start);
  //eudaqEv->SetTimestamp(this_ts_ns.count(), this_ts_ns.count());
    
}
//----------DOC-MARK-----END:DataTX-----DOC-MARK---------

void kpixProducer::RunKpix(){
  /*Open data file to write*/
  if (!m_nokpixbin)  m_kpix->command("OpenDataFile",""); //--> kpix data thread
  
  if (m_kpix->getVariable("RunState")->get() != "Stopped")
    EUDAQ_THROW("Check KPiX RunState ==> " + m_kpix->getVariable("RunState")->get() + "\n\t it SHALL be 'Stopped, CHECK!'");

  // start to run:
  m_kpix->command("SetRunState",m_kpixRunState);
  auto kpixStatus = m_kpix->getVariable("RunState")->get();
  std::cout<<"\t[KPiX:dev] Run State @mainLoop = "<<kpixStatus<<std::endl;
  auto _tp_start = std::chrono::steady_clock::now();
  
  do{
    kpixStatus = m_kpix->getVariable("RunState")->get();
    //std::cout<<"\t[KPiX:dev] Run State@mainLoop = "<<kpixStatus<<std::endl;

    m_kpix->poll(NULL);
    auto dataOverEvt = m_kpix->getVariable("DataRxCount")->get();
    //std::cout<< "\t[KPiX:dev] Data/Event ==> " << dataOverEvt <<std::endl;
    m_dataOverEvt=dataOverEvt;
   
    if (kpixStatus =="Stopped" ) {
      m_exit_of_run = true;
      break;
    } else if (m_exit_of_run){
      m_kpix->command("SetRunState","Stopped");
      break;
    } else /*do nothing*/;
    
    usleep(1000);
    
  }while(kpixStatus != "Stopped");

  _enableDataThread=false;
  m_exit_of_run=true;

  std::cout<<"FINISH: kpix finished running with #ofEvent => " << m_nEvt << " processed.\n";
  std::cout<<"\t m_exit_of_run == "<< (m_exit_of_run ? "true" : "false") <<std::endl;
  /*Close data file to write*/
  m_kpix->command("CloseDataFile","");

}


////////////////////////////////////////////////////////////////////////////////////////////////////
//-------old version of mainloop@Nov 23-------------//
/*
void kpixProducer::Mainloop(){
  m_kpix->command("OpenDataFile",""); //--> kpix data thread killed @Mengqing
  
  if (m_kpix->getVariable("RunState")->get() != "Stopped")
    EUDAQ_THROW("Check KPiX RunState ==> " + m_kpix->getVariable("RunState")->get() + "\n\t it SHALL be 'Stopped, CHECK!'");

  m_kpix->command("SetRunState",m_kpixRunState);
  auto kpixStatus = m_kpix->getVariable("RunState")->get();
  std::cout<<"\t[KPiX:dev] Run State @mainLoop = "<<kpixStatus<<std::endl;

  auto tp_start_run = std::chrono::steady_clock::now();
  bool save_ts=true;

  do{
    // sleep 1 second after each loop 
    auto tp_current_evt = std::chrono::steady_clock::now();
    auto tp_next = tp_current_evt +  std::chrono::seconds(1);
    std::this_thread::sleep_until(tp_next);
    
    //m_kpix->poll(NULL);

    kpixStatus = m_kpix->getVariable("RunState")->get();
    std::cout<<"\t[KPiX:dev] Run State@mainLoop = "<<kpixStatus<<std::endl;

    auto dataOverEvt = m_kpix->getVariable("DataRxCount")->get();
    std::cout<< "\t[KPiX:dev] Data/Event ==> " << dataOverEvt <<std::endl;
    m_dataOverEvt=dataOverEvt;

    // start wmq-dev: polling data from kpix to eudaq
    auto ev = eudaq::Event::MakeUnique("KpixRawEvt");
    auto databuff = pollKpixData(m_nEvt);
    
    //--> data is ready
    if (databuff!=NULL) {
      auto tp_trigger = std::chrono::steady_clock::now();
      std::cout << " [Kpix.Data] says: EventNumber = " << (databuff->data())[0] << std::endl ;
	
      if (save_ts) {
	std::chrono::nanoseconds this_ts_ns(tp_trigger - tp_start_run);
	ev->SetTimestamp(this_ts_ns.count(), this_ts_ns.count());
	//std::cout<<"\t[Loop]: Timestamp enabled to use\n"; //wmq
	//std::cout<<"\t[Loop]: time stamp at ==> "<<this_ts_ns.count()<<"ns\n";
      }
      auto buff = databuff->data();
      auto size = databuff->size();

      // std::cout<<"[producer.CHECK] buff = "<< buff << "\n"
	//       <<"                 size = "<< size << "\n"
	//       <<"                 &size= "<< &size<< std::endl;
      //
      //--> milestone: remember this pointer stuff @ August-17th
      //--> Todo: add '0000' at begining of each binary buff... @ Sept-29
      //ev->AddBlock(0, &size, 4);
      ev->AddBlock(0, buff, size*4);

      // save databuff also to the kpix output format via linux write(2) func
      if (dataFileFd_ >= 0 ){
	auto wra = write(dataFileFd_, &size, 4);
	auto wrb = write(dataFileFd_, buff, size*4);
	std::cout << "[dev] wra = " << wra <<"; "
		  << " wrb = " << wrb << std::endl;
      }
      
      delete databuff;
      SendEvent(std::move(ev));
    }
    // end wmq-dev: polling data from kpix to eudaq
 
    if (kpixStatus =="Stopped" ) {
      m_exit_of_run = true;
      break;
    } else if (m_exit_of_run){
      m_kpix->command("SetRunState","Stopped");
      break;
    } else ;

  }while(kpixStatus != "Stopped"); //-> used when kpix own data streaming not broken @mengqing
    //}while(m_nEvt<m_runcount);

  m_exit_of_run=true;

  std::cout<<"\n[FINISH]: kpix finished running with #ofEvent => " << m_nEvt << " processed.\n";
  std::cout<<"\t m_exit_of_run == "<< (m_exit_of_run ? "true" : "false") <<std::endl;
  // Close data file to write
  m_kpix->command("CloseDataFile","");

  ::close(dataFileFd_);
  dataFileFd_ = -1;
  
  // TODO: Do sth to get a stopped sign
}*/



//----------DOC-MARK-----END*IMP-----DOC-MARK----------

