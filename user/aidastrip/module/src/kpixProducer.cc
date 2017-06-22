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
  bool m_exit_of_run;

  //System *system_;
  
  UdpLink udpLink;
  std::string m_defFile;
  std::string m_debug;
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
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}

//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void kpixProducer::DoInitialise(){
  
  auto ini = GetInitConfiguration();
  // --- read the ini file:
  m_defFile = ini->Get("KPIX_CONFIG_FILE", "defaults.xml"); //kpixdev
  m_debug = ini->Get("KPIX_DEBUG", "false");
      
  try{
    if (m_debug!="true" && m_debug!="false")
      throw std::string("[kpixProducer::Init] KPIX_DEBUG value error! (must be 'true' or 'false')");
    else std::cout<<"[kpixProducer::Init] info, debug == "<< m_debug <<std::endl;
    bool b_m_debug = (m_debug=="true")? true : false;
  
    kpix =new KpixControl(&udpLink, m_defFile, 32);
    kpix->setDebug(b_m_debug);

    // Create and setup PGP link
    udpLink.setMaxRxTx(500000);
    udpLink.setDebug(b_m_debug);
    udpLink.open(8192,1,"192.168.1.16");
    //udpLink.openDataNet("127.0.0.1",8099);
    udpLink.enableSharedMemory("kpix",1);
    //usleep(100);
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
  kpix->parseXmlFile(m_defFile); // work as set defaults
  //  m_plane_id = conf->Get("PLANE_ID", 0);
 
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void kpixProducer::DoStartRun(){

}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void kpixProducer::DoStopRun(){
  // stop listen to the hardware
  udpLink.close();
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void kpixProducer::DoReset(){
  // stop listen to the hardware
  udpLink.close();
  kpix->parseXmlString("<system><command><HardReset/></command></system>");
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void kpixProducer::DoTerminate(){
  //-->you can call join to the std::thread in order to set it non-joinable to be safely destroyed
  
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------

//----------DOC-MARK-----END*IMP-----DOC-MARK----------

