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
//--> kpix lib:
#include "KpixControl.h"
#include "UdpLink.h"
#include "System.h"
#include "ControlServer.h"

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

  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixProducer");
  bool stop; //kpixdev
   
private:
 
  UdpLink udpLink;
  std::string m_defFile;
  std::string m_debug;
  std::string m_runstate;
  //KpixControl kpix(&udpLink, m_defFile, 32);
  KpixControl *kpix;
  //int m_port; 
  
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
  :eudaq::Producer(name, runcontrol), m_debug("False"), m_runstate("Running"){
  // Create and setup PGP link
  udpLink.setMaxRxTx(500000);
  udpLink.open(8192,1,"192.168.1.16");
  //udpLink.openDataNet("127.0.0.1",8099);
  //usleep(100);
}

//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void kpixProducer::DoInitialise(){
  
  auto ini = GetInitConfiguration();
  // --- read the ini file:
  m_defFile = ini->Get("KPIX_CONFIG_FILE", "defaults.xml"); //kpixdev
  m_debug = ini->Get("KPIX_DEBUG", "False");
      
  try{
    if (m_debug!="True" && m_debug!="False")
      throw std::string("      KPIX_DEBUG value error! (must be 'True' or 'False')\n");
    else std::cout<<"[Init:info] kpix system debug == "<< m_debug <<std::endl;
    bool b_m_debug = (m_debug=="True")? true : false;
  
    kpix =new KpixControl(&udpLink, m_defFile, 32);
    kpix->setDebug(b_m_debug);

    udpLink.setDebug(b_m_debug);
    udpLink.enableSharedMemory("kpix",1);

    std::string xmlString="<system><command><ReadStatus/>\n</command></system>";

    kpix->parseXmlString(xmlString);
    
  }catch(std::string error) {
    std::cout <<"Caught error: "<<std::endl;
    std::cout << error << std::endl;
  }

  
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void kpixProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  
  std::string conf_defFile = conf->Get("KPIX_CONFIG_FILE","");
  if (conf_defFile!="") m_defFile=conf_defFile; 
  kpix->parseXmlFile(m_defFile); // work as set defaults

  //--> read eudaq config to rewrite the kpix config:
  //--> Kpix Config override:
  std::string database = conf->Get("KPIX_DataBase","");
  std::string datafile = conf->Get("KPIX_DataFile","");
  std::string dataauto = conf->Get("KPIX_DataAuto","");
  if (database!="") kpix->getVariable("DataBase")->set(database); 
  if (datafile!="") kpix->getVariable("DataFile")->set(datafile);
  if (dataauto!="") kpix->getVariable("DataAuto")->set(dataauto);
  
  //--> Kpix Run Control
  m_runstate = conf->Get("KPIX_RunState","Running");
  int runcount = conf->Get("KPIX_RunCount",0);
  if (runcount!=0) kpix->getVariable("RunCount")->setInt(runcount);
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void kpixProducer::DoStartRun(){
  kpix->command("OpenDataFile","");
  kpix->command("SetRunState",m_runstate);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void kpixProducer::DoStopRun(){
  kpix->command("CloseDataFile","");
  kpix->command("SetRunState","Stopped");
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void kpixProducer::DoReset(){
  kpix->parseXmlString("<system><command><HardReset/></command></system>");
  
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void kpixProducer::DoTerminate(){
  //-->you can call join to the std::thread in order to set it non-joinable to be safely destroyed

  // stop listen to the hardware
  kpix->command("CloseDataFile",""); // no problem though file closed, as protected in kpix/generic/System.cpp
  udpLink.close();
  delete kpix;

}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------

//----------DOC-MARK-----END*IMP-----DOC-MARK----------

