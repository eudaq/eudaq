#include "HGCalProducer.h"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Event.hh"
#include "eudaq/RawEvent.hh"

#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>
#include <iterator>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <compressor.h>

namespace {
  auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<eudaq::HGCalProducer, const std::string&, const std::string&>(eudaq::HGCalProducer::m_id_factory);
}

namespace eudaq{

  
  static uint32_t runID=0;
  void startHGCCtrlThread(HGCalController* ctrl)
  {
    ctrl->startrun(runID);
  }

  HGCalProducer::HGCalProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
    m_run(0),
    m_eventN(0),
    m_triggerN(-1),
    m_state(STATE_UNCONF),
    m_doCompression(false),
    m_compressionLevel(5),
    m_hgcController(NULL)
  {}

  void HGCalProducer::DoInitialise()
  {
    auto ini = GetInitConfiguration();
    EUDAQ_INFO("HGCalProducer received initialise command");
    m_hgcController=new HGCalController();
    m_hgcController->init();
    //SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised");
  }

  void HGCalProducer::DoConfigure() 
  {
    auto config = GetConfiguration();
    EUDAQ_INFO("Receive config command with config file : "+config->Name());
    config->Print(std::cout);

    m_doCompression = config->Get("DoCompression",false);
    m_compressionLevel = config->Get("CompressionLevel",5);

    if(m_doCompression)
      std::cout << "We will perform compression with compression level = " << m_compressionLevel << std::endl;
    else
      std::cout << "We will not perform any compression" << std::endl;
    
    HGCalConfig hgcConfig;
    hgcConfig.connectionFile = config->Get("ConnectionFile","file://./etc/connection.xml");
    hgcConfig.rootFilePath = config->Get("TimingRootFilePath","./data_root");
    hgcConfig.rdoutMask = config->Get("RDoutMask",4);
    hgcConfig.blockSize = config->Get("DataBlockSize",30787);
    hgcConfig.uhalLogLevel = config->Get("UhalLogLevel", 5);
    hgcConfig.timeToWaitAtEndOfRun = config->Get("TimeToWaitAtEndOfRun", 1000);
    hgcConfig.saveRawData = config->Get("saveRawData", false);
    hgcConfig.checkCRC = config->Get("checkCRC", false);
    hgcConfig.throwFirstTrigger = config->Get("throwFirstTrigger", true);
    
    m_hgcController->configure(hgcConfig);
    
    m_state=STATE_CONFED;

  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  void HGCalProducer::DoStartRun() 
  {
    m_state = STATE_GOTORUN;
    m_run = GetRunNumber();
    runID=m_run;
    m_ev = 0;

    EUDAQ_INFO("HGCalProducer received start run command");

    m_hgcCtrlThread=boost::thread(startHGCCtrlThread,m_hgcController);

    m_state = STATE_RUNNING;
    //    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
  }

  void HGCalProducer::DoStopRun()
  {
    EUDAQ_INFO("HGCalProducer received stop run command");
    //try {
      m_hgcController->stoprun();
      //eudaq::mSleep(1000);
      m_state = STATE_GOTOSTOP;
    //   while (m_state == STATE_GOTOSTOP) {
    // 	eudaq::mSleep(1000); //waiting for EORE being send
    //   }
    //   //SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
    // } catch (const std::exception & e) {
    //   EUDAQ_ERROR("Stop Error: " + eudaq::to_string(e.what()));
    // } catch (...) {
    //   EUDAQ_ERROR("Stop Error");
    // }
  }

  void HGCalProducer::DoTerminate()
  {
    EUDAQ_INFO("HGCalProducer received terminate command");
    m_hgcController->stoprun();
    //eudaq::mSleep(1000);
    m_state = STATE_GOTOSTOP;
    // while (m_state == STATE_GOTOSTOP) {
    //   eudaq::mSleep(1000); //waiting for EORE being send
    // }
    m_hgcController->terminate();
    //m_state = STATE_GOTOTERM;
  }

  void HGCalProducer::RunLoop() 
  {
    while (m_state == STATE_RUNNING) {
      if( m_hgcController->getDequeData().empty() ){
	boost::this_thread::sleep( boost::posix_time::microseconds(100) );
	continue;
      }
      eudaq::EventUP ev = eudaq::Event::MakeUnique("HexaBoardRawDataEvent");
      eudaq::RawEvent* ev_raw=dynamic_cast<eudaq::RawEvent*>(ev.get());
      if(m_ev==0) ev_raw->SetBORE();
      HGCalDataBlocks dataBlk=m_hgcController->getDequeData()[0];
      int iblock(0);
      uint32_t head[2];
      for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it ){
	head[0]=it->first;
	head[1]=it->second[0];
	ev_raw->AddBlock( 2*iblock,head,sizeof(head) );
	if( m_doCompression==true ){
	  std::string compressedData=compressor::compress(it->second,m_compressionLevel);
	  ev_raw->AddBlock( 2*iblock+1,compressedData.c_str(), compressedData.size() );
	  std::cout<<"Running:  skiroc mask="<<std::setw(8)<<std::setfill('0')<<std::hex<<it->second[0]<<"  Size of the data (bytes): "<<std::dec<<it->second.size()*4<<"  compressedData.size() = " << compressedData.size() << std::endl;
	}
	else
	  ev_raw->AddBlock( 2*iblock+1,it->second );
	iblock++;
      }
      m_hgcController->getDequeData().pop_front();
	
      m_ev++;
      ev_raw->SetRunN(m_run);
      ev_raw->SetEventN(m_ev);
      //ev->SetTriggerN(); //possible but need to add  triggerid member in HGCalDataBlock
      SendEvent(std::move(ev));
    }
    if (m_state == STATE_GOTOSTOP) {
      std::cout << "HGCalProducer received stop command and starts stop procedure" << std::endl;
      while(1){
	std::cout << "HGCalProducer data deque size = " << m_hgcController->getDequeData().size() << std::endl;
	if( m_hgcController->getDequeData().empty() )
	  break;
	
	std::cout<<"Deque size = " << m_hgcController->getDequeData().size() << std::endl;
	eudaq::EventUP ev=eudaq::Event::MakeUnique("HexaBoardRawDataEvent");
	eudaq::RawEvent* ev_raw=dynamic_cast<eudaq::RawEvent*>(ev.get());
	HGCalDataBlocks dataBlk=m_hgcController->getDequeData()[0];
	int iblock(0);
	int head[1];
	for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it ){
	  head[0]=iblock+1;
	  ev_raw->AddBlock( 2*iblock,head,sizeof(head) );
	  if( m_doCompression==true ){
	    std::string compressedData=compressor::compress(it->second,m_compressionLevel);
	    ev_raw->AddBlock( 2*iblock+1,compressedData.c_str(), compressedData.size() );
	  }
	  else
	    ev_raw->AddBlock( 2*iblock+1,it->second );
	  iblock++;
	}
	m_hgcController->getDequeData().pop_front();
	if( m_hgcController->getDequeData().empty() ){
	  ev_raw->SetEORE();
	}
	m_ev++;
	ev_raw->SetRunN(m_run);
	ev_raw->SetEventN(m_ev);
	SendEvent(std::move(ev));
      }
      eudaq::mSleep(1000);
      m_state = STATE_CONFED;
    }
  }
}
